#ifndef phy_sys1_hpp__
#define phy_sys1_hpp__

/**
 * System Includes
 * 
 */
#include "model.hpp"            /* Phy_Model */
#include "../s3/window.hpp"     /* Window */
#include "../s3/cam.hpp"        /* Camera */
#include "../s4/pencoder.hpp"   /* Phase Mask Encoding */
#include "../s4/upscale.hpp"    /* Image Upscaler */
#include "../s4/utils.hpp"      /* s4 utilities */

/**
 * Used in model training
 * 
 */
#include "../s2/dataloader.hpp" /* Dataloader */
#include "../s2/quantize.hpp"   /* Quantizer */
#include "../s2/von_mises.hpp"  /* Von Mises Distribution */
#include "../s4/optimizer.hpp"  /* Optimizer */
#include "../s4/slicer.hpp"     /* Region Slicing */

#include "../device.hpp"

struct Phy_Sys1_Settings {
    /* Window Settings */
    int monitor = 0;
    s3_Windowing_Mode wmode = WINDOWED;

    /* Scaling Settings */
    int image_h     = 1024;
    int image_w     = 1024;

    /* Training Settings */
    int n_train     = 512;
    int n_test      = 512;
    int n_epochs    = 5;
    int n_actions   = 144;
    int bsz         = 32;
    double kappa    = 25.0f;

    /* Slicer Settings */
    double radius   = 10.0f;

    /* Constants  */
    const int plm_h  = 1600;
    const int plm_w  = 2560;
    const int cam_h  = 480;
    const int cam_w  = 640;
    const int n_bits = 24;
};

class Phy_Sys1 {
public:
    Phy_Sys1 (Phy_Sys1_Settings);
    ~Phy_Sys1 ();

    /**
     * @brief run
     * 
     *        Runs the entire system.
     */
    void run ();
    
private:
    /**
     * @brief Compute thread
     * 
     */
    void compute();
    void process(s4_Slicer&, torch::Tensor&);

    s4_Slicer get_slicer ();

    Phy_Sys1_Settings m_setting;

    // For compute loop
    std::atomic<bool> m_running {true};
    torch::nn::CrossEntropyLoss loss_fn{torch::nn::CrossEntropyLossOptions().reduction(torch::kNone)};
};

#endif
