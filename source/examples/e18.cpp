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
#include "shared.hpp"

// GL
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

struct e18_Image_Processor {
    e18_Image_Processor () {

    }

    void collect (torch::Tensor t) {

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

    /* Capture mutexes */
    std::atomic<bool> end_capture {false};

    /* Capture parameters */
    const int n_captures_per_frame = 20;


    e18_Camera_Reader (e18_Image_Processor *p_processor) :
    processor (p_processor) {
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
            mvt = new macOS_Vsync_Timer (window.monitor, timer);
        #endif

        // Setup image capture thread
        std::function<void()> capture_function = [this]()->void {
            this->capture_handler();
        };
        capture_thread = std::thread(
            capture_function
        );

    }

    ~e18_Camera_Reader () {
        /* Close mvt */
        if (mvt) delete mvt;

        /* Close Serial Port */
        serial.Close ();

        /* Close camera */
        camera->close();
        if (camera) delete camera;
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



        int image_count = 0;    /* This is entire system  */

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

            ++image_count;

            // tell system that a image has been removed from pending list
            captures_pending.fetch_sub(1, std::memory_order_release);

            /* combine into a single tensor t of shape {n, 1, h, w} */
            torch::Tensor combined = torch::stack(tensors)      // {N, H, W}
                                .unsqueeze(1)                 // {N, 1, H, W}
                                .contiguous();                // Ensure memory layout

            /* Yield image processing to the processor as processing may interfere 
            with image collection */
            processor->collect(combined);


        }

        /* Closing code... */


    }

};

// Scheduler
struct e18_Scheduler {
    /* This is the number of frames to draw after capturing */
    s3_Window *window = nullptr;
    e18_Camera_Reader *camera_reader = nullptr;
    int number_of_frames = 1;

    int frame_counter = 0;
    Shader shader = LoadShader(nullptr, "source/shaders/alpha_ignore.fs");
    
    /* This is used when drawing to screen */
    /* To update texture, just point it at another texture object */
    Texture *texture;


    e18_Scheduler (s3_Window *p_window, e18_Camera_Reader *p_camera_reader, int p_number_of_frames=1) :
    window(p_window),
    camera_reader(p_camera_reader),
    number_of_frames(p_number_of_frames) {

    }

    /**
     *  Called every so-often. Ideally, this should
     *  take next to no time, as to not disturb the system.
     * 
     */
    void UpdateMask () {

    }

    void InitCapture () {
        /* If not ready, then wait till the required number of 
           frames has passed */
        if (frame_counter % number_of_frames != 0) return;

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

    /**
     *  Update function *should* be called consistently, at every frame
     */
    void Update () {
        InitCapture (); /* Captures Image by telling Camera to start capturing */
        DrawTexture (); /* Draws image to screen */
        ++frame_counter;
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
    return 0;
}