// mask_frag.glsl  (GLSL 330 for desktop OpenGL)
#version 330

in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D uBase;

uniform sampler2D uTex;

// For tiling uBase
uniform float uTileX;
uniform float uTileY;

out vec4 finalColor;

float apply(int index, float value) {
    int q = int(round(value * 255.0));
    q = q | (1 << index);
    return float(q) / 255.0;
}

vec2 remapUV(vec2 uv, int idx) {
    float v = (uv.y + float(idx)) / 20.0;
    return vec2(uv.x, v);

}

void main() {
    vec3 acc = vec3(0.0);
    vec2 tiledCoord = fragTexCoord * vec2(uTileX, uTileY);
    acc = texture(uBase, tiledCoord).rgb;
    acc = vec3(0.0);

    for (int i = 0; i < 20; i++) {
        int index = i % 8;
        int channel = i / 8;

        vec2 coord = remapUV(fragTexCoord, i);
        float gray = texture(uTex, coord).r;

        if (gray < 0.5) {
            if (channel == 0) {
                acc.r = apply(index, acc.r);
            }
            else if (channel == 1) {
                acc.g = apply(index, acc.g);
            }
            else if (channel == 2) {
                acc.b = apply(index, acc.b);
            }
        }
    }

    finalColor = vec4(acc, 1.0);
}