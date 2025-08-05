#pragma once

#include "vulkan/vulkan.h"
#include "backends/imgui_impl_vulkan.h"

#include <string>
#include <iostream>

namespace vkb {
	// from Vulkan Samples open source
	// see https://github.com/KhronosGroup/Vulkan-Samples/blob/main/framework/common/strings.cpp
	const std::string to_string(VkResult result);
}

namespace Cubed
{
	ImGui_ImplVulkan_InitInfo* GetVulkanInfo();
}

// see  - https://github.com/KhronosGroup/Vulkan-Samples/blob/main/framework/common/error.h
#define VK_CHECK(x)                                                                    \
	do                                                                                 \
	{                                                                                  \
		VkResult err = x;                                                              \
		if (err)                                                                       \
		{                                                                              \
			std::cout << "Detected Vulkan error: " + vkb::to_string(err);			   \
		}                                                                              \
	} while (0)


