#include "pch.hpp" // IWYU pragma: keep

#include "actor_player_helpers.hpp"

#include "actor_player_components.hpp"
#include "modules/filesystem/filesystem_helpers.hpp"
#include "modules/raws/raws_components.hpp"
#include "modules/renderer/renderer_helpers.hpp"
#include "systems/system_anims/anims_components.hpp"

namespace game2d {

using namespace std::literals;

void
load_spritelayer_textures(entt::registry& r, GameData* data)
{
  // individual spritesheets...
  const auto char_base_path = "packs/PUNY_CHARACTERS_v2_1/Individual Spritesheets/";
  const auto layer_0_path = char_base_path + "Layer 0 - Skins/"s;
  // const auto layer_1_path = char_base_path + "Layer 1 - Shoes/"s;
  const auto layer_2_path = char_base_path + "Layer 2 - Clothes/"s;
  // const auto layer_3_path = char_base_path + "Layer 3 - Gloves/"s;
  const auto layer_4_path = char_base_path + "Layer 4 - Hairstyle/"s;
  const auto layer_5_path = char_base_path + "Layer 5 - Eyes/"s;
  const auto layer_6_path = char_base_path + "Layer 6 - Headgears/"s;
  // const auto layer_7_path = char_base_path + "Layer 7 - Add-ons/"s;

  auto base_path = SDL_GetBasePath();
  char full_path[256];
  SDL_snprintf(full_path, sizeof(full_path), "%sassets/", base_path);

  const auto available_0 = iterate_dir_recursive(full_path + layer_0_path);
  // const auto available_1 = iterate_dir_recursive(full_path + layer_1_path);
  const auto available_2 = iterate_dir_recursive(full_path + layer_2_path);
  // const auto available_3 = iterate_dir_recursive(full_path + layer_3_path);
  const auto available_4 = iterate_dir_recursive(full_path + layer_4_path);
  const auto available_5 = iterate_dir_recursive(full_path + layer_5_path);
  const auto available_6 = iterate_dir_recursive(full_path + layer_6_path);
  // const auto available_7 = iterate_dir_recursive(full_path + layer_7_path);

  if (available_0.size() == 0) {
    throw std::runtime_error("Unable to find .png in directory");
    exit(SDL_APP_FAILURE); // crash
  }
  if (available_2.size() == 0) {
    throw std::runtime_error("Unable to find .png in nested directory");
    exit(SDL_APP_FAILURE); // crash
  }

  auto choice_0 = 2; // Skins
  // auto choice_1 = 0;  // Shoes
  auto choice_2 = 4; // Clothes
  // auto choice_3 = 0;  // Gloves
  auto choice_4 = 12; // Hairstyle
  auto choice_5 = 0;  // Eyes
  auto choice_6 = 23; // Headgears
  // auto choice_7 = 0; // Add

  const auto tex_0 = create_and_upload_gpu_texture(data->device, layer_0_path + available_0[choice_0]);
  // const auto tex_1 = create_and_upload_gpu_texture(data->device, layer_1_path + available_1[choice_1]);
  const auto tex_2 = create_and_upload_gpu_texture(data->device, layer_2_path + available_2[choice_2]);
  // const auto tex_3 = create_and_upload_gpu_texture(data->device, layer_3_path + available_3[choice_3]);
  const auto tex_4 = create_and_upload_gpu_texture(data->device, layer_4_path + available_4[choice_4]);
  const auto tex_5 = create_and_upload_gpu_texture(data->device, layer_5_path + available_5[choice_5]);
  const auto tex_6 = create_and_upload_gpu_texture(data->device, layer_6_path + available_6[choice_6]);
  // const auto tex_7 = create_and_upload_gpu_texture(data->device, layer_7_path + available_7[choice_7]);

  data->unprocessed_textures.push_back(tex_0.texture);
  // data->unprocessed_textures.push_back(tex_1.texture);
  data->unprocessed_textures.push_back(tex_2.texture);
  // data->unprocessed_textures.push_back(tex_3.texture);
  data->unprocessed_textures.push_back(tex_4.texture);
  data->unprocessed_textures.push_back(tex_5.texture);
  data->unprocessed_textures.push_back(tex_6.texture);
  // data->unprocessed_textures.push_back(tex_7.texture);
}

void
attach_spritelayers(entt::registry& r, entt::entity player_e, const GameData* data)
{

  PlayerSpriteComponent sprite_c;
  sprite_c.sprites.push_back("Human1"); // layer 0
  // sprite_c.sprites.push_back("IronBoots");           // layer 1
  sprite_c.sprites.push_back("GoldArmour"); // layer 2
  // sprite_c.sprites.push_back("GlovesBrown");         // layer 3
  sprite_c.sprites.push_back("Beardstyle1Black");    // layer 4
  sprite_c.sprites.push_back("EyecolorBlue");        // layer 5
  sprite_c.sprites.push_back("AssasinBandanaBlack"); // layer 6
  // sprite_c.sprites.push_back("GoatHorns1");          // layer 7
  r.emplace<PlayerSpriteComponent>(player_e, sprite_c);

  const auto n_custom_spritesheet = data->n_preused_textures;
  const auto n_loaded_spritesheet = (int)data->unprocessed_textures.size();
  const auto n_textures_already_used = n_custom_spritesheet + n_loaded_spritesheet;

  auto tmp_x = 0.0f;
  auto tmp_idx = 0;

  for (int i = sprite_c.sprites.size() - 1; i >= 0; i--) {

    // const auto& sprite = sprite_c.sprites[i];

    auto sprite_e = r.create();

    SpriteComponent hmm = default_character_spritesheet();

    // todo: improve this
    hmm.spritesheet_idx = n_textures_already_used + i;

    attach_sprite(r,
                  sprite_e,
                  SpriteDef{
                    .size = { 64.0f, 64.0f },
                    .sprite = hmm,
                    .is_emitter = tmp_idx == 0,
                  });

    r.emplace<SpriteFollowParentComponent>(sprite_e,
                                           SpriteFollowParentComponent{
                                             .parent_e = player_e,
                                           });

    tmp_x += 100;
    tmp_idx += 1;
  }
}

} // namespace game2d