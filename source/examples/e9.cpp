#include "e8.hpp"

#include <iostream>
#include "raylib.h"
#include <cnpy.h>

#include "../macos/vsync_timer.hpp"

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "shared.hpp"
#include <OpenGL/gl.h>
#include <chrono>
#include <future>
#include <thread>

std::vector<Texture> e9_GenerateSynchronizationTextures (const int64_t n_bits);

template<typename T>
int64_t             e9_argmin(const std::vector<T>&); 

int64_t e9_time_now_us ();

int64_t e9_mod(int64_t x, int64_t m);

int e9 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.fmode   = NO_TARGET_FPS;
    //window.fps     = 60;
    window.monitor = 1;
    window.load();

    /* Serial connection */
    /* Serial is used to trigger the FPGA board to allow
       synchronization between camera and DLP/PLM */
    Serial serial {"/dev/tty.usbmodem8326898B1E1E1", 115200};
    serial.Open();

    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::atomic<int64_t> capture_pending {0};
    std::atomic<bool> enable_capture {false};
    std::function<void(std::atomic<uint64_t>&)> timer = [&serial, &enable_capture, &capture_pending, &capture_vsync_index](std::atomic<uint64_t>&counter) {
        if (enable_capture.load(std::memory_order_acquire)) {
            capture_vsync_index.enqueue(static_cast<int64_t>(counter.load(std::memory_order_acquire)));
            capture_pending.fetch_add(1,std::memory_order_release);
            serial.Signal();
            enable_capture.store(false,std::memory_order_release);
        }
    };
    macOS_Vsync_Timer mvt {window.monitor, timer};

    /* Create Camera */
    s3_Camera_Properties cam_properties;
    cam_properties.AcqFrameRate = 1800;
    cam_properties.Height       = 320;
    cam_properties.Width        = 240;

    s3_Camera camera {cam_properties};
    camera.open();

    const int64_t n_bits = 24;
    int64_t frame_counter = 0;
    auto textures = e9_GenerateSynchronizationTextures(n_bits);

    camera.start();

    moodycamel::ConcurrentQueue<int64_t> frames;
    moodycamel::ConcurrentQueue<int64_t> sent_vsync_index;

    std::vector<int16_t> capture_data, sent_data;

    std::atomic<bool> end_thread {false};
    std::atomic<bool> end_system {false};
    std::atomic<int64_t> capture_count {0};
    std::function<void()> capture_function = [&camera, &mvt, &end_thread, &capture_pending, &frames, &capture_data, &sent_data, &capture_count, &sent_vsync_index, &capture_vsync_index, &end_system](){
        int64_t i_c = 0;
        int64_t vs_1 = mvt.vsync_counter.load(std::memory_order_acquire);

        int64_t skip_amount = 24;
        int64_t us_1 = e9_time_now_us();
        bool frames_not_behind = false;
        bool frames_not_same   = false;
        int64_t c_diff_prev = 0;
        int64_t c_diff_sum  = 0;
        int64_t stop_counter = 10;
        while (!end_thread.load(std::memory_order_acquire)) {
            int64_t image_count = 0;
            int64_t m = INT64_MIN;
            int64_t m_i = 0;
            if (capture_pending.load(std::memory_order_acquire) <= 0) continue;
            while(image_count<n_bits) {
                torch::Tensor image = camera.sread();
                auto sum = image.sum().item<int64_t>();

                if (sum > m) {
                    m_i = image_count;
                    m   = sum;
                }
                ++image_count;
            }
            ++i_c;
            int64_t vs_2 = mvt.vsync_counter.load(std::memory_order_acquire);

            int64_t vs_diff = vs_2 - vs_1;
            vs_1 = vs_2;

            int64_t frame;
            while(!frames.try_dequeue(frame));

            int64_t diff_forward  = e9_mod(m_i - frame, n_bits);
            int64_t diff_backward = e9_mod(frame - m_i, n_bits);
            int64_t c_diff = std::min(diff_forward, diff_backward);

            int64_t sent_index;
            while (!sent_vsync_index.try_dequeue(sent_index));

            int64_t capture_index;
            while (!capture_vsync_index.try_dequeue(capture_index));

            std::cout << "1---------------------------------------------\n";
            std::cout << "INFO: [capture_thread] Actual: " << m_i << ", Expected: " << frame << '\n';
            std::cout << "INFO: [capture_thread] VSYNC Diff: " << vs_diff << '\n';
            std::cout << "INFO: [capture_thread] Captures Pending: " << capture_pending.load(std::memory_order_acquire) << '\n';
            std::cout << "INFO: [capture_thread] VSYNC index: " << vs_2 << '\n';
            std::cout << "INFO: [capture_thread] Sent VSYNC index: " << sent_index << '\n';
            std::cout << "INFO: [capture_thread] Capture VSYNC index: " << capture_index << '\n';
            std::cout << "INFO: [capture_thread] Difference: " << c_diff << '\n';

            if (i_c > skip_amount) {
                if (!frames_not_behind) {
                    if (m_i != e9_mod(frame-1, n_bits))
                        frames_not_behind = true;
                }

                if (!frames_not_same) {
                    if (m_i != e9_mod(frame, n_bits))
                        frames_not_same = true;
                }

                capture_data.push_back(static_cast<int16_t>(m_i));
                sent_data.push_back(static_cast<int16_t>(frame));

                c_diff_sum += (c_diff != c_diff_prev);

                /* Check if frames are behind */
                std::cout<<"INFO: [capture_thread] Is frames not behind (or was previous detected not to be): "<<frames_not_behind << '\n';
                std::cout<<"INFO: [capture_thread] Is frames not same   (or was previous detected not to be): "<<frames_not_same << '\n';
                std::cout<<"INFO: [capture_thread] Number of swaps of difference: " << c_diff_sum << '\n';
            }

            capture_count.fetch_add(1,std::memory_order_release);
            capture_pending.fetch_sub(1,std::memory_order_release);
            c_diff_prev = c_diff;

            if (c_diff_sum != 0) {
                --stop_counter;
                if (stop_counter <= 0)
                    end_system.store(true, std::memory_order_release);
            }
            std::cout << "2---------------------------------------------\n";
        }
    };
    std::thread capture_thread {capture_function};

    int64_t sent_count = 0;

    int64_t vsync_index_1 = mvt.vsync_counter.load(std::memory_order_acquire);
    while (!WindowShouldClose()) {
        auto &texture = textures[frame_counter % n_bits];

        if (end_system.load(std::memory_order_acquire))
            break;

        if (frame_counter == 1)
            while (sent_count != capture_count.load(std::memory_order_acquire));

        /* Draw loop */
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            texture,
            {0, 0, 1, 1}, // Source rectangle (1x1 texture)
            {0, 0, (float)window.Width, (float)window.Height}, // Destination rectangle (full screen)
            {0, 0}, 0.0f, WHITE
        );
        EndDrawing();

        int64_t vsync_index_2;
        do {
            vsync_index_2 = mvt.vsync_counter.load(std::memory_order_acquire);
        } while ((vsync_index_2 - vsync_index_1) <= 0);
        frames.enqueue(frame_counter % n_bits);
        sent_vsync_index.enqueue(vsync_index_2);
        while (enable_capture.load(std::memory_order_acquire));
        enable_capture.store(true, std::memory_order_release);

        ++frame_counter;
        ++sent_count;
        vsync_index_1 = vsync_index_2;
  
    }

    enable_capture.store(false, std::memory_order_release);
    while (capture_pending.load() > 0);

    end_thread.store(true);
    capture_thread.join();

    camera.close();
    serial.Close();

    for (auto &t : textures)
        UnloadTexture(t);

    // save data
    cnpy::npy_save("TimingData/2_capture.npy", capture_data, "w");
    cnpy::npy_save("TimingData/2_sent.npy", sent_data, "w");

    window.close(); /* close window */

    return 0;
}

std::vector<Texture> e9_GenerateSynchronizationTextures(const int64_t n_bits) {
    std::vector<Texture> textures;
    textures.reserve(n_bits);

    for (int64_t i = 0; i < n_bits; ++i) {
        unsigned char pixel[3] = {0, 0, 0};
        int byte_index = i / 8;
        int bit_index  = i % 8;
        pixel[byte_index] = 1 << bit_index;

        for (int j = 0; j < 3; ++j)
            pixel[j] = ~pixel[j];

        Image image = {
            .data = pixel,
            .width = 1,
            .height = 1,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
        };

        Texture tex = LoadTextureFromImage(image);
        SetTextureFilter(tex, TEXTURE_FILTER_POINT); // Use nearest neighbor
        textures.push_back(tex);
    }

    return textures;
}

template<typename T>
int64_t e9_argmin(const std::vector<T> &v) {
    int64_t m = INT64_MAX;
    int64_t j = 0;

    for (int i = 0; i < v.size(); ++i) {
        if (v[i] < m) {
            j = i;
            m = v[i];
        }
    }

    return j;    /* return index */
}

int64_t e9_mod(int64_t x, int64_t m) {
    int64_t r = x % m;
    return (r < 0) ? r + m : r;
}


int64_t e9_time_now_us () {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}