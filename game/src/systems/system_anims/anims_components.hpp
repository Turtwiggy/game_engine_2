#pragma once

#include "game_and_engine_interop.hpp"

#include <entt/fwd.hpp>

#include <string>

namespace game2d {

// rows

enum class SpriteDir
{
  S = 0,
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
  std::vector<std::pair<int, int>> anim; // frame idx of anim
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
  // if row is -1 -- means replace it with the direction row
  SpriteAnim{ .name = "idle", .anim = { { 0, -1 }, { 1, -1 } } }, //
  SpriteAnim{ .name = "walk", .anim = { { 2, -1 }, { 3, -1 }, { 4, -1 }, { 3, -1 } } },
  SpriteAnim{ .name = "sword", .anim = { { 5, -1 }, { 6, -1 }, { 7, -1 }, { 8, -1 } } },
  SpriteAnim{ .name = "bow", .anim = { { 9, -1 }, { 10, -1 }, { 11, -1 }, { 12, -1 } } },
  SpriteAnim{ .name = "stave", .anim = { { 13, -1 }, { 14, -1 }, { 15, -1 }, { 15, -1 } } },
  SpriteAnim{ .name = "throw", .anim = { { 16, -1 }, { 17, -1 }, { 18, -1 } } },
  SpriteAnim{ .name = "hurt", .anim = { { 19, -1 }, { 20, -1 }, { 21, -1 } } },
  SpriteAnim{ .name = "death", .anim = { { 22, -1 }, { 23, -1 }, { 24, -1 }, { 23, -1 } } },
  SpriteAnim{ .name = "carry", .anim = { { 26, -1 }, { 25, -1 }, { 27, -1 }, { 25, -1 } } },
  SpriteAnim{ .name = "jump", .anim = { { 0, -1 }, { 28, -1 } } },
  SpriteAnim{ .name = "spin", .anim = { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 } } }
};

SpriteComponent
default_character_spritesheet();

struct SpriteAnimationState
{
  std::string playing_animation_name;

  float timer = 0.0f;
  float duration = 0.5f; // seconds
  bool playing = true;
  bool looping = true;
};

struct SpriteDirComponent
{
  SpriteDir dir = SpriteDir::S;
  vec2 vel;
};

} // namespace game2d