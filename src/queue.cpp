/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sstream>
#include <utility>

#include "node.h"
#include "pipeline_system.h"
#include "queue.h"

void queue::set_consumer(node *node_ptr, int id) {
  consumer_ids.insert(id);
  consumer_ptrs.push_back(node_ptr);
}

void queue::set_provider(node *node_ptr) {
  provider_ptrs.push_back(node_ptr);
}

queue::queue(std::string name, pipeline_system &sys, int max_items)
    : name(std::move(name)), system(sys), max_items(max_items), consumer_ids({0}) {}

void queue::sleep_until_not_full() {
  std::unique_lock<std::mutex> lock(items_mut);
  if (items.size() < max_items) {
    return;
  }
  if (!active) {
    return;
  }
  cv.wait(lock, [&]() { return !is_full_unprotected() || !active; });
}

void queue::sleep_until_items_available(int id) {
  std::unique_lock<std::mutex> lock(items_mut);
  if (has_items_unprotected(id)) {
    return;
  }
  if (!active) {
    return;
  }
  cv.wait(lock, [this, id]() { return has_items_unprotected(id) || !active; });
}

void queue::push(std::shared_ptr<message_type> value) {
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

bool queue::is_full() {
  std::scoped_lock<std::mutex> lock(items_mut);
  return is_full_unprotected();
}

bool queue::is_full_unprotected() const {
  return items.size() >= max_items;
}

bool queue::has_items(int id) {
  std::scoped_lock<std::mutex> lock(items_mut);
  return has_items_unprotected(id);
}

bool queue::has_items_unprotected(int id) {
  return std::any_of(
      items.begin(), items.end(), [&id](const auto &item) { return item.first.find(id) != item.first.end(); });
}

std::shared_ptr<message_type> queue::pop(int id) {
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
  if (is_empty && terminating) {
    deactivate(lock);
  } else {
    lock.unlock();
    cv.notify_all();
  }
  return ret;
}

void queue::check_terminate() {
  std::unique_lock<std::mutex> lock(items_mut);
  auto terminate = true;
  for (auto upstream : provider_ptrs) {
    if (upstream->active()) {
      terminate = false;
    }
  }
  if (terminate) {
    terminating = true;
    // deactivate now (otherwise after pop() of the last item)
    if (items.empty()) {
      deactivate(lock);
    }
  }
}

void queue::deactivate(std::unique_lock<std::mutex> &lock) {
  system.stats_.set_active(name, false);
  active = false;
  lock.unlock();
  cv.notify_all();
}
