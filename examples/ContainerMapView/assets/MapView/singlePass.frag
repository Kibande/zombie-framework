#version 100
precision mediump float;

varying vec4 ex_Color;
varying vec2 ex_UV;
varying vec3 ex_Normal;

//out vec4 out_Color;

uniform sampler2D tex;

void main()
{
    vec3 lightDir = -normalize(vec3(0.3, -0.2, -1));

    gl_FragColor = texture2D(tex, ex_UV) * ex_Color * mix(0.9, 1.0, dot(ex_Normal, lightDir));
}
