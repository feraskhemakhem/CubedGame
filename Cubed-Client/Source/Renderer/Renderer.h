#pragma once

#include "Vulkan.h"

#include "glm/glm.hpp"

#include <fstream>

namespace Cubed {

	struct Buffer
	{
		VkBuffer Handle = nullptr;
		VkDeviceMemory Memory = nullptr;
		VkDeviceSize Size = 0;
		VkBufferUsageFlagBits Usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	};

	struct Camera
	{
		glm::vec3 Position{ 0, 0, 8 };
		glm::vec3 Rotation{ 0, 0, 0 }; // in degrees
	};

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
	};

	class Renderer
	{
	public:
		void Init();
		void Shutdown();

		void BeginScene(const Camera& camera);
		void EndScene(const Camera& camera);

		void Render();
		void RenderCube(const glm::vec3& position, const glm::vec3& rotation);
		void RenderUI();
	private:
		void InitPipeline();
		void InitBuffers();
		void CreateOrResizeBuffer(Buffer& buffer, uint64_t newSize);

		VkShaderModule LoadShader(const std::filesystem::path& path); // stage indicates what shader, and path is shader file

	private:
		// graphics pipeline
		VkPipeline m_GraphicsPipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;

		// buffers read through pipeline
		Buffer m_VertexBuffer, m_IndexBuffer;

		// push constant variables passed to vert shader
		struct PushConstants
		{
			glm::mat4 ViewProjection;
			glm::mat4 Transform;
		} m_PushConstants;
	};

}