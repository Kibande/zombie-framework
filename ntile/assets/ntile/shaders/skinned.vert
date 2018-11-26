#version 140

#define PT_COUNT 1

// All positions/directions except sun_dir are in eye space.

in  vec3 in_Position;
in  vec3 in_Normal;
in  vec2 in_UV;
in  vec4 in_Color;
in  vec4 in_Joints0;
in  vec4 in_Weights0;

out vec3 ex_Pos;
out vec3 ex_Normal;
out vec3 ex_LightSum;
out vec2 ex_UV;
out vec4 ex_Color;

uniform mat4 u_ModelViewProjectionMatrix;

uniform vec3 sun_dir, sun_amb, sun_diff, sun_spec;

uniform vec3 pt_pos[PT_COUNT],
        pt_amb[PT_COUNT],
        pt_diff[PT_COUNT],
        pt_spec[PT_COUNT];

uniform mat4 jointMatrices[20];

void main()
{
    mat4 skinMatrix =
        in_Weights0.x * jointMatrices[int(in_Joints0.x)] +
        in_Weights0.y * jointMatrices[int(in_Joints0.y)] +
        in_Weights0.z * jointMatrices[int(in_Joints0.z)] +
        in_Weights0.w * jointMatrices[int(in_Joints0.w)];

    ex_Pos = vec3( u_ModelViewProjectionMatrix * skinMatrix *vec4(in_Position, 1.0) );
    ex_Normal = normalize( /*gl_NormalMatrix */ in_Normal );
    ex_UV = in_UV;
    ex_Color = in_Color;

    gl_Position = u_ModelViewProjectionMatrix * skinMatrix * vec4(in_Position, 1.0);
    
    // Lighting

    if (in_Color.r < 0.5)
        ex_LightSum = sun_amb + sun_diff * (max(dot(in_Normal, sun_dir), 0.0)
                * max(3.0 * pow(dot(in_Normal, vec3(-0.707, 0.0, 0.707)), 4.0), 0.0));
    else
        ex_LightSum = sun_amb + sun_diff * max(dot(in_Normal, sun_dir), 0.0);

    for (int i = 0; i < PT_COUNT; i++)
    {
        vec3 lightDir = pt_pos[i] - ex_Pos;

        vec3 L = normalize( lightDir );
        float attenuation = 1.0 / (1.0 + length( lightDir ));

        ex_LightSum += attenuation * (pt_amb[i] + pt_diff[i] * max(dot(in_Normal, L), 0.0));
    }
}
