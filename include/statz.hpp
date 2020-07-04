/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>

struct statz {
  std::string name;
  bool is_storage;
  bool is_sleeping_until_not_full;
  bool is_sleeping_until_not_empty;
  int size;
  bool active;
};
