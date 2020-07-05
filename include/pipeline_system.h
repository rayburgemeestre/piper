/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "node.h"
#include "stats.h"
#include "storage_container.h"
#include "transform_type.hpp"

class pipeline_system {
public:
  std::vector<std::shared_ptr<storage_container>> containers;
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
  void link(std::shared_ptr<storage_container>);
  void link(node *);
  void sleep();
  void start();
  bool active();

  std::shared_ptr<storage_container> create_storage(size_t max_items);
  std::shared_ptr<storage_container> create_storage(const std::string &name, size_t max_items);

  template <typename F>
  void spawn_producer(std::string name, F &&fun, std::shared_ptr<storage_container> output);
  template <typename IN, typename F>
  void spawn_transformer(std::string name,
                         F &&fun,
                         std::shared_ptr<storage_container> input,
                         std::shared_ptr<storage_container> output,
                         std::optional<transform_type> tt = std::nullopt);
  template <typename IN, typename F>
  void spawn_consumer(std::string name, F &&fun, std::shared_ptr<storage_container> input);

  template <typename F>
  void spawn_producer(F &&fun, std::shared_ptr<storage_container> output);
  template <typename IN, typename F>
  void spawn_transformer(F &&fun,
                         std::shared_ptr<storage_container> input,
                         std::shared_ptr<storage_container> output,
                         std::optional<transform_type> tt = std::nullopt);
  template <typename IN, typename F>
  void spawn_consumer(F &&fun, std::shared_ptr<storage_container> input);
};

// spawn functions

template <typename F>
void pipeline_system::spawn_producer(F &&fun, std::shared_ptr<storage_container> output) {
  spawn_producer("", fun, output);
}

template <typename IN, typename F>
void pipeline_system::spawn_transformer(F &&fun,
                                        std::shared_ptr<storage_container> input,
                                        std::shared_ptr<storage_container> output,
                                        std::optional<transform_type> tt) {
  spawn_transformer<IN>("", fun, input, output, tt);
}

template <typename IN, typename F>
void pipeline_system::spawn_consumer(F &&fun, std::shared_ptr<storage_container> input) {
  spawn_consumer<IN>("", fun, input);
}

template <typename F>
void pipeline_system::spawn_producer(std::string name, F &&fun, std::shared_ptr<storage_container> output) {
  auto n = std::make_shared<node>(name, *this);
  n->set_produce_function(fun);
  n->set_output_storage(output);
  spawned.push_back(n);
}

template <typename IN, typename F>
void pipeline_system::spawn_transformer(std::string name,
                                        F &&fun,
                                        std::shared_ptr<storage_container> input,
                                        std::shared_ptr<storage_container> output,
                                        std::optional<transform_type> tt) {
  auto n = std::make_shared<node>(name, *this);
  static int uid = 1;
  if (tt && *tt == transform_type::same_pool) {
    n->set_id(0);
  } else {
    n->set_id(uid++);
  }

  auto wrapper_fun = [&](auto in) -> auto { return fun(std::dynamic_pointer_cast<IN>(in)); };

  n->set_transform_function(wrapper_fun);
  n->set_input_storage(input);
  n->set_output_storage(output);
  if (tt) {
    n->set_transform_type(*tt);
  }
  spawned.push_back(n);
}

template <typename IN, typename F>
void pipeline_system::spawn_consumer(std::string name, F &&fun, std::shared_ptr<storage_container> input) {
  auto n = std::make_shared<node>(name, *this);

  auto wrapper_fun = [&](std::shared_ptr<message_type> in) { return fun(std::dynamic_pointer_cast<IN>(in)); };

  n->set_consume_function(wrapper_fun);
  n->set_input_storage(input);
  spawned.push_back(n);
}
