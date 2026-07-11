/**

  @file      gfx.h
  @brief
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#pragma once


namespace ROSE {
  enum RenderBackend : unsigned char {
    Software,
    OpenGL,
    Vulkan,
    // DirectX,
    // Metal
  };

  void InitializeRenderBackend(RenderBackend backend);



} // namespace ROSE