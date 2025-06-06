#include "sys1.hpp"
#include "../third-party/concurrentqueue.h"
#include <thread>
#include <functional>

static moodycamel::ConcurrentQueue<torch::Tensor> computeq;
static moodycamel::ConcurrentQueue<torch::Tensor> lossq;

Phy_Sys1::Phy_Sys1 (Phy_Sys1_Settings setting)
: m_setting(setting) {}

Phy_Sys1::~Phy_Sys1 () {}

void Phy_Sys1::run () {
    /* Start compute thread */
    std::thread compute_thread(&Phy_Sys1::compute, this);


    /* Initialize screen */
    s3_Window window {};
    window.wmode   = m_setting.wmode;
    window.monitor = m_setting.monitor;
    window.load();

    /* Camera */
    s3_Camera_Properties props;
    s3_Camera camera {props};
    camera.open();

    /* Phase Mask Encoder */
    PEncoder pen {0, 0, m_setting.plm_h, m_setting.plm_w};

    /* Input image Upscaler */
    s4_Upscale i_upscale {m_setting.image_h, m_setting.image_w, (BINARY | NORMALIZE | ROUND | BILINEAR)};

    /* Phase Upscaler */
    s4_Upscale p_upscale {m_setting.image_h, m_setting.image_w, (BILINEAR)};

    /* Quantizer */
    Quantize q;

    /* Model */
    Phy_Model_Parameters pm_params;
    pm_params.parameter_H = m_setting.image_h;
    pm_params.parameter_W = m_setting.image_w;
    pm_params.kappa       = m_setting.kappa;
    Phy_Model model {pm_params};

    /* Optimizer(s) */
    torch::optim::Adam adam (model.parameters(), torch::optim::AdamOptions(0.1));
    s4_Optimizer opt {adam, model};

    /* Dataloaders */
    s2_Dataloader dl {"./Dataset"};
    s2_Data da = dl.load(TRAIN, m_setting.n_train);
    da.device(DEVICE);

    /* Update loop */
    camera.start();

    // Epoch
    for (int epoch = 0; epoch < m_setting.n_epochs; ++epoch) {

        // Batch
        for (int i = 0; i < da.len(); i += m_setting.bsz) {
            auto [simages, slabels] = da(i, m_setting.bsz);

            // Generate phase mask
            model.generate_actions(m_setting.n_actions);

            /* Upscale image */
            auto uimages = i_upscale(simages);

            // For each image, perturb the mask
            for (int j = 0; j < uimages.size(0); ++j) {
                auto uimage = uimages[j];
                auto label  = slabels[j];

                while (!model.is_actions_depleted()) {
                    auto phase = model.get_actions_sequentially();

                    /* Upscale phases */
                    auto uphase = p_upscale(phase);

                    /* Combine image + phase */
                    auto comb   = pen.BImageTensorMap(uimage, phase);

                    /* Generate Phase Mask for PLM */
                    auto mask_  = pen.MEncode_u8Tensor2(comb);
                    auto mask   = pen.u8MTensor_Image(mask_);

                    /* Display mask to screen */
                    auto texture = LoadTextureFromImage(mask);

                    BeginDrawing();
                    ClearBackground(BLACK);
                    DrawTextureEx(texture, {0, 0}, 0, 1.0, WHITE);
                    EndDrawing();
                    UnloadTexture(texture);

                    /* Read from camera */
                    while (camera.count < m_setting.n_bits) {
                        auto images = camera.read();    /* Can be [], [H, W] or [N, H, W] */
                        /* Place onto computeq */
                        computeq.enqueue(images);       /* Compute thread will handle computation and handling of images */
                    }
                    camera.count = 0;   /* reset count */
                }
                model.refill_actions_without_sampling();
            }

            // Update Mask

            // Read from lossq, and store reshape it as 

        }
    }

    camera.close();
    m_running = false;  // Tell compute thread to stop computing
    compute_thread.join();
}
void Phy_Sys1::process (s4_Slicer &s, torch::Tensor &t) {
    auto det = s.detect(t); // [B, 10]

    /* Store detection results into Loss queue */

}

void Phy_Sys1::compute () {
    /* Start slicer */
    s4_Slicer slicer = get_slicer();

    /* Compute loop */
    while (m_running) {
        torch::Tensor tensor;
        if (computeq.try_dequeue(tensor)) {
            process(slicer, tensor);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Avoid busy-waiting
        }
    }
}

s4_Slicer Phy_Sys1::get_slicer () {
    /* Create slicers */
    s4_Slicer_Region_Vector regions;
    float cx = m_setting.cam_w/2, cy = m_setting.cam_h/2;
    int pattern_radius = 128;
    for (int i = 0; i < 10; ++i) {
        float angle = 2 * M_PI * i / 10 + (9.0  * M_PI / 180.0);
        float x = cx + pattern_radius * std::cos(angle);
        float y = cy + pattern_radius * std::sin(angle);
        regions.push_back(std::make_shared<s4_Slicer_Circle>(x, y, m_setting.radius));
    }
    s4_Slicer slicer(regions, m_setting.cam_h, m_setting.cam_w);
    return slicer;
}