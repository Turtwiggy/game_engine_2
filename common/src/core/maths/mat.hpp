#pragma once

#include "core/maths/vec.hpp"

namespace game2d {

typedef struct Matrix4x4
{
  float m11, m12, m13, m14;
  float m21, m22, m23, m24;
  float m31, m32, m33, m34;
  float m41, m42, m43, m44;
} Matrix4x4;

Matrix4x4
operator*(const Matrix4x4& a, const Matrix4x4& b);

Matrix4x4
Matrix4x4_CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);

Matrix4x4
Matrix4x4_CreateView(vec2 pos);

} // namespace game2d