#pragma once

struct Camera;

/// <summary>
/// Interface for a state.
/// </summary>
struct State
{
  virtual void Resize(float w, float h)
  {
    (void)w;
    (void)h;
  }
  virtual void Entry()
  {
  }
  virtual void Do(Camera** ppCamera)
  {
    (void)ppCamera;
  }
  virtual void Exit()
  {
  }
};

struct StateMachine
{
  State* pStateCurrent;
  State* pStateNext;
};

void StateMachineResize(StateMachine* pStateMachine, float width, float height);
void StateMachineUpdate(StateMachine* pStateMachine, Camera** ppCamera);
