/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "message_type.hpp"

class pipeline_system;
class node;

class queue {
public:
  std::condition_variable cv;
  std::vector<std::pair<std::set<int>, std::shared_ptr<message_type>>> items;
  std::map<int, size_t> consumer_items_available;
  std::mutex items_mut;
  std::string name;
  pipeline_system &system;
  size_t max_items = 10;
  bool active = true;
  bool terminating = false;
  std::set<int> consumer_ids = {0};
  std::vector<node *> consumer_ptrs;
  std::vector<node *> provider_ptrs;

  explicit queue(std::string name, pipeline_system &sys, int max_items);

  void set_consumer(node *node_ptr, int id);
  void set_provider(node *node_ptr);
  void sleep_until_not_full();
  void sleep_until_items_available(int id);
  void push(std::shared_ptr<message_type> value);
  bool is_full();
  bool is_full_unprotected() const;
  bool has_items(int id);
  bool has_items_unprotected(int id);
  std::shared_ptr<message_type> pop(int id);
  void check_terminate();
  void deactivate(std::unique_lock<std::mutex> &lock);
  size_t size();
};
