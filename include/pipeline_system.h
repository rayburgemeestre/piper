/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>
#include <condition_variable>
#include <mutex>
#include <map>
#include <thread>
#include <string>
#include <functional>

#include "stats.h"
#include "transform_type.hpp"
#include "storage_container.h"
#include "node.h"

class pipeline_system {
public:
  std::vector<std::shared_ptr<storage_container> > containers;
  std::vector<node *> nodes;
  std::condition_variable cv;
  std::mutex mut;
  bool started = false;
  bool is_active = true;
  stats stats_;
  std::thread runner;
  std::mutex mut_timeout;
  std::condition_variable cv_timeout;
  std::vector<std::shared_ptr<node>> spawned;

  pipeline_system();
  ~pipeline_system();

  void run();
  void link(std::shared_ptr<storage_container> );
  void link(node *);
  void sleep();
  void start();
  bool active();

  std::shared_ptr<storage_container> create_storage(const std::string& name, size_t max_items);

  template <typename F = std::function<int *()>>
  void spawn_producer(std::string name, F fun, std::shared_ptr<storage_container> output);
  template <typename F = std::function<int(int i)>>
  void spawn_transformer(std::string name, F fun, std::shared_ptr<storage_container> input, std::shared_ptr<storage_container> output, std::optional<transform_type> tt = std::nullopt);
  template <typename F = std::function<void(int i)>>
  void spawn_consumer(std::string name, F fun, std::shared_ptr<storage_container> input);

  template <typename F = std::function<int *()>>
  void spawn_producer(F fun, std::shared_ptr<storage_container> output);
  template <typename F = std::function<int(int i)>>
  void spawn_transformer(F fun, std::shared_ptr<storage_container> input, std::shared_ptr<storage_container> output, std::optional<transform_type> tt = std::nullopt);
  template <typename F = std::function<void(int i)>>
  void spawn_consumer(F fun, std::shared_ptr<storage_container> input);
};

// spawn functions

template <typename F>
void pipeline_system::spawn_producer(F fun, std::shared_ptr<storage_container> output) {
  spawn_producer("", fun, output);
}

template <typename F>
void pipeline_system::spawn_transformer(F fun, std::shared_ptr<storage_container> input, std::shared_ptr<storage_container> output, std::optional<transform_type> tt) {
  spawn_transformer("", fun, input, output, tt);
}

template <typename F>
void pipeline_system::spawn_consumer(F fun, std::shared_ptr<storage_container> input) {
  spawn_consumer("", fun, input);
}

template <typename F>
void pipeline_system::spawn_producer(std::string name, F fun, std::shared_ptr<storage_container> output) {
  auto n = std::make_shared<node>(name, *this);
  n->set_produce_function(fun);
  n->set_output_storage(output);
  spawned.push_back(n);
}

template <typename F>
void pipeline_system::spawn_transformer(std::string name, F fun, std::shared_ptr<storage_container> input, std::shared_ptr<storage_container> output, std::optional<transform_type> tt) {
  auto n = std::make_shared<node>(name, *this);
  static int uid = 1;
  if (tt && *tt == transform_type::same_pool) {
    n->set_id(0);
  } else {
    n->set_id(uid++);
  }
  n->set_transform_function(fun);
  n->set_input_storage(input);
  n->set_output_storage(output);
  if (tt) {
    n->set_transform_type(*tt);
  }
  spawned.push_back(n);
}

template <typename F>
void pipeline_system::spawn_consumer(std::string name, F fun, std::shared_ptr<storage_container> input) {
  auto n = std::make_shared<node>(name, *this);
  n->set_consume_function(fun);
  n->set_input_storage(input);
  spawned.push_back(n);
}

// convenience functions

template <typename OUT>
std::function<std::shared_ptr<message_type>()> producer_function(std::function<std::shared_ptr<message_type>()> func) {
  return [=]() -> std::shared_ptr<message_type> { return func(); };
}

template <typename IN, typename OUT>
std::function<std::shared_ptr<message_type>(std::shared_ptr<message_type>)> transform_function(
  std::function<std::shared_ptr<OUT>(std::shared_ptr<IN>)> &&func) {
  std::function<std::shared_ptr<message_type>(std::shared_ptr<message_type>)> test(
    [=](std::shared_ptr<message_type> item) -> std::shared_ptr<message_type> {
      auto msg = std::dynamic_pointer_cast<IN>(item);
      return std::dynamic_pointer_cast<message_type>(func(msg));
    });
  return test;
}

template <typename IN>
std::function<void(std::shared_ptr<message_type>)> consume_function(std::function<void(std::shared_ptr<IN>)> &&func) {
  return [=](std::shared_ptr<message_type> item) -> void {
    auto msg = std::dynamic_pointer_cast<IN>(item);
    func(msg);
  };
}
