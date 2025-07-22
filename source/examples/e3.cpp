#include "e3.hpp"
#include <torch/torch.h>

#include "../s3/window.hpp"
#include "../s4/slicer.hpp"
#include "../device.hpp"

int e3 () {
    /* Initialize screen */
    s3_Window window {};
    window.wmode   = WINDOWED;
    window.Height = 240;
    window.Width  = 320;
    window.monitor = 0;
    window.load();

    /* Create slicers */
    s4_Slicer_Region_Vector regions;
    float cx = window.Height/2, cy = window.Width/2, radius = 14;
    int pattern_radius = 96;
    for (int i = 0; i < 10; ++i) {
        float angle = 2 * M_PI * i / 10 + (9.0  * M_PI / 180.0);
        float x = cx + pattern_radius * std::cos(angle);
        float y = cy + pattern_radius * std::sin(angle);
        regions.push_back(std::make_shared<s4_Slicer_Circle>(y, x, radius));
    }
    s4_Slicer slicer(regions, window.Height, window.Width);

    /* Visualize slicing to screen */
    Image image = slicer.visualize();
    auto texture = LoadTextureFromImage(image);

    while (!WindowShouldClose()) {

        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextureEx (texture, {0, 0}, 0, 1.0, WHITE);
        DrawFPS(10,10);
        EndDrawing();
    }

    UnloadTexture(texture);
    return 0;
}