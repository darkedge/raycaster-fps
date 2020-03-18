#pragma once
#include <stdint.h>

// Annotation macros
#define MJ_UNINITIALIZED
#define MJ_REF

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

// Raytracer resolution
static constexpr uint16_t MJ_RT_WIDTH  = 320;
static constexpr uint16_t MJ_RT_HEIGHT = 200;

// Window resolution
static constexpr uint16_t MJ_WND_WIDTH  = 1600;
static constexpr uint16_t MJ_WND_HEIGHT = 1000;

namespace mj
{
  // stream for reading
  class IStream
  {
  public:
    IStream(void* begin, size_t size)
    {
      this->end      = (char*)begin + size;
      this->position = (char*)begin;
    }

    char* Position()
    {
      return this->position;
    }

    size_t SizeLeft()
    {
      return this->end - this->position;
    }

    template <typename T>
    IStream& operator>>(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->position, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    IStream& Read(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->position, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    IStream& Skip(size_t numBytes)
    {
      if (SizeLeft() >= numBytes)
      {
        this->position += numBytes;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    void Fetch(T*&&) = delete;
    template <typename T>
    IStream& Fetch(T*& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        t = (T*)this->position;
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    bool Good()
    {
      return (this->end && this->position);
    }

  private:
    char* end;
    char* position;
  };
} // namespace mj