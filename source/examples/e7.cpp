#include "e7.hpp"

#include <iostream>
#include "raylib.h"
#include <cnpy.h>

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "shared.hpp"
#include <OpenGL/gl.h>
#include <thread>
#include <atomic>

#define save_data(d,f) {                                    \
    cnpy::npy_save(f,d,"w");                                \
    std::cout<<"INFO: [e7] Saved: " << f << " to disk.\n";  \
}                                                           \

int e7 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.monitor = 1;
    window.load();

    /* Serial connection */
    /* Serial is used to trigger the FPGA board to allow
       synchronization between camera and DLP/PLM */
    Serial serial {"/dev/tty.usbmodem8326898B1E1E1", 115200};
    serial.Open();

    /* Create Camera */
    s3_Camera_Properties cam_properties;
    cam_properties.AcqFrameRate = 1800;
    cam_properties.Height       = 320;
    cam_properties.Width        = 240;
    s3_Camera camera {cam_properties};
    camera.open();

    /* atomic */
    std::atomic<bool> capture_complete = false;
    std::atomic<bool> exit_thread      = false;

    const int64_t n_bits = 24;
    int64_t frame_index  = 0;

    std::vector<double> delays;
    std::vector<double> capture_times;
    std::vector<double> frame_times;

    camera.start();
    while (!WindowShouldClose()) {
        auto frame_start = std::chrono::high_resolution_clock::now();
  
        BeginDrawing();
        ClearBackground(BLACK);
        serial.Signal();
        EndDrawing();
        glFinish();

        int64_t image_count = 0;
        double first_read_delay = 0;
        auto signal_time = std::chrono::high_resolution_clock::now();

        while (image_count < n_bits) {
            camera.sread();
            ++image_count;
            if (image_count == 1) {
                auto first_read_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = first_read_time - signal_time;
                first_read_delay = elapsed.count();
            }
        }
        capture_complete.store(true);   /* tell handler thread to start counting down */

        auto capture_end = std::chrono::high_resolution_clock::now();

        auto frame_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> total_elapsed = frame_end - frame_start;
        std::chrono::duration<double> capture_time  = capture_end - signal_time;

        // Print timing info
        std::cout << "INFO: [e7] First Read Delay: " << first_read_delay * 1e6 << " us\n";
        std::cout << "INFO: [e7] Capture Time: " << capture_time * 1e6 << " us\n";
        std::cout << "INFO: [e7] Frame Rate: " << total_elapsed * 1e3 << " ms\n";

        delays.push_back(first_read_delay * 1e6);
        capture_times.push_back(capture_time.count() * 1e6);
        frame_times.push_back(total_elapsed.count() * 1e6);

        ++frame_index;
    }

    std::cout<<"INFO: [e7]: Amount of data recorderd: " << delays.size()<<'\n';
    save_data(delays, "signal_delay.npy");
    save_data(capture_times, "capture.npy");
    save_data(frame_times, "frame.npy");
    
    serial.Close();
    camera.close();

    return 0;
}