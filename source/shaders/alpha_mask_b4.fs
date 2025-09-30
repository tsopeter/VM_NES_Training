#version 330

in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);

    // Use red channel as grayscale input
    float gray = texel.r;

    if (gray > 0.5) {
        // Black with full opacity
        finalColor = vec4(0.0, 0.0, 240.0/255.0, 0.0);
    } else {
        // Black with full transparency
        finalColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}