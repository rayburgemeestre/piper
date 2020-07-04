/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stats.h"

#include <iostream>
#include <thread>

#include "util/a.hpp"

void stats::set_type(const std::string& name, bool is_storage) {
  std::scoped_lock sl(stats_mut);
  stats_[name].name = name;
  stats_[name].is_storage = is_storage;
  stats_[name].active = true;
}

void stats::set_sleep_until_not_full(const std::string& name, bool val) {
  std::scoped_lock sl(stats_mut);
  stats_[name].is_sleeping_until_not_full = val;
}

void stats::set_sleep_until_not_empty(const std::string& name, bool val) {
  std::scoped_lock sl(stats_mut);
  stats_[name].is_sleeping_until_not_empty = val;
}

void stats::set_size(const std::string& name, int size) {
  std::scoped_lock sl(stats_mut);
  stats_[name].size = size;
}

void stats::set_active(const std::string& name, bool active) {
  std::scoped_lock sl(stats_mut);
  stats_[name].active = active;
}

void stats::setup() {
  //  struct vis {
  //    line_type lt;
  //    std::string input;
  //    std::string storage;
  //    std::string output;
  //    std::string output_tt;
  //  };
  //
  //  std::vector<vis> lines;
  //  for (const auto &container : containers) {
  //    for (size_t i = 0; i < std::max(container->provider_ptrs.size(), container->consumer_ptrs.size()); i++) {
  //      struct vis v;
  //      v.storage = i == 0 ? container->name : "";
  //      if (i < container->provider_ptrs.size()) {
  //        auto provider = container->provider_ptrs[i];
  //        v.input = provider->name();
  //      }
  //      if (i < container->consumer_ptrs.size()) {
  //        auto consumer = container->consumer_ptrs[i];
  //        v.output = consumer->name();
  //        v.output_tt = (consumer->get_transform_type()
  //                           ? (*consumer->get_transform_type() == transform_type::same_workload ? "AND" : "OR")
  //                           : "");
  //      }
  //      lines.push_back(v);
  //    }
  //
  //    a(std::cout) << "container: " << container->name << std::endl;
  //    for (const auto &provider : container->provider_ptrs) {
  //      a(std::cout) << " - provider: " << provider->name() << " "
  //                   << (provider->get_transform_type()
  //                           ? (*provider->get_transform_type() == transform_type::same_workload ? "AND" : "OR")
  //                           : "")
  //                   << std::endl;
  //    }
  //    for (const auto &consumer : container->consumer_ptrs) {
  //      a(std::cout) << " - consumer: " << consumer->name() << " "
  //                   << (consumer->get_transform_type()
  //                           ? (*consumer->get_transform_type() == transform_type::same_workload ? "AND" : "OR")
  //                           : "")
  //                   << std::endl;
  //    }
  //  }
  //  for (const auto &line : lines) {
  //    std::string linetstr = ([&]() {
  //      if (line.lt == line_type::normal) return "normal";
  //      if (line.lt == line_type::extra_input) return "extra_input";
  //      if (line.lt == line_type::extra_output) return "extra_output";
  //      return "";
  //    })();
  //    std::cout << "line: " << linetstr << " | " << line.input << " | " << line.storage << " | " << line.output << " "
  //              << line.output_tt << std::endl;
  //  }
}

void stats::display() {
  std::scoped_lock lk(stats_mut);
  a(std::cout) << "--- begin ---" << std::endl;
  for (const auto& stat : stats_) {
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
  // std::this_thread::sleep_for(std::chrono::hours(1));
}
