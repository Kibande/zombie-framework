
#include "SMAA.h"

varying vec2 texcoord, pixcoord;
varying vec4 svPosition;
varying vec4 offset[3];

void main()
{
    texcoord = gl_MultiTexCoord0.xy;
    SMAABlendingWeightCalculationVS(gl_Vertex, svPosition, texcoord, pixcoord, offset);

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
