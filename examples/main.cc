// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "json_example.h"
#include "simple_example.h"
#include "vec_traits_in_namespace_example.h"
#include "vec_traits_passed_in_example.h"

int main(int, char*[]) {
  examples::JsonExample("./json_example.json");
  examples::SimpleExample("./simple_example.txt");
  examples::VecTraitsInNamespaceExample(
      "./vec_traits_in_namespace_example.txt");
  // thinks::VecTraitsPassedInExample("./vec_traits_passed_in_example.txt");

  return 0;
}
