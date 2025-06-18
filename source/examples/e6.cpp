#include "e6.hpp"

#include <iostream>
#include "raylib.h"

#include "../s3/reportable.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "shared.hpp"

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif


int e6 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.monitor = 1;
    window.fmode   = SET_TARGET_FPS; /* Rely on VSYNC only */
    window.fps     = 60;
    window.load();

    /* Serial connection */
    /* Serial is used to trigger the FPGA board to allow
       synchronization between camera and DLP/PLM */
    Serial serial {"/dev/tty.usbmodem8326898B1E1E1", 115200};
    serial.Open();

    /* Create Camera */
    s3_Camera_Reportable_Properties cam_properties;
    cam_properties.AcqFrameRate = 2000;
    cam_properties.Height       = 320;
    cam_properties.Width        = 240;

    s3_Camera_Reportable camera {cam_properties};
    camera.open();

    torch::Tensor result = torch::empty({});

    int64_t frame_count=0;
    int64_t image_count=0;
    int64_t total_number_of_images=0;
    const int64_t num_images=24;
    
    camera.start();
    camera.enable();

    /* Having each module separated into blocks 
       make determining time alot easier */

    /* Signal block */
    std::function<void()> signal_ = [&serial]()->void {
        serial.Signal();
    };

    /* Draw block */
    std::function<void()> draw_ = []()->void {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(10, 10);
        EndDrawing();
    };

    /* Capture block */
    std::function<void()> capture_ = [&camera, &num_images, &total_number_of_images]()->void {
        int64_t image_count=0;
        std::vector<torch::Tensor> images;
        while (image_count<num_images) {
            torch::Tensor image = camera.sread().squeeze();
            images.push_back(image);
            ++image_count;
        }
        total_number_of_images += image_count;
    };

    /* Report block */
    std::function<void()> report_ = [&frame_count, &total_number_of_images]()->void {
        float ft = GetFrameTime();
        std::cout<<"INFO: [e6] Frame "<<++frame_count<<'\n';
        std::cout<<"INFO: [e6] Total images: "<<total_number_of_images<<'\n';
        std::cout<<"INFO: [e6] Frametime: "<<ft*1e3<<" ms\n";
        std::cout<<"INFO: [e6] Frame Rate: "<<(1/ft)<<'\n';
    };

    while (!WindowShouldClose()) {
        examples::report_timer(draw_, "Draw");
        //examples::print_current_time_us();
        examples::report_timer(signal_, "Serial");
        examples::report_timer(capture_, "Capture");
        examples::report_timer(report_);
        std::cout<<"INFO: [e6] Camera captured a total of: "<<camera.count<<" images (internal buffer report)\n";
    }

    camera.close();
    serial.Close();
}
