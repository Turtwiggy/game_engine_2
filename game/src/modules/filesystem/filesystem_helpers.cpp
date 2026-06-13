#include "pch.hpp"

#include "filesystem_helpers.hpp"

#include "modules/sdl/sdl_exception.hpp"

namespace game2d {

using namespace std::literals;

std::vector<std::string>
iterate_dir_recursive(const std::string& path)
{
  std::vector<std::string> results;

  const std::vector<std::string> patterns = { "*.png", "**/*.png" };

  for (const auto& pattern : patterns) {

    int count = 0;
    char** files = SDL_GlobDirectory(path.c_str(), pattern.c_str(), 0, &count);

    if (!files)
      continue;

    results.reserve(results.size() + count);

    for (int i = 0; i < count; i++)
      results.emplace_back(files[i]);

    SDL_free(files);
  }

  return results;
}

} // namespace game2d