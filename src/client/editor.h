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
  void Init(ComPtr<ID3D11Device> pDevice) override;
  void Resize(float w, float h) override;
  void Entry() override;
  void Update(mj::ArrayList<DrawCommand>& drawList) override;

  void SetLevel(Level level, ComPtr<ID3D11Device> pDevice);

private:
  void CreateBlockCursor(ComPtr<ID3D11Device> pDevice);
  void DoMenu();
  void DoInput();

  InputCombo inputComboNew;
  InputCombo inputComboOpen;
  InputCombo inputComboSave;
  InputCombo inputComboSaveAs;
  
  Mesh levelMesh;

  // Block cursor
  Mesh blockCursor;
  ComPtr<ID3D11VertexShader> pVertexShader;
  ComPtr<ID3D11PixelShader> pPixelShader;

  Camera camera;
  int32_t mouseScrollFactor = 1;
};
