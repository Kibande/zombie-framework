
#define AMBIENT     vec3(0.2, 0.2, 0.2)
#define DIFFUSE     vec3(0.5, 0.5, 0.5)
#define RANGE       5.0

varying vec4 colour;
varying vec3 n, v, lightPos;
varying vec2 uv;

uniform sampler2D tex[1];

void main()
{
    vec3 ray = lightPos - v;
    vec3 light = (AMBIENT + DIFFUSE * abs( dot(normalize(ray), n) )) * RANGE / ( 2.0 * length(ray) );
    gl_FragColor = colour * texture2D(tex[0], uv) * vec4(light, 1.0);
}