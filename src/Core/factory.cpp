/**

  @file       factory.cpp
  @brief      
  @details    ~
  @author     Viola Case
  @date       22.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/

#include <ROSE/ROSE.h>
using namespace ROSE;
ROSE::UniquePtr<ROSE::Behavior> MakeCamera() {
  return MakeUnique<Camera>();
}

extern "C" void RoseRegisterModule(ROSE::BehaviorFactory&) {
  //ROSE::Camera::

}