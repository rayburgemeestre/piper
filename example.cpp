/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper.h"

#include <iostream>
#include <random>

struct random_xy : public message_type {
  double x;
  double y;
  random_xy(double x, double y) : x(x), y(y) {}
};

struct in_circle : public message_type {
  bool value;
  explicit in_circle(bool value) : value(value) {}
};

int main() {
  pipeline_system system;

  auto points = system.create_queue(5);
  auto results = system.create_queue(5);

  // produce endless stream of random X,Y coordinates.
  system.spawn_producer(
      [&]() -> auto {
        static std::mt19937 gen;
        auto x = (gen() / double(gen.max()));
        auto y = (gen() / double(gen.max()));
        return std::make_shared<random_xy>(x, y);
      },
      points);

  // check if these points are within a circle
  system.spawn_transformer<random_xy>(
      [](auto point) -> auto {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto dist = sqrt(pow(point->x - 0.5, 2) + pow(point->y - 0.5, 2));
        return std::make_shared<in_circle>(dist <= 0.5);
      },
      points,
      results);

  // estimate pi based on results
  system.spawn_consumer<in_circle>(
      [](auto result) {
        static size_t nom = 0, denom = 0;
        if (result->value) nom++;
        denom++;
        a(std::cout) << "Estimated pi: " << 4 * (nom / (double)denom) << std::endl;
      },
      results);

  system.start();
}
