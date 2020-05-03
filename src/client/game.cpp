#include "game.h"
#include "mj_common.h"
#include "raytracer.h"
#include "rasterizer.h"
#include <imgui.h>
#include "generated/text.h"
#include "mj_input.h"
#include <SDL.h>

static bool s_MouseLook = true;
static game::Data s_Data;

static void Reset()
{
  s_Data.s_FieldOfView.x = 60.0f;
  CameraInit(MJ_REF s_Data.s_Camera);
}

static void ShowBuildInfo()
{
  // FIXME-VIEWPORT: Select a default viewport
  const float DISTANCE = 10.0f;
  static int corner    = 0;
  if (corner != -1)
  {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos =
        ImVec2((corner & 1) ? (viewport->Pos.x + viewport->Size.x - DISTANCE) : (viewport->Pos.x + DISTANCE),
               (corner & 2) ? (viewport->Pos.y + viewport->Size.y - DISTANCE) : (viewport->Pos.y + DISTANCE));
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowViewport(viewport->ID);
  }
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  if (ImGui::Begin("Overlay", nullptr,
                   (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking |
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
  {
    ImGui::Text("%s, %s (%s #%s), %s\nStaged:%s\nUnstaged:%s", mj::txt::pBuildConfiguration, mj::txt::pGitCommitId,
                mj::txt::pGitBranch, mj::txt::pGitRevision, mj::txt::pDateTime, mj::txt::pGitDiffStaged,
                mj::txt::pGitDiff);
  }
  ImGui::End();
}

void game::Init()
{
  s_Data.s_Width  = glm::vec4(MJ_RT_WIDTH, 0.0f, 0.0f, 0.0f);
  s_Data.s_Height = glm::vec4(MJ_RT_HEIGHT, 0.0f, 0.0f, 0.0f);

  Reset();

  rt::Init();
  rs::Init();

  // Allow mouse movement tracking outside the window
  // MJ_DISCARD(SDL_CaptureMouse(SDL_TRUE));
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool)s_MouseLook));
}

void game::Resize(int width, int height)
{
  rt::Resize(width, height);
  rs::Resize(width, height);
}

void game::Update(int width, int height)
{
  if (mj::input::GetKeyDown(Key::F3))
  {
    s_MouseLook = !s_MouseLook;
    SDL_SetRelativeMouseMode((SDL_bool)s_MouseLook);
    if (!s_MouseLook)
    {
      // Only works if relative mouse mode is off
      SDL_WarpMouseInWindow(nullptr, width / 2, height / 2);
    }
  }
  if (s_MouseLook)
  {
    CameraMovement(MJ_REF s_Data.s_Camera);
  }

  // Reset button
  if (mj::input::GetKeyDown(Key::KeyR))
  {
    Reset();
  }

  ShowBuildInfo();

  ImGui::Begin("Debug");
  ImGui::Text("Player position: x=%.3f, z=%.3f", s_Data.s_Camera.position.x, s_Data.s_Camera.position.z);
  ImGui::End();

  auto mat     = glm::identity<glm::mat4>();
  s_Data.s_Mat = glm::translate(mat, glm::vec3(s_Data.s_Camera.position)) * glm::mat4_cast(s_Data.s_Camera.rotation);

  //rt::Update(width, height, &s_Data);
  rs::Update(width, height, &s_Data);
}

void game::Destroy()
{
  rt::Destroy();
  rs::Destroy();
}
