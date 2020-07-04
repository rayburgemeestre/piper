/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper.h"

#include <iostream>

struct job : public message_type {
  int number = 0;
};

struct final_job : public message_type {
  std::string result;
  final_job(std::string s) : result(std::move(s)) {}
};

int main() {
  auto generate_numbers = producer_function<job>([]() -> std::shared_ptr<job> {
    static int counter = 1, max = 10000;
    if (counter <= max) {
      auto the_job = std::make_shared<job>();
      the_job->number = counter++;
      return the_job;
    }
    return nullptr;
  });
  auto double_number = transform_function<job, job>([](std::shared_ptr<job> the_job) -> std::shared_ptr<job> {
    the_job->number *= 2;
    return the_job;
  });
  auto square_number =
      transform_function<job, final_job>([](std::shared_ptr<job> the_job) -> std::shared_ptr<final_job> {
        the_job->number *= the_job->number;
        // for demo purposes create a new type of job at some point, containing a summary string
        return std::make_shared<final_job>("Final result is: " + std::to_string(the_job->number));
      });
  auto print_number = consume_function<final_job>([](auto job) { a(std::cout) << job->result << std::endl; });

  pipeline_system system;

  auto jobs = system.create_storage("jobs", 5);
  auto jobs_processed = system.create_storage("jobs_processed", 5);
  auto jobs_collected = system.create_storage("jobs_collected", 5);

  // produce 10.000 jobs, containing just a number, and store them in "jobs"
  system.spawn_producer(generate_numbers, jobs);
  // transform jobs from "jobs" into "jobs_processed", multiplying the number by two
  system.spawn_transformer(double_number, jobs, jobs_processed);
  // transform jobs from "jobs_processed" with three workers in parallel (sharing same pool), store in "jobs_collected",
  // squaring the numbers
  system.spawn_transformer(square_number, jobs_processed, jobs_collected, transform_type::same_pool);
  system.spawn_transformer(square_number, jobs_processed, jobs_collected, transform_type::same_pool);
  system.spawn_transformer(square_number, jobs_processed, jobs_collected, transform_type::same_pool);
  // consume the jobs from "jobs_collected" (prints them)
  system.spawn_consumer(print_number, jobs_collected);

  system.start();
}
