#include "stdafx.h"
#include "state_machine.h"
#include "camera.h"

void StateMachineUpdate(StateMachine* pStateMachine, Camera** ppCamera)
{
  if (pStateMachine)
  {
    auto*& pCurrent = pStateMachine->pStateCurrent;
    auto*& pNext    = pStateMachine->pStateNext;

    if (pCurrent && pCurrent->Do)
    {
      pCurrent->Do(ppCamera);
    }

    if (pNext)
    {
      // Leave current
      if (pCurrent && pCurrent->Exit)
      {
        pCurrent->Exit();
      }

      // Enter next
      if (pNext->Entry)
      {
        pNext->Entry();
      }

      // Set next as current
      pCurrent = pNext;
      pNext    = nullptr;
    }
  }
}
