#pragma once

#include <mutex>
#include <vector>

namespace game2d {

template<typename T>
struct EventQueue
{
  void enqueue(const std::vector<T>&& t)
  {
    std::lock_guard<std::mutex> lock(m);
    data.insert(data.end(), std::make_move_iterator(t.begin()), std::make_move_iterator(t.end()));
  };

  std::vector<T> dequeue()
  {
    std::lock_guard<std::mutex> lock(m);

    if (data.size() > 0)
      return std::move(data);

    return {};
  }

private:
  std::vector<T> data;
  mutable std::mutex m;
};

} // namespace game2d