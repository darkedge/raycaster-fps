#pragma once
#include "state_machine.h"
#include "camera.h"
#include "mj_input.h"

class EditorState : public StateBase
{
  static constexpr float MOVEMENT_FACTOR   = 3.0f;
  static constexpr float MOUSE_DRAG_FACTOR = 0.025f;
  static constexpr float MOUSE_LOOK_FACTOR = 0.0025f;

public:
  void Resize(float w, float h) override;
  void Entry() override;
  void Do(Camera** ppCamera) override;
  void Exit() override;

private:
  void DoMenu();
  void DoInput();

  InputCombo inputComboOpen;
  InputCombo inputComboSave;
  InputCombo inputComboSaveAs;

  Camera camera;
  int32_t mouseScrollFactor = 1;
  mjm::quat rotation;
};
