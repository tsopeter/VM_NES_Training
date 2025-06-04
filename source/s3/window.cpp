#include "window.hpp"


s3_Window::s3_Window() {
    /**
     * Default values
     */
    Height  = 480;
    Width   = 640;
    monitor = 1;
    fps     = 60;
    title   = "Default Title";
    wmode   = WINDOWED;

    static_assert(GLSL_VERSION >= 330);
}

s3_Window::~s3_Window() {
    CloseWindow();
}

void s3_Window::load() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(Width, Height, title.c_str());
    SetWindowMonitor(monitor);
    if (wmode == BORDERLESS) ToggleBorderlessWindowed();
    SetTargetFPS(fps);
}