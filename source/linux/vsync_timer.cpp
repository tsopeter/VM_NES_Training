#include "vsync_timer.hpp"

#ifdef __linux__
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <cstring>
#include <iostream>

typedef int (*PFNGLXWAITVIDEOSYNCSGI)(int, int, unsigned int*);
static PFNGLXWAITVIDEOSYNCSGI glXWaitVideoSyncSGI = nullptr;

glx_Vsync_timer::glx_Vsync_timer (Display *display, Window window, std::function<void(std::atomic<uint64_t>&)>&f)
: m_display(display), m_window(window), m_f(f) {
    std::cout<<"INFO: [glx_Vsync_timer] Getting GLX support information\n";
    const char *extensions = glXQueryExtensionsString(m_display, DefaultScreen(m_display));
    if (!extensions || !strstr(extensions, "GLX_SGI_video_sync"))
        throw std::runtime_error("glx_Vsync_timer: GLX_SGI_video_sync not supported");

    std::cout<<"INFO: [glx_Vsync_timer] Getting visual information\n";
    XVisualInfo* vi = glXChooseVisual(display, DefaultScreen(display), nullptr);
    if (!vi)
        throw std::runtime_error("glx_Vsync_timer: Failed to choose visual for dummy context");

    std::cout<<"INFO: [glx_Vsync_timer] Creating context\n";
    GLXContext ctx = glXCreateContext(display, vi, nullptr, True);
    if (!ctx)
        throw std::runtime_error("glx_Vsync_timer: Failed to create dummy GLX context");

    std::cout<<"INFO: [glx_Vsync_timer] Creating dummy\n";
    Window dummyWin = XCreateSimpleWindow(display, RootWindow(display, vi->screen),
                                          0, 0, 1, 1, 0, 0, 0);

    // Runtime lel
    std::cout<<"INFO: [glx_Vsync_timer] Obtaining glXWaitVideoSyncSGI\n";
    glXWaitVideoSyncSGI = reinterpret_cast<PFNGLXWAITVIDEOSYNCSGI>(
        glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXWaitVideoSyncSGI"))
    );

    if (!glXWaitVideoSyncSGI)
        throw std::runtime_error("Failed to load glXWaitVideoSyncSGI");

    std::cout<<"INFO: [glx_Vsync_timer] Launching thread...\n";
    m_thread = std::thread(thread_entry, display, dummyWin, ctx, this);
}

glx_Vsync_timer::~glx_Vsync_timer () {
    m_stop.store (true, std::memory_order_release);
    if (m_thread.joinable())
        m_thread.join();
}

void glx_Vsync_timer::thread_entry(Display *d, Window win, GLXContext ctx, void *userData) {
    std::cout<<"INFO: [glx_Vsync_timer] Thread started.\n";
    if (!glXMakeCurrent(d, win, ctx)) {
        std::cerr << "ERROR: glXMakeCurrent failed\n";
        return;
    }

    auto *self = static_cast<glx_Vsync_timer*>(userData);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (!self->m_stop.load(std::memory_order_acquire)) {
        GLuint retraceCount = 0;
        if (glXWaitVideoSyncSGI(2, 0, &retraceCount) == 0) {
            std::cout<<"INFO: [glx_Vsync_timer] VSYNC signal detected.\n";
            self->vsync_counter.fetch_add(1, std::memory_order_acq_rel);
            self->vsync_ready.store(true, std::memory_order_release);
            self->m_f(self->vsync_counter);
        } else {
            usleep(100); /* 10 us */
        }
    }
}

Display *glx_Vsync_timer::XOpenDisplay_alias(const char *ptr) {
    return XOpenDisplay(ptr);
}

Window glx_Vsync_timer::glxGetCurrentDrawable_alias () {
    return glXGetCurrentDrawable();
}

#endif
