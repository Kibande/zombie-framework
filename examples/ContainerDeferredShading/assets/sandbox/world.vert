#version 140

in  vec3 in_Position;
in  vec3 in_Normal;
in  vec2 in_UV;

out vec3 ex_Position;
out vec3 ex_Normal;
out vec2 ex_UV;

uniform mat4 ModelMatrix;
uniform mat4 WorldMatrix;

uniform mat4 u_ModelViewProjectionMatrix;

void main( void )
{
    // Move the normals back from the camera space to the world space
    mat3 worldRotationInverse = transpose(mat3(WorldMatrix));
    
    gl_Position =   u_ModelViewProjectionMatrix * vec4(in_Position, 1.0);
    ex_Normal = in_Normal;
    ex_UV =         in_UV;
    //normals        = /*normalize(worldRotationInverse * gl_NormalMatrix * 
    //                           gl_Normal);*/
    //                 vec3(/*gl_ModelViewMatrix */ vec4(in_Normal, 0.0));
    ex_Position       = /*gl_ModelViewMatrix */ in_Position;
    //gl_FrontColor  = vec4(1.0, 1.0, 1.0, 1.0);
}
