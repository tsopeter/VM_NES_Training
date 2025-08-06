#include "e23.hpp"

// Standard Library
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <future>
#include <thread>
#include <pthread.h>
#include <sched.h>
#include <array>
#include <ctime>
#include <string>
#include <filesystem>
#include <fstream>

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
#include "../s2/dataloader.hpp"
#include "../s3/reportable.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "../s4/pencoder.hpp"
#include "../s2/quantize.hpp"
#include "../s2/von_mises.hpp"
#include "../s2/dist.hpp"
#include "../s4/optimizer.hpp"
#include "../s4/model.hpp"
#include "../s4/slicer.hpp"
#include "../s3/IP.hpp"
#include "../utils/utils.hpp"
#include "../utils/comms.hpp"
#include "../device.hpp"
#include "shared.hpp"

// GL
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

class e23_Scheduler {
public:

    e23_Scheduler () {

    }
};

int e23 () {
    Pylon::PylonAutoInitTerm init {};

    // Initialize screen
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.fmode   = NO_TARGET_FPS;
    window.monitor = 0;
    window.load();




    return 0;
}