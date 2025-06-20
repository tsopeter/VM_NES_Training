#include "e13.hpp"

#include <iostream>
#include "raylib.h"
#include <cnpy.h>
#include <unistd.h>

#include "../linux/vsync_timer.hpp"

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "shared.hpp"

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include <chrono>
#include <future>
#include <thread>

int e13 () {
    /* Initialize screen */
    s3_Window window {};
    window.Height  = 480;
    window.Width   = 640;
    window.wmode   = WINDOWED;
    window.fmode   = NO_TARGET_FPS;
    //window.fps     = 60;
    window.monitor = 0;
    window.load();

    
    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::atomic<int64_t> capture_pending {0};
    std::atomic<bool> enable_capture {false};
    std::function<void(std::atomic<uint64_t>&)> timer = [&enable_capture, &capture_pending, &capture_vsync_index](std::atomic<uint64_t>&counter) {
        if (enable_capture.load(std::memory_order_acquire)) {
            capture_vsync_index.enqueue(static_cast<int64_t>(counter.load(std::memory_order_acquire)));
            capture_pending.fetch_add(1,std::memory_order_release);
            enable_capture.store(false,std::memory_order_release);
        }
    };

    /*
    std::cout<<"INFO: [e13] Starting timer...\n";
    Display* dpy = glx_Vsync_timer::XOpenDisplay_alias(":0");
    if (!dpy) throw std::runtime_error("Failed to open X display.");
    Window win = glx_Vsync_timer::glxGetCurrentDrawable_alias();

    glx_Vsync_timer mvt(dpy, win, timer);
    */
    glx_Vsync_timer mvt(window.monitor, timer);

    int64_t frame_counter=0;
    uint64_t vsync_count=0;
    while (!WindowShouldClose()) {
        vsync_count = mvt.vsync_counter.load(std::memory_order_acquire);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(10,10);
        DrawText(TextFormat("VSYNC count: %llu", vsync_count), 10, 30, 20, RAYWHITE);
        DrawText(TextFormat("Frame count: %lld", frame_counter), 10, 50, 20, RAYWHITE);
        EndDrawing();
        ++frame_counter;
    }

    return 0;
}