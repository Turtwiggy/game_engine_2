#include "pch.hpp"

#include "anims_components.hpp"

#include "modules/raws/raws_components.hpp"

namespace game2d {

SpriteComponent
default_character_spritesheet()
{
  const SpriteComponent s{
    .sprite_max_x = 928 / 32, // size of texture px / size per sprite
    .sprite_max_y = 256 / 32,
    .spritesheet_idx = char_spritesheet_idx,
  };
  return s;
}

} // namespace game2d