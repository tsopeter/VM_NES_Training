#ifndef device_hpp__
#define device_hpp__

#ifndef DEVICE  /* Only use default if device is never defined in Makefile recipe */
#ifdef __APPLE__
    #define DEVICE torch::kMPS /* Apple Silicon specific device */
#else
    #define DEVICE torch::kCUDA /* Other platforms use CUDA */
#endif
#endif

#endif
