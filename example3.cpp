/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper.h"

struct simple_msg : public message_type {};

int main() {
  pipeline_system system(true); /* visualization is enabled in the constructor */

  auto jobs = system.create_queue("jobs", 10);
  auto processed = system.create_queue("processed", 10);
  auto collected = system.create_queue("collected", 10);

  system.spawn_producer(
      "producer", [&]() -> std::shared_ptr<simple_msg> { return std::make_shared<simple_msg>(); }, jobs);

  system.spawn_transformer<simple_msg>(
      "transformer",
      [&](std::shared_ptr<simple_msg> job) -> std::shared_ptr<simple_msg> { return job; },
      jobs,
      processed);

  system.spawn_transformer<simple_msg>(
      "worker 1",
      [&](std::shared_ptr<simple_msg> job) -> std::shared_ptr<simple_msg> { return job; },
      processed,
      collected,
      transform_type::same_pool);
  system.spawn_transformer<simple_msg>(
      "worker 2",
      [&](std::shared_ptr<simple_msg> job) -> std::shared_ptr<simple_msg> { return job; },
      processed,
      collected,
      transform_type::same_pool);
  system.spawn_transformer<simple_msg>(
      "worker 3",
      [&](std::shared_ptr<simple_msg> job) -> std::shared_ptr<simple_msg> { return job; },
      processed,
      collected,
      transform_type::same_pool);

  system.spawn_consumer<simple_msg>(
      "printer",
      [](auto result) { /* sleep to simulate slow processing */
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      },
      collected);
  system.start();
}
