#ifndef linux_vsync_timer_hpp__
#define linux_vsync_timer_hpp__

#ifdef __linux__
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <thread>
#include <atomic>
#include <functional>

class glx_Vsync_timer {
public:
    glx_Vsync_timer (Display*, Window, std::function<void()>&);
    ~glx_Vsync_timer ();
    
    std::atomic<uint64_t> vsync_counter {0};
    std::atomic<bool> vsync_ready {false};
private:
    static void thread_entry (
        Display*,
        Window,
        GLXContext,
        void *userData
    );

    Display *m_display = nullptr;
    Window   m_window  = 0;
    std::function<void()> &m_f;
    std::thread m_thread;
    std::atomic<bool> m_stop {false};

};
#endif

#endif
