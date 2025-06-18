#include "vsync_timer.hpp"

#include <iostream>
#include <atomic>
#include <cstdint>


#ifdef __APPLE__

CGDirectDisplayID macOS_Vsync_Timer::getDisplayByIndex(int index) {
    uint32_t displayCount = 0;
    CGGetActiveDisplayList(0, nullptr, &displayCount);

    if (displayCount == 0 || index >= displayCount) {
        return kCGNullDirectDisplay;  // Invalid
    }

    CGDirectDisplayID displays[displayCount];
    CGGetActiveDisplayList(displayCount, displays, &displayCount);
    return displays[index];
}

macOS_Vsync_Timer::macOS_Vsync_Timer (int display, 
    std::function<void(std::atomic<uint64_t>&)> &f) :
displ(display), m_f(f) {
    std::cout<<"INFO: [macOS_Vsync_Timer] Getting dislay...\n";
    id = getDisplayByIndex(displ);
    dispLink = nullptr;

    if (id == kCGNullDirectDisplay)
        throw std::runtime_error(
            "macOS_Vsync_Timer:: No display available.\n"
        );
    
    std::cout<<"INFO: [macOS_Vsync_Timer] Creating dislay link...\n";
    if (CVDisplayLinkCreateWithCGDisplay(id, &dispLink) != kCVReturnSuccess)
        throw std::runtime_error(
            "macOS_Vsync_Timer:: Unable to create CVDisplayLink\n"
        );
    
    std::cout<<"INFO: [macOS_Vsync_Timer] Creating callbacks...\n";
    CVDisplayLinkSetOutputCallback(dispLink, &macOS_Vsync_Timer::callback, this);
    CVDisplayLinkStart(dispLink);
}

macOS_Vsync_Timer::~macOS_Vsync_Timer () {
    CVDisplayLinkStop(dispLink);
    CVDisplayLinkRelease(dispLink);
}

CVReturn macOS_Vsync_Timer::callback(CVDisplayLinkRef,
                       const CVTimeStamp*,
                       const CVTimeStamp*,
                       CVOptionFlags,
                       CVOptionFlags*,
                       void* userInfo) {
    auto* self = static_cast<macOS_Vsync_Timer*>(userInfo);
    self->vsync_counter.fetch_add(1, std::memory_order_release);
    self->vsync_ready.store(true, std::memory_order_release);
    self->m_f(self->vsync_counter);
    return kCVReturnSuccess;
}

#endif
