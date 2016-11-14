#version 100

attribute vec3 in_Position;
attribute vec4 in_Color;
attribute vec2 in_UV;
varying vec4 ex_Color;
varying vec2 ex_UV;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(in_Position, 1.0);

    ex_UV =     vec2(in_UV.x, 1.0 - in_UV.y);     // such is life with openGL
    ex_Color =  in_Color;
}
