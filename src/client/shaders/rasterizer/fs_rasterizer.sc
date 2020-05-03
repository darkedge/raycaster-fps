$input v_texcoord0
// $input and $output must be defined first
// see defined in varying.def.sc

#include "../common.sh"

// Shader Resource Views
SAMPLER2DARRAY(s_TextureArray, 0);

void main()
{
  v_texcoord0.y = (1.0 - v_texcoord0.y);
  v_texcoord0.z = (2.0 * v_texcoord0.z - 1.0);
  gl_FragColor = texture2DArrayLod(s_TextureArray, v_texcoord0, 0);
}
