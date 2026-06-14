#include "pch.hpp" // IWYU pragma: keep

#include "anims_system.hpp"

#include "anims_components.hpp"
#include "game_and_engine_interop.hpp"
#include "systems/system_input/input_components.hpp"

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

  const auto update_anims = [](const InputComponent& input_c,
                               SpriteComponent& sprite_c,
                               SpriteDirComponent& sprite_dir_c,
                               SpriteAnimationState& animation,
                               float dt) {
    //
    // hack: choose anim by idx
    int idle_anim_idx = 0;
    int walk_anim_idx = 1;
    int anim_idx = idle_anim_idx;
    auto length = glm::length2(glm::vec2{ input_c.lx, input_c.ly });
    if (length > 0.0f)
      anim_idx = walk_anim_idx;

    // hack: loop through the anims.
    // int anim_idx = animation.anim_idx;
    // int max_anims = anims.size();

    const auto anim = anims[anim_idx];

    animation.timer += dt;

    const int n_frames = anim.anim.size();

    // hack: x ms per frame
    animation.duration = (float)n_frames * 0.2f;

    if (animation.timer >= animation.duration && !animation.looping) {
      // const int i0 = (int)(n_frames - 1);
      // const SpritePosition& frame = anim.animation_frames[i0];
      // sprite_c.tex_pos = frame;
      return;
    }

    // loop the timer
    if (animation.timer >= animation.duration) {
      animation.timer -= animation.duration * glm::floor(animation.timer / animation.duration);

      // hack: loop through anims
      // animation.anim_idx += 1;
      // animation.anim_idx %= anims.size();
    }

    // get the index of the frame to play
    const int i0 = get_index(animation.timer, animation.duration, n_frames);
    // SDL_Log("i0: %i", i0);

    const auto& anim_info = anim.anim[i0];
    const auto& dir = (int)sprite_dir_c.dir;

    sprite_c.sprite_pos_x = anim_info.first;
    sprite_c.sprite_pos_y = anim_info.second == -1 ? dir : anim_info.second;
  };

  const auto view0 = r.view<SpriteComponent, SpriteDirComponent, SpriteAnimationState>();
  for (const auto& [e, sprite_c, sprite_dir_c, animation] : view0.each()) {

    // [optional] InputComponent
    InputComponent input_c;
    if (r.all_of<InputComponent>(e))
      input_c = r.get<InputComponent>(e);

    update_anims(input_c, sprite_c, sprite_dir_c, animation, dt);
  }

  std::unordered_set<entt::entity> processed_parent_e;

  const auto view1 = r.view<SpriteComponent, SpriteFollowParentComponent>();
  for (const auto& [e, sprite_c, parent_c] : view1.each()) {

    auto parent_e = parent_c.parent_e;

    // only process once per sprite parent
    if (processed_parent_e.contains(parent_e))
      continue;

    const auto& input_c = r.get<InputComponent>(parent_e);
    auto& sprite_dir_c = r.get<SpriteDirComponent>(parent_e);
    auto& animation = r.get<SpriteAnimationState>(parent_e);

    update_anims(input_c, sprite_c, sprite_dir_c, animation, dt);

    processed_parent_e.emplace(parent_e);
  }

  //
}

} // namespace game2d