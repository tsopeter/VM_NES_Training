#include "e6.hpp"

#include <iostream>
#include "raylib.h"

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "shared.hpp"
#include <OpenGL/gl.h>


int e6 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.monitor = 1;
    window.load();

    /* Create Camera */
    s3_Camera_Properties cam_properties;
    cam_properties.AcqFrameRate = 2000;
    cam_properties.Height       = 320;
    cam_properties.Width        = 240;

    s3_Camera camera {cam_properties};
    camera.open();

    torch::Tensor result = torch::empty({});

    camera.disable();
    camera.start();
    camera.disable();
    int64_t frame_count=0;
    int64_t image_count=0;
    int64_t total_number_of_images=0;
    const int64_t num_images=10;
    
    while (!WindowShouldClose()) {
        //auto texture = examples::createTextureFromFrameNumber(frame_count, window.Height, window.Width);
        /* Drawing loop */
        BeginDrawing();
        ClearBackground(BLACK);
        //DrawTextureEx(texture, {0, 0}, 0, 1.0, WHITE);
        DrawFPS(10, 10);
        EndDrawing();

        std::vector<torch::Tensor> images;

        /* Triggered Externally by hardware (PLM or DLP) */
        glFinish();
        camera.enable();
        while (image_count<num_images) {
            torch::Tensor image = camera.read().squeeze();
            if (image.numel() != 0) {
                images.push_back(image);
                ++image_count;
            }
        }
        camera.disable();
        total_number_of_images += image_count;
        image_count=0;

        std::cout<<"INFO: [e6] Number of Images already captuerd: "<<camera.count<<'\n';
        break;

        std::cout<<"INFO: [e6] Images captured: "<<images.size()<<'\n';
        //Image image = TensorToTiledImage(images, window.Height, window.Width);
        //Texture texture = LoadTextureFromImage(image);

        float ft = GetFrameTime();
        std::cout<<"INFO: [e6] Frame "<<++frame_count<<'\n';
        std::cout<<"INFO: [e6] Total images: "<<total_number_of_images<<'\n';
        std::cout<<"INFO: [e6] Frametime: "<<ft*1e3<<" ms\n";
        std::cout<<"INFO: [e6] Frame Rate: "<<(1/ft)<<'\n';

        //UnloadImage(image);
        //UnloadTexture(texture);
    }

    camera.close();
}
