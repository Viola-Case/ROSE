/**

  @file      Example 1 - main.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      6.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/


#include <ROSE/ROSE.h>
#include <iostream>

#include <Windows.h>

using namespace ROSE;



int ROSE_main(int argc, char** argv) {

  List<int> integers{1,2,3};

  for (auto &i : integers) {
    std::cout << i << '\t';
  }
  std::cout << std::endl;

	return 0;
}