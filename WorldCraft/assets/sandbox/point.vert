#version 140

in  vec3 in_Position;
in  vec4 in_Color;
in  vec2 in_UV;
out vec4 ex_Color;
out vec2 ex_UV;

uniform mat4 u_ProjectionMatrix;

void main()
{
    gl_Position = u_ProjectionMatrix * vec4(in_Position, 1.0);
    ex_UV = vec2((in_Position.x + 1.0) * 0.5, (1.0 - in_Position.y) * 0.5);

    ex_Color = vec4(1.0, 1.0, 1.0, 1.0);
}
