#include "stdafx.h"
#include "state_machine.h"
#include "camera.h"

void StateMachine::Resize(float width, float height)
{
  if (this->pStateCurrent)
  {
    this->pStateCurrent->Resize(width, height);
  }
}

void StateMachine::Update(Camera** ppCamera)
{
  auto*& pCurrent = this->pStateCurrent;
  auto*& pNext    = this->pStateNext;

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
