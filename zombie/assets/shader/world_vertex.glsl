
varying vec4 colour;
varying vec3 n, v;
varying vec2 uv;

void main()
{
    colour = gl_Color;

    n = normalize(gl_NormalMatrix * gl_Normal);
    v = vec3(gl_ModelViewMatrix * gl_Vertex);
    uv = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}