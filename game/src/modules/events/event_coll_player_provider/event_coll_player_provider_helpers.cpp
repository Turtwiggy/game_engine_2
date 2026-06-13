#include "pch.hpp" // IWYU pragma: keep

#include "event_coll_player_provider_helpers.hpp"

#include "game_and_engine_interop.hpp"

#include "modules/actors/actor_player/actor_player_components.hpp"
#include "modules/box2d/box2d_helpers.hpp"
#include "modules/entt/entt_helpers.hpp"
#include "systems/system_items/items_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_components.hpp"

namespace game2d {

void
handle_on_coll_enter__player_provider(entt::registry& r, const OnCollisionEnter& evt)
{
  const auto parent_a_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_a).body_id);
  const auto parent_b_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_b).body_id);

  const auto [shape_a, shape_b] = coll<const PlayerComponent, const ContainerProviderComponent>(r, parent_a_e, parent_b_e);
  if (shape_a == entt::null || shape_b == entt::null)
    return; // not a coll of interest

  SDL_Log("collision enter with provider.");

  auto& provider_inv = r.get<InventoryComponent>(parent_b_e);
  if (provider_inv.items <= 0)
    return; // no more items to give
  provider_inv.items--;

  auto& player_inv = r.get<InventoryComponent>(parent_a_e);
  player_inv.items++;
}

void
handle_on_coll_enter__player_receiver(entt::registry& r, const OnCollisionEnter& evt)
{
  const auto parent_a_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_a).body_id);
  const auto parent_b_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_b).body_id);

  const auto [shape_a, shape_b] = coll<const PlayerComponent, const ContainerReceiverComponent>(r, parent_a_e, parent_b_e);

  if (shape_a == entt::null || shape_b == entt::null)
    return; // not a coll of interest

  SDL_Log("collision enter with reciever.");

  auto& player_inv = r.get<InventoryComponent>(parent_a_e);
  if (player_inv.items <= 0)
    return; // no item on player
  player_inv.items--;

  auto& consumer_inv = r.get<InventoryComponent>(parent_b_e);
  consumer_inv.items++;
}

void
handle_on_coll_enter__check_for_gameover(entt::registry& r, const OnCollisionEnter& evt)
{
  const auto parent_a_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_a).body_id);
  const auto parent_b_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_b).body_id);
  const auto [shape_a, shape_b] = coll<const PlayerComponent, const RedWizardComponent>(r, parent_a_e, parent_b_e);

  if (shape_a == entt::null || shape_b == entt::null)
    return; // not a coll of interest

  const auto& consumer_inv = r.get<const RedWizardComponent>(parent_b_e);
  // SDL_Log("consumer has: %i items", consumer_inv.items);
  // const bool gameover = consumer_inv.items >= 5;
  const bool gameover = true;

  if (gameover) {
    SDL_Log("dingding! gameover");
    create_empty<Request_GameOver>(r);
  }
}

} // namespace game2d