#pragma once

#include <mutex>
#include <vector>

namespace game2d {

template<typename T>
struct EventQueue
{
  void enqueue(const std::vector<T>& t)
  {
    std::lock_guard<std::mutex> lock(m);

    data.insert(data.end(), t.begin(), t.end());
  };

  std::vector<T> dequeue_all()
  {
    std::lock_guard<std::mutex> lock(m);

    if (data.size() > 0) {
      auto result = std::move(data);
      data.clear();
      return result;
    }

    return {};
  }

  void clear()
  {
    std::lock_guard<std::mutex> lock(m);

    data.clear();
  }

private:
  std::vector<T> data;
  mutable std::mutex m;
};

} // namespace game2d