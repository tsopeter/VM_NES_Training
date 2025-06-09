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
#include "examples/e5.hpp"
#include "examples/e6.hpp"

namespace examples_ {
    /**
     * @brief
     *  ## DisplayImages
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
     *  ## CombinePhaseAndImages
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
     *  ## BlackBoxOptimization
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
     *  ## SlicerUsage
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
     *  ## SoftwareTrigger
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
     *  ## HardwareTrigger
     * 
     *      Example for showing how to use hardware triggered camera
     *      and have it sync to vertical sync.
     * 
     * @param  None
     * @return Return value 0 or -1
     * 
     */
    inline auto HardwareTrigger       = e5;

    /**
     * @brief
     *  ## Synchronization
     * 
     *      Example for showing how synchronization between screen and camera could be done.
     * 
     * @param  None
     * @return Return value 0 or -1
     */
    inline auto Syncrhonization       = e6;
}


#endif
