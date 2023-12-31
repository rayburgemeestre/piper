/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "node.h"
#include "queue.h"
#include "stats.h"
#include "transform_type.hpp"

class pipeline_system {
public:
  bool visualization_enabled;
  std::vector<std::shared_ptr<queue>> containers;
  std::vector<node *> nodes;
  std::condition_variable cv;
  std::mutex mut;
  bool started = false;
  bool is_active = true;
  stats stats_;
  std::thread runner;
  std::vector<std::shared_ptr<node>> spawned;

  explicit pipeline_system();
  explicit pipeline_system(bool visualization_enabled);
  ~pipeline_system();

  void run();
  void link(std::shared_ptr<queue>);
  void link(node *);
  void sleep();
  void start(bool auto_join_threads = true);
  void explicit_join();
  bool active() const;

  std::shared_ptr<queue> create_queue(size_t max_items);
  std::shared_ptr<queue> create_queue(const std::string &name, size_t max_items);

  template <typename F>
  void spawn_producer(std::string name, F &&fun, std::shared_ptr<queue> output);
  template <typename IN, typename F>
  void spawn_transformer(std::string name,
                         F &&fun,
                         std::shared_ptr<queue> input,
                         std::shared_ptr<queue> output,
                         std::optional<transform_type> tt = std::nullopt);
  template <typename IN, typename F>
  void spawn_consumer(std::string name, F &&fun, std::shared_ptr<queue> input);

  template <typename F>
  void spawn_producer(F &&fun, std::shared_ptr<queue> output);
  template <typename IN, typename F>
  void spawn_transformer(F &&fun,
                         std::shared_ptr<queue> input,
                         std::shared_ptr<queue> output,
                         std::optional<transform_type> tt = std::nullopt);
  template <typename IN, typename F>
  void spawn_consumer(F &&fun, std::shared_ptr<queue> input);

  const stats &get_stats() const;
};

// spawn functions

template <typename F>
void pipeline_system::spawn_producer(F &&fun, std::shared_ptr<queue> output) {
  spawn_producer("", fun, output);
}

template <typename IN, typename F>
void pipeline_system::spawn_transformer(F &&fun,
                                        std::shared_ptr<queue> input,
                                        std::shared_ptr<queue> output,
                                        std::optional<transform_type> tt) {
  spawn_transformer<IN>("", fun, input, output, tt);
}

template <typename IN, typename F>
void pipeline_system::spawn_consumer(F &&fun, std::shared_ptr<queue> input) {
  spawn_consumer<IN>("", fun, input);
}

template <typename F>
void pipeline_system::spawn_producer(std::string name, F &&fun, std::shared_ptr<queue> output) {
  auto n = std::make_shared<node>(name, *this);
  n->set_produce_function(fun);
  n->set_output_queue(output);
  spawned.push_back(n);
}

template <typename IN, typename F>
void pipeline_system::spawn_transformer(std::string name,
                                        F &&fun,
                                        std::shared_ptr<queue> input,
                                        std::shared_ptr<queue> output,
                                        std::optional<transform_type> tt) {
  auto n = std::make_shared<node>(name, *this);
  static int uid = 1;
  if (tt || *tt == transform_type::same_pool) {
    n->set_id(0);
  } else {
    n->set_id(uid++);
  }

  /* Story time: this wrapper_fun was capturing by [&] initially, and caused quite a bit of issues
   * when the optimizer did its magic, apparently it got rid of the captures made by the lambda it
   * was wrapping around. This included all captures, such as 'this', which got random values.
   * I guess the entire lambda was basically returned to the free store. "fun" is an r-value, so it
   * makes sense it was considered a moved-away-from thing that goes out of scope once this function
   * exits. The point was to move it into this wrapper function of course, so copying it solves it.
   * I don't know why this issue didn't present itself during Debug builds.
   */
  auto wrapper_fun = [=](auto in) -> auto { return fun(std::dynamic_pointer_cast<IN>(in)); };

  n->set_transform_function(wrapper_fun);
  n->set_input_queue(input);
  n->set_output_queue(output);
  if (tt) {
    n->set_transform_type(*tt);
  }
  spawned.push_back(n);
}

template <typename IN, typename F>
void pipeline_system::spawn_consumer(std::string name, F &&fun, std::shared_ptr<queue> input) {
  auto n = std::make_shared<node>(name, *this);

  auto wrapper_fun = [=](std::shared_ptr<message_type> in) { return fun(std::dynamic_pointer_cast<IN>(in)); };

  n->set_consume_function(wrapper_fun);
  n->set_input_queue(input);
  spawned.push_back(n);
}
