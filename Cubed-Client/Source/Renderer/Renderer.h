#pragma once

#include "Vulkan.h"
#include <fstream>

namespace Cubed {

	class Renderer
	{
	public:
		void Init();
		void Shutdown();

		void Render();
	private:
		VkShaderModule LoadShader(const std::filesystem::path& path); // stage indicates what shader, and path is shader file
		void InitPipeline();

		VkPipeline m_GraphicsPipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;
	};

}