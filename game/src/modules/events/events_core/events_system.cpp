#include "pch.hpp"

#include "events_components.hpp"

#include "events_system.hpp"

#include "modules/box2d/box2d_components.hpp"
#include "modules/events/event_coll_log/event_coll_log_helpers.hpp"
#include "modules/events/event_coll_player_provider/event_coll_player_provider_helpers.hpp"

namespace game2d {

void
init_events_system(entt::registry& r)
{
  auto& evts_c = SINGLE_Events::get();

  // link event => function

  evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__log>(r);
  evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__player_provider>(r);
  evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__player_receiver>(r);
  evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__check_for_gameover>(r);

  evts_c.dispatcher.sink<OnCollisionExit>().connect<&handle_on_coll_exit__log>(r);

  // evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__check_for_gameover>(r);
}

} // namespace game2d