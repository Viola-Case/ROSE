#include <ROSE/ROSE.h>
#if !defined(ROSE_EDITOR)
#error ROSE EDITOR MUST BE BUILT WITH EDITOR CONFIG
#endif

#include <ROSE/Editor/ROSE_metadata.h>

namespace ROSE::Editor {


  class EditorApplication {
    
    void Run();
    void CleanUp();

    void GetWindow();

  };
}