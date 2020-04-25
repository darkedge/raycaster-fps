#include "../bgfx_compute.sh"
//#include "uniforms.sh"

// Shader Resource Views
BUFFER_RO(s_Grid, float, 0);
IMAGE2D_ARRAY_RO(s_TextureArray,rgba32f,1);
BUFFER_RO(s_Object, float, 2);
BUFFER_RO(s_Palette, float, 3);

IMAGE2D_RW(s_Texture, rgba32f, 5);

vec2 PixelToNDCSpace(uint x, uint y, uint width, uint height)
{
  return vec2(((float)x + 0.5) / width, ((float)y + 0.5) / height);
}

// aspect is x/y
vec2 NDCToScreenSpace(const float2 ndc, float aspect)
{
  return vec2((2.0 * ndc.x - 1.0) * aspect, 1.0 - 2.0 * ndc.y);
}

// fov is in radians
vec2 ScreenToCameraSpace(const float2 ss, float fov)
{
  return ss * tan(fov * 0.5);
}

NUM_THREADS(16, 16, 1)
void main()
{
  const int x = gl_GlobalInvocationID.x;
  const int y = gl_GlobalInvocationID.y;

  // TODO
  int width = 640;
  int height = 400;
  float s_FieldOfView = 45.0;
  if (x >= width || y >= height)
    return;

  vec2 ndc = PixelToNDCSpace(x, y, width, height);
  vec2 ss  = NDCToScreenSpace(ndc, (float)width / height);
  vec2 cs  = ScreenToCameraSpace(ss, radians(s_FieldOfView));

  imageStore(s_Texture, gl_GlobalInvocationID.xy, vec4(cs, 1.0, 1.0));
}
