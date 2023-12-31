/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <sys/prctl.h>

void set_thread_name(const std::string& thread_name) {
  prctl(PR_SET_NAME, thread_name.c_str(), NULL, NULL, NULL);
}
