#version 150

out vec4 fragColor;
in vec2 vTexCoord;

uniform sampler2D frame;

vec3 lab2xyz(vec3 c) {
    float fy = (c.x + 16.0) / 116.0;
    float fx = c.y / 500.0 + fy;
    float fz = fy - c.z / 200.0;
    vec3 tmp;
    if (fx > 0.206897) { tmp.x = fx * fx * fx; } else { tmp.x = (fx - 16.0 / 116.0) / 7.787; }
    if (fy > 0.206897) { tmp.y = fy * fy * fy; } else { tmp.y = (fy - 16.0 / 116.0) / 7.787; }
    if (fz > 0.206897) { tmp.z = fz * fz * fz; } else { tmp.z = (fz - 16.0 / 116.0) / 7.787; }
    return vec3(95.047 * tmp.x, 100.000 * tmp.y, 108.883 * tmp.z);
}

vec3 xyz2rgb(vec3 c) {
	const mat3 mat = mat3(
        3.2406, -1.5372, -0.4986,
        -0.9689, 1.8758, 0.0415,
        0.0557, -0.2040, 1.0570
	);
    vec3 v = mat * c / 100.0;
    vec3 r;
    if (v.r > 0.0031308) { r.x = ((1.055 * pow(v.r, (1.0 / 2.4))) - 0.055); } else { r.x = 12.92 * v.r; }
    if (v.g > 0.0031308) { r.y = ((1.055 * pow(v.g, (1.0 / 2.4))) - 0.055); } else { r.y = 12.92 * v.g; }
    if (v.b > 0.0031308) { r.z = ((1.055 * pow(v.b, (1.0 / 2.4))) - 0.055); } else { r.z = 12.92 * v.b; }
    return r;
}

vec3 lab2rgb(vec3 c) {
    return xyz2rgb(lab2xyz(vec3(100.0 * c.x, 2.0 * 127.0 * (c.y - 0.5), 2.0 * 127.0 * (c.z - 0.5))));
}

void main() {
    fragColor = vec4(lab2rgb(texture(frame, vTexCoord).xyz), 1.0);
}
