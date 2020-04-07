#include "game.h"

static float s_DeltaTime;

void SetDeltaTime(float t)
{
  s_DeltaTime = t;
}

float GetDeltaTime()
{
  return s_DeltaTime;
}
