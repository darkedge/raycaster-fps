#pragma once

struct Camera;

struct State
{
  void (*Entry)();
  void (*Do)(Camera** ppCamera);
  void (*Exit)();
};

struct StateMachine
{
  State* pStateCurrent;
  State* pStateNext;
};

void StateMachineUpdate(StateMachine* pStateMachine, Camera** ppCamera);
