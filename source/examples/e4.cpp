#include "e4.hpp"

#include <iostream>
#include "raylib.h"

#include "../s3/virtual.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"

int internal_trigger (s3_Virtual_Camera&);

int e4 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.wmode   = WINDOWED;
    window.monitor = 0;
    window.load();

    /* Create Camera */
    s3_Virtual_Camera_Properties cam_properties;
    s3_Virtual_Camera camera {cam_properties};
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

        internal_trigger(camera);
        while (camera.count == 0) {
            result = camera.read().squeeze();
        }
        camera.count = 0;
        std::cout<<"INFO: [e4] Frame "<<++frame_count<<'\n';
    }

    camera.close();
}

int internal_trigger (s3_Virtual_Camera &camera) {
    camera.camera.ExecuteSoftwareTrigger();
    return 0;
}