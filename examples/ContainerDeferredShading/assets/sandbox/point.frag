#version 140

in  vec4 ex_Color;
in  vec2 ex_UV;

out vec4 out_Color;

uniform sampler2D diffuseBuf; 
uniform sampler2D positionBuf;
uniform sampler2D normalsBuf;

uniform sampler2DShadow shadowMap;
uniform mat4 shadowMatrix;

uniform vec3 lightPos;
uniform vec3 lightAmbient, lightDiffuse;
uniform float lightRange;
uniform vec3 cameraPos;

const float shift_by_normal = 0.1f;
const float outward_shadowing_factor = 2.0f;
const float sampling_z_offset = -0.00005f;

const int pcf_samples = 3;
const float pcf_radius = 0.012f;
const float pcf_dist = pcf_radius / pcf_samples;

float calcShadow(vec4 position)
{
    mat4 bias = mat4(0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0);

    vec4 coord = bias * (shadowMatrix * position);
    coord.xyz *= 1.0 / coord.w;

    vec2 centerDist = abs(coord.xy - vec2(0.5, 0.5));
    float maxCenterDist = max(centerDist.x, centerDist.y);

    if (maxCenterDist > 0.499)
        return 0.0;

    float total = 0.0f;

    for (int x = 0; x < pcf_samples; x++)
        for (int y = 0; y < pcf_samples; y++)
        {
            const float base = -(pcf_samples - 1) * pcf_dist * 0.5;
            vec3 offset = vec3(base + x * pcf_dist, base + y * pcf_dist, sampling_z_offset);

            total += texture(shadowMap, coord.xyz + offset) * (1.0f / pow(pcf_samples, 2));
        }

    return total;
}

void main()
{
    vec3 image = texture( diffuseBuf, ex_UV ).xyz;
    vec3 position = texture( positionBuf, ex_UV ).xyz;
    vec3 normal = texture( normalsBuf, ex_UV ).xyz;

    float lightDist = length(lightPos - position);
    vec3 lightDir = normalize(lightPos - position);

    normal = normalize(normal);

    float attenuation = 1.0 / (1.0 + pow(lightDist / lightRange, 2.0));

    //vec3 eyeDir = normalize(cameraPos - position);
    //vec3 vHalfVector = normalize(lightDir + eyeDir);

    float lightDot = dot(normal, lightDir);

    vec3 lightColour = (lightAmbient
            + lightDiffuse * max(lightDot, 0.0)
            //+ pow(max(dot(normal,vHalfVector),0.0), 100) * 1.5
            );

    // Shadow Mapping:
    vec3 shadowPos = position + normal * shift_by_normal;
    float sam = calcShadow(vec4(shadowPos, 1.0));

    // apply more shadowing to outward facing polygons
    sam *= max(1.0 + min(lightDot * outward_shadowing_factor, 0.0), 0.0);

    /*if (attenuation < 0.02)
        lightColour *= vec3(0.0, 1.0, 1.0);

    if (abs(attenuation - 0.5) < 0.01)
        lightColour *= vec3(1.0, 0.0, 1.0);*/

    lightColour *= mix(1.0, sam, 0.7);

    vec3 gamma = vec3(1.0 / 2.2);
    out_Color = vec4(lightColour * attenuation * image
            , 1.0);
}
