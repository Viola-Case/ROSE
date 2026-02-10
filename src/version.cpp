#include <ROSE/ROSE.h>


/**
    @todo need to make this work outside the stack
**/
//const char *Rose_Versionnum_Str(unsigned int version) {
//
//}

namespace ROSE {
  unsigned int GetVersion() { return ROSE_VERSION; }
}