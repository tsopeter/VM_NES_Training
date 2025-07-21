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

// Camera read function
struct e18_Camera_Reader {
    // Camera Synchronization mutexes
    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::vector<uint64_t> vsync_timestamps;
    std::atomic<int64_t>  captures_pending {0};

    // These classes handle
    // vsync firing.
    #if defined(__linux__)
        glx_Vsync_timer *mvt = nullptr;
    #else
        macOS_Vsync_Timer *mvt = nullptr;
    #endif

    e18_Camera_Reader () {


        #if defined(__linux__)
            mvt = glx_Vsync_timer (0, )
        #else

        #endif
    }

    ~e18_Camera_Reader () {
        if (mvt) delete mvt;
    }

    /** 
     *  This function schedules the camera capture time
     *  to be inline with the vsync index. This ensures that
     *  we don't slowly drift out-of-sync with the vsync...
     * 
     */
    void schedule_camera_capture () {
        if (enable_capture)
    }









};

// Scheduler
struct e18_Scheduler {
    /**
     *  Called every so-often. Ideally, this should
     *  take next to no time, as to not disturb the system.
     * 
     */
    void UpdateMask () {

    }

    /**
     *  Draws the image to screen
     *  Ideally, this will be called every frame
     */
    void DrawToScreen () {




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
    Shader ignoreAlphaShader = LoadShader(nullptr, "source/shaders/alpha_ignore.fs");

    const char serial_port[] = 
    #if defined(__linux__) 
        "/dev/ttyACM0";
    #elif defined(__APPLE__) 
        "/dev/tty.usbmodem8326898B1E1E1";
    #endif
    Serial serial {serial_port, 115200 /* Baud rate */};



    return 0;
}