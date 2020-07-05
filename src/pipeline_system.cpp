/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "pipeline_system.h"
//#include "line_type.hpp"
#include "node.h"

#include <iostream>

#include "util/a.hpp"

pipeline_system::pipeline_system() : pipeline_system(false) {}

pipeline_system::pipeline_system(bool visualization_enabled)
    : visualization_enabled(visualization_enabled), runner(std::bind(&pipeline_system::run, this)) {}

pipeline_system::~pipeline_system() {
  is_active = false;
  runner.join();
}

void pipeline_system::sleep() {
  std::unique_lock<std::mutex> lock(mut);
  cv.wait(lock, [&]() { return started; });
}

bool pipeline_system::active() {
  return is_active;
}

void pipeline_system::link(std::shared_ptr<queue> s) {
  containers.push_back(s);
}

void pipeline_system::link(node *n) {
  nodes.push_back(n);
}

void pipeline_system::start() {
  for (const auto &node : nodes) {
    node->init();
  }
  stats_.setup(containers);
  {
    std::scoped_lock<std::mutex> lock(mut);
    started = true;
    cv.notify_all();
  }

  for (const auto &node : nodes) {
    node->join();
  }
}

void pipeline_system::run() {
  while (is_active && visualization_enabled) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stats_.display();
  }
}

std::shared_ptr<queue> pipeline_system::create_queue(size_t max_items) {
  static int i = 1;
  std::string name = "storage " + std::to_string(i++);
  return create_queue(name, max_items);
}

std::shared_ptr<queue> pipeline_system::create_queue(const std::string &name, size_t max_items) {
  auto instance = std::make_shared<queue>(name, *this, max_items);
  link(instance);
  stats_.set_type(name, true);
  return instance;
}
