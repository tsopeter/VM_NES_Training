#ifndef s3_window_hpp__
#define s3_window_hpp__

#include <iostream>
#include "raylib.h"                 // Library for simple Open GL drawing
#include "rlgl.h"
#include <OpenGL/gl.h>              // glFinish (maybe it's not necessary...)

#define PLATFORM_DESKTOP


// Make sures that GLSL version 3.30 is available on the platform before starting
#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

enum s3_Windowing_Mode {
    WINDOWED   = 0,
    BORDERLESS = 1
};

class s3_Window {
public:
    s3_Window();
    ~s3_Window();

    void load();


    int Height, Width;
    int monitor;
    int fps;
    std::string title;
    s3_Windowing_Mode wmode;
};

#endif
