#include "scheduler2.hpp"
#include <filesystem>
#include <fstream>

Scheduler2::Scheduler2 () {

}

Scheduler2::~Scheduler2 () {
    if (pen) delete pen;
    if (mvt) delete mvt;

    std::cout << "INFO: [Scheduler2::~Scheduler2] Scheduler2 destroyed, resources cleaned up.\n";

}

void Scheduler2::Start (
    int Monitor,
    int Height,
    int Width,
    s3_Windowing_Mode wmode,
    s3_TargetFPS_Mode fmode,
    int FPS,

    double Delay_us,
    int Average_N,
    int Burst_N,
    std::string Host_IP,
    int Host_Port,

    int PEncoder_Height,
    int PEncoder_Width,
    int Num_Levels,
    PLM_Device_Enum plm_device_enum,

    s4_Optimizer *opt
) {
    // Setup Windowing
    window.Height  = Height;
    window.Width   = Width;
    window.monitor = Monitor;
    window.wmode   = wmode;
    window.fmode   = fmode;
    window.fps     = FPS;

    // Setup FPGA
    adc.set_delay(Delay_us);
    adc.set_average_n(Average_N);
    adc.set_burst_n(Burst_N);

    // Setup Optimizer
    optimizer = opt;


    // Start up window
    window.load();
    
    // Start up FPGA
    adc.Set_IP_Address(Host_IP);
    adc.Set_Port(Host_Port);
    adc.start();
    //adc.spin_up_collection();


    // Setup PEncoder
    pen = new PEncoder(
        PEncoder_Height, 
        PEncoder_Width, 
        Num_Levels,
        plm_device_enum
    );
    pen->init_pbo();
    
    // Setup VSYNC timer
    timer_callback = [this](std::atomic<uint64_t> &arg) {
        this->schedule_fpga_capture(arg);
    };
    mvt = new sched2VSYNCtimer(0, timer_callback);

}

//////////////////////////////////////////////////////////////////
// VSYNC
void Scheduler2::schedule_fpga_capture(std::atomic<uint64_t> &counter) {
    m_vsync_count.store(
        counter.load(std::memory_order_acquire),
        std::memory_order_release
    );

    if (!enable_fpga.load(std::memory_order_acquire)) {
        return;
    }

    adc.trigger();

    captures_pending.fetch_add(1, std::memory_order_release);
    enable_fpga.store(false, std::memory_order_release);
}
