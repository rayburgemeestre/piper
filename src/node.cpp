/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "node.h"
#include "pipeline_system.h"
#include "storage_container.h"

// TODO: put in pipeline_system?, generate_name() ?
static int global_counter = 1;

node::node(pipeline_system &sys) : node("", sys) {}

node::node(std::string name, pipeline_system &sys) : system(sys), runner(std::bind(&node::run, this)), name(name) {
  sys.link(this);
  sys.set_stats(name, false);
}

void node::init() {
  if (name.empty()) {
    if (!input_storage && output_storage) {
      name = "producer " + std::to_string(global_counter);
    } else if (input_storage && output_storage) {
      name = "transformer " + std::to_string(global_counter);
    } else if (input_storage && !output_storage) {
      name = "consumer " + std::to_string(global_counter);
    }
    global_counter++;
  }
}

void node::set_input_storage(storage_container *ptr) {
  input_storage = ptr;
  input_storage->set_consumer(this, id);
}

// there can only be one
// TODO: different approach, we need more outputs actually, so we can just link up different consumers to different
// outputs. This way we don't accidentally push too much stuff , and keeps things simple.
void node::set_output_storage(storage_container *ptr) {
  output_storage = ptr;
  output_storage->set_provider(this);
}

void node::stop() {
  system.set_stats_active(name, false);
  active = false;
  if (input_storage) input_storage->stop();
  if (output_storage) output_storage->stop();
}

void node::run() {
  system.sleep();
  while (system.active() && active) {
    // only producer
    if (!input_storage && output_storage) {
      while (!output_storage->is_full() && active) {
        std::shared_ptr<message_type> ret = produce();
        if (!ret) {
          system.set_stats_active(name, false);
          active = false;
          output_storage->check_terminate();
          break;
        }
        output_storage->add(std::move(ret));
      }
      if (active) {
        system.set_stats_sleep_until_not_full(name, true);
        output_storage->sleep_until_not_full();
        system.set_stats_sleep_until_not_full(name, false);
      }
    }
    // transformer
    else if (input_storage && output_storage) {
      system.set_stats_sleep_until_not_empty(name, true);
      input_storage->sleep_until_items_available(id);
      system.set_stats_sleep_until_not_empty(name, false);
      while (input_storage->has_items(id)) {
        auto ret = input_storage->get(id);
        if (ret) {  // if we are sharing the ID with multiple threads, it can happen
          // that we will end up with checking if there is something, but while get()ing it
          // it was already gone, and nullptr will be returned.
          auto transformed = std::move(transform(std::move(ret)));
          system.set_stats_sleep_until_not_full(name, true);
          output_storage->sleep_until_not_full();
          system.set_stats_sleep_until_not_full(name, false);
          output_storage->add(std::move(transformed));
        }
      }
      if (!input_storage->active) {
        system.set_stats_active(name, false);
        active = false;
        output_storage->check_terminate();
      }
    }
    // only consumer
    else if (input_storage && !output_storage) {
      system.set_stats_sleep_until_not_empty(name, true);
      input_storage->sleep_until_items_available(id);
      system.set_stats_sleep_until_not_empty(name, false);
      while (input_storage->has_items(id)) {
        auto ret2 = input_storage->get(id);
        consume(std::move(ret2));
      }
      if (!input_storage->active) {
        system.set_stats_active(name, false);
        active = false;
      }
    }
  }
}

std::shared_ptr<message_type> node::produce() {
  return produce_fun();
}

std::shared_ptr<message_type> node::transform(std::shared_ptr<message_type> item) {
  return transform_fun(std::move(item));
}

void node::consume(std::shared_ptr<message_type> item) {
  return consume_fun(std::move(item));
}
