/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper.h"

#include <atomic>
#include <iostream>

struct seq : public message_type {
  size_t i;
  explicit seq(size_t i) : i(i) {}
};

struct seq_multiplied : public message_type {
  size_t i;
  explicit seq_multiplied(size_t i) : i(i) {}
};

void test(transform_type tt) {
  pipeline_system system;

  auto numbers = system.create_queue(1);
  auto results = system.create_queue(1);

  // produce sequence of 1..10.
  size_t max = 10;
  std::atomic<size_t> i = 1;
  system.spawn_producer(
      [&]() -> std::shared_ptr<seq> {
        if (i <= max) return std::make_shared<seq>(i++);
        return nullptr;
      },
      numbers);

  // multiply number by ten with three workers
  auto multiply_by_ten = [](auto seq) -> auto { return std::make_shared<seq_multiplied>(seq->i * 10); };
  for (int i = 0; i < 3; i++) {
    system.spawn_transformer<seq>(multiply_by_ten, numbers, results, tt);
  }

  // print results
  system.spawn_consumer<seq_multiplied>([](auto result) { a(std::cout) << result->i << ", "; }, results);

  system.start();
}

int main() {
  a(std::cout) << "workers same pool:" << std::endl;
  test(transform_type::same_pool);
  a(std::cout) << "" << std::endl << "---" << std::endl;

  a(std::cout) << "workers same workload:" << std::endl;
  test(transform_type::same_workload);
  a(std::cout) << "" << std::endl;
}
