#version 150 core

out vec4 fragColor;
in vec2 vTexCoord;

uniform sampler2D frame;
uniform sampler2D pattern;
uniform sampler1D centers;

void main() {
    vec3 color = texture(pattern, vTexCoord).xyz * 255.0;
    int index = int(color.x) * 256 * 256 + int(color.y) * 256 + int(color.z);
    fragColor = vec4(texture(frame, texelFetch(centers, index, 0).xy).xyz, 1.0);
}
