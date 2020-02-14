#include "mj_raytracer.h"
#include "mj_common.h"

using namespace mj::rt;

static Image s_Image;

static Sphere s_Sphere;

bool mj::rt::Init()
{
  return true;
}

static void IntersectRaySphere(const Ray& ray, const Sphere& sphere)
{

}

void mj::rt::Update()
{
  for (uint16_t x = 0; x < MJ_WIDTH; x++)
  {
    for (uint16_t y = 0; y < MJ_HEIGHT; y++)
    {
      
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
