#include "pch.hpp" // IWYU pragma: keep

#include "anims_system.hpp"

#include "anims_components.hpp"
#include "game_and_engine_interop.hpp"

namespace game2d {

int
get_index(const float time, const float duration, const int size)
{
  if (duration == 0.0f)
    return 0;
  const float r = time / duration;                      // a value between 0 and 1
  const int s = static_cast<int>(glm::floor(r * size)); // a value between 0 and size
  return glm::clamp(s, 0, size);                        // check between 0 and size
};

void
update_animator_system(entt::registry& r, const float dt)
{
#if defined(_DEBUG)
  // ZoneScoped;
#endif

  const auto& view = r.view<SpriteComponent, SpriteAnimationState, SpriteDirComponent>();
  for (const auto& [e, sprite_c, animation, sprite_dir_c] : view.each()) {

    // hack: choose anim by idx
    int idle_anim_idx = 0;
    int walk_anim_idx = 1;
    int anim_idx = idle_anim_idx;
    auto length = glm::length2(glm::vec2{ sprite_dir_c.vel.x, sprite_dir_c.vel.y });
    if (length > 1.0f)
      anim_idx = walk_anim_idx;

    auto anim = anims[anim_idx];

    animation.timer += dt;

    const int n_frames = anim.anim.size();

    if (animation.timer >= animation.duration && !animation.looping) {
      const int i0 = (int)(n_frames - 1);
      // const SpritePosition& frame = anim.animation_frames[i0];
      // sprite_c.tex_pos = frame;
      continue;
    }

    // loop the timer
    if (animation.timer >= animation.duration)
      animation.timer -= animation.duration * glm::floor(animation.timer / animation.duration);

    // get the index of the frame to play
    const int i0 = get_index(animation.timer, animation.duration, n_frames);
    // SDL_Log("i0: %i", i0);

    const auto& anim_info = anim.anim[i0];
    const auto& dir = (int)sprite_dir_c.dir;

    sprite_c.sprite_pos_x = anim_info.first;
    sprite_c.sprite_pos_y = anim_info.second == -1 ? dir : anim_info.second;
  }
}

} // namespace game2d