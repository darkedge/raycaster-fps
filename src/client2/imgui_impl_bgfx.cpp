/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>
#include "imgui.h"

#include "imgui_impl_bgfx.h"

#include "shaders/imgui/vs_ocornut_imgui.h"
#include "shaders/imgui/fs_ocornut_imgui.h"
#include "shaders/imgui/vs_imgui_image.h"
#include "shaders/imgui/fs_imgui_image.h"

struct FontRangeMerge
{
  const void* data;
  size_t size;
  ImWchar ranges[3];
};

inline bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices)
{
  return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout) &&
         (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
}

static void* memAlloc(size_t _size, void* _userData);
static void memFree(void* _ptr, void* _userData);

struct OcornutImguiContext
{
  void render(ImDrawData* _drawData)
  {
    const ImGuiIO& io  = ImGui::GetIO();
    const float width  = io.DisplaySize.x;
    const float height = io.DisplaySize.y;

    bgfx::setViewName(m_viewId, "ImGui");
    bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

    const bgfx::Caps* caps = bgfx::getCaps();
    {
      float ortho[16];
      bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
      bgfx::setViewTransform(m_viewId, NULL, ortho);
      bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));
    }

    // Render command lists
    for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii)
    {
      bgfx::TransientVertexBuffer tvb;
      bgfx::TransientIndexBuffer tib;

      const ImDrawList* drawList = _drawData->CmdLists[ii];
      uint32_t numVertices       = (uint32_t)drawList->VtxBuffer.size();
      uint32_t numIndices        = (uint32_t)drawList->IdxBuffer.size();

      if (!checkAvailTransientBuffers(numVertices, m_layout, numIndices))
      {
        // not enough space in transient buffer just quit drawing the rest...
        break;
      }

      bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
      bgfx::allocTransientIndexBuffer(&tib, numIndices);

      ImDrawVert* verts = (ImDrawVert*)tvb.data;
      bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

      ImDrawIdx* indices = (ImDrawIdx*)tib.data;
      bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

      uint32_t offset = 0;
      for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd;
           ++cmd)
      {
        if (cmd->UserCallback)
        {
          cmd->UserCallback(drawList, cmd);
        }
        else if (0 != cmd->ElemCount)
        {
          uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

          bgfx::TextureHandle th      = m_texture;
          bgfx::ProgramHandle program = m_program;

          if (NULL != cmd->TextureId)
          {
            union {
              ImTextureID ptr;
              struct
              {
                bgfx::TextureHandle handle;
                uint8_t flags;
                uint8_t mip;
              } s;
            } texture = { cmd->TextureId };
            state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                         ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                         : BGFX_STATE_NONE;
            th = texture.s.handle;
            if (0 != texture.s.mip)
            {
              const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
              bgfx::setUniform(u_imageLodEnabled, lodEnabled);
              program = m_imageProgram;
            }
          }
          else
          {
            state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
          }

          const uint16_t xx = uint16_t(bx::max(cmd->ClipRect.x, 0.0f));
          const uint16_t yy = uint16_t(bx::max(cmd->ClipRect.y, 0.0f));
          bgfx::setScissor(xx, yy, uint16_t(bx::min(cmd->ClipRect.z, 65535.0f) - xx),
                           uint16_t(bx::min(cmd->ClipRect.w, 65535.0f) - yy));

          bgfx::setState(state);
          bgfx::setTexture(0, s_tex, th);
          bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
          bgfx::setIndexBuffer(&tib, offset, cmd->ElemCount);
          bgfx::submit(m_viewId, program);
        }

        offset += cmd->ElemCount;
      }
    }
  }

  void create(float _fontSize, bx::AllocatorI* _allocator)
  {
    (void)_fontSize;
    m_allocator = _allocator;

    if (!_allocator)
    {
      static bx::DefaultAllocator allocator;
      m_allocator = &allocator;
    }

    m_viewId     = 255;
    m_lastScroll = 0;
    m_last       = bx::getHPCounter();

    ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

    m_imgui = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime   = 1.0f / 60.0f;
    io.IniFilename = NULL;

    setupStyle(true);

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    {
      bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::makeRef(vs_ocornut_imgui, sizeof(vs_ocornut_imgui)));
      bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::makeRef(fs_ocornut_imgui, sizeof(fs_ocornut_imgui)));
      m_program              = bgfx::createProgram(vsh, fsh, true);
    }

    u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    {
      bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::makeRef(vs_imgui_image, sizeof(vs_imgui_image)));
      bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::makeRef(fs_imgui_image, sizeof(fs_imgui_image)));
      m_imageProgram         = bgfx::createProgram(vsh, fsh, true);
    }

    m_layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    uint8_t* data;
    int32_t width;
    int32_t height;
    io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

    m_texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0,
                                      bgfx::copy(data, width * height * 4));
  }

  void destroy()
  {
    ImGui::DestroyContext(m_imgui);

    bgfx::destroy(s_tex);
    bgfx::destroy(m_texture);

    bgfx::destroy(u_imageLodEnabled);
    bgfx::destroy(m_imageProgram);
    bgfx::destroy(m_program);
  }

  void setupStyle(bool _dark)
  {
    // Doug Binks' darl color scheme
    // https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9
    ImGuiStyle& style = ImGui::GetStyle();
    if (_dark)
    {
      ImGui::StyleColorsDark(&style);
    }
    else
    {
      ImGui::StyleColorsLight(&style);
    }

    style.FrameRounding    = 4.0f;
    style.WindowBorderSize = 0.0f;
  }

  void beginFrame(int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, int _width, int _height, int _inputChar,
                  bgfx::ViewId _viewId)
  {
    m_viewId = _viewId;

    ImGuiIO& io = ImGui::GetIO();
    if (_inputChar >= 0)
    {
      io.AddInputCharacter(_inputChar);
    }

    io.DisplaySize = ImVec2((float)_width, (float)_height);

    const int64_t now       = bx::getHPCounter();
    const int64_t frameTime = now - m_last;
    m_last                  = now;
    const double freq       = double(bx::getHPFrequency());
    io.DeltaTime            = float(frameTime / freq);

    io.MousePos     = ImVec2((float)_mx, (float)_my);
    io.MouseDown[0] = 0 != (_button & IMGUI_MBUT_LEFT);
    io.MouseDown[1] = 0 != (_button & IMGUI_MBUT_RIGHT);
    io.MouseDown[2] = 0 != (_button & IMGUI_MBUT_MIDDLE);
    io.MouseWheel   = (float)(_scroll - m_lastScroll);
    m_lastScroll    = _scroll;

    ImGui::NewFrame();
  }

  void endFrame()
  {
    ImGui::Render();
    render(ImGui::GetDrawData());
  }

  ImGuiContext* m_imgui;
  bx::AllocatorI* m_allocator;
  bgfx::VertexLayout m_layout;
  bgfx::ProgramHandle m_program;
  bgfx::ProgramHandle m_imageProgram;
  bgfx::TextureHandle m_texture;
  bgfx::UniformHandle s_tex;
  bgfx::UniformHandle u_imageLodEnabled;
  int64_t m_last;
  int32_t m_lastScroll;
  bgfx::ViewId m_viewId;
};

static OcornutImguiContext s_ctx;

static void* memAlloc(size_t _size, void* _userData)
{
  BX_UNUSED(_userData);
  return BX_ALLOC(s_ctx.m_allocator, _size);
}

static void memFree(void* _ptr, void* _userData)
{
  BX_UNUSED(_userData);
  BX_FREE(s_ctx.m_allocator, _ptr);
}

void imguiCreate(float _fontSize, bx::AllocatorI* _allocator)
{
  s_ctx.create(_fontSize, _allocator);
}

void imguiDestroy()
{
  s_ctx.destroy();
}

void imguiBeginFrame(int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, uint16_t _width, uint16_t _height,
                     int _inputChar, bgfx::ViewId _viewId)
{
  s_ctx.beginFrame(_mx, _my, _button, _scroll, _width, _height, _inputChar, _viewId);
}

void imguiEndFrame()
{
  s_ctx.endFrame();
}
