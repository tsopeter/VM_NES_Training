#include "e13.hpp"

#include <iostream>
#include "raylib.h"
#include "rlgl.h"
#include <cnpy.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#ifdef __linux__
#include "../linux/vsync_timer.hpp"
#else
#include "../macos/vsync_timer.hpp"
#endif

#include "../s3/reportable.hpp"
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
#include <thread>

std::vector<Texture> e13_GenerateSynchronizationTextures (const int64_t n_bits, int Height=1, int Width=1);
int64_t e13_mod(int64_t x, int64_t m);
int32_t e13_pixel2value(unsigned char pixel[3]);

void DrawToScreen(Texture&, s3_Window&);

#ifdef (__linux__ || __APPLE__)
int e13 () {
    Pylon::PylonAutoInitTerm init {};
    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = WINDOWED;
    window.fmode   = NO_TARGET_FPS; // NO_TARGET_FPS; //SET_TARGET_FPS;
    window.fps     = 240;
    window.monitor = 1;
    window.load();

    Serial serial {"/dev/ttyACM0", 115200};
    serial.Open();

    /* Generate test images */
    const int64_t n_bits = 24;

    moodycamel::ConcurrentQueue<int64_t> capture_vsync_index;
    std::vector<uint64_t> vsync_timestamps;
    std::atomic<int64_t> capture_pending {0};
    std::atomic<bool> enable_capture {false};
    std::function<void(std::atomic<uint64_t>&)> timer = [&serial, &enable_capture, &capture_pending, &capture_vsync_index, &vsync_timestamps](std::atomic<uint64_t>&counter) {
        if (enable_capture.load(std::memory_order_acquire)) {
            vsync_timestamps.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count());
            capture_vsync_index.enqueue(static_cast<int64_t>(counter.load(std::memory_order_acquire)));
            serial.Signal();
            capture_pending.fetch_add(1,std::memory_order_release);
            enable_capture.store(false,std::memory_order_release);
        }
    };
    #ifdef __linux__
    glx_Vsync_timer mvt(0, timer);
    #else
    macOS_Vsync_Timer mvt(window.monitor, timer);
    #endif

    /* Create Camera */
    s3_Camera_Reportable_Properties cam_properties;
    cam_properties.AcqFrameRate = 1800;
    cam_properties.Height       = 320;
    cam_properties.Width        = 240;

    s3_Camera_Reportable camera {cam_properties, &mvt};
    camera.open();

    std::vector<uint64_t> frame_timestamps_v;
    std::atomic<bool> end_thread {false};
    moodycamel::ConcurrentQueue<int64_t> frames_buffer;
    moodycamel::ConcurrentQueue<int64_t> frames_vsync;
    moodycamel::ConcurrentQueue<int64_t> frames_held;
    moodycamel::ConcurrentQueue<uint64_t> frame_timestamps;
    moodycamel::ConcurrentQueue<int32_t> gl_buffer;
    std::atomic<int64_t> capture_count {0};
    std::atomic<bool> kill_process {false};
    const int n_textures = 2;
    std::function<void()> capture_function = [&mvt, &camera, &end_thread, &frames_buffer, &frames_vsync, &frame_timestamps, &frame_timestamps_v, &frames_held, &capture_pending, &n_bits, &capture_count, &kill_process, &gl_buffer, &n_textures]()->void {
        /*
        struct sched_param sched;
        sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
        if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched) != 0) {
            perror("Failed to set thread priority");
        }
        */

        int64_t i_c=0;
        bool det_error=false;
        int64_t cp_diff=0;
	    int64_t v_diff=0;
        uint64_t prev_timestamp=0;
        int64_t err_counter=0;
	    int64_t fire_kill_process=10;
        uint64_t prev_frame_timestamp=0;
        while (!end_thread.load(std::memory_order_acquire)) {
            if (capture_pending.load(std::memory_order_acquire) <= 0) continue;

            int     image_count=0;
            int64_t m   = 0;
            int64_t m_i = INT64_MIN;
            double cvsync=0.f;
            uint64_t cvsync_i=0;
            while(image_count<n_bits) {
                torch::Tensor image = camera.sread();
                auto sum = image.sum().item<int64_t>();
                while(!camera.vsync.try_dequeue(cvsync_i));
                cvsync+=cvsync_i;

                if (sum > m) {
                    m_i = image_count;
                    m   = sum;
                }
                ++image_count;
            }
            ++i_c;
            capture_pending.fetch_sub(1, std::memory_order_release);
            capture_count.fetch_add(1, std::memory_order_release);
            cvsync/=n_bits;

            int64_t frame;
            while (!frames_buffer.try_dequeue(frame));

            int64_t bit = frame % n_textures;

            int64_t fvsync;
            while (!frames_vsync.try_dequeue(fvsync));

            int64_t hvsync;
            while (!frames_held.try_dequeue(hvsync));

            uint64_t timestamp;
            while (!camera.timestamps.try_dequeue(timestamp));

            uint64_t frame_timestamp;
            while (!frame_timestamps.try_dequeue(frame_timestamp));

            frame_timestamps_v.push_back(frame_timestamp);

            int32_t gl_value;
            while (!gl_buffer.try_dequeue(gl_value));

            int64_t cp_diff_prev = cp_diff;
            v_diff = abs(bit - m_i) % n_bits;
	        v_diff = std::min(v_diff, 24 - v_diff);
            cp_diff = v_diff;
            if (i_c >= n_bits && cp_diff != cp_diff_prev) {
                det_error = true;
                err_counter++;
            }

            if(det_error) {
                --fire_kill_process;
                if (fire_kill_process <= 0)
                    kill_process.store(true, std::memory_order_release);
            }


            std::cout<<"1------------------------------------------------------------------\n";
            std::cout<<"INFO: [capture_thread] Frame: " << frame << '\n';
            std::cout<<"INFO: [capture_thread] Pending: " << capture_pending.load(std::memory_order_acquire) << '\n';
            std::cout<<"INFO: [capture_thread] Captured: " << m_i << ", Expected: " << bit << '\n';
            std::cout<<"INFO: [capture_thread] GL_Value: " << gl_value << '\n';
            std::cout<<"INFO: [capture_thread] Frames Held: " << hvsync << '\n';
            std::cout<<"INFO: [capture_thread] Sent on: " << fvsync << '\n';
            std::cout<<"INFO: [capture_thread] Captured on: " << cvsync << '\n';
            if (det_error)
                std::cout<<"\033[1;31mINFO: [capture_thread] Detected Error: true\033[0m\n";
            else
                std::cout<<"INFO: [capture_thread] Detected Error: false\n";
	        std::cout<<"INFO: [capture_thread] Difference: "<< v_diff << '\n';
            std::cout<<"INFO: [capture_thread] Images in Queue: " << camera.image_count.load(std::memory_order_acquire) << '\n';
            std::cout<<"INFO: [capture_thread] Capture Timestamp: " << timestamp/1'000 << " us \n";
            std::cout<<"INFO: [capture_thread] Delta: " << (timestamp - prev_timestamp)/1'000 << " us \n";
            std::cout<<"INFO: [capture_thread] Frame Delta: " << (frame_timestamp-prev_frame_timestamp) << '\n';
            std::cout<<"INFO: [capture_thread] Error Ratio: " << 100 * (static_cast<double>(err_counter) / static_cast<double>(capture_count.load(std::memory_order_acquire))) << "%\n";

            std::cout<<"2------------------------------------------------------------------\n";
            prev_timestamp = timestamp;
            prev_frame_timestamp = frame_timestamp;
        }
    };
    std::thread capture_thread {capture_function};

    int64_t frame_counter=0;
    uint64_t vsync_count=0;
    camera.start();

    int texHeight = 1600;
    int texWidth  = 2560;
    auto textures = e13_GenerateSynchronizationTextures(n_textures, texHeight, texWidth);
    int64_t vsync_index_1 = mvt.vsync_counter.load(std::memory_order_acquire);
    std::vector<uint64_t> frames_vsync_indexes;

    while (!WindowShouldClose()) {
        auto &texture = textures[frame_counter % n_textures];

        if (kill_process.load(std::memory_order_acquire))
            break;

        if (frame_counter==1)
            while (frame_counter!=capture_count.load(std::memory_order_acquire));

        frame_timestamps.enqueue(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());

        int64_t vsync_index_2;
        do {
            vsync_index_2 = mvt.vsync_counter.load(std::memory_order_acquire);
        } while ((vsync_index_2 - vsync_index_1) <= 0);
        
        /* Send enable if possible */
        while (enable_capture.load(std::memory_order_acquire));
        enable_capture.store(true, std::memory_order_release);

        frames_vsync.enqueue(vsync_index_2);
        frames_buffer.enqueue(frame_counter);
        frames_vsync_indexes.push_back(vsync_index_2);
        frames_held.enqueue((vsync_index_2 - vsync_index_1));

        ++frame_counter;
        vsync_index_1 = vsync_index_2;

        unsigned char pixel[3];
        glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

        gl_buffer.enqueue(e13_pixel2value(pixel));

        DrawToScreen(texture, window);

    }

    /* Close threads */
    while (capture_pending.load(std::memory_order_acquire));
    end_thread.store(true, std::memory_order_release);
    capture_thread.join();

    std::cout<<"INFO: [e13] Saving files...\n";
    cnpy::npy_save("TimingData/frame_timestamps.npy", frame_timestamps_v, "w");
    cnpy::npy_save("TimingData/frame_vsync_index.npy", frames_vsync_indexes, "w");
    cnpy::npy_save("TimingData/vsync_timestamps.npy", vsync_timestamps, "w");
    std::cout<<"INFO: [e13] Saved files.\n";

    serial.Close();
    camera.close();

    /* Unload textures */
    for (auto &texture : textures)
        UnloadTexture(texture);

    return 0;
}
#else
    int e13 () {
        throw std::runtime_error("ERROR: Not supported.\n");
    }
#endif

int64_t e13_mod(int64_t x, int64_t m) {
    int64_t r = x % m;
    return (r < 0) ? r + m : r;
}

std::vector<Texture> e13_GenerateSynchronizationTextures(const int64_t n_bits, int Height, int Width) {
    std::vector<Texture> textures;
    textures.reserve(n_bits);

    for (int64_t i = 0; i < n_bits; ++i) {
        int byte_index = i / 8;
        int bit_index  = i % 8;
        unsigned char color[3] = {0, 0, 0};
        color[byte_index] = 1 << bit_index;
        for (int j = 0; j < 3; ++j)
            color[j] = ~color[j];

        int pixel_count = Width * Height;
        unsigned char* pixels = new unsigned char[pixel_count * 3];
        for (int j = 0; j < pixel_count; ++j) {
            pixels[j * 3 + 0] = color[0];
            pixels[j * 3 + 1] = color[1];
            pixels[j * 3 + 2] = color[2];
        }

        Image image = {
            .data = pixels,
            .width = Width,
            .height = Height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
        };

        Texture tex = LoadTextureFromImage(image);
        SetTextureFilter(tex, TEXTURE_FILTER_POINT); // Use nearest neighbor
        textures.push_back(tex);

        delete[] pixels;
    }

    return textures;
}

void DrawToScreen(Texture &texture, s3_Window &window) {
    BeginDrawing();

        ClearBackground(BLACK);
        for (int i = 0; i < 1; ++i)
            DrawTexturePro(
                texture,
                {0, 0, 1, 1}, // Source rectangle
                {0, 0, (float)window.Width, (float)window.Height}, // Destination rectangle (full screen)
                {0, 0}, 0.0f, WHITE
            );
        DrawFPS(10,10);

    EndDrawing();
}

int32_t e13_pixel2value(unsigned char pixel[3]) {
    int32_t value = -1;

    for (int i = 0; i < 24; ++i) {
        int byte_index = i / 8;
        int bit_index = i % 8;

        unsigned char expected[3] = {0xFF, 0xFF, 0xFF};
        expected[byte_index] = ~(1 << bit_index);

        if (pixel[0] == expected[0] && pixel[1] == expected[1] && pixel[2] == expected[2]) {
            value = i;
            break;
        }
    }

    return value;
}
