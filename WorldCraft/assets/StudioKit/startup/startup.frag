#version 140

#define GREY    0.7

#define INNER   vec3(1.0, 1.0, 1.0)
#define OUTER   vec3(GREY, GREY, GREY)

in vec3 ex_Position;

out vec4 out_Color;

void main()
{
    float gradient = sqrt(ex_Position.x * ex_Position.x + ex_Position.y * ex_Position.y);

    out_Color = vec4(mix(INNER, OUTER, gradient), 1.0);
}
