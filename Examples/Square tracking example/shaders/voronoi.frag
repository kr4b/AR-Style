#version 150 core

out vec4 fragColor;
in vec2 vTexCoord;

uniform sampler2D frame;
uniform sampler2D pattern;
uniform sampler1D centers;

uniform vec3 position;
uniform bool highlight;
uniform mat4 projection;
uniform mat4 view;

void main() {
    vec4 viewPos = projection * view * vec4(position, 1.0);
    vec2 screenPos = (1.0 + viewPos.xy / viewPos.w) / 2.0;
    if (highlight && distance(vTexCoord, screenPos) < 0.0025) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        if (highlight) {
            fragColor = vec4(texture(frame, vTexCoord).xyz, 1.0);
        } else {
            vec3 color = texture(pattern, vTexCoord).xyz * 255.0;
            if (color == vec3(0.0)) {
                fragColor = vec4(1.0, 0.0, 0.0, 1.0);
                // fragColor = texture(frame, vTexCoord);
            } else {
                int index = int(color.x) * 256 * 256 + int(color.y) * 256 + int(color.z);
                fragColor = vec4(texture(frame, texelFetch(centers, index, 0).xy).xyz, 1.0);
            }
        }
    }
}
