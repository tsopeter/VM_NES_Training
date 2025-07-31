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
#include "../s2/dist.hpp"
#include "../s4/optimizer.hpp"
#include "../s4/model.hpp"
#include "../s4/slicer.hpp"
#include "../s3/IP.hpp"
#include "../utils/utils.hpp"
#include "shared.hpp"

// GL
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

torch::Tensor e18_gs_algorithm(const torch::Tensor &target, int iterations);

class e18_Normal : public Dist {
public:
    e18_Normal () {}

    e18_Normal (torch::Tensor &mu, double std) :
    m_mu(mu), m_std(std) {}

    ~e18_Normal () override {}

    torch::Tensor sample (int n) override {
        torch::NoGradGuard no_grad;
        
        // Broadcast m_mu to match the sample shape
        auto mu_shape = m_mu.sizes();
        auto sample_shape = torch::IntArrayRef({n}).vec();
        sample_shape.insert(sample_shape.end(), mu_shape.begin(), mu_shape.end());

        torch::Tensor eps = torch::randn(sample_shape, m_mu.options());
        return m_mu.unsqueeze(0).expand_as(eps) + m_std * eps;
    }

    torch::Tensor log_prob(torch::Tensor &t) override {
        // Compute log probability of t under Normal(m_mu, m_std)
        auto var = m_std * m_std;
        auto log_scale = std::log(m_std);
        auto log_probs = -0.5 * ((t - m_mu).pow(2) / var + 2 * log_scale + std::log(2 * M_PI));
        return log_probs;
    }

    void set_mu (torch::Tensor &mu, double kappa) {
        m_mu = mu;
        m_std = 1.0f/kappa;
    }



    torch::Tensor m_mu;
    double m_std;


};

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
        if (!slicer) {
            throw std::runtime_error ("Slicer not defined in process.\n");
        }


        int64_t N = t.size(0);
        // Tensors are stored as {N, H, W}
        
        // We want to apply the mask to each tensor
        auto predictions = slicer->detect(t).to(torch::kFloat32);    // [N, 10]

        // set the target to be always zero for now
        torch::Tensor targets = torch::zeros({1}, torch::kLong).to(t.device());

        // apply cross entropy loss
        auto loss = torch::nn::functional::cross_entropy(
            predictions,
            targets,
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        ); // [1]
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
            t=t.to(DEVICE); /* pass onto GPU */
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

    s4_Slicer *slicer = nullptr;

    std::function<void(std::atomic<uint64_t>&)> timer;
    std::function<void()> capture_function;

    bool capture_function_started = false;

    moodycamel::ConcurrentQueue<torch::Tensor> images;

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
        timer = [this](std::atomic<uint64_t>&a)->void {
            this->schedule_camera_capture(a);
        };

        // Connect classes for to callback vsync pulses
        
        #if defined(__linux__)
            mvt = new glx_Vsync_timer (0, timer);
        #else
            mvt = new macOS_Vsync_Timer (0, timer);
        #endif
        
        camera = new s3_Camera_Reportable (cam_properties, mvt);
        camera->open();
        camera->start();


        // Setup image capture thread
        capture_function = [this]()->void{this->capture_handler();};
        capture_thread = std::thread(capture_function);
        capture_function_started = true;

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
        if (capture_function_started)
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

    uint64_t CameraDelta () {
        return delta.load(std::memory_order_acquire);
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
            std::cout<<"INFO: [e18_Camera_Reader::schedule_camera_capture] Sending trigger signal...\n";
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

        std::cout<<"INFO: [e18_Camera_Reader::capture_handler] Init'd.\n";

        /* Capture loop */
        while (!end_capture.load(std::memory_order_acquire)) {
            int image_count_per_frame = 0;    /* This is per frame  */

            /* If no images are pending, don't spend time waiting for images
               to come */
            if (captures_pending.load(std::memory_order_acquire) <= 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            // Capture n images per frame
            // as each frame consists of n bitplanes
            std::cout<<"INFO: [e18_Camera_Reader::capture_handler] Reading images...\n";
            while (image_count_per_frame < n_captures_per_frame) {
                torch::Tensor image = camera->sread();

                /* For the first image, store onto images queue */
                if (image_count_per_frame==0)
                    images.enqueue(image);

                ++image_count_per_frame;
                processor->collect(image);
            }
            uint64_t timestamp = get_camera_timestamps_us ();
            uint64_t delta_z = timestamp - prev_timestamp;
            delta.store(delta_z, std::memory_order_release);
            prev_timestamp = timestamp;

            image_count.fetch_add(1, std::memory_order_release);

            // tell system that a image has been removed from pending list
            captures_pending.fetch_sub(1, std::memory_order_release);

        }

        /* Closing code... */


    }

};


class e18_Model : public s4_Model {
public:

    e18_Model () {} /* Default constructor */

    e18_Model (int64_t Height, int64_t Width, int64_t n) :
    m_Height(Height), m_Width(Width), m_n(n) {
        init (Height, Width, n);
    }

    void init (int64_t Height, int64_t Width, int64_t n) {
        m_Height = Height;
        m_Width  = Width;
        m_n      = n;


        std::cout<<"INFO: [e18_Model] Staging model...\n";
        std::cout<<"INFO: [e18_Model] Height: " << Height << ", Width: " << Width << ", Number of Perturbations (samples): " << n << '\n';

       // m_parameter = torch::rand({Height, Width}).to(DEVICE) * 2 * M_PI - M_PI;  /* Why does placing m_parameter on CUDA cause segmentation fault */

        torch::Tensor target = torch::zeros({Height, Width});

        // Crop the target to 8x8 and ensure float32
        target.index_put_({torch::indexing::Slice(0, Height/8), torch::indexing::Slice(0, Width/8)}, 1.0);


        m_parameter = e18_gs_algorithm(target, 100).to(DEVICE);
        m_parameter.set_requires_grad(true);
        std::cout<<"INFO: [e18_Model] Set parameters...\n";

        m_dist.set_mu(m_parameter, kappa);
        std::cout<<"INFO: [e18_Model] Created distribution...\n";
    }

    ~e18_Model () override {}

    torch::Tensor sample(int n) {
        torch::NoGradGuard no_grad;
        torch::Tensor action = m_dist.sample(n);
        m_action_s.push_back(action);
        return action;
    }

    void squash () {
        if (m_action_s.empty()) return;

        m_action = torch::cat(m_action_s, 0);  // [k * n, H, W]
        m_action_s.clear();
    }

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

    int64_t m_Height;
    int64_t m_Width;
    int64_t m_n;

    torch::Tensor m_parameter;
    torch::Tensor m_action;

    //VonMises m_dist {};
    e18_Normal m_dist {};

    const double std = 1e-1;
    const double kappa = 1.0f/std;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
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
    Texture texture;

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
    e18_Model model {};
    torch::optim::Adam *adam = nullptr;
    s4_Optimizer *opt = nullptr;
    Quantize quant {};

    PEncoder *pen = nullptr;

    uint64_t marker_1, marker_2;

    int64_t model_Height, model_Width;

    e18_Scheduler (s3_Window *p_window, e18_Camera_Reader *p_camera_reader, int64_t p_model_Height, int64_t p_model_Width, int p_number_of_frames=1) :
    window(p_window),
    camera_reader(p_camera_reader),
    model_Height(p_model_Height),
    model_Width(p_model_Width),
    number_of_frames(p_number_of_frames) {
        std::cout<<"INFO: [e18_Scheduler] Starting scheduler...\n";

        model.init(model_Height, model_Width, static_cast<int64_t>(n_samples));
        std::cout<<"INFO: [e18_Scheduler] e18_Model created on stack.\n";

        pen = new PEncoder (0, 0, window->Height, window->Width);
        std::cout<<"INFO: [e18_Scheduler] PEncoder created on heap.\n";

        adam = new torch::optim::Adam (model.parameters(), torch::optim::AdamOptions(0.1));
        std::cout<<"INFO: [e18_Scheduler] Adam Optimizer created on heap.\n";

        opt = new s4_Optimizer (*adam, model);
        std::cout<<"INFO: [e18_Scheduler] s4_Optimizer created on heap.\n";

        p_camera_reader->processor->SetSpitOutLimit(n_samples);
    }

    ~e18_Scheduler () {
        //if (model) delete model;
        if (pen) delete pen;
        if (adam) delete adam;
        if (opt) delete opt;
    }

    void UnloadTextures () {
        auto t1 = GetCurrentTime_us();
        for (int i = 0; i < n_frames_for_n_samples; ++i) {
            UnloadTexture(textures[i]);
        }
        auto t2 = GetCurrentTime_us();
        std::cout << "INFO: [e18_Scheduler::UnloadTextures] Took " << (t2 - t1) << " us\n";
    }

    void ResetTextureCounter () {
        tex_counter = 0;
    }
    
    void SwapToTexture (int i) {
        texture = textures[i];
    }


    bool ExceededFrameTime () {
        return FrameTimeDelta() > 35'000;
    }

    uint64_t FrameTimeDelta () {
        return (marker_2 - marker_1);
    }

    bool ExceededCameraTime () {
        return camera_reader->HasCameraExceededTime_us (35'000);
    }

    void InitPBO () {
        std::cout<<"INFO: [e18_Scheduler::InitPBO] Called...\n";
        pen->init_pbo();    /* initialize pbo if not already have */
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
        auto &parameters = model.get_parameters();
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

    void Squash () {
        model.squash();
    }

    void SwapMarkers () {
        uint64_t marker_t = marker_2;
        marker_2 = marker_1;
        marker_1 = marker_t;
    }

    void GenerateTextures_Sequentially () {
        static int64_t gts_count = 0;
        std::cout << "INFO: [e18_Scheduler::GenerateTextures_Sequentially] Called...\n";
        auto t1 = GetCurrentTime_us ();

        torch::Tensor action = model.sample(n_captures_per_frame);
        //torch::Tensor action = model.get_parameters().unsqueeze(0).repeat({20, 1, 1});
        auto t3 = GetCurrentTime_us ();
        Utils::SynchronizeCUDADevices();
        std::cout << "INFO: [e18_Scheduler::GenerateTextures_Sequentially] Sampling... Took: " << (t3 - t1) << " us\n";
 
        // Save model.get_parameters() as tensor.pt
        torch::save({model.get_parameters().cpu()}, "tensor.pt");

        //auto timage = pen->MEncode_u8Tensor2(action).contiguous();
        auto timage = pen->MEncode_u8Tensor3(action).contiguous().to(torch::kInt32);
        Utils::SynchronizeCUDADevices();


        auto t4 = GetCurrentTime_us ();
        std::cout << "INFO: [e18_Scheduler::GenerateTextures_Sequentially] Encoding... Took: " << (t4 - t3) << " us\n";
        
        texture = pen->u8Tensor_Texture(timage);    /* Reuse same texture id, basically... */
        auto t5 = GetCurrentTime_us ();
        std::cout << "INFO: [e18_Scheduler::GenerateTextures_Sequentially] Generating Textures... Took: " << (t5 - t4) << " us\n";

        // Save texture as an Image
        

        auto t2 = GetCurrentTime_us ();
        std::cout << "INFO: [e18_Scheduler::GenerateTextures_Sequentially] Took: " << (t2 - t1) << " us\n";

        if (gts_count==0) {
            // Export texture as Image to disk
            Image image = LoadImageFromTexture(texture);
            ExportImage(image, "example_gpu.png");
            UnloadImage(image);

            timage = timage.to(torch::kInt32).cpu().contiguous() & 0x00FF;
            timage = timage.to(torch::kUInt8);
            
            auto t = s4_Utils::TensorToImage(timage);
            ExportImage(t, "example_cpu.png");

            ++gts_count;
        }
    }

    void Squentially_Unload () {
        UnloadTexture (texture);
    }

    /* Create Textures */
    void GenerateTextures () {
        auto t1 = GetCurrentTime_us();
        tex_counter = 0;    /* reset counter */

        /* get actions */
        auto t3 = GetCurrentTime_us();
        torch::Tensor actions = model.sample();
        auto t4 = GetCurrentTime_us();

        std::cout << "INFO: [e18_Scheduler::GenerateTextures] Sampling took " << (t4 - t3) << " us\n";

        // Reshape and interpolate actions outside the loop
        auto t5 = GetCurrentTime_us ();
        auto reshaped = actions.view({n_frames_for_n_samples, n_captures_per_frame, model_Height, model_Width});
        auto splited = torch::nn::functional::interpolate(
            reshaped,
            torch::nn::functional::InterpolateFuncOptions()
                .size(std::vector<int64_t>({window->Height/2, window->Width/2}))
                .mode(torch::kNearest)
        );
        auto t6 = GetCurrentTime_us();
        std::cout << "INFO: [e18_Scheduler::GenerateTextures] Resizing took " << (t6 - t5) << " us\n";

        for (int i = 0; i < n_frames_for_n_samples; ++i) {
            auto t7 = GetCurrentTime_us();
            const auto &frame = splited[i];    /* [n_samples, H, W] */

            auto timage = pen->MEncode_u8Tensor2(frame).contiguous();
            auto t10 = GetCurrentTime_us();
            //auto image  = pen->u8MTensor_Image(timage);
            Texture tex = pen->u8Tensor_Texture(timage);
            auto t11 = GetCurrentTime_us();
            //Texture tex = LoadTextureFromImage(image);

            //UnloadImage(image);
            textures[i] = tex;
            auto t8 = GetCurrentTime_us();
            std::cout<<"INFO: [e18_Scheduler::GenerateTextuers] Mapping phase -> bit representation took " << (t10 - t7) << " us\n";
            std::cout<<"INFO: [e18_Scheduler::GenerateTextuers] Mapping from Tensor -> Image took " << (t11 - t10) << " us\n";
            std::cout<<"INFO: [e18_Scheduler::GenerateTextuers] Loading Image to Texture took " << (t8 - t11) << " us\n";
            std::cout<<"INFO: [e18_Scheduler::GenerateTextuers] Mapping from Tensor -> Texture took " << (t8 - t7) << " us\n";
        }
        auto t2 = GetCurrentTime_us();
        std::cout<<"INFO: [e18_Scheduler::GenerateTextures] Took " << (t2 - t1) << " us\n";
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
        std::cout<<"INFO: [e18_Scheduler::DrawTexture] Called...\n";
        BeginDrawing ();
            BeginShaderMode (shader);
            ClearBackground (BLACK);
            DrawTexturePro (
                texture,
                {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                {0, 0, static_cast<float>(window->Width), static_cast<float>(window->Height)},
                {0, 0},
                0.0f,
                WHITE
            );
            EndShaderMode ();
        EndDrawing ();
    }

    double Update () {
// Timing 
        auto t1 = GetCurrentTime_us();

        /* Obtain rewards from processor */
        auto *processor = camera_reader->processor;
        torch::Tensor rewards = processor->get_rewards ();
        double total_rewards = rewards.mean().item<double>();

        opt->step(rewards);

        auto t2 = GetCurrentTime_us();
        std::cout<<"INFO: [e18_Scheduler::Update] Took " << (t2 - t1) << " us\n";

        return total_rewards;
    }

    void UpdateBatchIndex () {
        ++batch_index;
    }
};

int e18 () {
    Pylon::PylonAutoInitTerm init {};    

    s3_IP_Client client {"192.168.193.20", 9001};

    while (!client.is_connected()) {
        client.connect();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

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
    std::cout<<"INFO: [e18] e18_Image_Processor init'd\n";

    e18_Camera_Reader reader {&processor};
    std::cout<<"INFO: [e18] e18_Camera_Reader init'd\n";
    
    e18_Scheduler scheduler {&window, &reader, 200*2, 320*2, 1};
    //e18_Scheduler scheduler {&window, &reader, 250, 400, 1};
    std::cout<<"INFO: [e18] e18_Scheduler init'd\n";

    int64_t step=0;
    torch::Tensor t;

    scheduler.SetMarker1(); /* Set marker 1 time */
    scheduler.InitPBO ();
    while (!WindowShouldClose()) {
        //scheduler.GenerateTextures();

        // Display each texture to screen
        if (step == 0)
            scheduler.prev_count = scheduler.get_vsync_count ();
        for (int i = 0; i < scheduler.n_frames_for_n_samples; ++i) {
            scheduler.InitCapture();
            //scheduler.SwapToTexture(i);
            scheduler.GenerateTextures_Sequentially ();
            scheduler.DrawTexture();
            scheduler.wait_for_n_vsync_pulses(1);
            scheduler.SetMarker2();

            std::cout << "INFO: [e18] Frame Time Delta: " << scheduler.FrameTimeDelta () << " us\n";
            std::cout << "INFO: [e18] Capture Time Delta: " << reader.CameraDelta() << " us\n";

            if (scheduler.ExceededCameraTime() || scheduler.ExceededFrameTime()) {
                /**/
                //scheduler.StoreCheckpoint();  /* Store checkpoint of important metadata */
            }
            scheduler.SwapMarkers();
            ++step;
            while(!reader.images.try_dequeue(t));

        }

        //scheduler.UnloadTextures();
        t = t.cpu();
        t = t.contiguous().view(-1);

        scheduler.Squash();
        Utils::data_structure *ds = new Utils::data_structure();
        
        ds->iteration=step,
        ds->total_rewards=0;//scheduler.Update(),

        client.Transmit((void*)(ds), sizeof(*ds));

        client.Transmit((void*)(t.data_ptr<uint8_t>()), t.numel());

        delete ds;  /* Remove ds from heap */

        // Save texture as a Image
        //TakeScreenshot("texture.png");
        //break;
    }

    Utils::data_structure ds {
        .iteration = -1,
        .total_rewards = 0
    };
    client.Transmit((void*)(&ds), sizeof (ds));
    client.disconnect();

    return 0;
}

torch::Tensor e18_gs_algorithm(const torch::Tensor &target, int iterations) {
    using namespace torch::indexing;

    // Ensure target is float32
    torch::Tensor Target = target.to(torch::kFloat32);

    int Height = Target.size(0);
    int Width  = Target.size(1);

    // Define phase-normalization function
    auto phase = [](const torch::Tensor& p) {
        return p / (p.abs() + 1e-8);
    };

    // Initialize with random phase
    auto rand_phase = torch::rand({Height, Width}, torch::kFloat32) * 2 * M_PI;
    rand_phase = rand_phase.to(Target.device());
    auto Object = torch::polar(torch::ones_like(rand_phase), rand_phase); // magnitude 1, phase = rand_phase

    for (int i = 0; i < iterations; ++i) {
        auto U  = torch::fft::ifft2(torch::fft::ifftshift(Object));
        auto Up = Target * phase(U);
        auto D  = torch::fft::fft2(torch::fft::fftshift(Up));
        Object  = phase(D);
    }

    return torch::angle(Object);
}
