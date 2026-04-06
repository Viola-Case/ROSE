#include <ROSE/ROSE.h>
#if !defined(ROSE_EDITOR)
#error ROSE EDITOR MUST BE BUILT WITH EDITOR CONFIG
#endif



namespace ROSE::Editor {


  class EditorApplication {
    
    void Run();
    void CleanUp();

    void GetWindow();

  };
}