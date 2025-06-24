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
    fmode   = NO_TARGET_FPS;

    static_assert(GLSL_VERSION >= 330);
}

s3_Window::~s3_Window() {
    close();
}

void s3_Window::close () {
    if (window_open) {
        CloseWindow();
        window_open = false;
    }
}

void s3_Window::load() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE);
    InitWindow(Width, Height, title.c_str());
    SetWindowMonitor(monitor);
    if (wmode == BORDERLESS) ToggleBorderlessWindowed();
    if (fmode == SET_TARGET_FPS)
        SetTargetFPS(fps);
    window_open = true;
}