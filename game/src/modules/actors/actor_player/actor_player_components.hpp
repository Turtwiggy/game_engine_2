#pragma once

#include <string>
#include <vector>

namespace game2d {

struct PlayerComponent
{
  bool placeholder = true;
};

struct SpriteInfo
{
  std::string name;
};

struct PlayerSpriteComponent
{
  std::vector<std::string> sprites;
};

} // namespace game2d