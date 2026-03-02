#include <ROSE/ROSE.h>

#include <iostream>
#include <cstdint>

// HOW TO USE THE DEBUGGER
// 
// 

int ROSE_main(int argc, char **argv) {
  // let's define an array or smth
  char buf[256];
  // let's say we want to view the contents of the array without printing them
  // sometimes we won't have access to a console
  // or maybe the output stream would get interrupted
  // or maybe multithreading terribleness

  // regardless, we want to be able to pause the code and say 'hey how do I 
  // look at the contents of this array'

  for (int i = 0; i < 256; ++i) { // kinda unsafe but not important
    *(buf + i) = i; // pointer arithmetic because i'm weird, ignore this strange syntax
  }

  // I want to look at the contents of buf without printing it right here.

  // we place a BREAKPOINT
  // <-

  // And then we run through the WINDOWS DEBUGGER
  // so im going to build and then run through the debugger with [F5]

  // And that's how the debugger works in a nutshell

  // It's really handy for large data types or if you're just trying to look at a whole lot at once
  // Or maybe you need to look at both an address and the contents of said address
  // Computers are kinda funky like that

  // hope that helps

  system("pause");

  return 0;
}