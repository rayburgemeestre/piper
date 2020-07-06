/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper.h"

int main() {
  pipeline_system system(true); /* visualization is enabled in the constructor */

  /* around 900.000 FPS on my laptop */
  if (false) {
    auto q1 = system.create_queue(100);
    system.spawn_producer(
        []() -> auto { return std::make_shared<message_type>(); }, q1);
    system.spawn_consumer<message_type>([](auto) {}, q1);
    system.start();
  }
  /* around 300.000 FPS on my laptop */
  if (false) {
    auto q1 = system.create_queue(100);
    auto q2 = system.create_queue(100);
    system.spawn_producer(
        []() -> auto { return std::make_shared<message_type>(); }, q1);
    system.spawn_transformer<message_type>(
        [&](auto job) -> auto { return job; }, q1, q2);
    system.spawn_consumer<message_type>([](auto) {}, q2);
    system.start();
  }
  /* around 285.000 FPS on my laptop */
  if (true) {
    auto q1 = system.create_queue(100);
    auto q2 = system.create_queue(100);
    system.spawn_producer(
        []() -> auto { return std::make_shared<message_type>(); }, q1);
    for (int i = 0; i < 4; i++)
      system.spawn_transformer<message_type>(
          [&](auto job) -> auto { return job; }, q1, q2);
    system.spawn_consumer<message_type>([](auto) {}, q2);
    system.start();
  }
}
