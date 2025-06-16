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

int e9 () {
    Pylon::PylonAutoInitTerm init {};

    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.fmode   = NO_TARGET_FPS;
    //window.fps     = 30;
    window.monitor = 1;
    window.load();

    /* Serial connection */
    /* Serial is used to trigger the FPGA board to allow
       synchronization between camera and DLP/PLM */
    Serial serial {"/dev/tty.usbmodem8326898B1E1E1", 115200};
    serial.Open();

    std::atomic<bool> enable_capture {false};
    std::vector<int64_t> vsync_time_stamps;
    std::function<void()> timer = [&serial,&vsync_time_stamps,&enable_capture]() {
        auto now = std::chrono::high_resolution_clock::now();
        int64_t us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        vsync_time_stamps.push_back(us);
        if (enable_capture.load()) {
            serial.Signal();
            enable_capture.store(false);
        }
    };

    /* Create timers */
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
    int64_t image_counter = 0;
    auto textures = e9_GenerateSynchronizationTextures(n_bits);
    int64_t previous_first_capture_time = 0;

    camera.start();
    std::vector<int8_t> is;
    std::vector<int8_t> ts;
    std::vector<int8_t> diffs;

    std::atomic<bool> end_thread {false};
    std::atomic<int8_t> actual {0}, expected {0};
    std::atomic<int64_t> cdiffs {0};
    std::function<void()> capture_function = [&ts, &is, &end_thread, &camera, &n_bits, &actual, &expected, &mvt, &cdiffs](){
        int64_t i_c = 0;
        int64_t vs_1 = mvt.vsync_counter.load();
        while (!end_thread) {
            int64_t image_count = 0;
            int64_t m = INT64_MIN;
            int64_t m_i = 0;
            while(image_count<n_bits) {
                torch::Tensor image = camera.sread();
                auto sum = image.sum().item<int64_t>();

                if (sum > m) {
                    m_i = image_count;
                    m   = sum;
                }
                ++image_count;
            }
            is.push_back(m_i);
            
            actual.store(m_i);
            expected.store(ts[i_c]);
            ++i_c;
            int64_t vs_2 = mvt.vsync_counter.load();
            auto d = vs_2 - vs_1;
            cdiffs.store(d);
            vs_1 = vs_2;
        }
    };
    std::thread capture_thread {capture_function};

    int64_t vs_c = mvt.vsync_counter.load();
    int64_t breakpoint_countdown = 10;
    bool    same_error_det = false;
    bool    one_behind_error_det = false;
    std::string notice_1 = "", notice_2 = "";
    while (!WindowShouldClose()) {
        auto &texture = textures[frame_counter % n_bits];

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

        int64_t m_i = 0;
        while (enable_capture.load());
        enable_capture.store(true);
        
        int64_t vs_c_1 = mvt.vsync_counter.load();
        int64_t vs_diff = vs_c_1 - vs_c;

        ts.push_back(frame_counter % n_bits);
        diffs.push_back(vs_c_1 - vs_c);

        int64_t al = actual.load(), el = expected.load();
        int64_t wel = (((el-1) % 24) + 24) % 24;

        bool same_det = al == el;
        bool one_behind_det = al == wel;

        if (!same_det) {
            same_error_det = true;
            notice_1 = "*";
        }

        if (same_error_det && !one_behind_det) {
            one_behind_error_det = true;
            notice_2 = "*";
        }

        auto ft = GetFrameTime();
        std::cout<<"INFO: [e9] FrameTime: "<<ft*1e6<<" us\n";
        std::cout<<"INFO: [e9] Actual: "<<al<<'\n';
        std::cout<<"INFO: [e9] Expected: "<<el<<'\n';
        std::cout<<"INFO: [e9] VSYNC Differences: "<<(vs_c_1 - vs_c) <<'\n';
        std::cout<<"INFO: [e9] Capture VSYNC: "<<cdiffs.load()<<'\n';
        std::cout<<"INFO: [e9] Same: "<<(al==el)<<" "<<notice_1<<'\n';
        std::cout<<"INFO: [e9] One Behind: "<<(al==wel)<<" "<<notice_2<<'\n';
        std::cout<<"INFO: [e9] Frame Index: "<<frame_counter<<'\n';

        if (one_behind_error_det)
            break;

        vs_c = vs_c_1;
        ++frame_counter;
    }

    end_thread.store(true);
    capture_thread.join();

    camera.close();
    serial.Close();

    // save to numpy
    cnpy::npy_save("TimingData/signal.npy", is.data(), {is.size()}, "w");
    cnpy::npy_save("TimingData/bits.npy", ts.data(), {ts.size()}, "w");
    cnpy::npy_save("TimingData/vs.npy", diffs.data(), {diffs.size()}, "w");
    

    for (auto &t : textures)
        UnloadTexture(t);

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