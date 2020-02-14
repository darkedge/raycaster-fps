#include "mj_raytracer.h"
#include "mj_common.h"
#include "mj_input.h"

#include "glm/gtc/matrix_transform.hpp"

using namespace mj::rt;

static Image s_Image;

static Sphere s_Sphere;
static Camera s_Camera;

bool mj::rt::Init()
{
  s_Sphere.origin = glm::vec3(0.0f, 0.0f, 10.0f);
  s_Sphere.radius = 1.0f;

  s_Camera.origin = glm::vec3(0.0f, 0.0f, 0.0f);
  s_Camera.direction = glm::vec3(0.0f, 0.0f, 1.0f);

  return true;
}

static bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, RaycastResult& result)
{
  glm::vec3 m = ray.origin - sphere.origin;
  float b = glm::dot(m, ray.direction);
  float c = glm::dot(m, m) - sphere.radius * sphere.radius;
  float determinant = b * b - c;
  if (determinant < 0)
  {
    return false;
  }
  else if (determinant == 0)
  {
    // One intersection
    result.length = -b;
    result.intersection = ray.origin - b * ray.direction;
    return true;
  }
  else
  {
    // Pick closest of two intersection
    float sqrt = glm::sqrt(determinant);
    float t = glm::min(-b + sqrt, -b - sqrt);
    result.length = t;
    result.intersection = ray.origin + t * ray.direction;
    return true;
  }
}

static glm::vec2 PixelToNDCSpace(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  return glm::vec2(((float) x + 0.5f) / MJ_WIDTH,
    ((float) y + 0.5f) / MJ_HEIGHT);
}

// aspect is x/y
static glm::vec2 NDCToScreenSpace(const glm::vec2& ndc, float aspect)
{
  return glm::vec2((2.0f * ndc.x - 1.0f) * aspect, 1.0f - 2.0f * ndc.y);
}

// fov is in radians
static glm::vec2 ScreenToCameraSpace(const glm::vec2& ss, float fov)
{
  return ss * glm::tan(fov * 0.5f);
}

// cs is camera space coordinates
static Ray CreateRay(const Camera& camera, const glm::vec2& cs)
{
  MJ_UNINITIALIZED Ray ray;
  ray.origin = camera.origin;
  ray.length = FLT_MAX;

  auto mat = glm::identity<glm::mat4>();
  mat = glm::translate(mat, camera.origin);
  ray.origin = camera.origin;
  glm::vec3 p = mat * glm::vec4(cs, 1, 1);
  ray.direction = glm::normalize(p - camera.origin);

  return ray;
}

void mj::rt::Update()
{
  for (uint16_t x = 0; x < MJ_WIDTH; x++)
  {
    for (uint16_t y = 0; y < MJ_HEIGHT; y++)
    {
      glm::vec2 ndc = PixelToNDCSpace(x, y, MJ_WIDTH, MJ_HEIGHT);
      glm::vec2 ss = NDCToScreenSpace(ndc, (float)MJ_WIDTH / MJ_HEIGHT);
      glm::vec2 cs = ScreenToCameraSpace(ss, glm::radians(90.0f));

      const Ray ray = CreateRay(s_Camera, cs);

      MJ_UNINITIALIZED RaycastResult result;
      if (IntersectRaySphere(ray, s_Sphere, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0xffffffff;
      }
      else
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0x000000ff;
      }
    }
  }
}

const Image& mj::rt::GetImage()
{
  return s_Image;
}

void mj::rt::Destroy()
{

}
