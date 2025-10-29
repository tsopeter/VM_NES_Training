#ifndef s5_utils_hpp__
#define s5_utils_hpp__

#include "raylib.h"

namespace s5_Utils {
    // Note: values are [-1, 1]
    struct Affine {
        float scale_x;
        float scale_y;
        float offset_x;
        float offset_y;

        Affine ();
    };

    void DrawTextureProAffine (
        Texture2D texture,
        Affine affine,
        int window_Width,
        int window_Height
    );
};


#endif
