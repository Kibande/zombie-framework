#version 100
precision mediump float;

uniform sampler2D tex;

varying vec4    ex_Color;
varying vec2    ex_UV;
varying float   ex_TexSel;

//out vec4 out_Color;

void main()
{
    if (ex_TexSel < -0.01)
        gl_FragColor = ex_Color;
    else
        gl_FragColor = texture2D(tex, ex_UV) * ex_Color;
        //gl_FragColor = vec4(v_uv, 0.0, 1.0);
}
