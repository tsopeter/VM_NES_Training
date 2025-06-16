#ifndef macos_vsync_timer_hpp__
#define macos_vsync_timer_hpp__

#include <functional>
#ifdef __APPLE__

#ifdef interface
#pragma push_macro("interface")
#undef interface
#endif

// Apple headers that use `interface` as a field name
#include <CoreVideo/CoreVideo.h>
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/hidsystem/IOHIDTypes.h> // or similar

#ifdef interface
#pragma pop_macro("interface")
#endif

class macOS_Vsync_Timer {
public:
    macOS_Vsync_Timer (int,std::function<void()>&);
    ~macOS_Vsync_Timer();

    std::atomic<uint64_t> vsync_counter {0};
    std::atomic<bool>     vsync_ready {false};
private:
    int displ;
    std::function<void()> &m_f;
    CGDirectDisplayID id;
    CVDisplayLinkRef dispLink;

    static CVReturn callback(CVDisplayLinkRef,
                       const CVTimeStamp*,
                       const CVTimeStamp*,
                       CVOptionFlags,
                       CVOptionFlags*,
                       void* userInfo);

    CGDirectDisplayID getDisplayByIndex(int);
};

#endif



#endif
