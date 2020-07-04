/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "pipeline_system.h"
#include "line_type.hpp"
#include "node.h"

#include <iostream>

#include "util/a.hpp"

pipeline_system::pipeline_system() : runner(std::bind(&pipeline_system::run, this)) {}
pipeline_system::~pipeline_system() {
  // std::cout << "SHOULD TERM " << std::endl;
  is_active = false;
  {
    std::unique_lock l(xx);
    x.notify_all();
  }
  runner.join();
}

void pipeline_system::set_stats(std::string name, bool is_storage) {
  std::scoped_lock sl(stats_mut);
  stats[name].name = name;
  stats[name].is_storage = is_storage;
  stats[name].active = true;
}
void pipeline_system::set_stats_sleep_until_not_full(std::string name, bool val) {
  std::scoped_lock sl(stats_mut);
  stats[name].is_sleeping_until_not_full = val;
}
void pipeline_system::set_stats_sleep_until_not_empty(std::string name, bool val) {
  std::scoped_lock sl(stats_mut);
  stats[name].is_sleeping_until_not_empty = val;
}
void pipeline_system::set_stats_size(std::string name, int size) {
  std::scoped_lock sl(stats_mut);
  stats[name].size = size;
}
void pipeline_system::set_stats_active(std::string name, bool active) {
  std::scoped_lock sl(stats_mut);
  stats[name].active = active;
}

void pipeline_system::sleep() {
  std::unique_lock<std::mutex> lock(mut);
  cv.wait(lock, [&]() { return started; });
}

bool pipeline_system::active() {
  return is_active;
}

void pipeline_system::link(storage_container *s) {
  containers.push_back(s);
}

void pipeline_system::link(node *n) {
  nodes.push_back(n);
}

void pipeline_system::stop() {
  for (const auto &node : nodes) {
    node->stop();  // cascades to storages
  }
}

void pipeline_system::start() {
  for (const auto &node : nodes) {
    node->init();
  }

  struct vis {
    line_type lt;
    std::string input;
    std::string storage;
    std::string output;
    std::string output_tt;
  };

  std::vector<vis> lines;
  for (const auto &container : containers) {
    // lines.reserve(std::max(container->provider_ptrs.size(), container->consumer_ptrs.size()));
    for (size_t i = 0; i < std::max(container->provider_ptrs.size(), container->consumer_ptrs.size()); i++) {
      struct vis v;
      v.storage = i == 0 ? container->name : "";
      if (i < container->provider_ptrs.size()) {
        auto provider = container->provider_ptrs[i];
        v.input = provider->name;
      }
      if (i < container->consumer_ptrs.size()) {
        auto consumer = container->consumer_ptrs[i];
        v.output = consumer->name;
        v.output_tt = (consumer->tt ? (*consumer->tt == transform_type::same_workload ? "AND" : "OR") : "");
      }
      lines.push_back(v);
    }

    a(std::cout) << "container: " << container->name << std::endl;
    for (const auto &provider : container->provider_ptrs) {
      a(std::cout) << " - provider: " << provider->name << " "
                   << (provider->tt ? (*provider->tt == transform_type::same_workload ? "AND" : "OR") : "")
                   << std::endl;
    }
    for (const auto &consumer : container->consumer_ptrs) {
      a(std::cout) << " - consumer: " << consumer->name << " "
                   << (consumer->tt ? (*consumer->tt == transform_type::same_workload ? "AND" : "OR") : "")
                   << std::endl;
    }
  }
  for (const auto &line : lines) {
    std::string linetstr = ([&]() {
      if (line.lt == line_type::normal) return "normal";
      if (line.lt == line_type::extra_input) return "extra_input";
      if (line.lt == line_type::extra_output) return "extra_output";
      return "";
    })();
    std::cout << "line: " << linetstr << " | " << line.input << " | " << line.storage << " | " << line.output << " "
              << line.output_tt << std::endl;
  }
  // std::exit(0);

  {
    std::scoped_lock<std::mutex> lock(mut);
    started = true;
    cv.notify_all();
  }

  for (const auto &node : nodes) {
    node->runner.join();
  }
}

void pipeline_system::run() {
  while (is_active) {
    std::unique_lock l(xx);
    x.wait_for(l, std::chrono::milliseconds(1000), [&]() { return !is_active; });
    // std::cout << "WAKEUP" << std::endl;
    // if (!is_active) break;
    {
      std::scoped_lock lk(stats_mut);
      if (false) {
        a(std::cout) << "--- begin ---" << std::endl;
        for (const auto &stat : stats) {
          auto p = stat.second;
          a(std::cout) << p.name << ":" << std::endl;
          a(std::cout) << "is_storage: " << std::boolalpha << p.is_storage << std::endl;
          a(std::cout) << "is_active: " << std::boolalpha << p.active << std::endl;
          if (p.is_storage) {
            a(std::cout) << "items.size(): " << p.size << std::endl;
          } else {
            a(std::cout) << "sleep until not empty: " << std::boolalpha << p.is_sleeping_until_not_empty << std::endl;
            a(std::cout) << "sleep until not full: " << std::boolalpha << p.is_sleeping_until_not_full << std::endl;
          }
          a(std::cout) << "" << std::endl;
        }
        a(std::cout) << "---  end  ---" << std::endl;
      }
      // if (is_active) std::this_thread::sleep_for(std::chrono::hours(1));
    }
    std::exit(1);
  }
}
