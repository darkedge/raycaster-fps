#pragma once
#include "level.h"
#include "mj_common.h"
#include "mj_math.h"

struct Camera;

template <typename T>
class ComPtr
{
public:
protected:
  T* ptr;
  template <class U>
  friend class ComPtr;

  void InternalAddRef() const
  {
    if (this->ptr != nullptr)
    {
      this->ptr->AddRef();
    }
  }

  unsigned long InternalRelease()
  {
    unsigned long ref = 0;
    T* temp           = this->ptr;

    if (temp != nullptr)
    {
      this->ptr = nullptr;
      ref       = temp->Release();
    }

    return ref;
  }

public:
  ComPtr() : ptr(nullptr)
  {
  }

  ComPtr(decltype(__nullptr)) : ptr(nullptr)
  {
  }

  template <class U>
  ComPtr(U* other) : ptr(other)
  {
    InternalAddRef();
  }

  ComPtr(const ComPtr& other) : ptr(other.ptr)
  {
    InternalAddRef();
  }

  ComPtr(ComPtr&& other) : ptr(nullptr)
  {
    if (this != reinterpret_cast<ComPtr*>(&reinterpret_cast<unsigned char&>(other)))
    {
      Swap(other);
    }
  }

  ~ComPtr()
  {
    InternalRelease();
  }

  ComPtr& operator=(decltype(nullptr))
  {
    InternalRelease();
    return *this;
  }

  ComPtr& operator=(T* other)
  {
    if (this->ptr != other)
    {
      ComPtr(other).Swap(*this);
    }
    return *this;
  }

  template <typename U>
  ComPtr& operator=(U* other)
  {
    ComPtr(other).Swap(*this);
    return *this;
  }

  ComPtr& operator=(const ComPtr& other)
  {
    if (this->ptr != other.ptr)
    {
      ComPtr(other).Swap(*this);
    }
    return *this;
  }

  template <class U>
  ComPtr& operator=(const ComPtr<U>& other)
  {
    ComPtr(other).Swap(*this);
    return *this;
  }

  ComPtr& operator=(ComPtr&& other)
  {
    ComPtr(static_cast<ComPtr&&>(other)).Swap(*this);
    return *this;
  }

  template <class U>
  ComPtr& operator=(ComPtr<U>&& other)
  {
    ComPtr(static_cast<ComPtr<U>&&>(other)).Swap(*this);
    return *this;
  }

  void Swap(ComPtr&& r)
  {
    T* tmp    = this->ptr;
    this->ptr = r.ptr;
    r.ptr     = tmp;
  }

  void Swap(ComPtr& r)
  {
    T* tmp    = this->ptr;
    this->ptr = r.ptr;
    r.ptr     = tmp;
  }

  operator bool() const
  {
    return this->ptr;
  }

  T* Get() const
  {
    return this->ptr;
  }

  T* operator->() const
  {
    return this->ptr;
  }

  T* const* GetAddressOf() const
  {
    return &this->ptr;
  }

  T** GetAddressOf()
  {
    return &this->ptr;
  }

  T** ReleaseAndGetAddressOf()
  {
    InternalRelease();
    return &this->ptr;
  }

  T* Detach()
  {
    T* ptr    = this->ptr;
    this->ptr = nullptr;
    return ptr;
  }

  void Attach(T* other)
  {
    if (this->ptr != nullptr)
    {
      auto ref = this->ptr->Release();
      MJ_DISCARD(ref);
      // Attaching to the same object only works if duplicate references are being coalesced. Otherwise
      // re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
      assert(ref != 0 || this->ptr != other);
    }

    this->ptr = other;
  }

  unsigned long Reset()
  {
    return InternalRelease();
  }

  HRESULT CopyTo(T** ptr) const
  {
    InternalAddRef();
    *ptr = this->ptr;
    return S_OK;
  }

  HRESULT CopyTo(REFIID riid, void** ptr) const
  {
    return this->ptr->QueryInterface(riid, ptr);
  }

  template <typename U>
  HRESULT CopyTo(U** ptr) const
  {
    return this->ptr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(ptr));
  }

  // query for U interface
  template <typename U>
  HRESULT As(ComPtr<U>* p) const
  {
    return this->ptr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
  }

  // query for riid interface and return as IUnknown
  HRESULT AsIID(REFIID riid, ComPtr<IUnknown>* p) const
  {
    return this->ptr->QueryInterface(riid, reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
  }
};

struct Vertex
{
  mjm::vec3 position;
  mjm::vec3 texCoord;
};

struct Mesh
{
  ComPtr<ID3D11Buffer> vertexBuffer;
  ComPtr<ID3D11Buffer> indexBuffer;
  uint32_t indexCount = 0;
};

struct DrawCommand
{
  Mesh* pMesh     = nullptr;
  Camera* pCamera = nullptr;
};

class Graphics
{
public:
  static void InsertWalls(mj::ArrayList<Vertex>& vertices, mj::ArrayList<int16_t>& indices, Level level);
  static void InsertCeiling(mj::ArrayList<Vertex>& vertices, mj::ArrayList<int16_t>& indices, float x, float z, float texture);
  static void InsertFloor(mj::ArrayList<Vertex>& vertices, mj::ArrayList<int16_t>& indices, float x, float y, float z, float texture);

  void Init(ComPtr<ID3D11Device> pDevice);
  void Resize(int width, int height);
  void Update(ComPtr<ID3D11DeviceContext> pContext, const mj::ArrayList<DrawCommand>& drawList);
  void* GetTileTexture(int x, int y);

private:
  void InitTexture2DArray(ComPtr<ID3D11Device> pDevice);

  ComPtr<ID3D11Texture2D> pTextureArray;
  ComPtr<ID3D11SamplerState> pTextureSamplerState;
  ComPtr<ID3D11VertexShader> pVertexShader;
  ComPtr<ID3D11PixelShader> pPixelShader;
  ComPtr<ID3D11ShaderResourceView> pShaderResourceView; // Texture array SRV
  ComPtr<ID3D11InputLayout> pInputLayout;
  ComPtr<ID3D11RasterizerState> pRasterizerState;
  ComPtr<ID3D11RasterizerState> pRasterizerStateCullNone;
  ComPtr<ID3D11BlendState> pBlendState;
  ComPtr<ID3D11Buffer> pResource;

  bx::DefaultAllocator defaultAllocator;
};
