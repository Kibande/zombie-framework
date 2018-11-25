#version 140

in  vec3 in_Position;
in  vec3 in_Normal;

out vec3 ex_Normal;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(in_Position, 1.0);
    ex_Normal = in_Normal;
}
