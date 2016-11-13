
#define PT_COUNT 1

// All positions/directions except sun_dir are in eye space.

uniform vec3 sun_dir, sun_amb, sun_diff, sun_spec;

uniform vec3 pt_pos[PT_COUNT],
        pt_amb[PT_COUNT],
        pt_diff[PT_COUNT],
        pt_spec[PT_COUNT];

varying vec3 pos, normal, light_sum;
varying vec2 uv;
varying vec4 colour;

void main()
{
    pos = vec3( gl_ModelViewMatrix * gl_Vertex );
    normal = normalize( /*gl_NormalMatrix */ gl_Normal );
    uv = gl_MultiTexCoord0.xy;
    colour = gl_Color;

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    
    // Lighting

    if (gl_Color.r < 0.5)
        light_sum = sun_amb + sun_diff * (max(dot(normal, sun_dir), 0.0)
                * max(3.0 * pow(dot(normal, vec3(-0.707, 0.0, 0.707)), 4.0), 0.0));
    else
        light_sum = sun_amb + sun_diff * max(dot(normal, sun_dir), 0.0);

    for (int i = 0; i < PT_COUNT; i++)
    {
        vec3 lightDir = pt_pos[i] - pos;

        vec3 L = normalize( lightDir );
        float attenuation = 1.0 / (1.0 + length( lightDir ));

        light_sum += attenuation * (pt_amb[i] + pt_diff[i] * max(dot(normal, L), 0.0));
    }
}