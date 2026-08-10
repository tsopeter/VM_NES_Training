#include <iostream>
#include "runner.hpp"
#include "s5/helpers.hpp"
#include "s2/dataloader.hpp"

void test ();

int main (int argc, char** argv) {
    std::string config_file = "config.txt";
    Runner r;


    // If training mode, the argument is structured as ./app <config>
    if (argc == 2) {
        std::string arg1 = argv[1];
        if (arg1 == "--test") {
            // Test code
            test ();

        }
        else {
            config_file = argv[1];
            r.Run (config_file);
        }
    }
    else {
        // Invalid arguments
        std::cerr << "Usage for training:  " << argv[0] << " <config>\n";
        return -1;
    }

    return 0;
}       

void test () {
    Scheduler2 scheduler;
    scheduler.Start (
        0,
        1600,
        2716,
        s3_Windowing_Mode::FULLSCREEN,
        s3_TargetFPS_Mode::NO_TARGET_FPS,
        30,

        0.0f, // Delay_us
        10,   // Average_N
        24,   // Burst_N
        "192.168.2.10",
        8000,
        1600, // PEncoder_Height
        2716, // PEncoder_Width
        32,   // Num_Levels
        PLM_Device_Enum::NIR,

        nullptr // opt
    );

    scheduler.SetRange(0.5); // Set range to 0.5 for testing

    // Random phase between 0 and 2*pi for (24, 1600, 2716)
    auto random_phase = torch::rand({24, 1600, 2716}) * 2 * M_PI;

    scheduler.SetTextureFromTensor(random_phase);
    std::cout << "INFO: [test] Displayed random phase texture with range 0.5.\n";

    while (!WindowShouldClose()) {
        scheduler.DrawTextureToScreen();
    }
}