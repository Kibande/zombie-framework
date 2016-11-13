
#include "SMAA.h"

varying vec2 texcoord;
varying vec4 svPosition;
varying vec4 offset[2];

void main()
{
    texcoord = gl_MultiTexCoord0.xy;
    SMAANeighborhoodBlendingVS(gl_Vertex, svPosition, texcoord, offset);

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
