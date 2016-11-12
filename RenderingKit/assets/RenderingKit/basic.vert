#version 140

in  vec3 in_Position;
in  vec4 in_Color;
out vec4 ex_Color;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(in_Position, 1.0);

    ex_Color =  in_Color;
}
