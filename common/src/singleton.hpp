#pragma once

#include <entt/fwd.hpp>

namespace game2d {

// https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern

template<class T>
class Singleton
{
public:
  static T& get()
  {
    static T instance;
    return instance;
  }

protected:
  Singleton() {}

public:
  Singleton(Singleton const&) = delete;
  void operator=(Singleton const&) = delete;
};

} // namespace game2d