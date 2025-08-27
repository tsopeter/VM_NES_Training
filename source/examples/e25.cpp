#include "e25.hpp"

#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s5/cam2.hpp"
#include <atomic>
#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <fstream>
#include <ostream>

std::vector<bool> ps;
std::atomic<bool> flip_detected {false};


int e25 () {
    Pylon::PylonAutoInitTerm autoInitTerm;

    s3_Window window;
    window.Height = 1600;
    window.Width  = 2560;
    window.monitor = 0;
    window.wmode   = s3_Windowing_Mode::FULLSCREEN;
    window.fmode   = s3_TargetFPS_Mode::NO_TARGET_FPS;
    window.fps     = 30;

    window.load();

    std::atomic<bool> end_thread {false};
    std::atomic<int64_t> frame_counter {0};

    Cam2 cam;
    cam.Height = 48;
    cam.Width  = 48;
    cam.ExposureTime = 59.0f;
    cam.BinningHorizontal = 1;
    cam.BinningVertical   = 1;
    cam.UseCentering = false;
    cam.OffsetX = 272;
    cam.OffsetY = 256;
    cam.UseZones = false;
    cam.NumberOfZones = 4;
    cam.ZoneSize = 65;
    cam.LineTrigger = 3;

    std::function<void()> capture_function = [&cam, &end_thread]() {
        int64_t count = 0;
        while (!end_thread.load(std::memory_order_acquire)) {
            u8Image image = cam.sread();
            ++count;
            if (count % 8 != 0)
                continue;

            int64_t sum = 0;
            for (auto &pixel : image) {
                sum += pixel;
            }

            ps.push_back(sum > 400'000);

            if (sum < 400'000) {
                flip_detected.store(true, std::memory_order_release);
            }
        }
    };

    std::thread capture_thread(capture_function);

    // Load image
    Image img = LoadImage("test_image_small.bmp");
    Texture tx = LoadTextureFromImage(img);

    bool first_frame = true;
    int64_t frame_count = 0;
    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro (
                tx,
                {0, 0, static_cast<float>(tx.width), static_cast<float>(tx.height)},
                {0, 0, static_cast<float>(window.Width), static_cast<float>(window.Height)},
                {0, 0},
                0.0f,
                WHITE
            );
            // Draw the frame_counter
            DrawText(std::to_string(frame_count).c_str(), 10, 10, 20, RED);
            if (flip_detected.load(std::memory_order_acquire)) {
                DrawText("FLIP DETECTED", 10, 40, 20, GREEN);
            }
            ++frame_count;
        EndDrawing();
        if (first_frame) {
            first_frame = false;
            std::this_thread::sleep_for(std::chrono::microseconds(16'000));
            cam.open();
            cam.start();
            
        }
    }

    UnloadImage(img);
    UnloadTexture(tx);

    end_thread.store(true, std::memory_order_release);
    capture_thread.join();

    // save ps to file
    std::ofstream ofs("ps.txt");
    for (const auto &b : ps) {
        ofs << b << "\n";
    }

    cam.close();

    return 0;
}