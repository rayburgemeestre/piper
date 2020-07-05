/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class queue;

class stats {
private:
  std::mutex stats_mut;
  struct node_stats {
    std::string name;
    bool is_storage;
    bool is_sleeping_until_not_full;
    bool is_sleeping_until_not_empty;
    int size;
    bool active;
    size_t counter;
    size_t last_counter;
  };
  std::map<std::string, node_stats> stats_;
  struct vis {
    std::string input;
    std::string storage;
    std::string output;
    std::string output_tt;
  };
  std::vector<vis> lines;

public:
  void set_type(const std::string& name, bool is_storage);
  void set_sleep_until_not_full(const std::string& name, bool val);
  void set_sleep_until_not_empty(const std::string& name, bool val);
  void set_size(const std::string& name, int size);
  void set_active(const std::string& name, bool active);
  void add_counter(const std::string& name);
  void setup(const std::vector<std::shared_ptr<queue>>& containers);
  void display();
};
