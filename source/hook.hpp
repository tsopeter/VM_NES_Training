#ifndef hook_hpp__
#define hook_hpp__

#include "s5/scheduler2.hpp"
#include "s5/vib.hpp"

class Additional_Hooks {
public:
    Additional_Hooks (Scheduler2* sched=nullptr);
    ~Additional_Hooks ();
    void SetScheduler (Scheduler2*);

    void SubTextureHook ();
    void SetHooks ();

private:
    Scheduler2* scheduler;
    Vibration vibration;
};

#endif