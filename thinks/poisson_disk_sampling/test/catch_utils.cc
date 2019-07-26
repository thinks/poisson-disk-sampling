// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "thinks/poisson_disk_sampling/test/catch_utils.h"

#include <sstream>

namespace thinks {

ExceptionContentMatcher::ExceptionContentMatcher(const std::string& target) : target_(target) {}

bool ExceptionContentMatcher::match(const std::exception& matchee) const {
  return matchee.what() == target_;
}

std::string ExceptionContentMatcher::describe() const {
  std::ostringstream oss;
  oss << "exception message is: '" << target_ << "'";
  return oss.str();
}

}  // namespace thinks
