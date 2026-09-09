/**

    @file      vulkanbackend.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      08.09.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace ROSE {

  struct VulkanIMPL {
    vk::ApplicationInfo vkAppInfo;
    List<String>        vkExtensions {};
  };

  VulkanRenderer::VulkanRenderer() : m_impl(new VulkanIMPL()) {}
  VulkanRenderer::~VulkanRenderer() { delete m_impl; }



  BackendStatus VulkanRenderer::Init(const RenderBackendContext &ctx) {
    auto &appInfo = m_impl->vkAppInfo;
    auto &extensions = m_impl->vkExtensions;
    appInfo.setApiVersion(VK_API_VERSION_1_3)
      .setApplicationVersion(VK_MAKE_API_VERSION(0, ROSE_VERSIONNUM_MAJOR(ctx.appVersion),
                                                 ROSE_VERSIONNUM_MINOR(ctx.appVersion),
                                                 ROSE_VERSIONNUM_PATCH(ctx.appVersion)))
      .setPApplicationName(ctx.appName)
      .setPEngineName("ROSE")
      .setEngineVersion(VK_MAKE_API_VERSION(0, ROSE_VERSION_MAJOR, ROSE_VERSION_MINOR, ROSE_VERSION_PATCH));


    uint32_t           extensionCount = 0;
    const char *const *exts = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    m_impl->vkExtensions.resize(extensionCount);
    for (int i = 0; i < extensionCount; i++) {
      ROSE_LOG_DEBUG("Vulkan extension {}: {}\n", i, exts[i]);
      m_impl->vkExtensions[i] = exts[i];
    }

    // something with extensions yadda yadda

    List<const char *> FINAL_EXTENSION_LIST_THIS_REQUIRES_A_VERY_SPECIFIC_MAKEUP{};
    for (const auto &s : extensions) {
      FINAL_EXTENSION_LIST_THIS_REQUIRES_A_VERY_SPECIFIC_MAKEUP.push_back(s.c_str());
    }

    auto createInfo = vk::InstanceCreateInfo{}
    .setEnabledExtensionCount(extensions.size())
    .setPpEnabledExtensionNames(FINAL_EXTENSION_LIST_THIS_REQUIRES_A_VERY_SPECIFIC_MAKEUP.data())
    .setPApplicationInfo(&appInfo);

    return BackendStatus::IHaveNoIdea;
  }

  void VulkanRenderer::Shutdown() {}
  void VulkanRenderer::BeginFrame() {}
  void VulkanRenderer::EndFrame() {}

  void VulkanRenderer::OnResize(int width, int height) {}
  void VulkanRenderer::Draw(const DrawCommand &) {}
} // namespace ROSE