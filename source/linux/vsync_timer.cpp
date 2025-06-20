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

typedef int (*PFNGLXGETVIDEOSYNCSGI)(unsigned int*);
static PFNGLXGETVIDEOSYNCSGI glXGetVideoSyncSGI = nullptr;

glx_Vsync_timer::glx_Vsync_timer (Display *display, Window window, std::function<void(std::atomic<uint64_t>&)>&f)
: m_display(display), m_window(window), m_f(f) {
    init_thread ();
}

glx_Vsync_timer::glx_Vsync_timer (int win, std::function<void(std::atomic<uint64_t>&)>& f)
: m_f(f) {
    if (win < 0 || win >= 10)
        throw std::runtime_error("INFO: [glx_Vsync_timer] We only support displays 0-9.\n");
    char display_alias[] = ":0";
    display_alias[1]=win+'0';
    std::cout<<"INFO: [glx_Vsync_timer] Connecting to monitor: "<<display_alias<<'\n';
    m_display = glx_Vsync_timer::XOpenDisplay_alias(display_alias);
    if (!m_display) throw std::runtime_error("Failed to open X display.");
    m_window = glx_Vsync_timer::glxGetCurrentDrawable_alias();
    init_thread ();
}

glx_Vsync_timer::~glx_Vsync_timer () {
    m_stop.store (true, std::memory_order_release);
    if (m_thread.joinable())
        m_thread.join();
    std::cout<<"[glx_Vsync_timer] Polling thread closed.\n";
}

void glx_Vsync_timer::init_thread () {
    int attrList[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };

    /**
        Makes sure that the graphics card supports GLX_SGI_video_sync extensions, which
        is required for synchronizing to the video sync signal. There doesn't seem
        to be any alternative...
     */
    std::cout<<"INFO: [glx_Vsync_timer] Getting GLX support information\n";
    const char *extensions = glXQueryExtensionsString(m_display, DefaultScreen(m_display));
    if (!extensions || !strstr(extensions, "GLX_SGI_video_sync"))
        throw std::runtime_error("glx_Vsync_timer: GLX_SGI_video_sync not supported");

    std::cout<<"INFO: [glx_Vsync_timer] Getting visual information\n";
    XVisualInfo* vi = glXChooseVisual(m_display, DefaultScreen(m_display), attrList);

    if (!vi)
        throw std::runtime_error("glx_Vsync_timer: Failed to choose visual for dummy context");

    std::cout<<"INFO: [glx_Vsync_timer] Creating context\n";
    GLXContext ctx = glXCreateContext(m_display, vi, nullptr, True);
    if (!ctx)
        throw std::runtime_error("glx_Vsync_timer: Failed to create dummy GLX context");

    std::cout<<"INFO: [glx_Vsync_timer] Creating dummy\n";
    Window dummyWin = XCreateSimpleWindow(m_display, RootWindow(m_display, vi->screen),
                                          0, 0, 1, 1, 0, 0, 0);

    /**
        A little hacky to get the glXWait/Get functions. We search the process for the functions
        and link it to our function points to expose it...

        Kinda crap, and wished they properly exposed it.
     */
    // Get Wait Video Sync
    std::cout<<"INFO: [glx_Vsync_timer] Obtaining glXWaitVideoSyncSGI\n";
    glXWaitVideoSyncSGI = reinterpret_cast<PFNGLXWAITVIDEOSYNCSGI>(
        glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXWaitVideoSyncSGI"))
    );

    if (!glXWaitVideoSyncSGI)
        throw std::runtime_error("Failed to load glXWaitVideoSyncSGI");

    // Get Get Video Sync
    std::cout<<"INFO: [glx_Vsync_timer] Obtaining glXGetVideoSyncSGI\n";
    glXGetVideoSyncSGI = reinterpret_cast<PFNGLXGETVIDEOSYNCSGI>(
        glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXGetVideoSyncSGI"))
    );

    if (!glXGetVideoSyncSGI)
        throw std::runtime_error("Failed to load glXGetVideoSyncSGI");

    std::cout<<"INFO: [glx_Vsync_timer] Launching thread...\n";
    m_thread = std::thread(thread_entry, m_display, dummyWin, ctx, this);
}

void glx_Vsync_timer::thread_entry(Display *d, Window win, GLXContext ctx, void *userData) {
    std::cout<<"INFO: [glx_Vsync_timer] Thread started.\n";
    if (!glXMakeCurrent(d, win, ctx)) {
        std::cerr << "ERROR: glXMakeCurrent failed\n";
        return;
    }

    auto *self = static_cast<glx_Vsync_timer*>(userData);

    GLuint count;
    while (!self->m_stop.load(std::memory_order_acquire)) {
        if (glXWaitVideoSyncSGI(1, 0, &count) == 0) {
            self->vsync_counter.fetch_add(1, std::memory_order_acq_rel);
            self->vsync_ready.store(true, std::memory_order_release);
            self->m_f(self->vsync_counter);
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
