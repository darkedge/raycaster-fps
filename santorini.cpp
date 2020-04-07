#include "santorini.h"
#include "imgui.h"
#include "mj_common.h"
#include <stdio.h>

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

// 0 1 2
// 3   4
// 5 6 7
enum EDirection
{
  EDirection_NorthWest,
  EDirection_North,
  EDirection_NorthEast,
  EDirection_West,
  EDirection_East,
  EDirection_SouthWest,
  EDirection_South,
  EDirection_SouthEast,
  EDirectionCount
};

struct MoveMask
{
  int spaces[EDirectionCount];
  int numSpaces;
};

struct Square
{
  int height = 0;
  bool dome  = false;
  int player = -1;
  int worker = -1;
};

static Player** s_ppPlayers;
static Square* s_pBoard;

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

static void NewGame()
{
  const int numSquares = s_NumRows * s_NumColumns;

  new (s_pBoard) Square[numSquares];

  if (numSquares >= s_NumPlayers * s_NumWorkers)
  {
    mj::IStream stream = s_TransientMemory;
    int* const pPositions  = stream.ReserveArrayUnaligned<int>(numSquares);
    if (pPositions)
    {
      // Random worker positions
      for (int i = 0; i < numSquares; i++)
      {
        pPositions[i] = i;
      }

      int spacesLeft = numSquares;
      for (int i = 0; i < s_NumPlayers; i++)
      {
        for (int j = 0; j < s_NumWorkers; j++)
        {
          Pos& pos  = s_ppPlayers[i]->pWorkers[j];
          int index = Xorshift32() % spacesLeft;
          int space = pPositions[index];
          pos.x     = space % s_NumColumns;
          pos.y     = space / s_NumColumns;

          Square& sq = s_pBoard[pos.y * s_NumColumns + pos.x];
          sq.player  = i;
          sq.worker  = j;

          spacesLeft--;
          mj::swap(pPositions[index], pPositions[spacesLeft]);
        }
      }
    }
  }
}

void SantoriniInit()
{
  mj::IStream stream(s_Memory, sizeof(s_Memory));
  s_ppPlayers = nullptr;
  s_pBoard   = nullptr;

  decltype(s_ppPlayers) ppPlayers = stream.ReserveArrayUnaligned<Player*>(s_NumPlayers);
  decltype(s_pBoard) pBoard      = stream.NewArrayUnaligned<Square>(s_NumRows * s_NumColumns);
  if (ppPlayers && pBoard)
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
      s_ppPlayers         = ppPlayers;
      s_pBoard           = pBoard;
      s_CurrentPlayer   = 0;
    }
  }

  NewGame();
}

static inline bool OnBoard(int x, int y)
{
  return (x >= 0) && (x < s_NumColumns) && (y >= 0) && (y < s_NumRows);
}

static MoveMask GetMoveMask(int player, int worker)
{
  Pos& pos     = s_ppPlayers[player]->pWorkers[worker];
  Square& from = s_pBoard[pos.y * s_NumColumns + pos.x];

  MJ_UNINITIALIZED MoveMask mask;
  mask.numSpaces = 0;
  for (int y = pos.y - 1; y <= pos.y + 1; y++)
  {
    for (int x = pos.x - 1; x <= pos.x + 1; x++)
    {
      if (!((x == pos.x) && (x == pos.y))) // Don't move in place
      {
        if (OnBoard(x, y))
        {
          int sq     = y * s_NumColumns + x;
          Square& to = s_pBoard[sq];
          if (!to.dome && ((to.height - from.height) <= 1) && (to.player == -1))
          {
            // Add to moveset
            mask.spaces[mask.numSpaces++] = sq;
          }
        }
      }
    }
  }

  return mask;
}

static MoveMask GetBuildMask(const Pos& pos)
{
  MJ_UNINITIALIZED MoveMask mask;
  mask.numSpaces = 0;
  for (int y = pos.y - 1; y <= pos.y + 1; y++)
  {
    for (int x = pos.x - 1; x <= pos.x + 1; x++)
    {
      if (!((x == pos.x) && (x == pos.y))) // Don't move in place
      {
        if (OnBoard(x, y))
        {
          int sq     = y * s_NumColumns + x;
          Square& to = s_pBoard[sq];
          if (!to.dome && (to.player == -1))
          {
            // Add to moveset
            mask.spaces[mask.numSpaces++] = sq;
          }
        }
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

  if (moveMask.numSpaces > 0)
  {
    // Pick random move
    int index    = Xorshift32() % moveMask.numSpaces;
    int space    = moveMask.spaces[index];
    Pos& pos     = s_ppPlayers[s_CurrentPlayer]->pWorkers[worker];
    Square& from = s_pBoard[pos.y * s_NumColumns + pos.x];
    pos.x        = space % s_NumColumns;
    pos.y        = space / s_NumColumns;

    // Move player on board
    Square& to = s_pBoard[space];
    mj::swap(to.player, from.player);
    mj::swap(to.worker, from.worker);

    if (to.height == 3)
    {
      printf("Player %d wins!\n", s_CurrentPlayer + 1);
    }
    else
    {
      // Random build
      MoveMask buildMask = GetBuildMask(pos);
      if (buildMask.numSpaces > 0)
      {
        index      = Xorshift32() % buildMask.numSpaces;
        space      = buildMask.spaces[index];
        Square& sq = s_pBoard[space];
        if (sq.height == 3)
        {
          sq.dome = true;
        }
        else
        {
          sq.height++;
        }
      }
      else
      {
        printf("P%d.%d cannot build!\n", s_CurrentPlayer + 1, worker + 1);
      }
    }
  }
  else
  {
    printf("P%d.%d cannot move!\n", s_CurrentPlayer + 1, worker + 1);
  }

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

  if (ImGui::Button("New Game") && s_ppPlayers)
  {
    NewGame();
  }

  if (ImGui::Button("Step") && s_ppPlayers)
  {
    MakeMove();
  }

  ImGui::Columns(s_NumColumns);

  if (s_pBoard)
  {
    for (int y = 0; y < s_NumRows; y++)
    {
      for (int x = 0; x < s_NumColumns; x++)
      {
        Square& sq = s_pBoard[y * s_NumColumns + x];

        ImGui::Text("%d", sq.height);
        if (sq.dome)
        {
          ImGui::SameLine();
          ImGui::Text("!");
        }

        if (sq.player >= 0)
        {
          ImGui::SameLine();
          ImGui::Text("P%d.%d", sq.player + 1, sq.worker + 1);
        }

        ImGui::NextColumn();
      }
    }
  }

  ImGui::End();
}
