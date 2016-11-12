#version 140

out vec4 out_Color;

uniform vec4 u_PickingColor;

void main()
{
    out_Color = u_PickingColor;
}
