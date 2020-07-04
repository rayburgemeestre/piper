/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "piper.h"

#include <iostream>
#include <sstream>

struct job : public message_type {
  int input = 0;
  int doubled = 0;
  int squared = 0;
};

struct final_job : public message_type {
  std::string msg;
};

int main() {
  auto generate_ten_numbers = producer_function<job>([]() -> std::shared_ptr<job> {
    static int counter = 1, max = 1000;
    if (counter <= max) {
      auto the_job = std::make_shared<job>();
      the_job->input = counter++;
      return the_job;
    }
    return nullptr;
  });
  auto double_number = transform_function<job, job>([](std::shared_ptr<job> the_job) -> std::shared_ptr<job> {
    the_job->doubled = 2 * the_job->input;
    return the_job;
  });
  auto square_number =
      transform_function<job, final_job>([](std::shared_ptr<job> the_job) -> std::shared_ptr<final_job> {
        // if workers are working in same pool, do not modify job ?
        the_job->squared = the_job->input * the_job->input;
        std::stringstream ss;
        ss << "FINAL JOB RESULT: " << the_job->doubled << " * " << the_job->doubled << " ==== " << the_job->squared;
        auto ret_job = std::make_shared<final_job>();
        ret_job->msg = ss.str();
        return ret_job;
      });
  auto print_number = consume_function<final_job>([](auto job) { a(std::cout) << job->msg << std::endl; });

  pipeline_system system;
  storage_container jobs("jobs", system, 5);
  storage_container jobs_processed("jobs_processed", system, 5);
  storage_container jobs_collected("jobs_collected", system, 5);
  system.spawn_producer(generate_ten_numbers, &jobs);
  system.spawn_transformer(double_number, &jobs, &jobs_processed);
  system.spawn_transformer("worker 1", square_number, &jobs_processed, &jobs_collected, transform_type::same_pool);
  system.spawn_transformer("worker 2", square_number, &jobs_processed, &jobs_collected, transform_type::same_pool);
  system.spawn_transformer("worker 3", square_number, &jobs_processed, &jobs_collected, transform_type::same_pool);
  system.spawn_consumer("consumer", print_number, &jobs_collected);
  system.start();
}
