/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "message_type.hpp"
#include "queue.h"
#include "transform_type.hpp"

class pipeline_system;

class node {
private:
  pipeline_system &system;
  int64_t id_ = 0;
  std::string name_;
  std::thread runner;
  bool active_ = true;
  std::shared_ptr<queue> input_queue;
  std::shared_ptr<queue> output_queue;
  std::optional<transform_type> transform_type_;
  using message_t = std::shared_ptr<message_type>;
  using produce_fun_t = std::function<message_t()>;
  using transform_fun_t = std::function<message_t(message_t)>;
  using consume_fun_t = std::function<void(message_t)>;
  produce_fun_t produce_fun = []() -> message_t { return nullptr; };
  transform_fun_t transform_fun = [](message_t a) -> message_t { return a; };
  consume_fun_t consume_fun = [](const message_t &) {};

public:
  explicit node(pipeline_system &sys);
  explicit node(const std::string &name, pipeline_system &sys);

  std::string name();
  bool active();
  std::optional<transform_type> get_transform_type();

  void set_id(int64_t id);
  void init();
  void set_input_queue(std::shared_ptr<queue> ptr);
  void set_output_queue(std::shared_ptr<queue> ptr);
  void set_transform_type(transform_type tt);
  void run();

  void set_produce_function(produce_fun_t fun);
  void set_transform_function(transform_fun_t fun);
  void set_consume_function(consume_fun_t fun);

  std::shared_ptr<message_type> produce();
  std::shared_ptr<message_type> transform(std::shared_ptr<message_type> item);
  void consume(std::shared_ptr<message_type> item);

  void sleep_until_items_available();
  void sleep_until_not_full();
  void deactivate();
  void join();
};
