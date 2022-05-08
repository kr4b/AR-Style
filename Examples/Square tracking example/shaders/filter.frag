#version 150

out vec4 fragColor;
in vec2 vTexCoord;

uniform float width;
uniform float height;

uniform sampler2D frame;

#define radius 9
#define sigd 9.0
#define sigr 8.5

float weight(vec3 a, vec3 b) {
    vec3 d = a - b;
    float d2 = dot(d, d);
    return exp(-0.5 * d2 / (sigr * sigr));
}

vec3 unnormalize(vec3 c) {
    return vec3(100.0 * c.x, 2.0 * 127.0 * (c.y - 0.5), 2.0 * 127.0 * (c.z - 0.5));
}

vec3 renormalize(vec3 c) {
    return vec3(c.x / 100.0, 0.5 + 0.5 * (c.y / 127.0), 0.5 + 0.5 * (c.z / 127.0));
}

void main() {
    vec3 color = vec3(0.0);
    float norm = 1.0;
    vec3 p1 = unnormalize(texture(frame, vTexCoord).xyz);
    color += p1;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if (i == 0 && j == 0) {
                continue;
            }
            float dx = vTexCoord.x + i / width;
            float dy = vTexCoord.y + j / height;
            if (dx < 0 || dy < 0 || dx > 1.0 || dy > 1.0) {
                continue;
            }

            vec3 p2 = unnormalize(texture(frame, vec2(dx, dy)).xyz);
            float f = exp(-0.5 * (i * i + j * j) / (sigd * sigd));
            float w = weight(p1, p2);
            norm += f * w;
            color += p2 * f * w;
        }
    }

    fragColor = vec4(renormalize(color / norm), 1.0);
}
