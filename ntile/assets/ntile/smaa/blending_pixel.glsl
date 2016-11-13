
#include "SMAA.h"

varying vec2 texcoord;
varying vec4 svPosition;
varying vec4 offset[2];

uniform sampler2D colorTex, blendTex;

void main()
{
    gl_FragColor = SMAANeighborhoodBlendingPS(texcoord, offset, colorTex, blendTex);
}
