#include "mj_raytracer.h"
#include "mj_common.h"

static mj::rt::Image s_Image;

bool mj::rt::Init()
{
  return true;
}

void mj::rt::Update()
{
  for (uint16_t x = 0; x < MJ_WIDTH; x++)
  {
    for (uint16_t y = 0; y < MJ_HEIGHT; y++)
    {
      if (x == y)
      {
        s_Image.p[y * MJ_WIDTH + x].rgba = 0xFF0000FF;
      }
    }
  }
}

const mj::rt::Image& mj::rt::GetImage()
{
  return s_Image;
}

void mj::rt::Destroy()
{

}
