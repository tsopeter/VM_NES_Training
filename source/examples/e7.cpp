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

    const int64_t n_bits = 24;
    camera.start();
    while (!WindowShouldClose()) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();

        int64_t image_count = 0;
        double first_read_delay = 0;
        auto signal_time = std::chrono::high_resolution_clock::now();
        serial.Signal();    // signal

        while (image_count < n_bits) {
            camera.sread();
            ++image_count;
            if (image_count == 1) {
                auto first_read_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = first_read_time - signal_time;
                first_read_delay = elapsed.count();
            }
        }
        
        auto frame_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> total_elapsed = frame_end - frame_start;

        // Print timing info
        std::cout << "INFO: [e7] First Read Delay: " << first_read_delay * 1e6 << " us\n";
        std::cout << "INFO: [e7] Frame Rate: " << total_elapsed * 1e3 << " ms\n";
    }
    
    serial.Close();
    camera.close();
    return 0;
}