/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <optional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>

#include "transform_type.hpp"
#include "message_type.hpp"

class pipeline_system;
class storage_container;

class node {
public:
  std::condition_variable cv;
  std::mutex mut;

  storage_container *input_storage = nullptr;
  storage_container *output_storage = nullptr;
  pipeline_system &system;
  std::thread runner;

  std::string name;
  bool active = true;
  int id = 0;
  std::optional<transform_type> tt;

  std::function<std::shared_ptr<message_type>()> produce_fun = []() -> std::shared_ptr<message_type> {
    return nullptr;
  };
  std::function<std::shared_ptr<message_type>(std::shared_ptr<message_type>)> transform_fun =
    [](std::shared_ptr<message_type> a) -> std::shared_ptr<message_type> {
    return a;
  };

  std::function<void(std::shared_ptr<message_type>)> consume_fun = [](std::shared_ptr<message_type>) {};

  explicit node(pipeline_system &sys);
  explicit node(std::string name, pipeline_system &sys);
  void init();
  void set_input_storage(storage_container *ptr);
  void set_output_storage(storage_container *ptr);
  void stop();
  void run();
  std::shared_ptr<message_type> produce();
  std::shared_ptr<message_type> transform(std::shared_ptr<message_type> item);
  void consume(std::shared_ptr<message_type> item);
};
