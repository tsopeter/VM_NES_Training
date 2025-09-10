#include "e26.hpp"

#include <torch/torch.h>

#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s5/scheduler2.hpp"
#include "../s5/hcomms.hpp"
#include "../utils/utils.hpp"
#include "../s4/utils.hpp"
#include "../s2/dataloader.hpp"
#include "../s2/von_mises.hpp"
#include <fstream>
#include <ostream>


namespace e26_global {
    // Random indicies for placing
    // phase mask in one of the 20 bit planes
    std::vector<int64_t> indicies = {
        5, 12, 3, 17, 8, 
        1, 14, 19, 6, 10, 
        0, 16, 9, 2, 13, 
        18, 7, 15, 4, 11
    };

    std::atomic<bool> kill_program {false};

    std::atomic<int64_t> start_time {0};
    std::atomic<int64_t> frame_count {0};
    std::atomic<int64_t> image_count {0};
}

std::pair<torch::Tensor, bool> e26_ProcessFunction (CaptureData ts) {
    static bool first = true;
    static int count = 0;
    static int indexes = 0;
    static double sums[20] = {};
    static int kills = 5;

    // sum up full
    double full_sum = ts.full.sum().to(torch::kFloat64).item<double>();

    // store into count
    sums[count] = full_sum;
    ++count;

    if (count == 20) {
        // find argmax of sums
        int max_index = 0;
        double max_value = sums[0];
        for (int i = 1; i < 20; ++i) {
            if (sums[i] > max_value) {
                max_value = sums[i];
                max_index = i;
            }
        }

        // Check if argmax is same as indicies[indexes]
        if (max_index != e26_global::indicies[indexes]) {
            std::cout << "Mismatch! Expected: " << e26_global::indicies[indexes] << ", Got: " << max_index << "\n";

            // Print out sums
            std::cout << "Sums: ";
            for (int i = 0; i < 20; ++i) {
                std::cout << sums[i] << " ";
            }
            std::cout << "\n";

            // kill program
            --kills;
            if (kills <= 0)
                e26_global::kill_program.store(true, std::memory_order_release);   
        }
        else {
            std::cout << "Match! Index: " << max_index << "\n";
        }
        indexes = (indexes + 1) % 20;
        count = 0;
    }

    if (first) {
        e26_global::start_time.store(Utils::GetCurrentTime_us(), std::memory_order_release);
        std::cout << "Start time recorded.\n";
    }

    // increment image count
    e26_global::image_count.fetch_add(1, std::memory_order_release);

    first = false;
    return {{}, false};
}

int e26 () {
    int mask_Height = 800/4, mask_Width = 1280/4;
    int Height = 480/2, Width = 640/2;
    Pylon::PylonAutoInitTerm init {};
    Scheduler2 scheduler {};

    PDFunction process_function = [](CaptureData ts)->std::pair<torch::Tensor, bool> {
        return e26_ProcessFunction(ts);
    };

    scheduler.Start (
        /* Windowing */
        0,
        1600,
        2560,
        FULLSCREEN,
        NO_TARGET_FPS,
        30,

        Height,
        Width,
        150.0f,
        1,
        1,
        3,
        false,
        4,
        60,
        true,
        0,
        0,
        8,

        mask_Height * 2,
        mask_Width  * 2,

        nullptr,
        process_function
    );

    scheduler.DisableSampleImageCapture();

    // Create a phase mask
    torch::Tensor target_mask = torch::ones({mask_Height, mask_Width}, torch::dtype(torch::kFloat32));

    // Get the phase mask using GS algorithm
    auto phase_mask = s4_Utils::GSAlgorithm(target_mask, 100).to(DEVICE);

    // Save target_mask as target.bmp
    {
        auto m = target_mask * 255;
        auto img = s4_Utils::TensorToImage(m);
        ExportImage(img, "target.bmp");
        UnloadImage(img);
    }

    auto Iterate = [&scheduler]->void {
        scheduler.DrawTextureToScreenTiled();
        scheduler.SetVSYNC_Marker();
        scheduler.WaitVSYNC_Diff(1);

        
        scheduler.DrawTextureToScreenTiled();
        scheduler.SetVSYNC_Marker();
        scheduler.WaitVSYNC_Diff(1);
        

        scheduler.ReadFromCamera();

    };

    int64_t index = 0;
    std::cout << "Starting main loop...\n";
    while (!WindowShouldClose()) {
        auto action = torch::ones({20, mask_Height, mask_Width}, torch::dtype(torch::kFloat32).device(DEVICE)) * (-2.8);
        action[e26_global::indicies[index]] = phase_mask;
        scheduler.SetTextureFromTensorTiled(action);
        Iterate();

        // increment frame count
        e26_global::frame_count.fetch_add(1, std::memory_order_release);

        index = (index + 1) % 20;
        if (e26_global::kill_program.load(std::memory_order_acquire)) {
            std::cout << "Killing program due to mismatch!\n";
            break;
        }
    }

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    // Calculate statistics
    int64_t total_frames = e26_global::frame_count.load(std::memory_order_acquire);
    int64_t total_images = e26_global::image_count.load(std::memory_order_acquire);
    int64_t start_time   = e26_global::start_time.load(std::memory_order_acquire);
    int64_t end_time     = Utils::GetCurrentTime_us();
    double total_time_s = static_cast<double>(end_time - start_time) / 1'000'000.0;
    double fps = static_cast<double>(total_frames) / total_time_s;
    double ips = static_cast<double>(total_images) / total_time_s;

    // Print statistics
    std::cout << "================ Statistics ================\n";
    std::cout << "Total Frames Displayed: " << total_frames << "\n";
    std::cout << "Total Images Captured:  " << total_images << "\n";
    std::cout << "Total Time Elapsed:     " << total_time_s << " seconds\n";
    std::cout << "Average FPS:            " << fps << "\n";
    std::cout << "Average IPS:            " << ips << "\n";
    std::cout << "============================================\n";

    return 0;
}