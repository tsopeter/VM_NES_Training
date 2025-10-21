#ifndef s3_window_hpp__
#define s3_window_hpp__

#include <iostream>
#include "raylib.h"                 // Library for simple Open GL drawing
#include "rlgl.h"

#define PLATFORM_DESKTOP


// Make sures that GLSL version 3.30 is available on the platform before starting
#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

enum s3_Windowing_Mode {
    WINDOWED   = 0,
    BORDERLESS = 1,
    FULLSCREEN = 2,
    NONE       = 3
};

enum s3_TargetFPS_Mode {
    SET_TARGET_FPS = 0,
    NO_TARGET_FPS  = 1
};

class s3_Window {
public:
    s3_Window();
    ~s3_Window();

    void load();
    void close();

    int Height, Width;
    int monitor;
    int fps;
    std::string title;
    s3_Windowing_Mode wmode;
    s3_TargetFPS_Mode fmode;

    bool window_open = false;
};

#endif
