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
  virtual void Init()
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
  virtual void Do(mj::ArrayList<DrawCommand>& drawList)
  {
    (void)drawList;
  }
  virtual void Exit()
  {
  }
  void SetMeta(Meta* pMeta)
  {
    assert(pMeta);
    assert(!this->pMeta);
    this->pMeta = pMeta;
  }

protected:
  Meta* pMeta = nullptr;
};

struct StateMachine
{
  StateBase* pStateCurrent = nullptr;
  StateBase* pStateNext    = nullptr;

  void Resize(float width, float height);
  void Update(mj::ArrayList<DrawCommand>& drawList);
};
