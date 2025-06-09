#include "e4.hpp"

#include <iostream>
#include "raylib.h"

#include "../s3/virtual.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "shared.hpp"

int internal_trigger (s3_Virtual_Camera&);

int e4 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.wmode   = WINDOWED;
    window.monitor = 0;
    window.load();

    /* Create Simple Camera */
    s3_Virtual_Camera_Properties vp;
    s3_Virtual_Camera camera {vp};
    camera.open();

    torch::Tensor result = torch::empty({});

    camera.start();
    int64_t frame_count=0;
    int64_t image_count=0;
    while (!WindowShouldClose()) {
        //auto texture = examples::createTextureFromFrameNumber(frame_count, window.Height, window.Width);

        /* Drawing loop */
        BeginDrawing();
        ClearBackground(BLACK);
        //DrawTextureEx(texture, {0, 0}, 0, 1.0, WHITE);
        DrawFPS(10, 10);
        EndDrawing();

        internal_trigger(camera);
        while (image_count < 24) {
            torch::Tensor t = camera.read().squeeze();
            if (t.numel()!=0)
                image_count++;
        }
        image_count=0;
        
        std::cout<<"INFO: [e4] Frame "<<++frame_count<<'\n';

        //UnloadTexture(texture);
    }

    camera.close();
}

int internal_trigger (s3_Virtual_Camera &camera) {
    camera.camera.ExecuteSoftwareTrigger();
    return 0;
}