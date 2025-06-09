#include "e0.hpp"
#include "../s3/window.hpp"
#include "../s2/dataloader.hpp"   /* Load data */
#include "../s4/upscale.hpp"      /* Image Upscaler */
#include "../s4/utils.hpp"        /* TensorToImage */
#include <raylib.h>

int e0 () {
    /* Initialize screen */
    s3_Window window {};
    window.wmode   = WINDOWED;
    window.monitor = 0;
    window.load();

    /* Dataloader */
    s2_Dataloader dl {"./Datasets"};
    s2_Data da = dl.load(TEST, 60);

    /* Upscaler */
    s4_Upscale upscale {256, 256, (BINARY | NORMALIZE | ROUND | BILINEAR)};
 
    for (int i = 0; i < da.len(); ++i) {
        auto [img, label]  = da[i];
        auto upscale_image = upscale(img) * 255;
        auto image         = s4_Utils::TensorToImage(upscale_image);

        auto texture       = LoadTextureFromImage(image);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureEx(texture, {0,0}, 0, 1.0, WHITE);
        DrawFPS(10, 10);
        EndDrawing();
        UnloadTexture(texture);
    }
    std::cout<<"[Number of Images Loaded]: "<<da.len()<<'\n';
    return 0;
}