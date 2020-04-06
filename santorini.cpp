#include "santorini.h"
#include "imgui.h"
#include "mj_common.h"

struct Pos
{
  int x = -1;
  int y = -1;
  Pos() = default;
  Pos(int x, int y)
  {
    this->x = x;
    this->y = y;
  }
};

struct Player
{
  Pos* pWorkers;
};

enum EDirection
{
  EDirection_NorthWest,
  EDirection_North,
  EDirection_NorthEast,
  EDirection_East,
  EDirection_SouthEast,
  EDirection_South,
  EDirection_SouthWest,
  EDirection_West,
  EDirectionCount
};

struct MoveMask
{
  // bool CanMove[EDirectionCount];
  int spaces[EDirectionCount];
  int numSpaces;
};

static Player** s_Players;

bool operator==(const Pos& lhs, const Pos& rhs)
{
  return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

static char s_Memory[1024];
static mj::IStream s_TransientMemory;

static int s_NumPlayers = 2;
static int s_NumWorkers = 2; // per player
static int s_CurrentPlayer;

// Marsaglia xorshift (13, 17, 5)
static uint32_t Xorshift32()
{
  static uint32_t s_Seed = 1337;
  s_Seed ^= s_Seed << 13;
  s_Seed ^= s_Seed >> 17;
  s_Seed ^= s_Seed << 5;
  return s_Seed;
}

static int s_NumRows    = 5;
static int s_NumColumns = 5;

void SantoriniInit()
{
  mj::IStream stream(s_Memory, sizeof(s_Memory));
  s_Players = nullptr;

  Player** ppPlayers = stream.ReserveArrayUnaligned<Player*>(s_NumPlayers);
  if (ppPlayers)
  {
    for (int i = 0; i < s_NumPlayers; i++)
    {
      ppPlayers[i] = stream.NewUnaligned<Player>();
      if (ppPlayers[i])
      {
        ppPlayers[i]->pWorkers = stream.NewArrayUnaligned<Pos>(s_NumWorkers);
      }
    }
    if (stream.Good())
    {
      s_TransientMemory = stream;
      s_Players         = ppPlayers;
      s_CurrentPlayer   = 0;
    }
  }
}

static inline bool OnBoard(int x, int y)
{
  return (x >= 0) && (x < s_NumColumns) && (y >= 0) && (y < s_NumRows);
}

static MoveMask GetMoveMask(int player, int worker)
{
  Pos& pos = s_Players[player]->pWorkers[worker];

  MoveMask mask;
  mask.numSpaces = 0;
  for (int x = pos.x - 1; x <= pos.x + 1; x++)
  {
    for (int y = pos.y - 1; y <= pos.y + 1; y++)
    {
      if (OnBoard(x, y))
      {
        int space                     = y * s_NumColumns + x;
        mask.spaces[mask.numSpaces++] = space;
      }
    }
  }

  return mask;
}

static void MakeMove()
{
  // Pick random worker
  int worker        = Xorshift32() % s_NumWorkers;
  MoveMask moveMask = GetMoveMask(s_CurrentPlayer, worker);

  // Pick random move
  int index = Xorshift32() % moveMask.numSpaces;
  int space = moveMask.spaces[index];
  Pos& pos  = s_Players[s_CurrentPlayer]->pWorkers[worker];
  pos.x     = space % s_NumColumns;
  pos.y     = space / s_NumColumns;

  // End turn
  s_CurrentPlayer++;
  if (s_CurrentPlayer >= s_NumPlayers)
  {
    s_CurrentPlayer = 0;
  }
}

void SantoriniUpdate()
{
  ImGui::Begin("Santorini");

  bool init = false;
  init |= ImGui::SliderInt("NumPlayers", &s_NumPlayers, 2, 10);
  init |= ImGui::SliderInt("NumWorkers", &s_NumWorkers, 1, 10);
  init |= ImGui::SliderInt("NumRows", &s_NumRows, 2, 10);
  init |= ImGui::SliderInt("NumColumns", &s_NumColumns, 2, 10);
  if (init)
  {
    SantoriniInit();
  }

  if (ImGui::Button("New Game") && s_Players)
  {
    const int numSlots = s_NumRows * s_NumColumns;
    if (numSlots >= s_NumPlayers * s_NumWorkers)
    {
      mj::IStream stream = s_TransientMemory;
      int* const pSlots  = stream.ReserveArrayUnaligned<int>(numSlots);
      if (pSlots)
      {
        // Random worker positions
        for (int i = 0; i < numSlots; i++)
        {
          pSlots[i] = i;
        }

        int spacesLeft = numSlots;
        for (int i = 0; i < s_NumPlayers; i++)
        {
          for (int j = 0; j < s_NumWorkers; j++)
          {
            Pos& pos  = s_Players[i]->pWorkers[j];
            int index = Xorshift32() % spacesLeft;
            int space = pSlots[index];
            pos.x     = space % s_NumColumns;
            pos.y     = space / s_NumColumns;
            spacesLeft--;
            mj::swap(pSlots[index], pSlots[spacesLeft]);
          }
        }
      }
    }
  }

  if (ImGui::Button("Step") && s_Players)
  {
    MakeMove();
  }

  ImGui::Columns(s_NumColumns);

  for (int i = 0; i < s_NumRows; i++)
  {
    for (int j = 0; j < s_NumColumns; j++)
    {
      ImGui::Text("%d", 0);
      const Pos pos(j, i);

      if (s_Players)
      {
        for (int k = 0; k < s_NumPlayers; k++)
        {
          for (int m = 0; m < s_NumWorkers; m++)
          {
            if (pos == s_Players[k]->pWorkers[m])
            {
              ImGui::SameLine();
              ImGui::Text("P%d.%d", k + 1, m + 1);
            }
          }
        }
      }

      ImGui::NextColumn();
    }
  }

  ImGui::End();
}
