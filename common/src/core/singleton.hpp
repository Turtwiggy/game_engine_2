#pragma once

#include <entt/fwd.hpp>
#include <entt/signal/dispatcher.hpp>

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

struct SINGLE_Events : public Singleton<SINGLE_Events>
{
  entt::dispatcher dispatcher;

  SINGLE_Events() = default;
};

} // namespace game2d