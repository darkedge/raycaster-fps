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

void StateMachine::Update(mj::ArrayList<DrawCommand>& drawList)
{
  auto*& pCurrent = this->pStateCurrent;
  auto*& pNext    = this->pStateNext;

  if (pCurrent)
  {
    pCurrent->Update(drawList);
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
