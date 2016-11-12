#version 140

in  vec2    in_Position;
in  vec2    in_UV;
in  float   in_TexSel;
in  vec4    in_Color;

out vec4    ex_Color;
out vec2    ex_UV;
out float   ex_TexSel;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(in_Position, 0.0, 1.0);

    ex_UV =     vec2(in_UV.x, 1.0 - in_UV.y);     // such is life with openGL
    ex_TexSel = in_TexSel;
    ex_Color =  in_Color;
}
