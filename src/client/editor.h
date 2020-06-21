#pragma once
#include "state_machine.h"
#include "camera.h"

namespace editor
{
  struct Editor : public State
  {
    static constexpr float MOVEMENT_FACTOR   = 3.0f;
    static constexpr float MOUSE_DRAG_FACTOR = 0.025f;
    static constexpr float MOUSE_LOOK_FACTOR = 0.0025f;

    Camera camera;
    int32_t mouseScrollFactor = 1;
    glm::quat rotation;

    void Resize(float w, float h) override;
    void Entry() override;
    void Do(Camera** ppCamera) override;
    void Exit() override;
  };
} // namespace editor
