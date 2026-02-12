/**

  @file      Example 1 - main.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      6.02.2026
  @copyright © Cool Guy, 2026. All right reserved.

**/


#include <ROSE/ROSE.h>
#include <iostream>

#include <Windows.h>

using namespace ROSE;



int ROSE_main(int argc, char** argv) {
  std::cout << "ROSE version " << ROSE::GetVersion() << std::endl;
	return 0;
}