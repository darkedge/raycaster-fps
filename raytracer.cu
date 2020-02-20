#include <cuda.h>
#define GLM_FORCE_CUDA
#include "mj_raytracer_cuda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline __device__ float IntersectRaySphere(const mj::rt::Ray& ray, const mj::rt::Sphere& sphere)
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

static inline __device__ float IntersectRayPlane(const mj::rt::Ray& ray, const mj::rt::Plane& plane)
{
  return -(glm::dot(ray.origin, plane.normal) + plane.distance) / glm::dot(ray.direction, plane.normal);
}

static inline __device__ glm::vec2 PixelToNDCSpace(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  return glm::vec2(((float) x + 0.5f) / width,
    ((float) y + 0.5f) / height);
}

// aspect is x/y
static inline __device__ glm::vec2 NDCToScreenSpace(const glm::vec2& ndc, float aspect)
{
  return glm::vec2((2.0f * ndc.x - 1.0f) * aspect, 1.0f - 2.0f * ndc.y);
}

// fov is in radians
static inline __device__ glm::vec2 ScreenToCameraSpace(const glm::vec2& ss, float fov)
{
  return ss * glm::tan(fov * 0.5f);
}

__global__ void cuda_raytracer(unsigned char* surface, int width, int height, size_t pitch, const mj::cuda::Constant* __restrict__ pConstant)
{
  const int x = blockIdx.x * blockDim.x + threadIdx.x;
  const int y = blockIdx.y * blockDim.y + threadIdx.y;
  float* pixel;

  if (x >= width || y >= height) return;

  // get a pointer to the pixel at (x,y)
  pixel = (float*) (surface + y * pitch) + 4 * x;

  glm::vec2 ndc = PixelToNDCSpace(x, y, width, height);
  glm::vec2 ss = NDCToScreenSpace(ndc, (float) width / height);
  glm::vec2 cs = ScreenToCameraSpace(ss, glm::radians(45.0f));

  mj::rt::Ray ray;
  glm::vec3 p = pConstant->mat * glm::vec4(cs, 1, 1);
  ray.origin = pConstant->s_Camera.position;
  ray.length = FLT_MAX;
  ray.direction = glm::normalize(p - pConstant->s_Camera.position);

  const mj::rt::Shape* pShape = nullptr;
  float t = FLT_MAX;
  for (const auto& shape : pConstant->s_Shapes)
  {
    switch (shape.type)
    {
    case mj::rt::Shape::Shape_Sphere:
      t = IntersectRaySphere(ray, shape.sphere);
      break;
    case mj::rt::Shape::Shape_Plane:
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
    glm::vec3 light = glm::normalize(glm::vec3(0.3f, 0.6f, -1.0f));

    // Get intersection normal
    glm::vec3 normal = glm::zero<glm::vec3>();
    const glm::vec3 intersection = ray.origin + ray.length * ray.direction;
    switch (pShape->type)
    {
    case mj::rt::Shape::Shape_Sphere:
      normal = (intersection - pShape->sphere.origin) / pShape->sphere.radius;
      break;
    case mj::rt::Shape::Shape_Plane:
      normal = pShape->plane.normal;
      break;
    default:
      break;
    }

    glm::vec3 color = pShape->color;
    color *= glm::clamp(glm::dot(normal, light), 0.0f, 1.0f);
    color = glm::sqrt(color);
    pixel[0] = color.x;
    pixel[1] = color.y;
    pixel[2] = color.z;
    pixel[3] = 1.0f;
  }
  else
  {
    pixel[0] = 0.0f;
    pixel[1] = 0.0f;
    pixel[2] = 0.0f;
    pixel[3] = 1.0f;
  }
}

extern "C"
void cuda_texture_2d(void* surface, int width, int height, size_t pitch, const mj::cuda::Constant* pConstant)
{
  cudaError_t error = cudaSuccess;

  dim3 Db = dim3(16, 16);   // block dimensions are fixed to be 256 threads
  dim3 Dg = dim3((width + Db.x - 1) / Db.x, (height + Db.y - 1) / Db.y);

  cuda_raytracer << <Dg, Db >> > ((unsigned char*) surface, width, height, pitch, pConstant);

  error = cudaGetLastError();

  if (error != cudaSuccess)
  {
    printf("cuda_kernel_texture_2d() failed to launch error = %d\n", error);
  }
}