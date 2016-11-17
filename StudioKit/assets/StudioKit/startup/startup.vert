#version 140

in  vec3 in_Position;
in  vec3 in_Normal;
in  vec4 in_Color;
in  vec2 in_UV;
out vec3 ex_Position;
out vec4 ex_Color;
out vec2 ex_UV;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(in_Position, 1.0);
    ex_Position = gl_Position.xyz;
}
