#include "vsync_timer.hpp"

#ifdef __linux__

glx_Vsync_timer::glx_Vsync_timer (Display *display, Window window, std::function<void()>&f)
: m_display(display), m_window(window), m_f(f) {
    GLXContext ctx = glXGetCurrentContext();
    m_thread = std::thread(thread_entry, display, window, ctx, this);
}

glx_Vsync_timer::~glx_Vsync_timer () {
    m_stop.store (true, std::memory_order_release);
    if (m_thread.joinable())
        m_thread.join();
}

void glx_Vsync_timer::thread_entry(Display *d, Window win, GLXContext ctx, void *userData) {
    glXMakeCurrent(d, win, ctx);
    auto *self = static_cast<glx_Vsync_timer*>(userData);

    while (!self->m_stop.load(std::memory_order_acquire)) {
        GLuint retraceCount;
        if (glXWaitVideoSyncSGI(2, 0, &retraceCount) == 0) {
            self->vsync_counter.fetch_add(1, std::memory_order_acq_rel);
            self->vsync_ready.store(true, std::memory_order_release);
            self->m_f(self->vsync_counter);
        } else {
            usleep(1000);
        }
    }
}

#endif
