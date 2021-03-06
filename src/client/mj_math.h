// Untemplated glm code to optimize compile time
#include <stdint.h>

#pragma once
//#define MJ_MATH_GLM

#ifdef MJ_MATH_GLM

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_LEFT_HANDED

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mjm = glm;

#else

#include <initializer_list>

namespace mjm
{
  struct int3
  {
    int32_t x;
    int32_t y;
    int32_t z;
  };

  bool operator==(const int3& a, const int3& b);
  bool operator!=(const int3& a, const int3& b);

  /// <summary>
  /// 3-component vector
  /// </summary>
  struct vec3
  {
    float x;
    float y;
    float z;

    static vec3 zero()
    {
      return vec3(0.0f, 0.0f, 0.0f);
    }

    vec3()
    {
    }

    vec3(std::initializer_list<float> l) = delete;

    vec3(float x, float y, float z) : x(x), y(y), z(z)
    {
    }

    float& operator[](size_t i)
    {
      switch (i)
      {
      default:
      case 0:
        return x;
      case 1:
        return y;
      case 2:
        return z;
      }
    }

    const float& operator[](size_t i) const
    {
      switch (i)
      {
      default:
      case 0:
        return x;
      case 1:
        return y;
      case 2:
        return z;
      }
    }
  };

  /// <summary>
  /// 4-component vector
  /// </summary>
  struct vec4
  {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    static vec4 zero()
    {
      return vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    vec4()
    {
    }

    vec4(std::initializer_list<float> l) = delete;

    vec4(float s) : x(s), y(s), z(s), w(s)
    {
    }

    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
    {
    }

    vec4(const vec3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {
    }

    operator vec3()
    {
      return vec3(x, y, z);
    }

    float& operator[](size_t i)
    {
      switch (i)
      {
      default:
      case 0:
        return x;
      case 1:
        return y;
      case 2:
        return z;
      case 3:
        return w;
      }
    }

    const float& operator[](size_t i) const
    {
      switch (i)
      {
      default:
      case 0:
        return x;
      case 1:
        return y;
      case 2:
        return z;
      case 3:
        return w;
      }
    }

    vec4& operator/=(float s)
    {
      this->w /= s;
      this->x /= s;
      this->y /= s;
      this->z /= s;
      return *this;
    }
  };

  /// <summary>
  /// Quaternion
  /// </summary>
  struct quat
  {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    quat(std::initializer_list<float> l) = delete;

    quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
    {
    }

    quat(const vec3& eulerAngle);

    quat(float w, float x, float y, float z) : x(x), y(y), z(z), w(w)
    {
    }

    quat(float s, const vec3& v) : x(v.x), y(v.y), z(v.z), w(s)
    {
    }

    quat(const vec3& u, const vec3& v);

    quat& operator*=(const quat& r);
  };

  /// <summary>
  /// 3x3 matrix
  /// </summary>
  struct mat3
  {
    vec3 columns[3];
    mat3()
        : columns{ vec3::zero(), //
                   vec3::zero(), //
                   vec3::zero() }
    {
    }

    mat3(float x0, float y0, float z0, //
         float x1, float y1, float z1, //
         float x2, float y2, float z2)
        : columns{ vec3(x0, y0, z0), //
                   vec3(x1, y1, z1), //
                   vec3(x2, y2, z2) }
    {
    }

    mat3(float s) : columns{ vec3(s, 0, 0), vec3(0, s, 0), vec3(0, 0, s) }
    {
    }

    vec3& operator[](size_t i)
    {
      return columns[i];
    }

    const vec3& operator[](size_t i) const
    {
      return columns[i];
    }
  };

  /// <summary>
  /// 4x4 matrix
  /// </summary>
  struct mat4
  {
    vec4 columns[4];
    mat4()
        : columns{ vec4::zero(), //
                   vec4::zero(), //
                   vec4::zero(), //
                   vec4::zero() }
    {
    }

    mat4(const vec4& c0, const vec4& c1, const vec4& c2, const vec4& c3)
        : columns{ c0, //
                   c1, //
                   c2, //
                   c3 }
    {
    }

    mat4(float x0, float y0, float z0, float w0, //
         float x1, float y1, float z1, float w1, //
         float x2, float y2, float z2, float w2, //
         float x3, float y3, float z3, float w3)
        : columns{ vec4(x0, y0, z0, w0), //
                   vec4(x1, y1, z1, w1), //
                   vec4(x2, y2, z2, w2), //
                   vec4(x3, y3, z3, w3) }
    {
    }

    mat4(const mat3& m)
        : columns{ vec4(m[0], 0), //
                   vec4(m[1], 0), //
                   vec4(m[2], 0), //
                   vec4(0, 0, 0, 1) }
    {
    }

    mat4(float s)
        : columns{ vec4(s, 0, 0, 0), //
                   vec4(0, s, 0, 0), //
                   vec4(0, 0, s, 0), //
                   vec4(0, 0, 0, s) }
    {
    }

    mat4& operator*=(const mjm::mat4& m);

    vec4& operator[](size_t i)
    {
      return columns[i];
    }

    const vec4& operator[](size_t i) const
    {
      return columns[i];
    }
  };

  vec3 operator-(const vec3& v);
  vec3 operator+(const vec3& a, const vec3& b);
  vec3 operator-(const vec3& a, const vec3& b);
  vec3 operator+=(vec3& a, const vec3& b);
  vec3 operator*(const vec3& a, const vec3& b);
  vec3 operator*(const vec3& v, float s);
  vec3 operator*(float s, const vec3& v);

  vec4 operator+(const vec4& a, const vec4& b);
  vec4 operator-(const vec4& a, const vec4& b);
  vec4 operator*(const vec4& v, float s);
  vec4 operator/(const vec4& v, float s);
  vec4 operator*(float s, const vec4& v);
  vec4 operator*(const vec4& a, const vec4& b);

  mat4 operator*(const mat4& m1, const mat4& m2);
  mat4 operator*(const mat4& m, float s);

  float dot(const vec3& a, const vec3& b);
  float dot(const vec4& a, const vec4& b);
  vec3 cross(const vec3& x, const vec3& y);
  float inversesqrt(float x);
  vec3 normalize(const vec3& v);
  vec4 normalize(const vec4& v);
  float dot(const quat& a, const quat& b);
  float length(const quat& q);
  quat normalize(const quat& q);
  vec3 operator*(const quat& q, const vec3& v);
  quat operator*(const quat& q, const quat& p);
  vec4 operator*(const mat4& m, const vec4& v);
  quat quat_cast(const mat3& m);
  quat quatLookAtLH(const vec3& direction, const vec3& up);
  quat angleAxis(float angle, const vec3& v);
  mat4 eulerAngleY(float angleY);

  template <typename T>
  T identity();
  template <>
  mat3 identity<mat3>();
  template <>
  mat4 identity<mat4>();

  template <typename T>
  float* value_ptr(T& t);

  template <>
  float* value_ptr<vec3>(vec3& t);
  template <>
  float* value_ptr<vec4>(vec4& t);
  template <>
  float* value_ptr<mat3>(mat3& t);
  template <>
  float* value_ptr<mat4>(mat4& t);

  mat4 inverse(const mat4& m);
  vec3 unProjectZO(const vec3& win, const mat4& model, const mat4& proj, const vec4& viewport);
  mat4 translate(const mat4& m, const vec3& v);
  mat4 transpose(const mat4& m);

  mat3 mat3_cast(const quat& q);
  mat4 mat4_cast(const quat& q);

  mat4 perspectiveLH_ZO(float fovy, float aspect, float zNear, float zFar);

  float radians(float degrees);
  float degrees(float radians);

  vec3 cos(const vec3& v);
  vec3 sin(const vec3& v);

} // namespace mjm
#endif
