#pragma once

#include <entt/entt.hpp>

#include <optional>
#include <string>

namespace game2d {

template<class T>
entt::entity
create_empty(entt::registry& r, const std::optional<T>& val = std::nullopt)
{
  // val passed in. could be useful for debugging

#if defined(_DEBUG)
  // const std::string name = typeid(T).name();
  // const std::string tag = cleanup_tag_str(name);
#else
#endif
  const std::string tag = "default";

  const auto e = r.create();
  // r.emplace<TagComponent>(e, tag);

  if (val.has_value())
    r.emplace<T>(e, val.value());
  else
    r.emplace<T>(e); // default

  return e;
}

} // namespace game2d