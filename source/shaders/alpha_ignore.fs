#version 330

in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    texel.a = 1.0;                    // Force alpha to 1.0
    finalColor = texel * fragColor;
}