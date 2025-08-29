#include "e25.hpp"

#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s5/cam2.hpp"
#include "../s3/Serial.hpp"
#include <atomic>
#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <fstream>
#include <ostream>


int e25 () {
    Pylon::PylonAutoInitTerm autoInitTerm;

    std::vector<bool> ps;
    std::atomic<bool> flip_detected {false};

    s3_Window window;
    window.Height = 1600;
    window.Width  = 2560;
    window.monitor = 0;
    window.wmode   = s3_Windowing_Mode::FULLSCREEN;
    window.fmode   = s3_TargetFPS_Mode::NO_TARGET_FPS;
    window.fps     = 30;

    window.load();

    Serial serial;
    serial.Open ("/dev/ttyACM0", 115200);

    std::atomic<bool> end_thread {false};
    std::atomic<int64_t> frame_counter {0};
    std::atomic<int> sent {0};

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

    std::function<void()> capture_function = [&sent, &ps, &flip_detected, &cam, &end_thread]() {
        int64_t count = 0;
        while (!end_thread.load(std::memory_order_acquire)) {
            u8Image image = cam.sread();
            ++count;

            std::cout << "Count: " << count << '\n';

            if (count % 20 == 0)
                sent.fetch_sub(1, std::memory_order_release);

            if (count % 20 != 0)
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

    cam.open();
    cam.start();

    bool first_frame = true;
    int64_t frame_count = 0;
    while (!WindowShouldClose()) {
        for (int i = 0; i < 2; ++i) {
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
            EndDrawing();
        }
        ++frame_count;
        serial.Signal();
        while (sent.load(std::memory_order_acquire) != 0) {}
        sent.fetch_add(1, std::memory_order_release);
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
    serial.Close();

    return 0;
}