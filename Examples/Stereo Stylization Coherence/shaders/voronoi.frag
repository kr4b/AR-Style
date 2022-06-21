#version 150 core

out vec4 fragColor;
in vec2 vTexCoord;

uniform float width;
uniform float height;
uniform int cellWidth;
uniform int cellHeight;

uniform sampler2D frame;

void main() {
    float total = 0.0;
    for (int i = 0; i < cellWidth; i++) {
        for (int j = 0; j < cellHeight; j++) {
            float value = texture(frame, vTexCoord + vec2(float(i) / width, -float(j) / height)).x;
            if (value <= 10e-6) {
                fragColor = vec4(0.0);
                return;
            }
            total += value;
        }
    }

    fragColor = vec4(total / float(cellWidth * cellHeight));
}
