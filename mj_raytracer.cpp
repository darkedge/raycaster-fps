#include "mj_raytracer.h"
#include "mj_common.h"
#include "mj_input.h"
#include "camera.h"

#include "glm/gtc/matrix_transform.hpp"

using namespace mj::rt;

static Image s_Image;

enum DemoShape {
  DemoShape_RedSphere,
  DemoShape_YellowSphere,
  DemoShape_BlueSphere,
  DemoShape_GreenSphere,
  DemoShape_WhitePlane,
  DemoShape_CyanPlane,
  DemoShape_Count
};

static Shape s_Shapes[DemoShape_Count];
static Camera s_Camera;

static const float s_FieldOfView = 45.0f; // Degrees

bool mj::rt::Init()
{
  s_Shapes[DemoShape_RedSphere].type = Shape::Shape_Sphere;
  s_Shapes[DemoShape_RedSphere].sphere.origin = glm::vec3(2.0f, 0.0f, 10.0f);
  s_Shapes[DemoShape_RedSphere].sphere.radius = 1.0f;
  s_Shapes[DemoShape_RedSphere].color = 0xff0000ff;

  s_Shapes[DemoShape_YellowSphere].type = Shape::Shape_Sphere;
  s_Shapes[DemoShape_YellowSphere].sphere.origin = glm::vec3(0.0f, -2.0f, 10.0f);
  s_Shapes[DemoShape_YellowSphere].sphere.radius = 1.0f;
  s_Shapes[DemoShape_YellowSphere].color = 0xffff00ff;

  s_Shapes[DemoShape_BlueSphere].type = Shape::Shape_Sphere;
  s_Shapes[DemoShape_BlueSphere].sphere.origin = glm::vec3(0.0f, 2.0f, 10.0f);
  s_Shapes[DemoShape_BlueSphere].sphere.radius = 1.0f;
  s_Shapes[DemoShape_BlueSphere].color = 0x0000ffff;

  s_Shapes[DemoShape_GreenSphere].type = Shape::Shape_Sphere;
  s_Shapes[DemoShape_GreenSphere].sphere.origin = glm::vec3(-2.0f, 0.0f, 10.0f);
  s_Shapes[DemoShape_GreenSphere].sphere.radius = 1.0f;
  s_Shapes[DemoShape_GreenSphere].color = 0x00ff00ff;

  s_Shapes[DemoShape_WhitePlane].type = Shape::Shape_Plane;
  s_Shapes[DemoShape_WhitePlane].plane.normal = glm::vec3(0.0f, -1.0f, 0.0f);
  s_Shapes[DemoShape_WhitePlane].plane.distance = 5.0f;
  s_Shapes[DemoShape_WhitePlane].color = 0xffffffff;

  s_Shapes[DemoShape_CyanPlane].type = Shape::Shape_Plane;
  s_Shapes[DemoShape_CyanPlane].plane.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  s_Shapes[DemoShape_CyanPlane].plane.distance = 5.0f;
  s_Shapes[DemoShape_CyanPlane].color = 0x00ffffff;

  s_Camera.position = glm::vec3(0.0f, 0.0f, 0.0f);
  s_Camera.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
  CameraInit(MJ_REF s_Camera);

  return true;
}

static float IntersectRaySphere(const Ray& ray, const Sphere& sphere)
{
  glm::vec3 m = ray.origin - sphere.origin;
  float b = glm::dot(m, ray.direction);
  float c = glm::dot(m, m) - sphere.radius * sphere.radius;
  if (c > 0.0f && b > 0.0f)
  {
    return -1.0f;
  }
  float determinant = b * b - c;
  if (determinant < 0)
  {
    return -1.0f;
  }
  else
  {
    return -b - glm::sqrt(determinant);
  }
}

static float IntersectRayPlane(const Ray& ray, const Plane& plane)
{
  return -(glm::dot(ray.origin, plane.normal) + plane.distance) / glm::dot(ray.direction, plane.normal);
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
      ray.length = FLT_MAX;
      ray.direction = glm::normalize(p - s_Camera.position);

      const Shape* pShape = nullptr;
      float t = FLT_MAX;
      for (const auto& shape : s_Shapes)
      {
        switch (shape.type)
        {
        case Shape::Shape_Sphere:
          t = IntersectRaySphere(ray, shape.sphere);
          break;
        case Shape::Shape_Plane:
          t = IntersectRayPlane(ray, shape.plane);
          break;
        default:
          break;
        }
        if (t >= 0.0f && t < ray.length)
        {
          ray.length = t;
          pShape = &shape;
        }
      }
      if (pShape)
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = pShape->color;
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
