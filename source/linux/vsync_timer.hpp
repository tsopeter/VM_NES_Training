#ifndef linux_vsync_timer_hpp__
#define linux_vsync_timer_hpp__

#ifdef __linux__
#include <thread>
#include <atomic>
#include <functional>

// Forward declare X11 types
typedef struct _XDisplay Display;
typedef unsigned long Window;
typedef struct __GLXcontextRec *GLXContext;

class glx_Vsync_timer {
public:
    glx_Vsync_timer (Display*, Window, std::function<void(std::atomic<uint64_t>&)>&);
    ~glx_Vsync_timer ();

    static Display *XOpenDisplay_alias(const char*);
    static Window   glxGetCurrentDrawable_alias ();
    
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
    std::function<void(std::atomic<uint64_t>&)> &m_f;
    std::thread m_thread;
    std::atomic<bool> m_stop {false};

};
#endif

#endif