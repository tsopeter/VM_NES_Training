#include "utils.hpp"


void s5_Utils::DrawTextureProAffine (
    Texture2D texture,
    Affine affine,
    int window_Width,
    int window_Height
) {
    // Source rectangle: whole texture
    Rectangle src = { 0, 0, (float)texture.width, (float)texture.height };

    float scale_x = 1.0f/affine.scale_x;
    float scale_y = 1.0f/affine.scale_y;
    float degrees = affine.rotation * 180.0f / 3.14159265f; // convert to degrees

    float offset_x = -affine.offset_x * window_Width / 2;
    float offset_y = -affine.offset_y * window_Height / 2;

    // Destination rectangle: scaled and offset
    float start_x = window_Width/2 + offset_x * scale_x;
    float start_y = window_Height/2 + offset_y * scale_y;
    float size_x   = window_Width * scale_x;
    float size_y   = window_Height * scale_y;
    Rectangle dst = {
        start_x,                        // translation in pixels
        start_y,
        size_x,
        size_y
    };

    Vector2 origin = {(float)window_Width/2 * scale_x, (float)window_Height/2 * scale_y};
    DrawTexturePro(texture, src, dst, origin, degrees, WHITE);
}

s5_Utils::Affine::Affine () 
    : scale_x(1.0f), scale_y(1.0f), offset_x(0.0f), offset_y(0.0f), rotation(0.0f) {
}