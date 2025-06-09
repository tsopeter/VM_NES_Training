#ifndef device_hpp__
#define device_hpp__

/**
 * device.hpp is responsible for defining the global device for all torch::Tensors.
 * While most modules defer assign tensors to devices at runtime, some are
 * statically defined at compile-time.
 * 
 * 
 */

#ifndef DEVICE  /* Only use default if device is never defined in Makefile recipe */
#ifdef __APPLE__
    #define DEVICE torch::kMPS /* Apple Silicon specific device */
#else
    #define DEVICE torch::kCUDA /* Other platforms use CUDA */
#endif
#endif

#endif
