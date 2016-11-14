#version 100
precision mediump float;

varying vec4 ex_Color;
varying vec2 ex_UV;

//out vec4 out_Color;

uniform sampler2D tex;

void main()
{
    gl_FragColor = texture2D(tex, ex_UV) * ex_Color;
}
