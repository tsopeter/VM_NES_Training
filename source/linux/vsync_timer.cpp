#include "vsync_timer.hpp"

#ifdef __linux__

glx_Vsync_timer::glx_Vsync_timer (Display *display, Window window, std::function<void()>&f)
: m_display(display), m_window(window), m_f(f) {

}

~glx_Vsync_timer::glx_Vsync_timer () {
    m_stop.store (true, std::memory_order_release);
    if (m_thread.joinable())
        m_thread.join();
}

void glx_Vsync_timer::thread_entry (Display *d, Window win, GLXContext ctx, void *userData) {
    auto *self = static_cast<glx_Vsync_timer*>(userData);

}

#endif
