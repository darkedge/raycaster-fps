#pragma once
#include "graphics.h"

struct Camera;
class Meta;

/// <summary>
/// Interface for a state.
/// </summary>
class StateBase
{
public:
  virtual void Init(ComPtr<ID3D11Device> pDevice)
  {
  }
  virtual void Resize(float w, float h)
  {
    (void)w;
    (void)h;
  }
  virtual void Entry()
  {
  }
  virtual void Update(ComPtr<ID3D11DeviceContext> pContext, mj::ArrayList<DrawCommand>& drawList)
  {
    (void)pContext;
    (void)drawList;
  }
  virtual void Exit()
  {
  }
  void SetMeta(Meta* ptr)
  {
    assert(ptr);
    assert(!this->pMeta);
    this->pMeta = ptr;
  }

protected:
  Meta* pMeta = nullptr;
};

struct StateMachine
{
  StateBase* pStateCurrent = nullptr;
  StateBase* pStateNext    = nullptr;

  void Resize(float width, float height);
  void Update(ComPtr<ID3D11DeviceContext> pContext, mj::ArrayList<DrawCommand>& drawList);
};
