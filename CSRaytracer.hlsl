#define FLT_MAX 3.402823466e+38F

struct Camera
{
  float3 position;
  float4 rotation;
};

struct Ray
{
  float3 origin;
  float3 direction;
  float length;
};

struct RaycastHit
{
  float t;
  float2 uv;
};

StructuredBuffer<uint> s_Grid : register(t0);

cbuffer Constants : register(b0)
{
  float4x4 mat;
  Camera s_Camera;
  float s_FieldOfView;
  int width;
  int height;
};

RWTexture2D<float4> s_Texture;

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

static inline bool IntersectRayGrid(const Ray ray, out RaycastHit result)
{
  // Step values
  const int stepX = ray.direction.x < 0 ? -1 : 1;
  const int stepZ = ray.direction.z < 0 ? -1 : 1;

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

  while (blockPosX >= 0 && blockPosX < 64 && blockPosZ >= 0 && blockPosZ < 64)
  {
    uint block = s_Grid[blockPosZ * 64 + blockPosX];

    if (block == 1)
    {
      if (tMax == tMaxZ)
      {
        tMax -= tDeltaZ;
        result.t = tMax;
        if (stepZ > 0)
        {
          result.uv.x = ray.origin.x + tMax * ray.direction.x - blockPosX;
        }
        else
        {
          result.uv.x = 1.0f - (ray.origin.x + tMax * ray.direction.x - blockPosX);
        }
        result.uv.y = ray.origin.y + tMax * ray.direction.y;
      }
      else
      {
        tMax -= tDeltaX;
        result.t = tMax;
        if (stepX > 0)
        {
          result.uv.x = 1.0f - (ray.origin.z + tMax * ray.direction.z - blockPosZ);
        }
        else
        {
          result.uv.x = ray.origin.z + tMax * ray.direction.z - blockPosZ;
        }
        result.uv.y = ray.origin.y + tMax * ray.direction.y;
      }
      return true;
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
      if (tMaxY < tMaxZ)
      {
        result.t    = tMaxY;
        result.uv.x = ray.origin.x + result.t * ray.direction.x - blockPosX;
        result.uv.y = ray.origin.z + result.t * ray.direction.z - blockPosZ;
        return true;
      }
      else
      {
        blockPosZ += stepZ;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
      }
    }
  }

  return false;
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
  RaycastHit hit;
  if (IntersectRayGrid(ray, hit))
  {
    ray.length = hit.t;

    float3 color = float3(1.0f, 1.0f, 1.0f);

    s_Texture[dispatchThreadId.xy] = float4(hit.uv, 0.0f, 1.0f);
  }
  else
  {
    s_Texture[dispatchThreadId.xy] = float4(1.0f, 0.0f, 1.0f, 1.0f);
  }
}
