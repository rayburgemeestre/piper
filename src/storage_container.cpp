/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sstream>

#include "node.h"
#include "pipeline_system.h"
#include "storage_container.h"

void storage_container::set_consumer(node *node_ptr, int id) {
  consumer_ids.insert(id);
  consumer_ptrs.push_back(node_ptr);
}

void storage_container::set_provider(node *node_ptr) {
  provider_ptrs.push_back(node_ptr);
}

storage_container::storage_container(const std::string &name, pipeline_system &sys, int max_items)
    : name(name), system(sys), max_items(max_items) {}

void storage_container::sleep_until_not_full() {
  std::unique_lock<std::mutex> lock(items_mut);
  if (items.size() < max_items) {
    return;
  }
  if (!active) {
    return;
  }
  cv.wait(lock, [&]() { return !is_full_unprotected() || !active || !system.active(); });
}

void storage_container::sleep_until_items_available(int id) {
  std::unique_lock<std::mutex> lock(items_mut);
  if (has_items_unprotected(id)) {
    return;
  }
  if (!active) {
    return;
  }
  cv.wait(lock, [this, id]() { return has_items_unprotected(id) || !active || !system.active(); });
}

void storage_container::push(std::shared_ptr<message_type> value) {
  {
    std::unique_lock<std::mutex> scoped_lock(items_mut);
    items.emplace_back(std::make_pair(consumer_ids, std::move(value)));
    system.stats_.set_size(name, items.size());
    //    for (const auto &consumer : consumer_ids) {
    //      consumer_items_available[consumer]++;
    //    }
  }
  cv.notify_all();
}

bool storage_container::is_full() {
  std::scoped_lock<std::mutex> lock(items_mut);
  return is_full_unprotected();
}

bool storage_container::is_full_unprotected() {
  return items.size() >= max_items;
}

bool storage_container::has_items(int id) {
  std::scoped_lock<std::mutex> lock(items_mut);
  return has_items_unprotected(id);
}

bool storage_container::has_items_unprotected(int id) {
  //  return consumer_items_available[id] != 0;
  for (const auto &item : items) {
    if (item.first.find(id) != item.first.end()) {
      return true;  // there is something for this id
    }
  }
  return false;
}

std::shared_ptr<message_type> storage_container::pop(int id) {
  std::unique_lock<std::mutex> lock(items_mut);
  auto find = items.end();
  std::shared_ptr<message_type> ret = nullptr;
  for (auto &pair : items) {
    if (pair.first.find(id) != pair.first.end()) {
      pair.first.erase(id);
      //      consumer_items_available[id]--;
      if (!pair.first.empty()) {
        ret = pair.second;
      } else {
        ret = std::move(pair.second);
      }
      if (pair.first.empty()) {
        find = std::find(items.begin(), items.end(), pair);
      }
      break;
    }
  }
  if (find != items.end()) {
    items.erase(find);
    system.stats_.set_size(name, items.size());
  }
  bool is_empty = items.empty();
  lock.unlock();
  if (is_empty && terminating) {
    deactivate();
  } else {
    cv.notify_all();
  }
  return ret;
}

void storage_container::check_terminate() {
  auto terminate = true;
  for (auto upstream : provider_ptrs) {
    if (upstream->active()) {
      terminate = false;
    }
  }
  if (terminate) {
    terminating = true;
    std::unique_lock<std::mutex> lock(items_mut);
    // deactivate now (otherwise after pop() of the last item)
    if (items.empty()) {
      lock.unlock();
      deactivate();
    }
  }
}

void storage_container::deactivate() {
  system.stats_.set_active(name, false);
  active = false;
  cv.notify_all();
  for (const auto &consumer : consumer_ptrs) {
    consumer->deactivate();
  }
}
