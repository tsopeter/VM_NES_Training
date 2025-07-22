#include "e18.hpp"

/**
    @author: Peter Tso
    @email: tsopeter@ku.edu


    For this simple task, we will train a single image to
    point to a certain location on the camera using
    Von Mises based Natural Evolution Strategies.

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

struct e18_Image_Processor {
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

    e18_Image_Processor () {
        /* start up processing thread */
        processing_thread = std::thread ([this]()->void{this->processor();});
    }

    void set_slicer (s4_Slicer *p_slicer) {
        slicer = p_slicer;
    }

    ~e18_Image_Processor () {
        end_thread.store(true, std::memory_order_release);
        processing_thread.join();
    }

    void process_tensor (torch::Tensor &t) {
        int64_t N = t.size(0);
        // Tensors are stored as {N, H, W}
        
        // We want to apply the mask to each tensor
        auto predictions = slicer->detect(t);    // [N, 10]

        // set the target to be always zero for now
        torch::Tensor target = torch::zeros({N}, t.options());

        // apply cross entropy loss
        auto loss = torch::nn::functional::cross_entropy(
            predictions,
            target.to(torch::kLong),
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        ); // [N]
        rewards.push_back(loss);

        ++count;
        if (count >= limit) {
            CompressAndPlaceOntoOutputQueue ();
            count = 0;
        }
    }

    void CompressAndPlaceOntoOutputQueue () {
        if (rewards.empty()) return;
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
    }

    void collect (torch::Tensor t) {
        /* Add to input queue for processing thread to handle */
        input_queue.enqueue(t);
    }
};

// Camera read function
struct e18_Camera_Reader {
    // Init parameters
    e18_Image_Processor *processor = nullptr;


    // Camera Synchronization mutexes
    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::vector<uint64_t> vsync_timestamps;
    std::atomic<int64_t>  captures_pending {0};
    std::atomic<bool> enable_capture {false};

    // These classes handle
    // vsync firing.
    #if defined(__linux__)
        glx_Vsync_timer *mvt = nullptr;
    #else
        macOS_Vsync_Timer *mvt = nullptr;
    #endif

    // These classes handle
    // communication to FPGA
    const std::string port_name = 
    #if defined(__linux__) 
        "/dev/ttyACM0";
    #elif defined(__APPLE__) 
        "/dev/tty.usbmodem8326898B1E1E1";
    #endif
    Serial serial;

    // These classes handle
    // camera itself
    s3_Camera_Reportable_Properties cam_properties;
    s3_Camera_Reportable *camera = nullptr;

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

    e18_Camera_Reader (e18_Image_Processor *p_processor) :
    processor (p_processor) {

        cam_properties.AcqFrameRate = 1800;
        cam_properties.Height       = 320;
        cam_properties.Width        = 240;

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
        #if defined(__linux__)
            mvt = new glx_Vsync_timer (0, timer);
        #else
            mvt = new macOS_Vsync_Timer (0, timer);
        #endif

        // Setup image capture thread
        capture_thread = std::thread([this]()->void{this->capture_handler();});

        // setup slicer
        s4_Slicer_Region_Vector regions;
        float cx = cam_properties.Height/2, cy = cam_properties.Width/2, radius = 10;
        int pattern_radius = 128;

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

    ~e18_Camera_Reader () {
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

    uint64_t get_camera_timestamps_us () {
        uint64_t timestamp;
        while (!camera->timestamps.try_dequeue(timestamp));
        return timestamp / 1'000;   /* ns -> us */
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

                ++image_count_per_frame;
            }
            uint64_t timestamp = get_camera_timestamps_us ();
            delta.store(timestamp - prev_timestamp, std::memory_order_release);
            prev_timestamp = timestamp;

            image_count.fetch_add(1, std::memory_order_release);

            // tell system that a image has been removed from pending list
            captures_pending.fetch_sub(1, std::memory_order_release);

            /* combine into a single tensor t of shape {n, 1, h, w} */
            torch::Tensor combined = torch::stack(tensors)      // {N, H, W}
                                .contiguous();                // Ensure memory layout

            /* Yield image processing to the processor as processing may interfere 
            with image collection */
            processor->collect(combined);


        }

        /* Closing code... */


    }

};


class e18_Model : public s4_Model {
public:
    e18_Model (int64_t Height, int64_t Width, int64_t n) :
    m_Height(Height), m_Width(Width), m_n(n) {
        m_parameter = (torch::rand({Height, Width}) * 2 * M_PI) - M_PI;
        m_parameter.set_requires_grad(true);

        m_dist.set_mu(m_parameter, kappa);
    }

    ~e18_Model () override {}

    torch::Tensor sample () {
        torch::NoGradGuard no_grad;
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

    const double std = 1e-1;
    const double kappa = 1.0f/std;
};

struct e18_Checkpoint {
    torch::Tensor parameter;
    int64_t image_count;
};

// Scheduler
struct e18_Scheduler {
    /* This is the number of frames to draw after capturing */
    s3_Window *window = nullptr;
    e18_Camera_Reader *camera_reader = nullptr;
    int number_of_frames = 1;

    int frame_counter = 0;
    int tex_counter = 0;
    Shader shader = LoadShader(nullptr, "source/shaders/alpha_ignore.fs");
    
    /* This is used when drawing to screen */
    /* To update texture, just point it at another texture object */
    Texture *texture;

    /* vsync counters */
    uint64_t prev_count;

    /* Vitals information */
    int64_t  batch_index = 0;

    /* Number of samples */
    static constexpr int n_captures_per_frame = 20;
    static constexpr int n_samples = 120;
    static constexpr int n_frames_for_n_samples = n_samples / n_captures_per_frame;

    std::array<Texture, n_frames_for_n_samples> textures;

    /* Model */
    e18_Model *model = nullptr;
    torch::optim::Adam *adam = nullptr;
    s4_Optimizer *opt = nullptr;

    PEncoder *pen = nullptr;

    uint64_t marker_1, marker_2;

    e18_Scheduler (s3_Window *p_window, e18_Camera_Reader *p_camera_reader, int p_number_of_frames=1) :
    window(p_window),
    camera_reader(p_camera_reader),
    number_of_frames(p_number_of_frames) {
        model = new e18_Model (window->Height, window->Width, static_cast<int64_t>(n_samples));
        pen = new PEncoder (0, 0, window->Height, window->Width);
        adam = new torch::optim::Adam (model->parameters(), torch::optim::AdamOptions(0.1));
        opt = new s4_Optimizer (*adam, *model);
    }

    ~e18_Scheduler () {
        if (model) delete model;
        if (pen) delete pen;
        if (adam) delete adam;
        if (opt) delete opt;
    }

    void UnloadTextures () {
        for (int i = 0; i < n_frames_for_n_samples; ++i) {
            UnloadTexture(textures[i]);
        }
    }

    void ResetTextureCounter () {
        tex_counter = 0;
    }
    
    void SwapToTexture (int i) {
        texture = &textures[i];
    }


    bool ExceededFrameTime () {
        return (marker_2 - marker_1) > 35'000;
    }

    bool ExceededCameraTime () {
        return camera_reader->HasCameraExceededTime_us (35'000);
    }

    //  Loads the parameters and accompanying data
    //
    e18_Checkpoint LoadCheckpoint (const std::filesystem::path &checkpoint_path) {
        e18_Checkpoint ckpt;

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
        torch::Tensor actions = model->sample();

        /* encode actions into Textures using PEncoder */
        // split actions into [n_frames_for_n_samples, n_samples, H, W]
        auto splited = actions.view({n_frames_for_n_samples, n_captures_per_frame, window->Height, window->Width});

        for (int i = 0; i < n_frames_for_n_samples; ++i) {
            auto frame = splited[i];    /* [n_samples, H, W] */

            auto timage = pen->MEncode_u8Tensor2(frame).contiguous();
            auto image  = pen->u8MTensor_Image(timage);
            Texture tex = LoadTextureFromImage(image);

            UnloadImage(image);
            textures[i] = tex;
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

    /**
     *  Called when drawing to screen.
     *  This should be called consistently, at every frame.
     *  Why?, If we deviate, we can result in improper timing, which we
     *  do not want... :<
     */
    void DrawTexture () {
        BeginDrawing ();
            BeginShaderMode (shader);
            ClearBackground (BLACK);
            DrawTexturePro (
                *texture,
                {0, 0, static_cast<float>(window->Width), static_cast<float>(window->Height)},
                {0, 0, static_cast<float>(window->Width), static_cast<float>(window->Height)},
                {0, 0},
                0.0f,
                WHITE
            );
            EndShaderMode ();
        EndDrawing ();
    }

    void Update () {
        /* Obtain rewards from processor */
        auto *processor = camera_reader->processor;
        torch::Tensor rewards = processor->get_rewards ();

        opt->step(rewards);
    }

    void UpdateBatchIndex () {
        ++batch_index;
    }
};

int e18 () {
    Pylon::PylonAutoInitTerm init {};    

    /* Init Window */
    s3_Window window {};
    window.Height = 1600;
    window.Width  = 2560;

    #ifdef __APPLE__
        window.wmode = BORDERLESS;
    #else
        window.wmode = FULLSCREEN;
    #endif

    window.fmode   = NO_TARGET_FPS;
    window.fps     = 30;
    window.monitor = 0;
    window.load();

    e18_Image_Processor processor {};
    e18_Camera_Reader reader {&processor};
    e18_Scheduler scheduler {&window, &reader, 1};

    int n_steps = 10'000;   /* determines the number of frames */

    scheduler.SetMarker1(); /* Set marker 1 time */
    for (int step = 0; step < n_steps; step += scheduler.n_frames_for_n_samples) {
        scheduler.GenerateTextures();

        // Display each texture to screen
        if (step == 0)
            scheduler.prev_count = scheduler.get_vsync_count ();
        for (int i = 0; i < scheduler.n_frames_for_n_samples; ++i) {
            scheduler.InitCapture();
            scheduler.SwapToTexture(i);
            scheduler.DrawTexture();
            scheduler.wait_for_n_vsync_pulses(1);
            scheduler.SetMarker2();

            if (scheduler.ExceededCameraTime() || scheduler.ExceededFrameTime()) {
                /**/
                scheduler.StoreCheckpoint();  /* Store checkpoint of important metadata */
            }
            scheduler.SwapMarkers();
        }

        scheduler.UnloadTextures();
        scheduler.Update();
    }

    return 0;
}