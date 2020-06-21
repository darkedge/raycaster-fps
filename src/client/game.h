#pragma once
#include "state_machine.h"
#include "camera.h"

namespace game
{
  struct Game : public State
  {
    static constexpr float MOVEMENT_FACTOR = 3.0f;
    static constexpr float ROT_SPEED       = 0.0025f;

    float lastMousePos;
    float currentMousePos;
    float yaw;
    glm::quat rotation;

    Camera camera;

    void Entry() override;
    void Do(Camera** ppCamera) override;
  };
} // namespace game
