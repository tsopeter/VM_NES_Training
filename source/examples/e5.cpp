#include "e5.hpp"

#include <iostream>
#include "raylib.h"

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"

int e5 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.wmode   = WINDOWED;
    window.monitor = 0;
    window.load();

    /* Create Camera */
    s3_Camera_Properties cam_properties;
    s3_Camera camera {cam_properties};
    camera.open();

    torch::Tensor result = torch::empty({});

    camera.start();
    int64_t frame_count=0;
    while (!WindowShouldClose()) {
        /* Drawing loop */
        BeginDrawing();
        ClearBackground(BLACK);
        //DrawTextureEx(texture, {0, 0}, 0, 1.0, WHITE);
        DrawFPS(10, 10);
        EndDrawing();

        /* Triggered Externally by hardware (PLM or DLP) */
        while (camera.count == 0) {
            result = camera.read().squeeze();
        }
        camera.count = 0;
        std::cout<<"INFO: [e5] Frame "<<++frame_count<<'\n';
    }

    camera.close();
}