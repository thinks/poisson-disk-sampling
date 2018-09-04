// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <json_example.h>
#include <simple_example.h>
#include <vec_example.h>


int main(int argc, char* argv[])
{
  examples::JsonExample("./json_example.json");
  examples::SimpleExample("./simple_example.txt");
  examples::VecExample("./vec_example");

  return 0;
}
