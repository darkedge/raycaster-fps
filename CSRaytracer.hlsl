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

struct Shape
{
  int type;
  float3 color;

  float3 originOrNormalOrMinXyz;
  float radiusOrDistanceOrMaxX;
  float maxY;
  float maxZ;
};

StructuredBuffer<Shape> s_Shapes : register(t0); // Shader resource binding

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
static inline float IntersectRayAABB(const Ray ray, const float3 amin, const float3 amax)
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
    return tmin;
  }
  else
  {
    return -1.0f;
  }
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

  int shapeId = -1;
  float t     = FLT_MAX;
  for (int i = 0; i < 6; i++)
  {
    Shape shape = s_Shapes[i];
    switch (shape.type)
    {
    case 0:
      t = IntersectRaySphere(ray, shape.originOrNormalOrMinXyz, shape.radiusOrDistanceOrMaxX);
      break;
    case 1:
      t = IntersectRayPlane(ray, shape.originOrNormalOrMinXyz, shape.radiusOrDistanceOrMaxX);
      break;
    case 2:
      t = IntersectRayAABB(ray, shape.originOrNormalOrMinXyz,
                           float3(shape.radiusOrDistanceOrMaxX, shape.maxY, shape.maxZ));
      break;
    case 3:
      // TODO
      break;
    default:
      break;
    }
    if (t >= 0.0f && t < ray.length)
    {
      ray.length = t;
      shapeId    = i;
    }
  }
  if (shapeId != -1)
  {
    float3 light = normalize(float3(0.3f, 0.6f, -1.0f));

    // Get intersection normal
    float3 normal             = float3(0.0f, 0.0f, 0.0f);
    const float3 intersection = ray.origin + ray.length * ray.direction;
    switch (s_Shapes[shapeId].type)
    {
    case 0:
      normal = (intersection - s_Shapes[shapeId].originOrNormalOrMinXyz) / s_Shapes[shapeId].radiusOrDistanceOrMaxX;
      break;
    case 1:
      normal = s_Shapes[shapeId].originOrNormalOrMinXyz;
      break;
    case 2:
    {
      float3 max = { s_Shapes[shapeId].radiusOrDistanceOrMaxX, s_Shapes[shapeId].maxY, s_Shapes[shapeId].maxZ };
      // https://blog.johnnovak.net/2016/10/22/the-nim-raytracer-project-part-4-calculating-box-normals/
      float3 c   = (s_Shapes[shapeId].originOrNormalOrMinXyz + max) * 0.5f; // aabb center
      float3 p   = intersection - c;                                        // vector from intersection to center
      float3 d   = (max - s_Shapes[shapeId].originOrNormalOrMinXyz) * 0.5f; //??
      float bias = 1.0001f;

      normal = normalize(float3((float)((int)(p.x / abs(d.x) * bias)), (float)((int)(p.y / d.y * bias)),
                                (float)((int)(p.z / d.z * bias))));
    }
    break;
    case 3:
      // TODO
      break;
    default:
      break;
    }

    float3 color = s_Shapes[shapeId].color;
    color *= clamp(dot(normal, light), 0.0f, 1.0f);
    color = sqrt(color);

    s_Texture[dispatchThreadId.xy] = float4(color, 1.0f);
  }
  else
  {
    s_Texture[dispatchThreadId.xy] = float4(1.0f, 0.0f, 1.0f, 1.0f);
  }
}
