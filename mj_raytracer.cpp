#include "mj_raytracer.h"
#include "mj_common.h"
#include "mj_input.h"
#include "camera.h"

#include "glm/gtc/matrix_transform.hpp"

using namespace mj::rt;

static Image s_Image;

static Sphere s_RedSphere;
static Sphere s_YellowSphere;
static Sphere s_BlueSphere;
static Sphere s_GreenSphere;
static Plane s_WhitePlane;
static Plane s_CyanPlane;
static Camera s_Camera;

static const float s_FieldOfView = 45.0f; // Degrees

bool mj::rt::Init()
{
  s_RedSphere.origin = glm::vec3(2.0f, 0.0f, 10.0f);
  s_RedSphere.radius = 1.0f;

  s_YellowSphere.origin = glm::vec3(0.0f, -2.0f, 10.0f);
  s_YellowSphere.radius = 1.0f;

  s_BlueSphere.origin = glm::vec3(0.0f, 2.0f, 10.0f);
  s_BlueSphere.radius = 1.0f;

  s_GreenSphere.origin = glm::vec3(-2.0f, 0.0f, 10.0f);
  s_GreenSphere.radius = 1.0f;

  s_WhitePlane.normal = glm::vec3(0.0f, -1.0f, 0.0f);
  s_WhitePlane.distance = 5.0f;

  s_CyanPlane.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  s_CyanPlane.distance = 5.0f;

  s_Camera.position = glm::vec3(0.0f, 0.0f, 0.0f);
  s_Camera.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
  CameraInit(MJ_REF s_Camera);

  return true;
}

static bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, RaycastResult& result)
{
  glm::vec3 m = ray.origin - sphere.origin;
  float b = glm::dot(m, ray.direction);
  float c = glm::dot(m, m) - sphere.radius * sphere.radius;
  if (c > 0.0f && b > 0.0f)
  {
    return false;
  }
  float determinant = b * b - c;
  if (determinant < 0)
  {
    return false;
  }
  else
  {
    // Pick closest of two intersections
    result.length = -b - glm::sqrt(determinant);
    result.intersection = ray.origin + result.length * ray.direction;
    result.normal = (result.intersection - sphere.origin) / sphere.radius;
    return true;
  }
}

static bool IntersectRayPlane(const Ray& ray, const Plane& plane, RaycastResult& result)
{
  result.length= -(glm::dot(ray.origin, plane.normal) + plane.distance) / glm::dot(ray.direction, plane.normal);
  if (result.length > 0.0f)
  {
    return true;
  }

  return false;
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

void mj::rt::Update()
{
  CameraMovement(MJ_REF s_Camera);

  MJ_UNINITIALIZED Ray ray;
  auto mat = glm::identity<glm::mat4>();
  mat = glm::translate(mat, s_Camera.position) * glm::mat4_cast(s_Camera.rotation);

  ray.origin = s_Camera.position;
  ray.length = FLT_MAX;

  // Reset button
  if (mj::input::GetKeyDown(Key::KeyR))
  {
    Init();
  }

  for (uint16_t x = 0; x < MJ_WIDTH; x++)
  {
    for (uint16_t y = 0; y < MJ_HEIGHT; y++)
    {
      glm::vec2 ndc = PixelToNDCSpace(x, y, MJ_WIDTH, MJ_HEIGHT);
      glm::vec2 ss = NDCToScreenSpace(ndc, (float) MJ_WIDTH / MJ_HEIGHT);
      glm::vec2 cs = ScreenToCameraSpace(ss, glm::radians(s_FieldOfView));

      //const Ray ray = CreateRay(s_Camera, cs);
      glm::vec3 p = mat * glm::vec4(cs, 1, 1);
      ray.direction = glm::normalize(p - s_Camera.position);

      MJ_UNINITIALIZED RaycastResult result;
      if (IntersectRaySphere(ray, s_RedSphere, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0xff0000ff;
      }
      else if (IntersectRaySphere(ray, s_YellowSphere, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0xffff00ff;
      }
      else if (IntersectRaySphere(ray, s_BlueSphere, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0x0000ffff;
      }
      else if (IntersectRaySphere(ray, s_GreenSphere, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0x00ff00ff;
      }
      else if (IntersectRayPlane(ray, s_WhitePlane, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0xffffffff;
      }
      else if (IntersectRayPlane(ray, s_CyanPlane, MJ_REF result))
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0x00ffffff;
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
