#include "mj_math.h"
#include <cmath>

#ifndef MJ_MATH_GLM

mjm::quat::quat(const vec3& eulerAngle)
{
  vec3 c = mjm::cos(eulerAngle * 0.5f);
  vec3 s = mjm::sin(eulerAngle * 0.5f);

  this->w = c.x * c.y * c.z + s.x * s.y * s.z;
  this->x = s.x * c.y * c.z - c.x * s.y * s.z;
  this->y = c.x * s.y * c.z + s.x * c.y * s.z;
  this->z = c.x * c.y * s.z - s.x * s.y * c.z;
}

mjm::quat::quat(const vec3& u, const vec3& v)
{
  float norm_u_norm_v = sqrt(dot(u, u) * dot(v, v));
  float real_part     = norm_u_norm_v + dot(u, v);
  vec3 t;

  if (real_part < 1.e-6f * norm_u_norm_v)
  {
    // If u and v are exactly opposite, rotate 180 degrees
    // around an arbitrary orthogonal axis. Axis normalisation
    // can happen later, when we normalise the quaternion.
    real_part = 0.0f;
    t         = abs(u.x) > abs(u.z) ? vec3(-u.y, u.x, 0.0f) : vec3(0.0f, -u.z, u.y);
  }
  else
  {
    // Otherwise, build quaternion the standard way.
    t = cross(u, v);
  }

  *this = normalize(quat(real_part, t.x, t.y, t.z));
}

mjm::quat& mjm::quat::operator*=(const quat& r)
{
  const quat p(*this);
  const quat q(r);

  this->w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
  this->x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
  this->y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
  this->z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
  return *this;
}

bool mjm::operator!=(const int3& a, const int3& b)
{
  return !(a == b);
}

bool mjm::operator==(const int3& a, const int3& b)
{
  return (a.x == b.x) && //
         (a.y == b.y) && //
         (a.z == b.z);
}

mjm::vec3 mjm::operator-(const vec3& v)
{
  return vec3(-v.x, -v.y, -v.z);
}

mjm::vec3 mjm::operator+(const vec3& a, const vec3& b)
{
  return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

mjm::vec3 mjm::operator-(const vec3& a, const vec3& b)
{
  return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

mjm::vec3 mjm::operator+=(vec3& a, const vec3& b)
{
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

mjm::vec4 mjm::operator+(const vec4& a, const vec4& b)
{
  return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

mjm::vec4 mjm::operator-(const vec4& a, const vec4& b)
{
  return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

mjm::vec3 mjm::operator*(const vec3& a, const vec3& b)
{
  return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

mjm::vec3 mjm::operator*(const vec3& v, float s)
{
  return vec3(v.x * s, v.y * s, v.z * s);
}

mjm::vec3 mjm::operator*(float s, const vec3& v)
{
  return vec3(v.x * s, v.y * s, v.z * s);
}

mjm::vec4 mjm::operator*(const vec4& v, float s)
{
  return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

mjm::vec4 mjm::operator/(const vec4& v, float s)
{
  return vec4(v.x / s, v.y / s, v.z / s, v.w / s);
}

mjm::vec4 mjm::operator*(float s, const vec4& v)
{
  return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

mjm::vec4 mjm::operator*(const vec4& a, const vec4& b)
{
  return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

mjm::mat4 mjm::operator*(const mat4& m1, const mat4& m2)
{
  const vec4 SrcA0 = m1[0];
  const vec4 SrcA1 = m1[1];
  const vec4 SrcA2 = m1[2];
  const vec4 SrcA3 = m1[3];

  const vec4 SrcB0 = m2[0];
  const vec4 SrcB1 = m2[1];
  const vec4 SrcB2 = m2[2];
  const vec4 SrcB3 = m2[3];

  mat4 Result;
  Result[0] = SrcA0 * SrcB0[0] + SrcA1 * SrcB0[1] + SrcA2 * SrcB0[2] + SrcA3 * SrcB0[3];
  Result[1] = SrcA0 * SrcB1[0] + SrcA1 * SrcB1[1] + SrcA2 * SrcB1[2] + SrcA3 * SrcB1[3];
  Result[2] = SrcA0 * SrcB2[0] + SrcA1 * SrcB2[1] + SrcA2 * SrcB2[2] + SrcA3 * SrcB2[3];
  Result[3] = SrcA0 * SrcB3[0] + SrcA1 * SrcB3[1] + SrcA2 * SrcB3[2] + SrcA3 * SrcB3[3];
  return Result;
}

mjm::mat4 mjm::operator*(const mat4& m, float s)
{
  return mat4(m[0] * s, //
              m[1] * s, //
              m[2] * s, //
              m[3] * s);
}

float mjm::dot(const vec3& a, const vec3& b)
{
  vec3 tmp(a * b);
  return tmp.x + tmp.y + tmp.z;
}

float mjm::dot(const vec4& a, const vec4& b)
{
  vec4 tmp(a * b);
  return tmp.x + tmp.y + tmp.z;
}

mjm::vec3 mjm::cross(const vec3& x, const vec3& y)
{
  return vec3(x.y * y.z - y.y * x.z, //
              x.z * y.x - y.z * x.x, //
              x.x * y.y - y.x * x.y);
}

float mjm::inversesqrt(float x)
{
  return 1.0f / sqrt(x);
}

mjm::vec3 mjm::normalize(const vec3& v)
{
  return v * inversesqrt(dot(v, v));
}

mjm::vec4 mjm::normalize(const vec4& v)
{
  return v * inversesqrt(dot(v, v));
}

float mjm::dot(const quat& a, const quat& b)
{
  vec4 tmp(a.w * b.w, a.x * b.x, a.y * b.y, a.z * b.z);
  return (tmp.x + tmp.y) + (tmp.z + tmp.w);
}

float mjm::length(const quat& q)
{
  return sqrt(dot(q, q));
}

mjm::quat mjm::normalize(const quat& q)
{
  float len = length(q);
  if (len <= 0.0f) // Problem
    return quat(1.0f, 0.0f, 0.0f, 0.0f);
  float oneOverLen = 1.0f / len;
  return quat(q.w * oneOverLen, q.x * oneOverLen, q.y * oneOverLen, q.z * oneOverLen);
}

mjm::vec3 mjm::operator*(const quat& q, const vec3& v)
{
  const vec3 QuatVector(q.x, q.y, q.z);
  const vec3 uv(cross(QuatVector, v));
  const vec3 uuv(cross(QuatVector, uv));

  return v + ((uv * q.w) + uuv) * 2.0f;
}

mjm::quat mjm::operator*(const quat& q, const quat& p)
{
  return quat(q) *= p;
}

mjm::vec4 mjm::operator*(const mat4& m, const vec4& v)
{
  const vec4 Mov0(v[0]);
  const vec4 Mov1(v[1]);
  const vec4 Mul0 = m[0] * Mov0;
  const vec4 Mul1 = m[1] * Mov1;
  const vec4 Add0 = Mul0 + Mul1;
  const vec4 Mov2(v[2]);
  const vec4 Mov3(v[3]);
  const vec4 Mul2 = m[2] * Mov2;
  const vec4 Mul3 = m[3] * Mov3;
  const vec4 Add1 = Mul2 + Mul3;
  const vec4 Add2 = Add0 + Add1;
  return Add2;
}

mjm::quat mjm::quat_cast(const mat3& m)
{
  float fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
  float fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
  float fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
  float fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

  int biggestIndex               = 0;
  float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
  if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
  {
    fourBiggestSquaredMinus1 = fourXSquaredMinus1;
    biggestIndex             = 1;
  }
  if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
  {
    fourBiggestSquaredMinus1 = fourYSquaredMinus1;
    biggestIndex             = 2;
  }
  if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
  {
    fourBiggestSquaredMinus1 = fourZSquaredMinus1;
    biggestIndex             = 3;
  }

  float biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<float>(1)) * static_cast<float>(0.5);
  float mult       = static_cast<float>(0.25) / biggestVal;

  switch (biggestIndex)
  {
  case 0:
    return quat(biggestVal, (m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult);
  case 1:
    return quat((m[1][2] - m[2][1]) * mult, biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult);
  case 2:
    return quat((m[2][0] - m[0][2]) * mult, (m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult);
  case 3:
    return quat((m[0][1] - m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal);
  default: // Silence a -Wswitch-default warning in GCC. Should never actually get here. Assert is just for sanity.
    return quat(1, 0, 0, 0);
  }
}

mjm::quat mjm::quatLookAtLH(const vec3& direction, const vec3& up)
{
  mat3 Result;

  Result[2] = direction;
  Result[0] = normalize(cross(up, Result[2]));
  Result[1] = cross(Result[2], Result[0]);

  return quat_cast(Result);
}

mjm::quat mjm::angleAxis(float angle, const vec3& v)
{
  const float a(angle);
  const float s = std::sin(a * 0.5f);

  return quat(std::cos(a * 0.5f), v * s);
}

mjm::mat4 mjm::eulerAngleY(float angleY)
{
  float cosY = std::cos(angleY);
  float sinY = std::sin(angleY);

  return mat4(                 //
      cosY, 0.0f, -sinY, 0.0f, //
      0.0f, 1.0f, 0.0f, 0.0f,  //
      sinY, 0.0f, cosY, 0.0f,  //
      0.0f, 0.0f, 0.0f, 1.0f); //
}

template <typename T>
T mjm::identity();

template <>
mjm::mat3 mjm::identity<mjm::mat3>()
{
  return mat3(1.0f);
}

template <>
mjm::mat4 mjm::identity<mjm::mat4>()
{
  return mat4(1.0f);
}

template <typename T>
float* mjm::value_ptr(T& t);

template <>
float* mjm::value_ptr<mjm::vec3>(vec3& t)
{
  return &t.x;
}

template <>
float* mjm::value_ptr<mjm::vec4>(vec4& t)
{
  return &t.x;
}

template <>
float* mjm::value_ptr<mjm::mat3>(mat3& t)
{
  return &t[0].x;
}

template <>
float* mjm::value_ptr<mjm::mat4>(mat4& t)
{
  return &t[0].x;
}

mjm::mat4 mjm::inverse(const mat4& m)
{
  float Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
  float Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
  float Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

  float Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
  float Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
  float Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

  float Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
  float Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
  float Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

  float Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
  float Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
  float Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

  float Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
  float Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
  float Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

  float Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
  float Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
  float Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

  vec4 Fac0(Coef00, Coef00, Coef02, Coef03);
  vec4 Fac1(Coef04, Coef04, Coef06, Coef07);
  vec4 Fac2(Coef08, Coef08, Coef10, Coef11);
  vec4 Fac3(Coef12, Coef12, Coef14, Coef15);
  vec4 Fac4(Coef16, Coef16, Coef18, Coef19);
  vec4 Fac5(Coef20, Coef20, Coef22, Coef23);

  vec4 Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
  vec4 Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
  vec4 Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
  vec4 Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

  vec4 Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
  vec4 Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
  vec4 Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
  vec4 Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

  vec4 SignA(+1, -1, +1, -1);
  vec4 SignB(-1, +1, -1, +1);
  mat4 Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

  vec4 Row0(Inverse[0][0], Inverse[1][0], Inverse[2][0], Inverse[3][0]);

  vec4 Dot0(m[0] * Row0);
  float Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

  float OneOverDeterminant = static_cast<float>(1) / Dot1;

  return Inverse * OneOverDeterminant;
}

mjm::vec3 mjm::unProjectZO(const vec3& win, const mat4& model, const mat4& proj, const vec4& viewport)
{
  mat4 Inverse = inverse(proj * model);

  vec4 tmp = vec4(win, 1.0f);
  tmp.x    = (tmp.x - viewport[0]) / viewport[2];
  tmp.y    = (tmp.y - viewport[1]) / viewport[3];
  tmp.x    = tmp.x * 2.0f - 1.0f;
  tmp.y    = tmp.y * 2.0f - 1.0f;

  vec4 obj = Inverse * tmp;
  obj /= obj.w;

  return vec3(obj);
}

mjm::mat4 mjm::translate(const mat4& m, const vec3& v)
{
  mat4 Result(m);
  Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
  return Result;
}

mjm::mat4 mjm::transpose(const mat4& m)
{
  mat4 Result;
  Result[0][0] = m[0][0];
  Result[0][1] = m[1][0];
  Result[0][2] = m[2][0];
  Result[0][3] = m[3][0];

  Result[1][0] = m[0][1];
  Result[1][1] = m[1][1];
  Result[1][2] = m[2][1];
  Result[1][3] = m[3][1];

  Result[2][0] = m[0][2];
  Result[2][1] = m[1][2];
  Result[2][2] = m[2][2];
  Result[2][3] = m[3][2];

  Result[3][0] = m[0][3];
  Result[3][1] = m[1][3];
  Result[3][2] = m[2][3];
  Result[3][3] = m[3][3];
  return Result;
}

mjm::mat3 mjm::mat3_cast(const quat& q)
{
  mat3 Result(1.0f);
  float qxx(q.x * q.x);
  float qyy(q.y * q.y);
  float qzz(q.z * q.z);
  float qxz(q.x * q.z);
  float qxy(q.x * q.y);
  float qyz(q.y * q.z);
  float qwx(q.w * q.x);
  float qwy(q.w * q.y);
  float qwz(q.w * q.z);

  Result[0][0] = 1.0f - 2.0f * (qyy + qzz);
  Result[0][1] = 2.0f * (qxy + qwz);
  Result[0][2] = 2.0f * (qxz - qwy);

  Result[1][0] = 2.0f * (qxy - qwz);
  Result[1][1] = 1.0f - 2.0f * (qxx + qzz);
  Result[1][2] = 2.0f * (qyz + qwx);

  Result[2][0] = 2.0f * (qxz + qwy);
  Result[2][1] = 2.0f * (qyz - qwx);
  Result[2][2] = 1.0f - 2.0f * (qxx + qyy);
  return Result;
}

mjm::mat4 mjm::mat4_cast(const quat& q)
{
  return mat4(mat3_cast(q));
}

mjm::mat4 mjm::perspectiveLH_ZO(float fovy, float aspect, float zNear, float zFar)
{
  const float tanHalfFovy = tan(fovy / 2.0f);

  mat4 Result(0.0f);
  Result[0][0] = 1.0f / (aspect * tanHalfFovy);
  Result[1][1] = 1.0f / (tanHalfFovy);
  Result[2][2] = zFar / (zFar - zNear);
  Result[2][3] = 1.0f;
  Result[3][2] = -(zFar * zNear) / (zFar - zNear);
  return Result;
}

float mjm::radians(float degrees)
{
  return degrees * 0.01745329251994329576923690768489f;
}

float mjm::degrees(float radians)
{
  return radians * 57.295779513082320876798154814105f;
}

mjm::vec3 mjm::cos(const vec3& v)
{
  using std::cos;
  return vec3(cos(v.x), cos(v.y), cos(v.z));
}

mjm::vec3 mjm::sin(const vec3& v)
{
  using std::sin;
  return vec3(sin(v.x), sin(v.y), sin(v.z));
}
#endif
