#version 140

in  vec3 ex_Position;
in  vec3 ex_Normal;
in  vec2 ex_UV;

out vec4 out_Color;
out vec4 out_Position;
out vec4 out_Normal;

uniform sampler2D tex;

void main()
{
	out_Color = vec4(texture(tex, ex_UV).rgb, 1.0);
	out_Position = vec4(ex_Position, 1.0);

    if (ex_Normal.x >= -1.0 && ex_Normal.x <= 1.0)  //FIXMEEEEEJEIJEIEJEIJ
        out_Normal = vec4(ex_Normal, 1.0);
    else
        out_Normal = vec4(0.0, 0.0, 0.0, 1.0);
}
