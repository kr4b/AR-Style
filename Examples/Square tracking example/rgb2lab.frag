#version 150

out vec4 fragColor;
in vec2 vTexCoord;

uniform sampler2D frame;

vec3 rgb2xyz(vec3 c) {
    vec3 tmp;
    if (c.r > 0.04045) { tmp.x = pow((c.r + 0.055) / 1.055, 2.4); } else { tmp.x = c.r / 12.92; }
    if (c.g > 0.04045) { tmp.y = pow((c.g + 0.055) / 1.055, 2.4); } else { tmp.y = c.g / 12.92; }
    if (c.b > 0.04045) { tmp.z = pow((c.b + 0.055) / 1.055, 2.4); } else { tmp.z = c.b / 12.92; }

    const mat3 mat = mat3(
		0.4124, 0.3576, 0.1805,
        0.2126, 0.7152, 0.0722,
        0.0193, 0.1192, 0.9505
	);

    return 100.0 * mat * tmp;
}

vec3 xyz2lab(vec3 c) {
    vec3 n = c / vec3(95.047, 100, 108.883);
    vec3 v;
    if (n.x > 0.008856) { v.x = pow(n.x, 1.0 / 3.0); } else { v.x = (7.787 * n.x) + (16.0 / 116.0); }
    if (n.y > 0.008856) { v.y = pow(n.y, 1.0 / 3.0); } else { v.y = (7.787 * n.y) + (16.0 / 116.0); }
    if (n.z > 0.008856) { v.z = pow(n.z, 1.0 / 3.0); } else { v.z = (7.787 * n.z) + (16.0 / 116.0); }
    return vec3((116.0 * v.y) - 16.0, 500.0 * (v.x - v.y), 200.0 * (v.y - v.z));
}

vec3 rgb2lab(vec3 c) {
    vec3 lab = xyz2lab(rgb2xyz(c));
    return vec3(lab.x / 100.0, 0.5 + 0.5 * (lab.y / 127.0), 0.5 + 0.5 * (lab.z / 127.0));
}

void main() {
    fragColor = vec4(rgb2lab(texture(frame, vTexCoord).xyz), 1.0);
}
