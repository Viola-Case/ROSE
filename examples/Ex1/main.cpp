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

using namespace ROSE;



int main(int argc, char** argv) {

  //std::string g;

  uint8_t a = '1';

  std::printf("The FNV1a hash of 1 is %llx.\n", FNV1A(&a,1));



  std::cout << std::endl;

	return 0;
}