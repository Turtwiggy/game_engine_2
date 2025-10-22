#pragma once

#include "modules/models/model_components.hpp"
#include "modules/models_animations/model_animation_components.hpp"

namespace game2d {

Animation
load_animation(const Model& model);
void
load_animations(SINGLE_AnimatorComponent& anims, SINGLE_ModelsComponent& models);

void
play_animation(SINGLE_AnimatorComponent& anims, Animation* a);
void
update_animation(SINGLE_AnimatorComponent& anims, float dt);

} // namespace game2d