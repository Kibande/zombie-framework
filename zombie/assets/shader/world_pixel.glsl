
uniform vec3 ambient;

uniform int pt_count;
uniform vec3 pt_pos[4], pt_ambient[4], pt_diffuse[4];
uniform float pt_range[4];

varying vec4 colour;
varying vec3 n, v;
varying vec2 uv;

uniform sampler2D tex[1];

void main()
{
    vec3 light = ambient;

    for (int i = 0; i < pt_count; i++)
    {
        vec3 ray = pt_pos[i] - v;
        light += ( pt_ambient[i] + pt_diffuse[i] * max( dot( normalize( ray ), n ), 0.0 ) ) * pt_range[i] / (2.0 * length(ray));
    }

    gl_FragColor = colour * texture2D(tex[0], uv) * vec4(light, 1.0);
}