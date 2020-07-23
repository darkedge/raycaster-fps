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
  if ((_ptr) != nullptr) \
  { \
    (_ptr)->Release(); \
    (_ptr) = nullptr; \
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

  /// <summary>
  /// Reduced functionality std::vector replacement
  /// </summary>
  template <typename T>
  class ArrayList
  {
  public:
    ArrayList()
    {
      pData = (T*)malloc(capacity * sizeof(T));
    }
    ArrayList(uint32_t capacity) : capacity(capacity)
    {
      pData = (T*)malloc(capacity * sizeof(T));
    }
    ~ArrayList()
    {
      free(pData);
    }

    bool Add(const T& t)
    {
      if (numElements < capacity)
      {
        *(pData + numElements) = t;
        numElements++;
        return true;
      }
      else
      {
        if (Double())
        {
          return Add(t);
        }
        else
        {
          return false;
        }
      }
    }

    void Clear()
    {
      for (auto& t : *this)
      {
        t.~T();
      }
      this->numElements = 0;
    }

    uint32_t Size() const
    {
      return numElements;
    }

    uint32_t ElemSize() const
    {
      return sizeof(T);
    }

    uint32_t ByteWidth() const
    {
      return Size() * ElemSize();
    }

    T* Get() const
    {
      return pData;
    }

    T* begin() const
    {
      return pData;
    }

    T* end() const
    {
      return pData + Size();
    }

    T& operator[](uint32_t index)
    {
      assert(index < numElements);
      return pData[index];
    }

  private:
    bool Double()
    {
      uint32_t newCapacity = 2 * capacity;
      T* ptr               = (T*)realloc(pData, newCapacity * ElemSize());
      if (ptr)
      {
        capacity = newCapacity;
        pData    = ptr;
        return true;
      }
      else
      {
        // pData stays valid
        return false;
      }
    }

    T* pData             = nullptr;
    uint32_t numElements = 0;
    uint32_t capacity    = 4;
  };

  /// <summary>
  /// Read/write stream wrapped around a memory buffer
  /// </summary>
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
