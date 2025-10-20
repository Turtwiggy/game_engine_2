#pragma once

namespace game2d {

// Vertex Formats

/*

struct vertex
{
  vec3 position;
  vec2 uv;
};

typedef struct PositionVertex
{
  float x, y, z;
} PositionVertex;

typedef struct PositionColorVertex
{
  float x, y, z;
  Uint8 r, g, b, a;
} PositionColorVertex;

*/

typedef struct PositionTextureVertex
{
  float x, y, z;
  float u, v;
} PositionTextureVertex;

typedef struct Index
{
  Uint16 i;
} Index;

using VertexFinal = PositionTextureVertex;

} // namespace game2d