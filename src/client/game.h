#pragma once
#include "state_machine.h"
#include "camera.h"

class GameState : public StateBase
{
public:
  // StateBase
  void Entry() override;
  void Update(ComPtr<ID3D11DeviceContext> pContext, mj::ArrayList<DrawCommand>& drawList) override;

  void SetLevel(Level level, ComPtr<ID3D11Device> pDevice);

private:
  static constexpr float MOVEMENT_FACTOR = 3.0f;
  static constexpr float ROT_SPEED       = 0.0025f;

  float lastMousePos;
  float currentMousePos;
  float yaw;

  Mesh levelMesh;

  Camera camera;
};
