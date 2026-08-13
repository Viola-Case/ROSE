/**

  @file      imgui.h
  @brief     Binds a consumer's copy of Dear ImGui to the one ROSE_Core owns.
  @details   ImGui is a static library, so ROSE_Core.dll and every executable that
             calls ImGui directly each link their own copy, each with its own
             GImGui pointer and its own allocator. Application::Init() creates the
             context inside Core's copy; without this bridge, an ImGui::Begin() in
             game code would run against a null context in the executable's copy.

             AttachImGui() is deliberately inline: it has to be compiled into the
             caller's module so that ImGui::SetCurrentContext() resolves to the
             caller's copy of ImGui. The two getters it calls are exported from
             Core and hand back Core's context and allocator.

             This header is not part of the ROSE.h umbrella because it drags
             <imgui.h> in. Include it only where you call ImGui yourself.
  @author    Viola Case
  @date      10.08.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <imgui.h>
#include <ROSE/Core/api.h>

namespace ROSE {
  //! Core's ImGuiContext*, or nullptr before Application::Init() has run.
  ROSE_API(Core) void *GetImGuiContext() noexcept;

  //! Core's ImGui allocator, so both copies allocate from the same place.
  ROSE_API(Core) void GetImGuiAllocatorFunctions(void **allocFn, void **freeFn, void **userData) noexcept;

  /*!
   * Points this module's ImGui at Core's context. Call once, after
   * Application::Init() and before any ImGui call from outside Core.
   *
   * Executables that run their own ImGui context (the editor, the input tests)
   * must not call this -- they are not sharing Core's UI.
   */
  inline void AttachImGui() noexcept {
    ImGui::SetCurrentContext(static_cast<ImGuiContext *>(GetImGuiContext()));

    void *allocFn = nullptr, *freeFn = nullptr, *userData = nullptr;
    GetImGuiAllocatorFunctions(&allocFn, &freeFn, &userData);
    ImGui::SetAllocatorFunctions(reinterpret_cast<ImGuiMemAllocFunc>(allocFn),
                                 reinterpret_cast<ImGuiMemFreeFunc>(freeFn), userData);
  }
} // namespace ROSE
