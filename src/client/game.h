#pragma once
#include "state_machine.h"
#include "camera.h"

class GameState : public StateBase
{
public:
  void Entry() override;
  void Do(Camera** ppCamera) override;

private:
  static constexpr float MOVEMENT_FACTOR = 3.0f;
  static constexpr float ROT_SPEED       = 0.0025f;

  float lastMousePos;
  float currentMousePos;
  float yaw;

  Camera camera;
};
