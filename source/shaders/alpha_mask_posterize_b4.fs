#version 330

in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);

    // grayscale from red channel in [0,1]
    float gray = texel.r;

    // 5 output levels (blue channel, high nibble controls light):
    // 0b0000_0000 =   0   -> most light
    // 0b0001_0000 =  16
    // 0b0011_0000 =  48
    // 0b0111_0000 = 112
    // 0b1111_0000 = 240  -> least light
    //
    // thresholds are midpoints between these 8-bit values:
    // between  0  and 16  ->  8
    // between 16  and 48  -> 32
    // between 48  and 112 -> 80
    // between 112 and 240 -> 176

    float blue;

    if (gray < (  8.0 / 255.0)) {
        blue =   0.0 / 255.0;   // 0x00 -> brightest
    }
    else if (gray < ( 32.0 / 255.0)) {
        blue =  16.0 / 255.0;   // 0x10
    }
    else if (gray < ( 80.0 / 255.0)) {
        blue =  48.0 / 255.0;   // 0x30
    }
    else if (gray < (176.0 / 255.0)) {
        blue = 112.0 / 255.0;   // 0x70
    }
    else {
        blue = 240.0 / 255.0;   // 0xF0 -> darkest (most blocking)
    }

    // RGB = black, blue encodes mask, alpha = 1
    finalColor = vec4(0.0, 0.0, blue, 1.0);
}