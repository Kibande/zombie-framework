#version 140

in vec3 ex_Pos;
in vec3 ex_Normal;
in vec3 ex_LightSum;
in vec2 ex_UV;
in vec4 ex_Color;

out vec4 out_Color;

uniform sampler2D worldTex;
uniform vec4 blend_colour;

void main()
{
    out_Color = vec4(clamp(ex_LightSum * ex_Color.rgb, 0.0, 1.0), ex_Color.a)
            * blend_colour * texture2D(worldTex, ex_UV);
}
