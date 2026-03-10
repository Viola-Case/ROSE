/**

  @file      Example 1 - main.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      6.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#define ROSE_MAIN_HANDLED

#include <ROSE/ROSE.h>
#include <iostream>

#include <string>

#include <Windows.h>

using namespace ROSE;



int main(int argc, char** argv) {

  //std::string g;

  List<int> integers{1,2,3};

  std::printf("integers is at %p.\n", reinterpret_cast<int>(integers.data()));

  List<int> newList(Move(integers));

  std::printf("integers is at %p, newList is at %p.", reinterpret_cast<int>(integers.data()), reinterpret_cast<int>(newList.data()));

  //for (auto &i : integers) {
  //  std::cout << i << '\t';
  //}
  std::cout << std::endl;

	return 0;
}