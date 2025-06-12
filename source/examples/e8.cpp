#include "e8.hpp"

#include <iostream>
#include "raylib.h"
#include <cnpy.h>

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "shared.hpp"
#include <OpenGL/gl.h>
#include <chrono>


std::vector<Texture> GenerateSynchronizationTextures (const int64_t n_bits);

template<typename T>
int64_t             argmin(const std::vector<T>&); 

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

    camera.start();
    std::vector<int64_t> is;
    std::vector<int64_t> timings;
    double fts = 0;
    while (!WindowShouldClose()) {
        std::cout << "INFO: [e8] Frame Index: " << frame_counter << '\n';
        std::cout << "INFO: [e8] Bit Index  : " << frame_counter % n_bits << '\n';
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

        /* Image capture loop */
        int64_t image_count=0;
        int64_t m   = INT64_MIN;
        int64_t m_i = 0;
        auto capture_start = std::chrono::high_resolution_clock::now();
        serial.Signal();    /* Signal to FPGA to start capturing */
        while(image_count<n_bits) {
            torch::Tensor image = camera.sread();
            auto sum = image.sum().item<int64_t>();
            //std::cout<<"INFO: [e8:"<<image_count<<"] Value: "<<sum<<'\n';
            if (sum > m) {
                m_i = image_count;
                m   = sum;
            }
            ++image_count;
        }
        auto capture_end = std::chrono::high_resolution_clock::now();
        int64_t capture_time = std::chrono::duration_cast<std::chrono::microseconds>(capture_end - capture_start).count();


        image_counter += image_count;

        std::cout<<"INFO: [e8] Capture time: "<<capture_time<<" us\n";

        std::cout<<"INFO: [e8] Got    index : "<<m_i<<'\n';
        std::cout<<"INFO: [e8] Actual index : "<<(frame_counter % n_bits)<<'\n';
        is.push_back(m_i);

        auto ft = GetFrameTime();
        fts += ft;
        std::cout<<"INFO: [e8] Frame Rate: "<<(1/ft)<<'\n';
        timings.push_back(static_cast<int64_t>(capture_time));    // us

        ++frame_counter;
    }

    camera.close();
    serial.Close();

    // save to numpy
    cnpy::npy_save("TimingData/signal.npy", is.data(), {is.size()}, "w");
    cnpy::npy_save("TimingData/timing.npy", timings.data(), {timings.size()}, "w");

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