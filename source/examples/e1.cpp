#include "e1.hpp"
#include <iostream>

#include "../s2/quantize.hpp"
#include "../s2/dataloader.hpp"
#include "../s2/von_mises.hpp"
#include "../s3/window.hpp"
#include "../s4/upscale.hpp"
#include "../s4/utils.hpp"
#include "../s4/pencoder.hpp"

#include "../device.hpp"   /* Includes device macro */


int e1 () {
    /* Initialize screen */
    s3_Window window {};
    window.wmode   = WINDOWED;
    window.monitor = 0;
    window.load();

    int64_t s = 128;
    int64_t k = 200;

    /* Init distribution */
    torch::Tensor mu = s4_Utils::GetRandomPhaseMask(s, s).to(DEVICE);
    VonMises von_mises {mu, 25};

    /* Dataloader */
    s2_Dataloader dl {"./Datasets"};
    s2_Data da = dl.load(TEST, 60);
    da.device(DEVICE);

    /* Upscaler */
    s4_Upscale upscale   {(int)k, (int)k, (BINARY | NORMALIZE | ROUND | BILINEAR)};
    s4_Upscale upscale_p {(int)k, (int)k, (BILINEAR)};
 
    /* Phase Encoder */
    PEncoder pen {0, 0, window.Height, window.Width};

    /* Quantizer */
    Quantize q;

    while (!WindowShouldClose()) {
        auto samples = von_mises.sample(144);   // [N, H, W]

        for (int i = 0; i < da.len(); ++i) {
            auto [simg, slabel] = da[i];

            /* Upscale */
            auto img = upscale(simg);

            for (int j = 0; j < 144; j += 24) {
                auto bsamples = samples.slice(0, j, j + 24); // [B, H, W]

                // upscale bsamples
                bsamples = upscale_p(bsamples);
                
                /* Combine bsamples and img */
                auto comb = pen.BImageTensorMap(bsamples, img);

                /* Transform into Image */
                auto timage = pen.MEncode_u8Tensor2(comb);
                auto  image = pen.u8MTensor_Image(timage);

                /* Draw */
                Texture texture = LoadTextureFromImage(image);
                BeginDrawing();
                ClearBackground(BLACK);
                DrawTextureEx(texture, {0, 0}, 0, 1, WHITE);
                DrawFPS(10, 10);
                EndDrawing();
                UnloadTexture(texture);
            }
        }
    }

    return 0;
}