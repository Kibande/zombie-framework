#version 140

uniform sampler2D tex;

in  vec4    ex_Color;
in  vec2    ex_UV;
in  float   ex_TexSel;

out vec4 out_Color;

void main()
{
    if (ex_TexSel < -0.01)
        out_Color = ex_Color;
    else
        out_Color = texture(tex, ex_UV) * ex_Color;
        //gl_FragColor = vec4(v_uv, 0.0, 1.0);
}
