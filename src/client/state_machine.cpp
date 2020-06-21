#include "stdafx.h"
#include "state_machine.h"
#include "camera.h"

void StateMachineResize(StateMachine* pStateMachine, float width, float height)
{
  if (pStateMachine)
  {
    if (pStateMachine->pStateCurrent)
    {
      pStateMachine->pStateCurrent->Resize(width, height);
    }
  }
}

void StateMachineUpdate(StateMachine* pStateMachine, Camera** ppCamera)
{
  if (pStateMachine)
  {
    auto*& pCurrent = pStateMachine->pStateCurrent;
    auto*& pNext    = pStateMachine->pStateNext;

    if (pCurrent)
    {
      pCurrent->Do(ppCamera);
    }

    if (pNext)
    {
      // Leave current
      if (pCurrent)
      {
        pCurrent->Exit();
      }

      // Enter next
      pNext->Entry();

      // Set next as current
      pCurrent = pNext;
      pNext    = nullptr;
    }
  }
}
