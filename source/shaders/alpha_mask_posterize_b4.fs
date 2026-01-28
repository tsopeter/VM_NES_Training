#version 330

in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float uThreshold; // uThreshold controls the maximum value for darkest level

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);

    // grayscale from red channel in [0,1]
    float gray = texel.r;

    // 5 output levels (blue channel, high nibble controls light):
    // 0b0000_0000 =   0   -> most light
    // 0b0001_0000 =  16   b4
    // 0b0011_0000 =  48   b5
    // 0b0111_0000 = 112   b6
    // 0b1111_0000 = 240  -> least light (unused) b7
    

    float blue;
    //gray = 1.0 - gray; // invert gray so that 0=black, 1=white
    //gray *= uThreshold; // scale gray by threshold to control darkest level


    if (gray < 0.25 * uThreshold) {
        blue = 0.0 / 255.0;     // 0x00 -> brightest
    }
    else if (gray < 0.5 * uThreshold) {
        blue = 16.0 / 255.0;    // 0x10 0b0001_0000
    }
    else if (gray < 0.75 * uThreshold) {
        blue = 48.0 / 255.0;    // 0x30 0b0011_0000
    }
    else if (gray < 1.0 * uThreshold) {
        blue = 112.0 / 255.0;   // 0x70 0b0111_0000
        //blue = 240.0 / 255.0;   // 0xF0 0b1111_0000
    }
    else {
        blue = 240.0 / 255.0;   // 0xF0 0b1111_0000 -> darkest
    }

    // RGB = black, blue encodes mask, alpha = 1
    finalColor = vec4(0.0, 0.0, blue, 1.0);
}