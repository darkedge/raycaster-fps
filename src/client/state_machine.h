#pragma once

struct Camera;

struct State
{
  void (*Resize)(float w, float h);
  void (*Entry)();
  void (*Do)(Camera** ppCamera);
  void (*Exit)();
};

struct StateMachine
{
  State* pStateCurrent;
  State* pStateNext;
};

void StateMachineResize(StateMachine* pStateMachine, float width, float height);
void StateMachineUpdate(StateMachine* pStateMachine, Camera** ppCamera);
