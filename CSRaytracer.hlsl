#define FLT_MAX 3.402823466e+38F
#define EPSILON 0.00001f

struct Camera
{
  float3 position;
  int frame;
  float4 rotation;
};

struct Ray
{
  float3 origin;
  float3 direction;
  float length;
};

// Shader Resource Views
StructuredBuffer<uint> s_Grid : register(t0);
Texture2DArray s_TextureArray : register(t1);
StructuredBuffer<uint> s_Object : register(t2);
StructuredBuffer<uint> s_Palette : register(t3);

sampler Sampler : register(s0);

cbuffer Constants : register(b0)
{
  float4x4 mat;
  Camera s_Camera;
  float s_FieldOfView;
  int width;
  int height;
};

RWTexture2D<float4> s_Texture;

static const uint k = 1664525U; // Numerical Recipes

float3 hash(uint3 x)
{
  x = ((x >> 8U) ^ x.yzx) * k;
  x = ((x >> 8U) ^ x.yzx) * k;
  x = ((x >> 8U) ^ x.yzx) * k;

  return float3(x) * (1.0 / float(0xffffffffU));
}

#if 0
// https://gamedev.stackexchange.com/a/146362
static inline bool IntersectRayAABB(const Ray ray, const float3 amin, const float3 amax, out RaycastHit result)
{
  float3 dirInv = 1.0f / ray.direction;

  float t1 = (amin.x - ray.origin.x) * dirInv.x;
  float t2 = (amax.x - ray.origin.x) * dirInv.x;

  float tmin = min(t1, t2);
  float tmax = max(t1, t2);

  t1 = (amin.y - ray.origin.y) * dirInv.y;
  t2 = (amax.y - ray.origin.y) * dirInv.y;

  tmin = max(tmin, min(t1, t2));
  tmax = min(tmax, max(t1, t2));

  t1 = (amin.z - ray.origin.z) * dirInv.z;
  t2 = (amax.z - ray.origin.z) * dirInv.z;

  tmin = max(tmin, min(t1, t2));
  tmax = min(tmax, max(t1, t2));

  if (tmax >= tmin)
  {
    result.t  = tmin;
    result.uv = float2(1.0f, 0.0f);
    return true;
  }
  else
  {
    return false;
  }
}
#endif

static inline float4 AttenuateSample(float t, float4 smpl)
{
  return smpl * rsqrt(t);
}

// https://www.gamedev.net/forums/topic/662529-converting-uint-to-float4-in-hlsl/
float4 MGetVertexColour(uint inCol)
{
  if (inCol == 0xffffffff)
    return float4(1, 1, 1, 1);

  float a = ((inCol & 0xff000000) >> 24);
  float b = ((inCol & 0xff0000) >> 16);
  float g = ((inCol & 0xff00) >> 8);
  float r = ((inCol & 0xff));
  return float4(r, g, b, a) * 0.00392156862f; // 1/255
}

static inline float IntersectObject(float3 pos, float3 dir, out float4 objectColor, int stepX, int stepY, int stepZ,
                                    float tDeltaX, float tDeltaY, float tDeltaZ, out float t)
{
  // Remap ray position to object space
  pos = frac(pos) * 64.0f;

  // TODO: reuse values from IntersectRayGridfloat t;

  // t values
  float tMax  = 0.0f;
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
    uint color = s_Object[blockPosZ * 64 * 64 + blockPosY * 64 + blockPosX];

    if (color != 0)
    {
      objectColor = MGetVertexColour(s_Palette[color - 1]);
      t = tMax * 0.015625f;
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

static inline float4 IntersectRayGrid(const Ray ray)
{
  float t;
  float u;
  float v;

  // Step values
  const int stepX = ray.direction.x < 0.0f ? -1 : 1;
  const int stepY = ray.direction.y < 0.0f ? -1 : 1;
  const int stepZ = ray.direction.z < 0.0f ? -1 : 1;

  // Inverse direction
  const float tDeltaX = abs(1.0f / ray.direction.x);
  const float tDeltaY = abs(1.0f / ray.direction.y);
  const float tDeltaZ = abs(1.0f / ray.direction.z);

  // t values
  float tMax = 0.0f;
  float tMaxX =
      (ray.direction.x > 0 ? ceil(ray.origin.x) - ray.origin.x : ray.origin.x - floor(ray.origin.x)) * tDeltaX;
  float tMaxY =
      (ray.direction.y > 0 ? ceil(ray.origin.y) - ray.origin.y : ray.origin.y - floor(ray.origin.y)) * tDeltaY;
  float tMaxZ =
      (ray.direction.z > 0 ? ceil(ray.origin.z) - ray.origin.z : ray.origin.z - floor(ray.origin.z)) * tDeltaZ;

  int blockPosX = floor(ray.origin.x);
  int blockPosZ = floor(ray.origin.z);

  while ((blockPosX >= 0) && (blockPosX < 64) && (blockPosZ >= 0) && (blockPosZ < 64))
  {
    if (blockPosX == 54 && blockPosZ == 37)
    {
      float t;
      if (tMax == 0.0f)
      {
        t = tMax;
      }
      else if (tMax == tMaxZ)
      {
        t = tMax - tDeltaZ;
      }
      else if (tMax == tMaxY)
      {
        t = tMax - tDeltaY;
      }
      else
      {
        t = tMax - tDeltaX;
      }

      float4 objectColor;
      float tObj;
      if (IntersectObject(ray.origin + (t + EPSILON) * ray.direction, ray.direction, objectColor, stepX, stepY,
                                   stepZ, tDeltaX, tDeltaY, tDeltaZ, tObj))
      {
        return AttenuateSample(t + tObj, objectColor);
      }
    }
    uint block = s_Grid[blockPosZ * 64 + blockPosX];

    if (block == 1) // Wall hit
    {
      if (tMax == tMaxZ)
      {
        tMax -= tDeltaZ;
        t = tMax;
        if (stepZ > 0)
        {
          u = ray.origin.x + tMax * ray.direction.x - blockPosX;
        }
        else
        {
          u = 1.0f - (ray.origin.x + tMax * ray.direction.x - blockPosX);
        }
        v = ray.origin.y + tMax * ray.direction.y;
      }
      else
      {
        tMax -= tDeltaX;
        t = tMax;
        if (stepX > 0)
        {
          u = 1.0f - (ray.origin.z + tMax * ray.direction.z - blockPosZ);
        }
        else
        {
          u = ray.origin.z + tMax * ray.direction.z - blockPosZ;
        }
        v = ray.origin.y + tMax * ray.direction.y;
      }

      return AttenuateSample(t, s_TextureArray.SampleLevel(Sampler, float3(u, (1.0f - v), 0.0f), 0));
    }

    if (tMaxX < tMaxY)
    {
      if (tMaxX < tMaxZ)
      {
        blockPosX += stepX;
        tMaxX += tDeltaX;
        tMax = tMaxX;
      }
      else
      {
        blockPosZ += stepZ;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
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
          return AttenuateSample(t, s_TextureArray.SampleLevel(Sampler, float3(u, v, 2.0f), 0));
        }
        else
        {
          return AttenuateSample(t, s_TextureArray.SampleLevel(Sampler, float3(u, v, 1.0f), 0));
        }
      }
      else
      {
        blockPosZ += stepZ;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
      }
    }
  }

  return float4(1.0f, 0.0f, 1.0f, 1.0f);
}

static inline float IntersectRaySphere(const Ray ray, const float3 origin, float radius)
{
  float3 m  = ray.origin - origin;
  float b   = dot(m, ray.direction);
  float c   = dot(m, m) - radius * radius;
  float ret = -1.0f;
  if (!(c > 0.0f && b > 0.0f))
  {
    float determinant = b * b - c;
    if (determinant >= 0)
    {
      ret = -b - sqrt(determinant);
    }
  }

  return ret;
}

static inline float IntersectRayPlane(const Ray ray, float3 normal, float distance)
{
  return -(dot(ray.origin, normal) + distance) / dot(ray.direction, normal);
}

static inline float2 PixelToNDCSpace(uint x, uint y, uint width, uint height)
{
  return float2(((float)x + 0.5f) / width, ((float)y + 0.5f) / height);
}

// aspect is x/y
static inline float2 NDCToScreenSpace(const float2 ndc, float aspect)
{
  return float2((2.0f * ndc.x - 1.0f) * aspect, 1.0f - 2.0f * ndc.y);
}

// fov is in radians
static inline float2 ScreenToCameraSpace(const float2 ss, float fov)
{
  return ss * tan(fov * 0.5f);
}

// clang-format off
[numthreads(16,16,1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
// clang-format on
{
  const int x = dispatchThreadId.x;
  const int y = dispatchThreadId.y;

  if (x >= width || y >= height)
    return;

  float2 ndc = PixelToNDCSpace(x, y, width, height);
  float2 ss  = NDCToScreenSpace(ndc, (float)width / height);
  float2 cs  = ScreenToCameraSpace(ss, radians(s_FieldOfView));

  Ray ray;
  float3 p      = mul(mat, float4(cs, 1, 1)).xyz;
  ray.origin    = s_Camera.position;
  ray.length    = FLT_MAX;
  ray.direction = normalize(p - s_Camera.position);

  // Do grid intersection
  s_Texture[dispatchThreadId.xy] = IntersectRayGrid(ray);
}
