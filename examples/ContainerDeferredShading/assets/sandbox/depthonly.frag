#version 140

in  vec3 ex_Normal;

out vec4 out_Color;

void main()
{
	out_Color = vec4(ex_Normal, 1.0);
}
