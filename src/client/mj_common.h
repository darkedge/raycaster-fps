#pragma once
#include <stdint.h>
#include <string.h> // memcpy
#include <new>

// Annotation macros

// Variable is uninitialized
#define MJ_UNINITIALIZED
// Argument is passed by reference
#define MJ_REF
// Return value is discarded
#define MJ_DISCARD(x) (void)(x)

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

#define SAFE_RELEASE(_ptr) \
  if ((_ptr) != nullptr)   \
  {                        \
    (_ptr)->Release();     \
    (_ptr) = nullptr;      \
  }

// Raytracer resolution
static constexpr uint16_t MJ_RT_WIDTH  = 1600;
static constexpr uint16_t MJ_RT_HEIGHT = 1000;

// Window resolution
static constexpr uint16_t MJ_WND_WIDTH  = 1600;
static constexpr uint16_t MJ_WND_HEIGHT = 1000;

static constexpr uint16_t MJ_FS_WIDTH  = 1920;
static constexpr uint16_t MJ_FS_HEIGHT = 1080;

namespace mj
{
  template <typename T>
  void swap(T& a, T& b)
  {
    T c = a;
    a   = b;
    b   = c;
  }

  // stream for reading
  class MemoryBuffer
  {
  public:
    MemoryBuffer() : end(nullptr), position(nullptr)
    {
    }

    MemoryBuffer(void* pBegin, void* end) : end((char*)end), position((char*)pBegin)
    {
    }

    MemoryBuffer(void* pBegin, size_t size) : end((char*)pBegin + size), position((char*)pBegin)
    {
    }

    MemoryBuffer(const MemoryBuffer& other) : end(other.end), position(other.position)
    {
    }

    MemoryBuffer& operator=(const MemoryBuffer& rhs)
    {
      this->end      = rhs.end;
      this->position = rhs.position;
      return *this;
    }

    char* Position()
    {
      return this->position;
    }

    size_t SizeLeft()
    {
      return (this->end - this->position);
    }

    template <typename T>
    MemoryBuffer& operator>>(T& t)
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
    MemoryBuffer& Write(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(this->position, &t, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Write(void* pData, size_t size)
    {
      if (SizeLeft() >= size)
      {
        memcpy(this->position, pData, size);
        this->position += size;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Read(T& t)
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

    MemoryBuffer& Skip(size_t numBytes)
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
    T* ReserveArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->position;
        this->position += (numElements * sizeof(T));
        return t;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    template <typename T, typename... Args>
    T* NewUnaligned(Args... args)
    {
      if (SizeLeft() >= sizeof(T))
      {
        T* t = (T*)this->position;
        this->position += sizeof(T);
        return new (t) T(args...);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    template <typename T>
    T* NewArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->position;
        this->position += (numElements * sizeof(T));
        for (size_t i = 0; i < numElements; i++)
        {
          new (t + i) T();
        }
        return t;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
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
