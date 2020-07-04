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

#include "statz.hpp"
#include "transform_type.hpp"
#include "storage_container.h"
#include "node.h"

class pipeline_system {
public:
  std::vector<storage_container *> containers;
  std::vector<node *> nodes;
  std::condition_variable cv;
  std::mutex mut;
  bool started = false;
  bool is_active = true;

  std::mutex stats_mut;
  std::map<std::string, statz> stats;
  std::thread runner;

  std::mutex xx;
  std::condition_variable x;

  pipeline_system();
  ~pipeline_system();
  void run();
  void set_stats(std::string name, bool is_storage);
  void set_stats_sleep_until_not_full(std::string name, bool val);
  void set_stats_sleep_until_not_empty(std::string name, bool val);
  void set_stats_size(std::string name, int size);
  void set_stats_active(std::string name, bool active);
  void link(storage_container *);
  void link(node *);
  void sleep();
  void start();
  void stop();
  bool active();

  template <typename F = std::function<int *()>>
  node *spawn_producer(std::string name, F fun, storage_container *output);
  template <typename F = std::function<int(int i)>>
  node *spawn_transformer(std::string name, F fun, storage_container *input, storage_container *output, std::optional<transform_type> tt = std::nullopt);
  template <typename F = std::function<void(int i)>>
  node *spawn_consumer(std::string name, F fun, storage_container *input);

  template <typename F = std::function<int *()>>
  node *spawn_producer(F fun, storage_container *output);
  template <typename F = std::function<int(int i)>>
  node *spawn_transformer(F fun, storage_container *input, storage_container *output, std::optional<transform_type> tt = std::nullopt);
  template <typename F = std::function<void(int i)>>
  node *spawn_consumer(F fun, storage_container *input);
};

// spawn functions

template <typename F>
node *pipeline_system::spawn_producer(F fun, storage_container *output) {
  return spawn_producer("", fun, output);
}

template <typename F>
node *pipeline_system::spawn_transformer(F fun, storage_container *input, storage_container *output, std::optional<transform_type> tt) {
  return spawn_transformer("", fun, input, output, tt);
}

template <typename F>
node *pipeline_system::spawn_consumer(F fun, storage_container *input) {
  return spawn_consumer("", fun, input);
}

template <typename F>
node *pipeline_system::spawn_producer(std::string name, F fun, storage_container *output) {
  auto n = new node(name, *this);
  n->produce_fun = fun;
  n->set_output_storage(output);
  return n;
}

template <typename F>
node *pipeline_system::spawn_transformer(std::string name, F fun, storage_container *input, storage_container *output, std::optional<transform_type> tt) {
  auto n = new node(name, *this);
  static int uid = 1;
  if (tt && *tt == transform_type::same_pool) {
    n->id = 0;
  } else {
    n->id = uid++;
  }
  n->transform_fun = fun;
  n->set_input_storage(input);
  n->set_output_storage(output);
  if (tt) {
    n->tt = tt;
  }
  return n;
}

template <typename F>
node *pipeline_system::spawn_consumer(std::string name, F fun, storage_container *input) {
  auto n = new node(name, *this);
  n->consume_fun = fun;
  n->set_input_storage(input);
  return n;
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
