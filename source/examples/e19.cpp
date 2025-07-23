#include "e19.hpp"

/**
    @author: Peter Tso
    @email: tsopeter@ku.edu


    For this simple task, we will train a single image to
    point to a certain location on the camera using
    Von Mises based Natural Evolution Strategies.

    This is to test the logic of the system, not necessarily timing.
    Additional modules will be used in place of others (e.g., Camera) to mimic 
    functionality.

 */

// Standard Library
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <future>
#include <thread>
#include <pthread.h>
#include <sched.h>
#include <array>
#include <ctime>
#include <string>
#include <filesystem>
#include <fstream>

// Third-Party
#include "raylib.h"
#include "rlgl.h"
#include <cnpy.h>

#ifdef __APPLE__
    #include "../macos/vsync_timer.hpp"
#else
    #include "../linux/vsync_timer.hpp"
#endif

// Modules
#include "../s3/reportable.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "../s4/pencoder.hpp"
#include "../s2/quantize.hpp"
#include "../s2/von_mises.hpp"
#include "../s4/optimizer.hpp"
#include "../s4/model.hpp"
#include "../s4/slicer.hpp"
#include "shared.hpp"

// GL
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

moodycamel::ConcurrentQueue<torch::Tensor> e19_input_queue;
moodycamel::ConcurrentQueue<torch::Tensor> e19_camera_input_queue;

struct e19_Window {
    int Height, Width;
    int monitor;
    int fps;
    std::string title;
    int wmode;
    int fmode;
    bool window_open = false;

    void load () {}
    void close () {}

    e19_Window () {}

};

struct e19_Image_Processor {
    /* Slicer */
    s4_Slicer *slicer = nullptr;

    std::thread processing_thread;
    moodycamel::ConcurrentQueue<torch::Tensor> input_queue;
    moodycamel::ConcurrentQueue<torch::Tensor> output_queue;

    /* Synchronization mutexes */
    std::atomic<bool> end_thread {false};

    std::vector<torch::Tensor> rewards;
    int limit = 0;
    int count = 0;

    e19_Image_Processor () {
        /* start up processing thread */
        processing_thread = std::thread ([this]()->void{this->processor();});
    }

    void set_slicer (s4_Slicer *p_slicer) {
        slicer = p_slicer;
    }

    ~e19_Image_Processor () {
        end_thread.store(true, std::memory_order_release);
        processing_thread.join();
    }

    void process_tensor (torch::Tensor &t) {
        int64_t N = t.size(0);
        // Tensors are stored as {N, H, W}

        std::cout<<"INFO: [e19_Image_Processor] process_tensor, input shape=" << t.sizes () << '\n';
        std::cout<<"INFO: [e19_Image_processor] count="<<count<<'\n';
        
        // We want to apply the mask to each tensor
        auto predictions = slicer->detect(t);    // [1, 10]

        std::cout<<"INFO: [e19_Image_Processor] Prediction shape="<<predictions.sizes()<<'\n';

        // set the target to be always zero for now
        torch::Tensor targets = torch::zeros({1}, torch::kLong).to(t.device());
 
        torch::Tensor loss = torch::nn::functional::cross_entropy(
            predictions,
            targets,
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        );
        rewards.push_back(loss);

        ++count;
        std::cout<<"INFO: [e19_Image_processor] Processed image: " << count << '\n';

        if (count >= limit) {
            CompressAndPlaceOntoOutputQueue ();
            count = 0;
        }
    }

    void CompressAndPlaceOntoOutputQueue () {
        if (rewards.empty()) return;
        std::cout<<"INFO: [e19_Image_Processor] Stacking tensors...\n";
        torch::Tensor stacked_rewards = torch::stack(rewards);  // [Q, N]
        stacked_rewards = -stacked_rewards.view({-1});           // [Q * N]
        output_queue.enqueue(stacked_rewards);
        rewards.clear();
    }

    torch::Tensor get_rewards () {
        // get from output queue
        torch::Tensor output;
        while (!output_queue.try_dequeue(output));
        return output;
    }

    void processor () {
        while (!end_thread.load(std::memory_order_acquire)) {
            torch::Tensor t;
            if (!input_queue.try_dequeue(t)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }
            process_tensor(t);
        }
    }

    void SetSpitOutLimit (int p_limit) {
        limit = p_limit;
        std::cout<<"INFO: [e19_Image_Processor] Setting limit to: " << limit << '\n';
    }

    void collect (torch::Tensor t) {
        /* Add to input queue for processing thread to handle */
        input_queue.enqueue(t);
    }
};

struct e19_Dummy_Camera_Properties {
    int AcqFrameRate;
    int Height;
    int Width;
};

struct e19_Dummy_Camera {
    int counter = 0;

    e19_Dummy_Camera () {}

    torch::Tensor sread () {
        // std::cout<<"INFO: [e19_Dummy_Camera] sread () called.\n";
        /* read from e19_camera_input_queue */
        torch::Tensor t;
        while (!e19_camera_input_queue.try_dequeue(t));
        // std::cout<<"INFO: [e19_Dummy_Camera] sread: " << t.sizes() << '\n';

        // Apply IFFT2 and IFFTSHIFT as lens approximation

        return torch::abs(torch::fft::ifftshift(torch::fft::ifft2(t))).pow(2);
    }

    void close () {}
    void open () {}
};

struct e19_Dummy_VSYNC_timer {
    std::thread t;
    std::function<void(std::atomic<uint64_t>&)> f;
    std::atomic<bool> end_thread {false};
    std::atomic<uint64_t> vsync_counter {0};

    e19_Dummy_VSYNC_timer (int a, std::function<void(std::atomic<uint64_t>&)> b) {
        f = b;
        t = std::thread ([this]()->void {
            while (!this->end_thread.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                this->vsync_counter.fetch_add(1, std::memory_order_release);
                this->f(this->vsync_counter);
            }
        });
    }
    ~e19_Dummy_VSYNC_timer () {
        end_thread.store(true, std::memory_order_release);
        t.join();
    }

};

struct e19_Dummy_Serial {
    /* Serial calls camera directly */
    e19_Dummy_Camera *camera;

    e19_Dummy_Serial () {};
    e19_Dummy_Serial (const std::string &p, int br) {}

    void Signal () {
        /* obtain from input queue and place onto dummy camera's camera_input_queue */
        torch::Tensor t;    // [n, H, W]
        while (!e19_input_queue.try_dequeue(t));

        // std::cout<<"INFO: [e19_Dummy_Serial] Obtained tensor of shape: " << t.sizes() << " from e19_input_queue.\n";

        // split into n, [H, W] tensors and place onto queue

        int64_t N = t.size(0);
        for (int i = 0; i < N; ++i) {
            auto tt = t[i];
            e19_camera_input_queue.enqueue(tt);
            // std::cout<<"INFO: [e19_Dummy_Serial] Placing tensor slice " << i << " of shape " << tt.sizes() << " onto e19_camera_input_queue.\n";
        }
    }

    void Open () {}
    void Close () {}

    void set_port_name (const std::string &p) {}
    void set_baud_rate (int br) {}
};


// Camera read function
struct e19_Camera_Reader {
    // Init parameters
    e19_Image_Processor *processor = nullptr;

    // Camera Synchronization mutexes
    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::vector<uint64_t> vsync_timestamps;
    std::atomic<int64_t>  captures_pending {0};
    std::atomic<bool> enable_capture {false};

    // These classes handle
    // vsync firing.
    e19_Dummy_VSYNC_timer *mvt = nullptr;

    // These classes handle
    // communication to FPGA
    const std::string port_name = 
    #if defined(__linux__) 
        "/dev/ttyACM0";
    #elif defined(__APPLE__) 
        "/dev/tty.usbmodem8326898B1E1E1";
    #endif
    //Serial serial;
    e19_Dummy_Serial serial;

    // These classes handle
    // camera itself
    //s3_Camera_Reportable_Properties cam_properties;
    //s3_Camera_Reportable *camera = nullptr;
    e19_Dummy_Camera_Properties cam_properties;
    e19_Dummy_Camera *camera = nullptr;

    /* Capture thread */
    std::thread capture_thread;

    // Capture thread externals
    std::atomic<uint64_t> delta {0};
    std::atomic<uint64_t> image_count {0};

    /* Capture mutexes */
    std::atomic<bool> end_capture {false};

    /* Capture parameters */
    const int n_captures_per_frame = 20;

    s4_Slicer *slicer;

    e19_Camera_Reader (e19_Image_Processor *p_processor) :
    processor (p_processor) {

        cam_properties.AcqFrameRate = 1800;
        cam_properties.Height       = 240;
        cam_properties.Width        = 320;

        // Setup serial port to communicate
        // to FPGA (which handles communication between DLP/PLM + Camera)
        serial.set_port_name (port_name);
        serial.set_baud_rate(115200);
        serial.Open();

        // Setup callback to vsync pulses
        std::function<void(std::atomic<uint64_t>&)> timer = [this](std::atomic<uint64_t>&a)->void {
            this->schedule_camera_capture(a);
        };

        // Connect classes for to callback vsync pulses
        mvt = new e19_Dummy_VSYNC_timer (0, timer);

        // Setup image capture thread
        capture_thread = std::thread([this]()->void{this->capture_handler();});

        // setup slicer
        s4_Slicer_Region_Vector regions;
        float cx = cam_properties.Height/2, cy = cam_properties.Width/2, radius = 10;
        int pattern_radius = 96;

        // Creates pattern in a circular format
        for (int i = 0; i < 10; ++i) {
            float angle = 2 * M_PI * i / 10 + (9.0  * M_PI / 180.0);
            float x = cx + pattern_radius * std::cos(angle);
            float y = cy + pattern_radius * std::sin(angle);
            regions.push_back(std::make_shared<s4_Slicer_Circle>(y, x, radius));
        }

        slicer = new s4_Slicer (
          regions, cam_properties.Height, cam_properties.Width  
        );
        processor->set_slicer(slicer);
    }

    ~e19_Camera_Reader () {
        /* End capture thread */
        end_capture.store(true, std::memory_order_release);
        capture_thread.join();

        /* Close mvt */
        if (mvt) delete mvt;

        /* Close Serial Port */
        serial.Close ();

        /* Close camera */
        camera->close();
        if (camera) delete camera;

        if (slicer) delete slicer;
    }

    Texture TensorToTexture (torch::Tensor &t, float clamp_limit=0.01f) {
        // Assumes t is a 2D tensor [H, W]
        torch::Tensor t_cpu = t.detach().to(torch::kCPU).contiguous();

        int height = t_cpu.size(0);
        int width = t_cpu.size(1);

        // Clamp to [0, 1] and scale to 0–255, then convert to uint8
        
        /* Re orientate */
        t_cpu = t_cpu - t_cpu.min();

        if (clamp_limit <= 0) {
            t_cpu = t_cpu / t_cpu.max();
        }
        else {
            t_cpu = t_cpu.clamp(0.0, clamp_limit) / clamp_limit;
        }
        t_cpu = (t_cpu * 255).round().to(torch::kUInt8).contiguous();

        Image image = {
            .data = (void*)t_cpu.data_ptr(),
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
        };

        Texture tex = LoadTextureFromImage(image);
        return tex;
    }

    int64_t GetImageCount () {
        return image_count.load(std::memory_order_acquire);
    }

    //
    // These two functions make up the basis
    // for synchronizing the camera
    // with the frame rate 

    void send_ready_to_capture () {
        enable_capture.store(true, std::memory_order_release);
    }

    void wait_till_capture_enable_is_false () {
        while (enable_capture.load(std::memory_order_acquire));
    }

    uint64_t get_vsync_count () {
        return mvt->vsync_counter.load(std::memory_order_acquire);
    }

    bool HasCameraExceededTime_us (uint64_t time) {
        uint64_t d = delta.load(std::memory_order_acquire);

        if (d > 100'000'000)    /* improbable (just to handle the initial starting where delta >>>> 0) */
            return false;
        else if (d < time)
            return false;
        else
            return true;
    }

    /** 
     *  This function schedules the camera capture time
     *  to be inline with the vsync index. This ensures that
     *  we don't slowly drift out-of-sync with the vsync...
     * 
     */
    void schedule_camera_capture (std::atomic<uint64_t> &counter) {
        if (enable_capture.load(std::memory_order_acquire)) {
            vsync_timestamps.push_back (
                std::chrono::duration_cast<std::chrono::microseconds> (
                    std::chrono::high_resolution_clock::now().time_since_epoch ()
                ).count ()
            );

            capture_vsync_index.enqueue (
                static_cast<int64_t>(counter.load(std::memory_order_acquire))
            );

            serial.Signal();

            captures_pending.fetch_add(1, std::memory_order_release);
            enable_capture.store(false,std::memory_order_release);
        }
    }

    void capture_handler () {
        /* Opening code... */
        uint64_t prev_timestamp = 0;

        /* Capture loop */
        while (!end_capture.load(std::memory_order_acquire)) {
            int image_count_per_frame = 0;    /* This is per frame  */
            std::vector<torch::Tensor> tensors;

            /* If no images are pending, don't spend time waiting for images
               to come */
            if (captures_pending.load(std::memory_order_acquire) <= 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            // Capture n images per frame
            // as each frame consists of n bitplanes
            while (image_count_per_frame < n_captures_per_frame) {
                torch::Tensor image = camera->sread();
                tensors.push_back(image);
                

                // std::cout<<"INFO: [e19_Camera_Reader] Read bitplane: " << image_count_per_frame << '\n';
                ++image_count_per_frame;
                processor->collect(image);
            }

            image_count.fetch_add(1, std::memory_order_release);

            // tell system that a image has been removed from pending list
            captures_pending.fetch_sub(1, std::memory_order_release);

        }

        /* Closing code... */


    }

};


class e19_Model : public s4_Model {
public:
    e19_Model (int64_t Height, int64_t Width, int64_t n) :
    m_Height(Height), m_Width(Width), m_n(n) {
        m_parameter = torch::rand({Height, Width}).to(DEVICE);
        m_parameter.set_requires_grad(true);

        m_dist.set_mu(m_parameter, kappa);
        //m_dist.m_mu = m_parameter;
        //m_dist.m_kappa = torch::ones_like(m_parameter) * kappa;
        //m_dist.m_r = m_dist.m_rejection_r ();
    }

    ~e19_Model () override {}

    torch::Tensor sample () {
        torch::NoGradGuard no_grad;
        std::cout<<"INFO: [e19_Model] Sampling from distribution with " << m_n << " samples\n";
        m_action = m_dist.sample(m_n);
        return m_action;
    }

    torch::Tensor logp_action () override {
        return m_dist.log_prob(m_action);
    }

    std::vector<torch::Tensor> parameters () {
        return {m_parameter};
    }

    torch::Tensor &get_parameters () {
        return m_parameter;
    }

    int64_t N_samples () const override {
        return m_n;
    }

private:
    int64_t m_Height;
    int64_t m_Width;
    int64_t m_n;

    torch::Tensor m_parameter;
    torch::Tensor m_action;

    VonMises m_dist {};

    const double std = 5e-1;
    const double kappa = 1.0f/std;
};

struct e19_Checkpoint {
    torch::Tensor parameter;
    int64_t image_count;
};

// Scheduler
struct e19_Scheduler {
    /* This is the number of frames to draw after capturing */
    e19_Window *window = nullptr;
    e19_Camera_Reader *camera_reader = nullptr;
    int number_of_frames = 1;

    int frame_counter = 0;
    int tex_counter = 0;

    /* vsync counters */
    uint64_t prev_count;

    /* Vitals information */
    int64_t  batch_index = 0;

    /* Number of samples */
    static constexpr int n_captures_per_frame = 20;
    static constexpr int n_samples = 240;
    static constexpr int n_frames_for_n_samples = n_samples / n_captures_per_frame;

    std::array<torch::Tensor, n_frames_for_n_samples> textures;

    /* Model */
    e19_Model *model = nullptr;
    torch::optim::Adam *adam = nullptr;
    s4_Optimizer *opt = nullptr;

    PEncoder *pen = nullptr;

    uint64_t marker_1, marker_2;

    float power = 10;
    torch::Tensor rewards;

    e19_Scheduler (e19_Window *p_window, e19_Camera_Reader *p_camera_reader, int p_number_of_frames=1) :
    window(p_window),
    camera_reader(p_camera_reader),
    number_of_frames(p_number_of_frames) {
        model = new e19_Model (window->Height, window->Width, static_cast<int64_t>(n_samples));
        pen = new PEncoder (0, 0, window->Height, window->Width);
        adam = new torch::optim::Adam (model->parameters(), torch::optim::AdamOptions(0.1));
        opt = new s4_Optimizer (*adam, *model);

        // set reader
        camera_reader->processor->SetSpitOutLimit(n_samples);
    }

    ~e19_Scheduler () {
        if (model) delete model;
        if (pen) delete pen;
        if (adam) delete adam;
        if (opt) delete opt;
    }

    void UnloadTextures () {
        /*
        for (int i = 0; i < n_frames_for_n_samples; ++i) {
            UnloadTexture(textures[i]);
        }
        */
    }

    void ResetTextureCounter () {
        tex_counter = 0;
    }
    
    void SwapToTexture (int i) {
        //texture = &textures[i];
        e19_input_queue.enqueue(textures[i] * power);
        std::cout<<"INFO: [e19] Enqueued tensor of shape: " << textures[i].sizes() << " onto e19_input_queue\n";
    }

    //  Loads the parameters and accompanying data
    //
    e19_Checkpoint LoadCheckpoint (const std::filesystem::path &checkpoint_path) {
        e19_Checkpoint ckpt;

        torch::load(ckpt.parameter, checkpoint_path / "model.pt");
        std::ifstream loc_file(checkpoint_path / "loc.txt");
        loc_file >> ckpt.image_count;
        loc_file.close();
        return ckpt;
    }

    // Stores the parameters list as a pt so you
    // can load it later
    void StoreCheckpoint () {
        //
        // Create a unique name for the checkpoint based on the name
        std::time_t now = std::time(nullptr);
        std::tm *ltm = std::localtime(&now);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", ltm);
        std::string checkpoint_datetime = std::string(buffer);

        //
        // Create a folder with the timestamp name under CheckPoints/timestamp/...
        std::string checkpoint_path = "CheckPoints/" + checkpoint_datetime;
        std::filesystem::create_directories(checkpoint_path);

        //
        // Store vital information such as capture index (image_count) in a file called loc.txt
        int64_t image_count = camera_reader->GetImageCount ();
        std::ofstream loc_file(checkpoint_path + "/loc.txt");
        loc_file << image_count << std::endl;
        loc_file.close();


        //
        // Store the parameters of model
        auto &parameters = model->get_parameters();
        std::string param_path = checkpoint_path + "/model.pt";
        torch::save(parameters, param_path);
    }

    uint64_t GetCurrentTime_us () {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }

    void SetMarker1 () {
        marker_1 = GetCurrentTime_us ();
    }

    void SetMarker2 () {
        marker_2 = GetCurrentTime_us ();
    }

    void SwapMarkers () {
        uint64_t marker_t = marker_2;
        marker_2 = marker_1;
        marker_1 = marker_t;
    }

    /* Create Textures */
    void GenerateTextures () {
        tex_counter = 0;    /* reset counter */

        /* get actions */
        std::cout<<"INFO: [e19] Generating actions...\n";
        torch::Tensor actions = model->sample();
        std::cout<<"INFO: [e19] Actions finished generating...\n";

        /* encode actions into Textures using PEncoder */
        // split actions into [n_frames_for_n_samples, n_samples, H, W]
        auto splited = actions.view({n_frames_for_n_samples, n_captures_per_frame, window->Height, window->Width});

        // Apply exp(1j * splited)
        splited = torch::polar(torch::ones_like(splited), splited);  // keep as complex
        for (int i = 0; i < n_frames_for_n_samples; ++i) {
            textures[i] = splited[i];  // [n_captures_per_frame, H, W] as complex
        }
        
    }

    uint64_t get_vsync_count () {
        return camera_reader->get_vsync_count();
    }

    void wait_for_n_vsync_pulses (int n) {
        uint64_t current_count;
        while ((current_count = get_vsync_count()) - prev_count < n);
        prev_count = current_count;
    }

    void InitCapture () {
        camera_reader->wait_till_capture_enable_is_false();
        camera_reader->send_ready_to_capture();
    }

    void Update () {
        /* Obtain rewards from processor */
        auto *processor = camera_reader->processor;
        if (processor == nullptr) throw std::runtime_error("ERROR: [e19] processor is null.\n");

        std::cout<<"INFO: [e19] Obtaining rewards from processing thread.\n";
        rewards = processor->get_rewards ();

        opt->step(rewards);
    }

    void UpdateBatchIndex () {
        ++batch_index;
    }
};

int e19 () {
    Pylon::PylonAutoInitTerm init {};    

    /* Init Window */
    e19_Window window {};
    window.Height = 240;
    window.Width  = 320;

    window.fmode   = NO_TARGET_FPS;
    window.fps     = 30;
    window.monitor = 0;
    window.load();

    e19_Image_Processor processor {};
    std::cout<<"INFO: [e19] Loaded image processor\n";

    e19_Camera_Reader reader {&processor};
    std::cout<<"INFO: [e19] Loaded Camera Reader\n";

    e19_Scheduler scheduler {&window, &reader, 1};
    std::cout<<"INFO: [e19] Loaded System Scheduler\n";


    /** Actually init window */
    s3_Window swindow;
    swindow.Height = 240<<2;
    swindow.Width  = 320<<2;

    swindow.wmode   = WINDOWED;
    swindow.fmode   = NO_TARGET_FPS;
    swindow.fps     = 30;
    swindow.monitor = 0;
    swindow.load();


    int n_steps = 1000;   /* determines the number of frames */

    int step=0;
    while (!WindowShouldClose()) {
        scheduler.GenerateTextures();
        std::cout<<"INFO: [e19] Creating perturbations\n";

        for (int i = 0; i < scheduler.n_frames_for_n_samples; ++i) {
            std::cout<<"INFO: [e19] step: " << step + i << '\n';
            scheduler.SwapToTexture(i);
            scheduler.InitCapture();
        }
        std::cout<<"INFO: [e19] Calling Update...\n";
        scheduler.Update();
        auto average_rewards = scheduler.rewards.mean().item<double>();
        std::string reward_text = "Avg Reward: " + std::to_string(average_rewards);

        auto p = scheduler.model->get_parameters();
        
        //assert (p.grad().defined());
        //auto g = p.grad();

        p = torch::fft::ifftshift(torch::fft::ifft2(p)).abs().pow(2);
        Texture texture = scheduler.camera_reader->TensorToTexture(p, 0.001f);
        
        //Image img = scheduler.camera_reader->processor->slicer->visualize();
        //Texture texture = LoadTextureFromImage(img);

        /* Draw the image onto the screen for visualizing it */
        BeginDrawing ();
            DrawTexturePro (texture, 
                {0, 0, static_cast<float>(window.Width), static_cast<float>(window.Height)}, 
                {0, 0, static_cast<float>(swindow.Width), static_cast<float>(swindow.Height)},
                 {0, 0}, 0.0f, WHITE);
            DrawFPS(10,10);
            DrawText(reward_text.c_str(), 10, 30, 20, RAYWHITE);
        EndDrawing ();

        UnloadTexture(texture);
        step += scheduler.n_frames_for_n_samples;
    }

    return 0;
}