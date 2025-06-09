#ifndef examples_shared_hpp__
#define examples_shared_hpp__

#include <optional>
#include <torch/torch.h>
#include <raylib.h>
#include <cstdint>
#include "../s3/cam.hpp"
#include "../s3/virtual.hpp"

/**
 * shared.hpp contains a number of shared functions that are widely used across
 * the examples.
 */

namespace examples {

/**
 * @brief
 *  ## createTextureFromFrameNumber
 * 
 *      Encodes the frame number as bit-planes in a R8G8B8 image. Where there is
 *      a 1-bit, the entire plane is lit, and 0-bit the entire plane is unlit.
 * 
 *      Obviously, this depends on how the DLP is structured.
 * 
 * @param frame_number Frame number to encode
 * @param Height       Height of the screen
 * @param Width        Width of the screen
 * 
 * @return Texture
 */
Texture createTextureFromFrameNumber (int64_t frame_number, int Height, int Width);

enum CameraType {
    EXTERNAL = 0,
    SOFTWARE = 1
};

struct SimpleParameters {
    CameraType type = EXTERNAL;
    int Height      = 480;
    int Width       = 640;
    int fps         = 751;
};

class   SimpleCamera {
public:
    SimpleCamera (CameraType, void*);
    SimpleCamera (SimpleParameters);
    ~SimpleCamera();

    void                         start ();
    std::optional<torch::Tensor> read  ();
    torch::Tensor                read  (int);
    void                         open  ();
    void                         close ();
    void                         trigger ();
private:
    s3_Camera *ext_cam = nullptr;
    s3_Virtual_Camera *sft_cam = nullptr;
    Pylon::PylonAutoInitTerm init {};
};
}

#endif
