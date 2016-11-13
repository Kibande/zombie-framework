
varying vec3 pos, normal, light_sum;
varying vec2 uv;
varying vec4 colour;

uniform sampler2D worldTex;
uniform vec4 blend_colour;

void main()
{
    gl_FragColor = vec4(clamp(light_sum * colour.rgb, 0.0, 1.0), colour.a)
            * blend_colour * texture2D(worldTex, uv);
}
