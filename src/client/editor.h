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

  void SetLevel(const Level* pLvl, ComPtr<ID3D11Device> pDevice);

private:
  class DragAction
  {
  private:
  };

  class BlockSelection
  {
  private:
    struct BlockPos
    {
      int32_t x;
      int32_t z;
    };
    struct DragSelection
    {
      BlockPos begin;
      BlockPos end;
    };
    mj::ArrayList<DragSelection> selections;
  };

  class BlockCursor
  {
  public:
    void Init(ComPtr<ID3D11Device> pDevice);
    void Update(const EditorState& pState, mj::ArrayList<DrawCommand>& drawList);

  private:
    Mesh blockCursor;
    ComPtr<ID3D11VertexShader> pVertexShader;
    ComPtr<ID3D11PixelShader> pPixelShader;
    mjm::mat4 blockCursorMatrix;
  };

  void DoMenu();
  void DoInput();

  InputCombo inputComboNew;
  InputCombo inputComboOpen;
  InputCombo inputComboSave;
  InputCombo inputComboSaveAs;

  Mesh levelMesh;
  const Level* pLevel = nullptr;
  BlockCursor blockCursor;
  DragAction dragAction;
  BlockSelection blockSelection;

  Camera camera;
  int32_t mouseScrollFactor = 1;
};
