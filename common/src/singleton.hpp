#pragma once

#include <entt/fwd.hpp>

namespace game2d {

// https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern

template<class T>
class Singleton
{

  Singleton& operator=(const Singleton&) = default;
  Singleton& operator=(Singleton&&) = default;

public:
  static T& get()
  {
    static T instance;
    return instance;
  }

  void reset()
  {
    Singleton instance;
    *this = std::move(instance);
  }

protected:
  Singleton() {}

public:
  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) = delete;
};

} // namespace game2d