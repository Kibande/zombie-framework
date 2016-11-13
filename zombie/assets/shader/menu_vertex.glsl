
#define LIGHT_POS       vec4(0.0, 0.0, -1.0, 1.0)

varying vec4 colour;
varying vec3 n, v, lightPos;
varying vec2 uv;

void main()
{
    colour = gl_Color;

    n = normalize(gl_NormalMatrix * gl_Normal);
    v = vec3(gl_ModelViewMatrix * gl_Vertex);
    lightPos = vec3(gl_ModelViewMatrix * LIGHT_POS);
    uv = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}