#pragma once
#include "state_machine.h"
#include "camera.h"
#include "mj_input.h"
#include "graphics.h"

class EditorState : public StateBase
{
  static constexpr float MOVEMENT_FACTOR   = 3.0f;
  static constexpr float MOUSE_DRAG_FACTOR = 0.025f;
  static constexpr float MOUSE_LOOK_FACTOR = 0.0025f;

public:
  // StateBase
  void Init() override;
  void Resize(float w, float h) override;
  void Entry() override;
  void Do(Camera** ppCamera) override;

  void SetLevel(Level level, ComPtr<ID3D11Device> pDevice);

private:
  void DoMenu();
  void DoInput();

  InputCombo inputComboNew;
  InputCombo inputComboOpen;
  InputCombo inputComboSave;
  InputCombo inputComboSaveAs;
  
  ComPtr<ID3D11Buffer> pCeilingVertexBuffer;
  ComPtr<ID3D11Buffer> pCeilingIndexBuffer;

  Camera camera;
  int32_t mouseScrollFactor = 1;
};
