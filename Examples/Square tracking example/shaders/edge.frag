#version 150

out vec4 fragColor;
in vec2 vTexCoord;

uniform float width;
uniform float height;

uniform sampler2D frame;

#define radius 9
#define tau 0.995
#define sige 2.0
#define sigr 1.265 * sige
#define phi 5.0
#define ke 1 / (2.0 * 3.14159265 * sige * sige)
#define kr 1 / (2.0 * 3.14159265 * sigr * sigr)

void main() {
    float s1 = 0;
    float s2 = 0;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            float dx = vTexCoord.x + i / width;
            float dy = vTexCoord.y + j / height;
            if (dx < 0 || dy < 0 || dx > 1.0 || dy > 1.0) {
                continue;
            }

            float p = texture(frame, vec2(dx, dy)).x * 100.0;
            float d = -0.5 * (i * i + j * j);
            float f1 = exp(d / (sige * sige));
            float f2 = exp(d / (sigr * sigr));
            s1 += p * f1;
            s2 += p * f2;
        }
    }

    float s = ke * s1 - kr * tau * s2;
    float c = 1.0;
    if (s < 0) {
        c = 1 + tanh(phi * s);
    }
    vec3 p = texture(frame, vTexCoord).xyz;
    // fragColor = vec4(c, 0.0, 0.0, 1.0);
    fragColor = vec4(p.x * c, (p.y - 0.5) * c + 0.5, (p.z - 0.5) * c + 0.5, 1.0);
}
