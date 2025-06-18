#ifndef examples_hpp__
#define examples_hpp__

/**
 * examples.hpp shows multiple examples of how to use modules. At the current moment,
 * it is a glorified unit-test.
 * 
 * 
 */

#include "examples/e0.hpp"
#include "examples/e1.hpp"
#include "examples/e2.hpp"
#include "examples/e3.hpp"
#include "examples/e4.hpp"
#include "examples/e6.hpp"
#include "examples/e7.hpp"
#include "examples/e8.hpp"
#include "examples/e9.hpp"

namespace examples_ {
    /**
     * @brief
     *  ## DisplayImages (e0)
     * 
     *      Example for showing how to use dataloader
     *      and Upscaler to display images to the screen.
     * 
     * @param  None
     * @return Return value 0 or -1
     * 
     */
    inline auto DisplayImages         = e0;

    /**
     * @brief
     *  ## CombinePhaseAndImages (e1)
     * 
     *      Example for showing how to combine images and phase masks together using
     *      PEncoder
     * 
     * @param  None
     * @return Return value 0 or -1
     * 
     */
    inline auto CombinePhaseAndImages = e1;

    /**
     * @brief
     *  ## BlackBoxOptimization (e2)
     * 
     *      Example for showing how to define distributions and models, and
     *      how to perform black-box optimization using s4_Optimizer on a
     *      toy example.
     * 
     * @param  None
     * @return Return value 0 or -1
     * 
     */
    inline auto BlackBoxOptimization  = e2;

    /**
     * @brief
     *  ## SlicerUsage (e3)
     * 
     *      Example for showing how to use the s4_Slicer
     * 
     * @param  None
     * @return Return value 0 or -1
     * 
     */
    inline auto SlicerUsage           = e3;

    /**
     * @brief
     *  ## SoftwareTrigger (e4)
     * 
     *      Example for showing how to use software triggered camera
     *      and have it sync with image change.
     * 
     * @param  None
     * @return Return value 0 or -1
     * 
     */
    inline auto SoftwareTrigger       = e4;

    /**
     * @brief
     *  ## Hardware Capture Timing (e6)
     * 
     *      Example for showing how to capture a series of images from camera tied to PLM/DLP
     * 
     * @param  None
     * @return Return value 0 or -1
     */
    inline auto HardwareTrigger       = e6;

    /**
     * @brief
     *  ## Serial (e7)
     * 
     *      Example of how the serial class can be used
     * 
     * @param None
     * @return Return value 0 or -1
     */
    inline auto SerialExample         = e7;

    /**
     * @brief
     *  ## Synchronization (e8) (*deprecated, use GLSync/e9 instead)
     * 
     *      Example of testing synchronization
     * 
     * @param None
     * @return Return value 0 or -1
     */
    inline auto Synchronization       = e8;

    /**
     * @brief
     *  ## GLSync Example (e9)
     * 
     *      Example of using sync timer classes to synchronize
     *      camera and display.
     * 
     * @param None
     * @return Return value 0 or -1
     */
    inline auto GLSyncExample  = e9;
}


#endif
