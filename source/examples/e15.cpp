#include "e15.hpp"

#include <iostream>
#include "raylib.h"
#include "rlgl.h"
#include <cnpy.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#include "../linux/vsync_timer.hpp"

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

std::vector<Texture> e15_GenerateSynchronizationTextures (const int64_t n_bits, int Height=1, int Width=1);
int32_t pixel2value(unsigned char[3]);

int e15 () {
    s3_Window window {};
    window.Height  = 480;
    window.Width   = 640;
    window.wmode   = WINDOWED;
    window.fmode   = NO_TARGET_FPS; //SET_TARGET_FPS;
    window.fps     = 60;
    window.monitor = 0;
    window.load();

    const int64_t n_bits = 24;
    auto textures = e15_GenerateSynchronizationTextures(n_bits);
    int64_t frame_counter=0;

    while (!WindowShouldClose()) {
        auto &texture = textures[frame_counter % n_bits];
        ++frame_counter;

        /******************
         * Draw to Screen *
         ******************/
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

        unsigned char pixel[3];
        glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

        int actual = pixel2value(pixel);
        #ifdef __linux__
            int expected = (frame_counter - 0) % n_bits;
        #else
            int expected = (frame_counter - 1) % n_bits;
        #endif
        int diff = std::abs(actual - expected);
        int min_diff = std::min(diff, static_cast<int>(n_bits - diff));
        if (min_diff != 1)
            std::cout << "\033[1;31m"; // Red
        std::cout << "INFO: [e15] Pixel: " << actual << ", Bit: " << expected << '\n';
        if (min_diff != 1)
            std::cout << "\033[0m"; // Reset

    }


    return 0;
}

std::vector<Texture> e15_GenerateSynchronizationTextures(const int64_t n_bits, int Height, int Width) {
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

int32_t pixel2value(unsigned char pixel[3]) {
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