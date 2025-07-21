#include "e14.hpp"

#include "../s3/cam.hpp"
#include "../s3/window.hpp"
#include "../s4/utils.hpp"
#include "../s3/Serial.hpp"
#include "shared.hpp"

std::vector<Texture> e14_GenerateSynchronizationTextures (const int64_t n_bits, bool flip=true);
std::vector<Texture> e14_GenerateAlternate(const int64_t n_bits);

int e14 () {
    /* Initialize screen */
    s3_Window window {};
    window.Height  = 1600;
    window.Width   = 2560;
    window.wmode   = BORDERLESS;
    window.fmode   = NO_TARGET_FPS;
    //window.fps     = 60;
    window.monitor = 1;
    window.load();
    ToggleFullscreen();
    SetWindowPosition(GetMonitorPosition(window.monitor).x, GetMonitorPosition(window.monitor).y);

    const int64_t n_bits = 24;
    auto textures = e14_GenerateSynchronizationTextures(n_bits,true);

    int64_t frame_counter=0;
    int64_t num_textures = textures.size();

    {
        Image cpu = LoadImageFromTexture(textures[0]);
        unsigned char *data = static_cast<unsigned char*>(cpu.data);
        printf("Current Bit [%d]: [%02X %02X %02X]\n", 0, data[0], data[1], data[2]);
        UnloadImage(cpu);
    }
    while (!WindowShouldClose()) {
        auto &texture = textures[frame_counter % num_textures];
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            texture,
            {0, 0, 1, 1}, // Source rectangle (1x1 texture)
            {0, 0, (float)window.Width, (float)window.Height}, // Destination rectangle (full screen)
            {0, 0}, 0.0f, WHITE
        );
        DrawFPS(10,10);
        EndDrawing();
        if (IsKeyPressed(KEY_ENTER)) {
            ++frame_counter;

            Image cpu = LoadImageFromTexture(textures[frame_counter % num_textures]);
            unsigned char *data = static_cast<unsigned char*>(cpu.data);
            printf("Current Bit [%ld]: [%02X %02X %02X]\n", frame_counter % num_textures, data[0], data[1], data[2]);
            UnloadImage(cpu);
        }
    }

    /* Unload textures */
    for (auto &texture : textures)
        UnloadTexture(texture);

    return 0;
}   

std::vector<Texture> e14_GenerateAlternate(const int64_t n_bits) {
    return {};
}

std::vector<Texture> e14_GenerateSynchronizationTextures(const int64_t n_bits, bool flip) {
    std::vector<Texture> textures;
    textures.reserve(n_bits);

    for (int64_t i = 0; i < n_bits; ++i) {
        unsigned char pixel[3] = {0, 0, 0};
        int byte_index = i / 8;
        int bit_index  = i % 8;
        pixel[byte_index] = 1 << bit_index;

        if (flip)
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