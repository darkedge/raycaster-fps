#include "mj_raytracer_cuda.h"
#include "game.h"
#include "mj_input.h"

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>

#include "glm/gtc/matrix_transform.hpp"

// This will output the proper error string when calling cudaGetLastError
#define getLastCudaError(msg) __getLastCudaError(msg, __FILE__, __LINE__)

inline void __getLastCudaError(const char* errorMessage, const char* file, const int line)
{
  cudaError_t err = cudaGetLastError();

  if (cudaSuccess != err)
  {
    fprintf(stderr,
            "%s(%i) : getLastCudaError() CUDA error :"
            " %s : (%d) %s.\n",
            file, line, errorMessage, static_cast<int>(err), cudaGetErrorString(err));
    // exit(EXIT_FAILURE);
  }
}

// The CUDA kernel launchers that get called
extern "C"
{
  bool cuda_texture_2d(void* surface, size_t width, size_t height, size_t pitch, const mj::cuda::Constant* pConstant);
}

static cudaGraphicsResource* s_pCudaResource;
static void* s_pCudaLinearMemory;
static size_t s_Pitch;

static mj::cuda::Constant s_Constant;
static mj::cuda::Constant* s_pDevicePtr;

static void Reset()
{
  s_Constant.s_Camera.position = glm::vec3(0.0f, 0.0f, 0.0f);
  s_Constant.s_Camera.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
  CameraInit(MJ_REF s_Constant.s_Camera);
}

void mj::cuda::Init(ID3D11Texture2D* s_pTexture)
{
  cudaGraphicsD3D11RegisterResource(&s_pCudaResource, s_pTexture, cudaGraphicsRegisterFlagsNone);
  getLastCudaError("cudaGraphicsD3D11RegisterResource (g_texture_2d) failed");
  cudaMallocPitch(&s_pCudaLinearMemory, &s_Pitch, MJ_RT_WIDTH * sizeof(float) * 4, MJ_RT_HEIGHT);
  getLastCudaError("cudaMallocPitch (g_texture_2d) failed");
  cudaMemset(s_pCudaLinearMemory, 1, s_Pitch * MJ_RT_HEIGHT);

  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].sphere.origin = glm::vec3(2.0f, 0.0f, 10.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].sphere.radius = 1.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].color         = glm::vec3(1.0f, 0.0f, 0.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].sphere.origin = glm::vec3(0.0f, -2.0f, 10.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].sphere.radius = 1.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].color         = glm::vec3(1.0f, 1.0f, 0.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].sphere.origin = glm::vec3(0.0f, 2.0f, 10.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].sphere.radius = 1.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].color         = glm::vec3(0.0f, 0.0f, 1.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].type     = mj::rt::Shape::Shape_AABB;
  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].aabb.min = glm::vec3(-2.5f, -0.5f, 9.5f);
  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].aabb.max = glm::vec3(-1.5f, 0.5f, 10.5f);
  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].color    = glm::vec3(0.0f, 1.0f, 0.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].type           = mj::rt::Shape::Shape_Plane;
  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].plane.normal   = glm::vec3(0.0f, -1.0f, 0.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].plane.distance = 5.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].color          = glm::vec3(1.0f, 1.0f, 1.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].type           = mj::rt::Shape::Shape_Plane;
  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].plane.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].plane.distance = 3.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].color          = glm::vec3(0.0f, 1.0f, 1.0f);

  Reset();

  size_t numBytes = sizeof(s_Constant);
  cudaMalloc((void**)&s_pDevicePtr, numBytes);
  cudaMemcpy(s_pDevicePtr, &s_Constant, numBytes, cudaMemcpyHostToDevice);
}

void mj::cuda::Update()
{
  CameraMovement(MJ_REF s_Constant.s_Camera);

  // Reset button
  if (mj::input::GetKeyDown(Key::KeyR))
  {
    Reset();
  }

  auto mat       = glm::identity<glm::mat4>();
  s_Constant.mat = glm::translate(mat, s_Constant.s_Camera.position) * glm::mat4_cast(s_Constant.s_Camera.rotation);

  cudaMemcpy(s_pDevicePtr, &s_Constant, sizeof(s_Constant), cudaMemcpyHostToDevice);

  cudaStream_t stream = 0;
  cudaGraphicsMapResources(1, &s_pCudaResource, stream);
  getLastCudaError("cudaGraphicsMapResources(3) failed");

  { // Run kernel
    cudaArray* cuArray;
    cudaGraphicsSubResourceGetMappedArray(&cuArray, s_pCudaResource, 0, 0);
    getLastCudaError("cudaGraphicsSubResourceGetMappedArray (cuda_texture_2d) failed");

    // kick off the kernel and send the staging buffer cudaLinearMemory as an argument to allow the kernel to write to
    // it
    cuda_texture_2d(s_pCudaLinearMemory, MJ_RT_WIDTH, MJ_RT_HEIGHT, s_Pitch, s_pDevicePtr);
    getLastCudaError("cuda_texture_2d failed");

    // then we want to copy cudaLinearMemory to the D3D texture, via its mapped form : cudaArray
    cudaMemcpy2DToArray(cuArray,                                       // dst array
                        0, 0,                                          // offset
                        s_pCudaLinearMemory, s_Pitch,                  // src
                        MJ_RT_WIDTH * sizeof(glm::vec4), MJ_RT_HEIGHT, // extent
                        cudaMemcpyDeviceToDevice);                     // kind
    getLastCudaError("cudaMemcpy2DToArray failed");
  }

  cudaGraphicsUnmapResources(1, &s_pCudaResource, stream);
  getLastCudaError("cudaGraphicsUnmapResources(3) failed");
}

void mj::cuda::Destroy()
{
  // unregister the Cuda resources
  cudaGraphicsUnregisterResource(s_pCudaResource);
  getLastCudaError("cudaGraphicsUnregisterResource (g_texture_2d) failed");
  cudaFree(s_pCudaLinearMemory);
  getLastCudaError("cudaFree (g_texture_2d) failed");
}
