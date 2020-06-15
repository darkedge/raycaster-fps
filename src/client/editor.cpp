#include "stdafx.h"
#include "editor.h"
#include "mj_input.h"
#include "graphics.h"

struct ETool
{
  enum Enum
  {
    Select,
    Paint,
    Erase,
    Dropper,
    COUNT
  };
};

#if 0
void editor::Show()
{
  ImGui::Begin("Editor");
  static int selectedTool = ETool::Select;

  if (ImGui::IsWindowFocused())
  {
    if (mj::input::GetKeyDown(Key::KeyQ))
    {
      selectedTool = ETool::Select;
    }
    else if (mj::input::GetKeyDown(Key::KeyW))
    {
      selectedTool = ETool::Paint;
    }
    else if (mj::input::GetKeyDown(Key::KeyE))
    {
      selectedTool = ETool::Erase;
    }
    else if (mj::input::GetKeyDown(Key::KeyR))
    {
      selectedTool = ETool::Dropper;
    }
  }

  ImGui::RadioButton("[Q] Select", &selectedTool, ETool::Select);
  ImGui::RadioButton("[W] Paint", &selectedTool, ETool::Paint);
  ImGui::RadioButton("[E] Erase", &selectedTool, ETool::Erase);
  ImGui::RadioButton("[R] Dropper", &selectedTool, ETool::Dropper);

  static int selected[64 * 64] = {};
  for (int i = 0; i < 64 * 64; i++)
  {
    ImGui::PushID(i);

    void* pTexture = rs::GetTileTexture(i % 64, i / 64);
    if (pTexture)
    {
      ImGui::Image(pTexture, ImVec2(50, 50));
    }
#if 0
    if (ImGui::Selectable("##dummy", selected[i] != 0, 0, ImVec2(50, 50)))
    {
      
    }
#endif
    if ((i % 64) < 63)
      ImGui::SameLine();
    ImGui::PopID();
  }
  ImGui::End();
}
#endif

void editor::Entry()
{
}

void editor::Do(Camera** ppCamera)
{
}

void editor::Exit()
{
}