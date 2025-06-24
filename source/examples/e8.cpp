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

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include <chrono>
#include <future>

std::vector<Texture> GenerateSynchronizationTextures (const int64_t n_bits);

template<typename T>
int64_t             argmin(const std::vector<T>&); 

#ifdef __APPLE__
int e8 () {
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
    std::function<void(std::atomic<uint64_t>&)> timer = [&serial,&vsync_time_stamps,&enable_capture](std::atomic<uint64_t>&_counter) {
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
    auto textures = GenerateSynchronizationTextures(n_bits);
    int64_t previous_first_capture_time = 0;

    camera.start();
    std::vector<int64_t> is;
    std::vector<int64_t> timings;
    std::vector<int64_t> ts;
    std::vector<int64_t> firsts;
    std::vector<int64_t> startings;
    std::vector<int64_t> diffs;
    std::vector<int64_t> mtd;
    std::vector<int64_t> adv_c;
    double fts = 0;

    int64_t vs_c = mvt.vsync_counter.load();
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

        auto capture_start = std::chrono::high_resolution_clock::now();
        startings.push_back(std::chrono::duration_cast<std::chrono::microseconds>(capture_start.time_since_epoch()).count());

        int64_t image_count   = 0;
        int64_t m   = INT64_MIN;
        int64_t m_i = 0;
        int64_t capture_time = 0;
        int64_t total_time   = 0;
        int64_t first_time   = 0;

        //serial.Signal();
        while(image_count<n_bits) {
            torch::Tensor image = camera.sread();
            auto sum = image.sum().item<int64_t>();

            auto tj_1 = std::chrono::high_resolution_clock::now();
            if (image_count==0) {
                first_time     = std::chrono::duration_cast<std::chrono::microseconds>(tj_1 - capture_start).count();
            }

            if (sum > m) {
                m_i = image_count;
                m   = sum;
            }
            ++image_count;
        }
        auto capture_end = std::chrono::high_resolution_clock::now();
        capture_time = std::chrono::duration_cast<std::chrono::microseconds>(capture_end - capture_start).count();
        total_time += capture_time;

        image_counter += image_count;
        int64_t vs_c_1 = mvt.vsync_counter.load();
        int64_t vs_diff = vs_c_1 - vs_c;


        /* Set run count based on previous timing data */
        auto ft = GetFrameTime();
        fts += ft;
        std::cout << "INFO: [e8] -----------------------------------------\n";
        std::cout << "INFO: [e8] Frame Index: " << frame_counter << '\n';
        std::cout << "INFO: [e8] Bit Index  : " << frame_counter % n_bits << '\n';
        std::cout<<"INFO: [e8] Capture time: "<<total_time<<" us\n";
        std::cout<<"INFO: [e8] Got    index : "<<m_i<<'\n';
        std::cout<<"INFO: [e8] Actual index : "<<(frame_counter % n_bits)<<'\n';
        std::cout<<"INFO: [e8] Frame Rate: "<<(1/ft)<<'\n';
        std::cout<<"INFO: [e8] First Capture Time: "<<first_time<<'\n';
        std::cout<<"INFO: [e8] VSync Count: " << mvt.vsync_counter.load() << '\n';
        std::cout<<"INFO: [e8] VSync Diff : " << (vs_c_1 - vs_c) << '\n';

        is.push_back(m_i);
        ts.push_back((frame_counter-2) % n_bits);
        timings.push_back(static_cast<int64_t>(total_time));    // us
        firsts.push_back(first_time);
        diffs.push_back(vs_c_1 - vs_c);

        vs_c = vs_c_1;
        ++frame_counter;
    }

    camera.close();
    serial.Close();

    // save to numpy
    cnpy::npy_save("TimingData/signal.npy", is.data(), {is.size()}, "w");
    cnpy::npy_save("TimingData/timing.npy", timings.data(), {timings.size()}, "w");
    cnpy::npy_save("TimingData/bits.npy", ts.data(), {ts.size()}, "w");
    cnpy::npy_save("TimingData/first.npy", firsts.data(), {firsts.size()}, "w");
    cnpy::npy_save("TimingData/vsync.npy", vsync_time_stamps.data(), {vsync_time_stamps.size()}, "w");
    cnpy::npy_save("TimingData/start.npy", startings.data(), {startings.size()}, "w");
    cnpy::npy_save("TimingData/vs.npy", diffs.data(), {diffs.size()}, "w");
    cnpy::npy_save("TimingData/adv.npy", adv_c.data(), {adv_c.size()}, "w");

    for (auto &t : textures)
        UnloadTexture(t);

    window.close(); /* close window */

    /* Print out FPS */
    std::cout<<"INFO: [e8] Average Frame Rate: "<<(frame_counter)/(fts)<<'\n';

    /* Print out number of frames captured */
    std::cout<<"INFO: [e8] Frames Sent: " << frame_counter*n_bits << '\n';

    /* Print out number of frames received */
    std::cout<<"INFO: [e8] Images Recv: " << image_counter << '\n';

    /* Print out internal count */
    std::cout<<"INFO: [e8] Internal count: " << camera.count << '\n';

    return 0;
}

std::vector<Texture> GenerateSynchronizationTextures(const int64_t n_bits) {
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
#else
    int e8 () {
        throw std::runtime_error("Error: [e8] is only implemented for macOS machines.");
        return 0;
    }
#endif

template<typename T>
int64_t argmin(const std::vector<T> &v) {
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