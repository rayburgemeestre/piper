/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "node.h"
#include "pipeline_system.h"
#include "queue.h"
#include "util/threadname.hpp"

static int global_counter = 1;

node::node(pipeline_system& sys) : node("", sys) {}

node::node(const std::string& name, pipeline_system& sys)
    : system(sys), name_(name), runner(std::bind(&node::run, this)) {
  sys.link(this);
  sys.stats_.set_type(name, false);
}

std::string node::name() {
  return name_;
}

bool node::active() {
  return active_;
}

std::optional<transform_type> node::get_transform_type() {
  return transform_type_;
}

void node::set_id(int64_t id) {
  this->id_ = id;
}

void node::init() {
  if (name_.empty()) {
    if (!input_queue && output_queue) {
      name_ = "producer " + std::to_string(global_counter);
    } else if (input_queue && output_queue) {
      name_ = "transformer " + std::to_string(global_counter);
    } else if (input_queue && !output_queue) {
      name_ = "consumer " + std::to_string(global_counter);
    }
    global_counter++;
  }
  set_thread_name(name_);
}

void node::set_input_queue(std::shared_ptr<queue> ptr) {
  input_queue = ptr;
  input_queue->set_consumer(this, id_);
}

void node::set_output_queue(std::shared_ptr<queue> ptr) {
  output_queue = ptr;
  output_queue->set_provider(this);
}

void node::set_transform_type(transform_type tt) {
  transform_type_ = tt;
}

void node::run() {
  system.sleep();
  while (system.active() && active_) {
    // producer
    if (!input_queue && output_queue) {
      while (!output_queue->is_full() && active_) {
        std::shared_ptr<message_type> ret = produce();
        if (ret) {
          output_queue->push(std::move(ret));
        } else {
          deactivate();
          break;
        }
      }
      if (active_) {
        sleep_until_not_full();
      }
    }
    // transformer
    else if (input_queue && output_queue) {
      sleep_until_items_available();
      while (input_queue->has_items(id_)) {
        if (auto ret = input_queue->pop(id_)) {
          auto transformed = transform(std::move(ret));
          sleep_until_not_full();
          output_queue->push(std::move(transformed));
        }
      }
      if (!input_queue->active) {
        deactivate();
      }
    }
    // consumer
    else if (input_queue && !output_queue) {
      sleep_until_items_available();
      while (input_queue->has_items(id_)) {
        auto ret2 = input_queue->pop(id_);
        consume(std::move(ret2));
      }
      if (!input_queue->active) {
        deactivate();
      }
    }
  }
}

void node::set_produce_function(produce_fun_t fun) {
  produce_fun = std::move(fun);
}

void node::set_transform_function(transform_fun_t fun) {
  transform_fun = std::move(fun);
}

void node::set_consume_function(consume_fun_t fun) {
  consume_fun = std::move(fun);
}

std::shared_ptr<message_type> node::produce() {
  system.stats_.add_counter(name_);
  return produce_fun();
}

std::shared_ptr<message_type> node::transform(std::shared_ptr<message_type> item) {
  system.stats_.add_counter(name_);
  return transform_fun(std::move(item));
}

void node::consume(std::shared_ptr<message_type> item) {
  system.stats_.add_counter(name_);
  return consume_fun(std::move(item));
}

void node::sleep_until_items_available() {
  system.stats_.set_sleep_until_not_empty(name_, true);
  input_queue->sleep_until_items_available(id_);
  system.stats_.set_sleep_until_not_empty(name_, false);
}

void node::sleep_until_not_full() {
  system.stats_.set_sleep_until_not_full(name_, true);
  output_queue->sleep_until_not_full();
  system.stats_.set_sleep_until_not_full(name_, false);
}

void node::deactivate() {
  system.stats_.set_active(name_, false);
  active_ = false;
  if (output_queue) output_queue->check_terminate();
}

void node::join() {
  runner.join();
}
