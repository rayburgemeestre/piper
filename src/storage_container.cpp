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
  consumers.insert(id);
  consumer_ptrs.push_back(node_ptr);
}

void storage_container::set_provider(node *node_ptr) {
  provider_ptrs.push_back(node_ptr);
}

storage_container::storage_container(std::string name, pipeline_system &sys, int max_items)
    : name(name), system(sys), max_items(max_items) {
  sys.link(this);
  sys.set_stats(name, true);
}

void storage_container::sleep_until_not_full() {
  // std::unique_lock<std::mutex> lock(mut_not_full);
  std::unique_lock<std::mutex> lock(items_mut);
  if (items.size() < max_items) {
    return;  // no sleep
  }
  if (!active) return;  // do not sleep
  // cv_not_full.wait_for(lock, std::chrono::milliseconds(100), [&]() { return !is_full() || !active; });
  // cv_not_full.wait(lock, [&]() { return !is_full_unprotected() || !active; });
  cv.wait(lock, [&]() { return !items_is_full || !active; });
}

void storage_container::sleep_until_items_available(int id) {
  // std::unique_lock<std::mutex> lock(mut_has_items[id]);
  std::unique_lock<std::mutex> lock(items_mut);
  if (has_items_no_lock(id)) return;  // do not sleep!
  if (!active) return;                // do not sleep
  // cv_has_items[id].wait_for(lock, std::chrono::milliseconds(100), [this, id]() { return has_items(id) || !active; });
  // cv_has_items[id].wait(lock, [this, id]() { return has_items(id) || !active; });
  // cv_has_items[id].wait(lock, [this, id]() { return has_items_no_lock(id) || !active; });
  cv.wait(lock, [this, id]() { return items_has_items || !active; });
}

void storage_container::stop() {
  system.set_stats_active(name, false);
  active = false;

  ////    std::scoped_lock<std::mutex> lock2(mut_not_full);
  //    for (auto &pair : cv_has_items) {
  //      pair.second.notify_all();
  //    }
  //    cv_not_full.notify_all();
  cv.notify_all();
}

void storage_container::add(std::shared_ptr<message_type> value) {
  {
    std::unique_lock<std::mutex> scoped_lock(items_mut);
    items.push_back(std::make_pair(consumers, std::move(value)));
    system.set_stats_size(name, items.size());

    items_is_full = is_full_unprotected();
    items_has_items = !items.empty();

    // notify_has_items();
  }

  // instead of: notify_all();
  // std::scoped_lock<std::mutex> lock2(mut_has_items);
  // notify_has_items();
  cv.notify_all();
}

bool storage_container::is_full() {
  std::scoped_lock<std::mutex> lock(items_mut);
  return is_full_unprotected();
}

bool storage_container::is_full_unprotected() {
  return items.size() >= max_items;
}

size_t storage_container::num_items(int id) {
  std::scoped_lock<std::mutex> lock(items_mut);
  // TODO: something more efficient than this of course
  size_t counter = 0;
  for (const auto &item : items) {
    if (item.first.find(id) != item.first.end()) {
      counter++;
    }
  }
  return counter;
}

bool storage_container::has_items(int id) {
  std::scoped_lock<std::mutex> lock(items_mut);
  return has_items_no_lock(id);
}

bool storage_container::has_items_no_lock(int id) {
  // TODO: something more efficient than this of course
  for (const auto &item : items) {
    if (item.first.find(id) != item.first.end()) {
      return true;  // there is something for this id
    }
  }
  return false;
}

#include <iostream>
#include "util/a.hpp"
std::shared_ptr<message_type> storage_container::get(int id) {
  // std::unique_lock<std::mutex> sl(sleep_mut);
  std::unique_lock<std::mutex> lock(items_mut);

  auto find = items.end();
  std::stringstream ss;
  std::shared_ptr<message_type> ret = nullptr;
  int num;
  for (auto &pair : items) {
    if (pair.first.find(id) != pair.first.end()) {
      pair.first.erase(id);

      // to be returned
      // we need to copy in case we have more than one remaining consumers for this thing
      // todo: move to shared ptrs, so we can avoid the copy here.
      // using unique_ptrs was nice to force us to get our moves right.
      if (!pair.first.empty()) {  // (note we already erased one), more left still
        //         not last one, we need to copy it
        //        message_type *msg = pair.second.get();
        //        auto copy = std::unique_ptr<message_type>(msg->clone());
        ret = pair.second;  // std::move(copy);
      } else {
        // last one , we can move
        ret = std::move(pair.second);
      }
      a(std::cout) << "Found for ID: " << id << " " << ret << std::endl;

      // all consumers are done with this
      if (pair.first.empty()) {
        find = std::find(items.begin(), items.end(), pair);
      }
      break;  // we got one bitch, fucking exit in that case!!! FFS!!!
    }
  }
  if (find != items.end()) {
    items.erase(find);
    system.set_stats_size(name, items.size());
  }

  // hmmm
  items_is_full = is_full_unprotected();
  items_has_items = !items.empty();

  lock.unlock();

  {
    // std::scoped_lock<std::mutex> lock2(mut_not_full);
    // notify_not_full();
    cv.notify_all();
  }

  // check_terminate();
  return ret;
}

void storage_container::check_terminate() {
  auto terminate = true;
  for (auto upstream : provider_ptrs) {
    if (upstream->active) {
      terminate = false;
    }
  }
  if (terminate) {
    system.set_stats_active(name, false);
    active = false;
    cv.notify_all();
  }
}
