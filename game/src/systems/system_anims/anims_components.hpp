#pragma once

#include "game_and_engine_interop.hpp"

#include <entt/fwd.hpp>

#include <string>

namespace game2d {

// rows

enum class SpriteDir
{
  S,
  SE,
  E,
  NE,
  N,
  NW,
  W,
  SW
};

struct SpriteData
{
  int row;
  SpriteDir dir;
};

// columns

struct SpriteAnim
{
  std::string name;
  uint32_t start_frame = 0;
  uint32_t end_frame = 0;
};

// data

const std::vector<SpriteData> dirinfo = {
  //
  SpriteData{ .row = 0, .dir = SpriteDir::S },
  SpriteData{ .row = 1, .dir = SpriteDir::SE },
  SpriteData{ .row = 2, .dir = SpriteDir::E },
  SpriteData{ .row = 3, .dir = SpriteDir::NE },
  SpriteData{ .row = 4, .dir = SpriteDir::N },
  SpriteData{ .row = 5, .dir = SpriteDir::NW },
  SpriteData{ .row = 6, .dir = SpriteDir::W },
  SpriteData{ .row = 7, .dir = SpriteDir::SW }
  //
};

const std::vector<SpriteAnim> anims{
  SpriteAnim{ .name = "idle", .start_frame = 0, .end_frame = 1 },
  SpriteAnim{ .name = "walk", .start_frame = 2, .end_frame = 4 },
  SpriteAnim{ .name = "carry", .start_frame = 25, .end_frame = 26 },
  SpriteAnim{ .name = "jump", .start_frame = 27, .end_frame = 28 },
  SpriteAnim{ .name = "sword", .start_frame = 5, .end_frame = 8 },
  SpriteAnim{ .name = "bow", .start_frame = 9, .end_frame = 12 },
  SpriteAnim{ .name = "stave", .start_frame = 13, .end_frame = 15 },
  SpriteAnim{ .name = "throw", .start_frame = 16, .end_frame = 18 },
  SpriteAnim{ .name = "hurt", .start_frame = 19, .end_frame = 21 },
  SpriteAnim{ .name = "death", .start_frame = 22, .end_frame = 24 },
};

SpriteComponent
default_character_spritesheet();

} // namespace game2d