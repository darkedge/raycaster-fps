#include "stdafx.h"

#include "mj_math.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>

void TestMath()
{
  static constexpr float MOUSE_LOOK_FACTOR = 0.0025f;

  float dx = 1.0f;
  float dy = 1.0f;

  // GLM
  {
    using namespace glm;
    const auto RIGHT    = vec3(1.0f, 0.0f, 0.0f);
    const auto UP       = vec3(0.0f, 1.0f, 0.0f);
    const auto FORWARD  = vec3(0.0f, 0.0f, 1.0f);
    const auto LEFT     = vec3(-1.0f, 0.0f, 0.0f);
    const auto DOWN     = vec3(0.0f, -1.0f, 0.0f);
    const auto BACKWARD = vec3(0.0f, 0.0f, -1.0f);
    quat rotation       = quatLookAtLH(normalize(FORWARD + 2.0f * DOWN), UP);
    printf("glm1: %f %f %f %f\n", rotation.x, rotation.y, rotation.z, rotation.w);
    quat mp = angleAxis(MOUSE_LOOK_FACTOR * dx, UP);
    printf(" mp2: %f %f %f %f\n", mp.x, mp.y, mp.z, mp.w);
    rotation = mp * rotation;
    printf("glm3: %f %f %f %f\n", rotation.x, rotation.y, rotation.z, rotation.w);
    rotation = angleAxis(MOUSE_LOOK_FACTOR * dy, rotation * RIGHT) * rotation;
    printf("glm4: %f %f %f %f\n", rotation.x, rotation.y, rotation.z, rotation.w);
  }

  // MJM
  {
    using namespace mjm;
    const auto RIGHT    = vec3(1.0f, 0.0f, 0.0f);
    const auto UP       = vec3(0.0f, 1.0f, 0.0f);
    const auto FORWARD  = vec3(0.0f, 0.0f, 1.0f);
    const auto LEFT     = vec3(-1.0f, 0.0f, 0.0f);
    const auto DOWN     = vec3(0.0f, -1.0f, 0.0f);
    const auto BACKWARD = vec3(0.0f, 0.0f, -1.0f);
    quat rotation       = quatLookAtLH(normalize(FORWARD + 2.0f * DOWN), UP);
    printf("mjm1: %f %f %f %f\n", rotation.x, rotation.y, rotation.z, rotation.w);
    quat mp = angleAxis(MOUSE_LOOK_FACTOR * dx, UP);
    printf(" mp2: %f %f %f %f\n", mp.x, mp.y, mp.z, mp.w);
    rotation = mp * rotation;
    printf("mjm3: %f %f %f %f\n", rotation.x, rotation.y, rotation.z, rotation.w);
    rotation = angleAxis(MOUSE_LOOK_FACTOR * dy, rotation * RIGHT) * rotation;
    printf("mjm4: %f %f %f %f\n", rotation.x, rotation.y, rotation.z, rotation.w);
  }
}

int main()
{
  TestMath();

  return 0;
}
