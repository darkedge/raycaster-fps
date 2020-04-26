#include "../bgfx_compute.sh"
//#include "uniforms.sh"

#define FLT_MAX 3.402823466e+38F
#define EPSILON 0.00001f

struct Ray
{
  float3 origin;
  float3 direction;
  float length;
};

// Shader Resource Views
BUFFER_RO(s_Grid, float, 0);
SAMPLER2DARRAY(s_TextureArray,1);
BUFFER_RO(s_Object, float, 2);
BUFFER_RO(s_Palette, float, 3);

// Constants
uniform mat4 mat;
uniform vec4 s_CameraPos;
uniform vec4 s_FieldOfView;
uniform vec4 width;
uniform vec4 height;

IMAGE2D_RW(s_Texture, rgba32f, 5);

const uint k = 1664525U; // Numerical Recipes

vec3 hash(uint3 x)
{
  x = ((x >> 8U) ^ x.yzx) * k;
  x = ((x >> 8U) ^ x.yzx) * k;
  x = ((x >> 8U) ^ x.yzx) * k;

  return vec3(x) * (1.0 / float(0xffffffffU));
}

vec4 AttenuateSample(float t, vec4 smpl)
{
  return smpl * rsqrt(t);
}

// https://www.gamedev.net/forums/topic/662529-converting-uint-to-vec4-in-hlsl/
vec4 MGetVertexColour(uint inCol)
{
  float a = ((inCol & 0xff000000) >> 24);
  float b = ((inCol & 0xff0000) >> 16);
  float g = ((inCol & 0xff00) >> 8);
  float r = ((inCol & 0xff));
  return vec4(r, g, b, a) * 0.00392156862f; // 1/255
}

float IntersectObject(vec3 pos, vec3 dir, out vec4 objectColor, int stepX, int stepY, int stepZ,
                                    float tDeltaX, float tDeltaY, float tDeltaZ, out float t)
{
  // t values
  float tMax  = 0.0;
  float tMaxX = (dir.x > 0 ? ceil(pos.x) - pos.x : pos.x - floor(pos.x)) * tDeltaX;
  float tMaxY = (dir.y > 0 ? ceil(pos.y) - pos.y : pos.y - floor(pos.y)) * tDeltaY;
  float tMaxZ = (dir.z > 0 ? ceil(pos.z) - pos.z : pos.z - floor(pos.z)) * tDeltaZ;

  int blockPosX = floor(pos.x);
  int blockPosY = floor(pos.y);
  int blockPosZ = floor(pos.z);

  while ((blockPosX >= 0) && (blockPosX < 64) && //
         (blockPosY >= 0) && (blockPosY < 64) && //
         (blockPosZ >= 0) && (blockPosZ < 64))
  {
    float color = s_Object[blockPosZ * 64 * 64 + blockPosY * 64 + blockPosX];

    if (color != 0.0)
    {
      objectColor = MGetVertexColour(s_Palette[color - 1]);
      t           = tMax * 0.015625f; // (1.0 / 64.0)
      return true;
    }

    if (tMaxX < tMaxY)
    {
      if (tMaxX < tMaxZ)
      {
        blockPosX += stepX;
        tMax = tMaxX;
        tMaxX += tDeltaX;
      }
      else
      {
        blockPosZ += stepZ;
        tMax = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    }
    else
    {
      if (tMaxY < tMaxZ)
      {
        blockPosY += stepY;
        tMax = tMaxY;
        tMaxY += tDeltaY;
      }
      else
      {
        blockPosZ += stepZ;
        tMax = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    }
  }

  return false;
}

vec4 IntersectRayGrid(const Ray ray)
{
  float t;
  float u;
  float v;

  // Step values
  const int stepX = ray.direction.x < 0.0 ? -1 : 1;
  const int stepY = ray.direction.y < 0.0 ? -1 : 1;
  const int stepZ = ray.direction.z < 0.0 ? -1 : 1;

  // Inverse direction
  const float tDeltaX = abs(1.0 / ray.direction.x);
  const float tDeltaY = abs(1.0 / ray.direction.y);
  const float tDeltaZ = abs(1.0 / ray.direction.z);

  // t values
  float tMax  = 0.0;
  float tMaxX = (((stepX + 1) >> 1) - stepX * frac(ray.origin.x)) * tDeltaX;
  float tMaxY = (((stepY + 1) >> 1) - stepY * frac(ray.origin.y)) * tDeltaY;
  float tMaxZ = (((stepZ + 1) >> 1) - stepZ * frac(ray.origin.z)) * tDeltaZ;

  int blockPosX   = floor(ray.origin.x);
  int blockPosZ   = floor(ray.origin.z);
  bool horizontal = false;

  while ((blockPosX >= 0) && (blockPosX < 64) && (blockPosZ >= 0) && (blockPosZ < 64))
  {
    if (blockPosX == 54 && blockPosZ == 37)
    {
      vec3 pos = (ray.origin + (tMax + EPSILON) * ray.direction - vec3(blockPosX, 0.0, blockPosZ)) * 64.0;

      vec4 objectColor;
      float tObj;
      if (IntersectObject(pos, ray.direction, objectColor, stepX, stepY, stepZ, tDeltaX, tDeltaY, tDeltaZ, tObj))
      {
        return AttenuateSample(tMax + tObj, objectColor);
      }
    }
    float block = s_Grid[blockPosZ * 64 + blockPosX];

    if (block == 1.0) // Wall hit
    {
      if (horizontal) // Walls along X axis
      {
        u = ((-stepZ + 1) >> 1) + stepZ * (ray.origin.x + tMax * ray.direction.x - blockPosX);
        v = ray.origin.y + tMax * ray.direction.y;
      }
      else // Walls along Z axis
      {
        u = ((stepX + 1) >> 1) - stepX * (ray.origin.z + tMax * ray.direction.z - blockPosZ);
        v = ray.origin.y + tMax * ray.direction.y;
      }

      return AttenuateSample(tMax, texture2DArrayLod(s_TextureArray, vec3(u, (1.0 - v), 0.0), 0));
    }

    if (tMaxX < tMaxY)
    {
      if (tMaxX < tMaxZ)
      {
        blockPosX += stepX;
        tMax = tMaxX;
        tMaxX += tDeltaX;
        horizontal = false;
      }
      else
      {
        horizontal = true;
        blockPosZ += stepZ;
        tMax = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    }
    else
    {
      if (tMaxY < tMaxZ) // Floor/ceiling hit
      {
        t = tMaxY;
        u = ray.origin.x + t * ray.direction.x - blockPosX;
        v = ray.origin.z + t * ray.direction.z - blockPosZ;

        if (ray.direction.y > 0)
        {
          return AttenuateSample(t, texture2DArrayLod(s_TextureArray, vec3(u, v, 2.0), 0));
        }
        else
        {
          return AttenuateSample(t, texture2DArrayLod(s_TextureArray, vec3(u, v, 1.0), 0));
        }
      }
      else
      {
        horizontal = true;
        blockPosZ += stepZ;
        tMax = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    }
  }

  return vec4(1.0, 0.0, 1.0, 1.0);
}

float IntersectRaySphere(const Ray ray, const vec3 origin, float radius)
{
  vec3 m  = ray.origin - origin;
  float b   = dot(m, ray.direction);
  float c   = dot(m, m) - radius * radius;
  float ret = -1.0;
  if (!(c > 0.0 && b > 0.0))
  {
    float determinant = b * b - c;
    if (determinant >= 0)
    {
      ret = -b - sqrt(determinant);
    }
  }

  return ret;
}

float IntersectRayPlane(const Ray ray, vec3 normal, float distance)
{
  return -(dot(ray.origin, normal) + distance) / dot(ray.direction, normal);
}

vec2 PixelToNDCSpace(uint x, uint y, uint width, uint height)
{
  return vec2(((float)x + 0.5) / width, ((float)y + 0.5) / height);
}

// aspect is x/y
vec2 NDCToScreenSpace(const vec2 ndc, float aspect)
{
  return vec2((2.0 * ndc.x - 1.0) * aspect, 1.0 - 2.0 * ndc.y);
}

// fov is in radians
vec2 ScreenToCameraSpace(const vec2 ss, float fov)
{
  return ss * tan(fov * 0.5);
}

NUM_THREADS(16, 16, 1)
void main()
{
  const int x = gl_GlobalInvocationID.x;
  const int y = gl_GlobalInvocationID.y;

  if (x >= width.x || y >= height.x)
    return;

  vec2 ndc = PixelToNDCSpace(x, y, width.x, height.x);
  vec2 ss  = NDCToScreenSpace(ndc, (float)width.x / height.x);
  vec2 cs  = ScreenToCameraSpace(ss, radians(s_FieldOfView));

  //imageStore(s_Texture, gl_GlobalInvocationID.xy, vec4(cs, 1.0, 1.0));

  Ray ray;
  float3 p      = mul(mat, float4(cs, 1, 1)).xyz;
  ray.origin    = s_CameraPos.xyz;
  ray.length    = FLT_MAX;
  ray.direction = normalize(p - s_CameraPos.xyz);

  // Do grid intersection
  imageStore(s_Texture, gl_GlobalInvocationID.xy, IntersectRayGrid(ray));
}
