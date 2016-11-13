
#include "SMAA.h"

varying vec2 texcoord, pixcoord;
varying vec4 svPosition;
varying vec4 offset[3];

uniform sampler2D edgesTex, areaTex, searchTex;

void main()
{
    gl_FragColor = SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset,
            edgesTex, areaTex, searchTex, int4(0, 0, 0, 0));
}
