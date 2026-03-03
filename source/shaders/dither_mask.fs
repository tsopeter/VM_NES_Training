#version 330
in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float uThreshold;

out vec4 finalColor;

// 4x4 Bayer matrix normalized (values in [0,1))
const float bayer[16] = float[16](
	0.0/16.0, 8.0/16.0, 2.0/16.0, 10.0/16.0,
	12.0/16.0, 4.0/16.0, 14.0/16.0, 6.0/16.0,
	3.0/16.0, 11.0/16.0, 1.0/16.0, 9.0/16.0,
	15.0/16.0, 7.0/16.0, 13.0/16.0, 5.0/16.0
);

void main() {
	vec4 texel = texture(texture0, fragTexCoord);

	// Use red channel as grayscale input
	float gray = texel.r;

	// Pixel coordinates for spatial dithering
	int mx = int(mod(gl_FragCoord.x, 4.0));
	int my = int(mod(gl_FragCoord.y, 4.0));
	int idx = my * 4 + mx;
	float threshold = bayer[idx] * uThreshold;

	// Compare grayscale value against the Bayer threshold
	if (gray > threshold) {
		finalColor = vec4(0.0, 0.0, 240.0/255.0, 1.0);
	} else {
		finalColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}