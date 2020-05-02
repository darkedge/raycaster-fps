$input v_texcoord0
// $input and $output must be defined first
// see defined in varying.def.sc

#include "../common.sh"

// Equivalent:
// uniform SamplerState SampleTypeSampler : register(s[0]);
// uniform Texture2D SampleTypeTexture : register(t[0]);
SAMPLER2D(SampleType, 0);

void main()
{
    gl_FragColor = texture2D(SampleType, v_texcoord0);
}
