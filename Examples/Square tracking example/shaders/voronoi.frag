#version 150 core

out vec4 fragColor;
in vec2 vTexCoord;

uniform sampler2D frame;
uniform sampler2D pattern;
uniform sampler1D centers;

uniform vec3 highlight;
uniform mat4 projection;
uniform mat4 view;

void main() {
    vec4 viewPos = projection * view * vec4(highlight, 1.0);
    if (highlight != vec3(0.0) && distance(vTexCoord * vec2(2.0, -2.0) - vec2(1.0, -1.0), viewPos.xy / viewPos.w) < 0.02) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        vec3 color = texture(pattern, vTexCoord).xyz * 255.0;
        int index = int(color.x) * 256 * 256 + int(color.y) * 256 + int(color.z);
        if (highlight == vec3(0.0)) {
            fragColor = vec4(texture(frame, texelFetch(centers, index, 0).xy).xyz, 1.0);
        } else {
            fragColor = vec4(texture(frame, vTexCoord).xyz, 1.0);
        }
    }
}
