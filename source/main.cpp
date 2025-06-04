#include <iostream>
#include <unistd.h>
#include <chrono>
#include <Eigen/Dense>
#include "cnpy.h"

/**
 * s2
 * 
 * /s2/ denotes small modules
 */
#include "s2/von_mises.hpp"
#include "s2/quantize.hpp"
#include "s2/dataloader.hpp"

/**
 * s3
 * 
 * /s3/ denotes large modules
 */
#include "s3/cam.hpp"
#include "s3/window.hpp"

/**
 * s4
 * 
 * /s4/ denotes modules dedicated to neural network
 */
#include "s4/pencoder.hpp"
#include "s4/utils.hpp"
#include "s4/upscale.hpp"

#include "device.hpp"   /* Includes device macro */

class dl2 {
private:
    int m_num_images;
    int m_counter;
    int m_num_samples;
    torch::Tensor m_phase_mask;
    torch::Tensor m_samples;

    s2_Dataloader m_dl {"./Datasets"};
    s2_Data m_dd {};
    PEncoder m_pp {};
    s4_Upscale m_us {};
    VonMises m_vm {};
    Quantize m_q {};

    torch::Device m_device = DEVICE;
    torch::Tensor m_ones = torch::ones(std::vector<int64_t>{24, 128, 128}).to(torch::kFloat32).to(m_device);

public:
    dl2 (int n, int h, int w, int H, int W) : m_num_images(n) {
        m_dd = m_dl.load(TEST, n);
        m_dd.device(m_device);

        m_pp.m_x = 0;
        m_pp.m_y = 0;
        m_pp.m_h = H;
        m_pp.m_w = W;

        m_us.m_h = h;
        m_us.m_w = w;
        m_us.m_mode = (BINARY | BILINEAR | NORMALIZE | ROUND);
        m_us.assign_args();

        m_phase_mask = s4_Utils::GetRandomPhaseMask(h, w).to(torch::kFloat32).to(m_device);

        m_vm = VonMises(m_phase_mask, 25.0);

        m_num_samples = 144;
        m_samples = m_vm.sample(m_num_samples);
        m_counter = m_num_samples;
    }

    Image operator[](int i) {
        if (m_counter <= 0) {
            m_samples = m_vm.sample(m_num_samples);
            m_counter = m_num_samples;
        }

        i %= m_num_images;
        auto [img, _] = m_dd[i];
        img = m_us(img);    /* upscale */

        auto samples = m_samples.slice(0, m_num_samples - m_counter, m_num_samples - m_counter + 24);
        m_counter -= 24;

        samples    = m_pp.ImageTensorMap(samples, img); /* apply */
        auto m1 = m_pp.MEncode_u8Tensor2(samples);
        auto image = m_pp.u8MTensor_Image(m1);

        return image;
    }

    ~dl2 () {
        m_vm.print_stats();
    }
};

int main (int argc, const char *argv[]) {
    s3_Window win {};
    win.monitor = 0;
    win.load(); /* init screen */

    dl2 d {
        24, 128, 128, win.Height, win.Width
    };

    int i = 0;
    while (!WindowShouldClose()) {
        /* CPU -> GPU */
        auto image     = d[i];
        auto texture   = LoadTextureFromImage(image);

        BeginDrawing();
        ClearBackground(WHITE);

        DrawTextureEx(texture, {0, 0}, 0.0, 1.0, WHITE);
        DrawFPS(10, 10);
        EndDrawing();

        ++i;
        UnloadTexture(texture);
    }
    return 0;
}