#include <ROSE/ROSE.h>

#include <iostream>
#include <cstdint>

// HOW TO USE THE DEBUGGER
// 
// 

int main(int argc, char **argv) {
  

  std::cout << ROSE::ByteSwap(static_cast<uint16_t>(3));
  


  return 0;
}