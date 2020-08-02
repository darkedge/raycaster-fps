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
  void Update(ComPtr<ID3D11DeviceContext> pContext, mj::ArrayList<DrawCommand>& drawList) override;

  void SetLevel(const Level* pLvl, ComPtr<ID3D11Device> pDevice);

private:
  class BlockSelection
  {
  public:
    void Begin(BlockPos pos)
    {
      assert(!isDragging);
      this->begin = pos;
      isDragging  = true;
    }

    void End(BlockPos pos)
    {
      assert(isDragging);
      Add(begin, pos);
      isDragging = false;
    }

    void AddSingle(BlockPos pos)
    {
      assert(!isDragging);
      Add(pos, pos);
    }

    bool IsDragging() const
    {
      return isDragging;
    }

    void Clear()
    {
      selections.Clear();
    }

  private:
    struct DragSelection
    {
      MJ_UNINITIALIZED BlockPos begin;
      MJ_UNINITIALIZED BlockPos end;
    };

    void Add(BlockPos b, BlockPos e)
    {
      auto* pSelection = selections.EmplaceSingle();
      if (pSelection)
      {
        pSelection->begin = b;
        pSelection->end   = e;
      }
    }

    MJ_UNINITIALIZED BlockPos begin;
    bool isDragging = false;
    mj::ArrayList<DragSelection> selections;
  };

  class BlockCursor
  {
  public:
    void Init(ComPtr<ID3D11Device> pDevice);
    void Update(EditorState& pState, ComPtr<ID3D11DeviceContext> pContext, mj::ArrayList<DrawCommand>& drawList);

  private:
    void AdjustMesh(ComPtr<ID3D11DeviceContext> pContext, const BlockPos& begin, const BlockPos& end);

    Mesh mesh;
    ComPtr<ID3D11VertexShader> pVertexShader;
    ComPtr<ID3D11PixelShader> pPixelShader;
    BlockPos firstPosition;
    BlockPos lastPosition;
    mjm::mat4 worldMatrix;
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
  BlockSelection blockSelection;

  Camera camera;
  int32_t mouseScrollFactor = 1;
};
