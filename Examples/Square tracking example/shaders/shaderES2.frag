#ifdef GL_ES
precision mediump float;
#endif

in vec2 vTexCoord;
uniform float width;
uniform float height;
uniform sampler2D frame;

void main() {
    gl_FragColor = texture(frame, vTexCoord);
};
