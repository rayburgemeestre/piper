/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stats.h"

#include <iostream>
#include <sstream>
#include <thread>

#include "node.h"
#include "queue.h"
#include "util/a.hpp"

void stats::set_type(const std::string& name, bool is_storage) {
  std::scoped_lock sl(stats_mut);
  stats_[name].name = name;
  stats_[name].is_storage = is_storage;
  stats_[name].active = true;
  stats_[name].counter = 0;
  stats_[name].last_counter = 0;
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
void stats::add_counter(const std::string& name) {
  std::scoped_lock sl(stats_mut);
  stats_[name].counter++;
}

/**
 * This will be the only function in the stats class dealing with queues and nodes.
 * When displaying metrics we cannot assume these objects are still running.
 */
void stats::setup(const std::vector<std::shared_ptr<queue>>& containers) {
  for (const auto& container : containers) {
    size_t n = std::max(container->provider_ptrs.size(), container->consumer_ptrs.size());
    for (size_t i = 0; i < n; i++) {
      struct vis v;
      v.storage = "";
      if (i == 0) {
        v.storage = container->name;
        v.input = "** external **";
      }
      if (i < container->provider_ptrs.size()) {
        auto provider = container->provider_ptrs[i];
        v.input = provider->name();
      }
      if (i < container->consumer_ptrs.size()) {
        const auto consumer = container->consumer_ptrs[i];
        v.output = consumer->name();
        const auto tt = (*consumer->get_transform_type() == transform_type::same_workload ? "AND" : "OR");
        v.output_tt = (consumer->get_transform_type() ? tt : "");
      }
      lines.push_back(v);
    }
  }
}

void stats::display() {
  std::scoped_lock lk(stats_mut);
  auto fit_str = [](const std::string& in, size_t max_len) {
    std::stringstream ss;
    std::string s = in.substr(0, max_len);
    if (s.size() < max_len) {
      size_t remain = max_len - s.size();
      size_t left = size_t(remain / 2.0);
      size_t right = remain - left;
      for (size_t i = 0; i < left; i++) {
        ss << " ";
      }
      ss << s;
      for (size_t i = 0; i < right; i++) {
        ss << " ";
      }
    } else {
      ss << s;
    }
    return ss.str();
  };

  bool first = true;
  for (const auto& line : lines) {
    auto inpu = fit_str(line.input, 17);
    auto stor = fit_str(line.storage, 17);
    auto outp = fit_str(line.output, 17);
    auto insl = fit_str("", 17);    // input sleep
    auto ousl = fit_str("", 17);    // output sleep
    auto strq = fit_str("", 15);    // store queue
    auto inpfps = fit_str("", 19);  // input fps
    auto outfps = fit_str("", 19);  // output fps
    auto ifp = fit_str("", 11);     // input fps (small);
    auto ofp = fit_str("", 11);     // output fps (small);
    auto X = fit_str(line.output_tt, 10);

    if (stats_.find(line.input) != stats_.end()) {
      if (stats_[line.input].is_sleeping_until_not_empty) {
        insl = fit_str("[sleeping]", 17);
      }
      if (stats_[line.input].is_sleeping_until_not_full) {
        insl = fit_str("[sleeping]", 17);
      }
      inpfps = fit_str(std::to_string(stats_[line.input].counter - stats_[line.input].last_counter) + " FPS", 19);
      ifp = fit_str(std::to_string(stats_[line.input].counter - stats_[line.input].last_counter) + " FPS", 11);
    }
    if (stats_.find(line.output) != stats_.end()) {
      if (stats_[line.output].is_sleeping_until_not_empty) {
        ousl = fit_str("[sleeping]", 17);
      }
      if (stats_[line.output].is_sleeping_until_not_full) {
        ousl = fit_str("[sleeping]", 17);
      }
      outfps = fit_str(std::to_string(stats_[line.output].counter - stats_[line.output].last_counter) + " FPS", 19);
      ofp = fit_str(std::to_string(stats_[line.output].counter - stats_[line.output].last_counter) + " FPS", 11);
    }
    if (stats_.find(line.storage) != stats_.end()) {
      strq = fit_str("Q:" + std::to_string(stats_[line.storage].size), 15);
    }

    // clang-format off
    if (first) {
      std::cout << "\033[2J\033[1;1H";
      a(std::cout) << R"(      input                        storage                      output       )" << std::endl;
    }
    if (line.input.empty()) {
      a(std::cout) << R"(                                      |                                      )" << std::endl;
      a(std::cout) << R"(                                      |         )"<< X <<"+-----------------+" << std::endl;
      a(std::cout) << R"(                                      |------------------>|)" << outp << R"(|)" << std::endl;
      a(std::cout) << R"(                                      |                   +-----------------+)" << std::endl;
      a(std::cout) << R"(                                      |)" << outfps << R"( )" << ousl << R"( )" << std::endl;
    } else if (line.output.empty()) {
      a(std::cout) << R"(                                      ^                                      )" << std::endl;
      a(std::cout) << R"(+-----------------+                   |                                      )" << std::endl;
      a(std::cout) << R"(|)" << inpu << R"(|-------------------+                                      )" << std::endl;
      a(std::cout) << R"(+-----------------+                   |                                      )" << std::endl;
      a(std::cout) << R"( )" << insl << R"( )" << inpfps << R"(|                                      )" << std::endl;
    } else {
      a(std::cout) << R"(                                                                             )" << std::endl;
      a(std::cout) << R"(+-----------------+          /-----------------\)"<< X <<"+-----------------+" << std::endl;
      a(std::cout) << R"(|)" << inpu << R"(|--------->|)" << stor << R"(|--------->|)" << outp << R"(|)" << std::endl;
      a(std::cout) << R"(+-----------------+          \-----------------/          +-----------------+)" << std::endl;
      a(std::cout) << R"( )" << insl << " " << ifp << " " << strq << " " << ofp << " " << ousl << R"( )" << std::endl;
    }
    // clang-format on
    first = false;
  }
  for (auto& [_, stats] : stats_) {
    stats.last_counter = stats.counter;
  }
}

std::map<std::string, stats::node_stats> stats::get_raw() const {
  std::scoped_lock lk(stats_mut);
  return stats_;
}
