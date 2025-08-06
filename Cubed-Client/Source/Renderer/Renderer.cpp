#include "Renderer.h"

#include "Walnut/Application.h"
#include "Walnut/Core/Log.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

#include <array>

namespace Cubed {

	// ripped from imgui implementation of vulkan
	static uint32_t ImGui_ImplVulkan_MemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
	{
		VkPhysicalDevice physicalDevice = GetVulkanInfo()->PhysicalDevice;

		VkPhysicalDeviceMemoryProperties prop;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &prop);
		for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
			if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
				return i;
		return 0xFFFFFFFF; // Unable to find memoryType
	}

	void Renderer::Init()
	{
		InitBuffers();
		InitPipeline();
	}

	void Renderer::Shutdown()
	{

	}

	void Renderer::BeginScene(const Camera& camera)
	{
		auto wd = Walnut::Application::GetMainWindowData();
		float viewportHeight = (float)wd->Height;
		float viewportWidth = (float)wd->Width;

		VkCommandBuffer commandBuffer = Walnut::Application::GetActiveCommandBuffer();

		// set up camera transform
		glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f), camera.Position)
			* glm::eulerAngleXYZ(glm::radians(camera.Rotation.x), glm::radians(camera.Rotation.y), glm::radians(camera.Rotation.z));

		m_PushConstants.ViewProjection = glm::perspectiveFov(glm::radians(45.0f), viewportWidth, viewportHeight, 0.1f, 1000.0f)
			* glm::inverse(cameraTransform);

		// set viewport for drawing later
		// y and height are wonky because we flip the viewport to make it look "normal"
		VkViewport vp{
			.y = viewportHeight,
			.width = viewportWidth,
			.height = -viewportHeight,
			.minDepth = 0.0f,
			.maxDepth = 1.0f };
		// Set viewport dynamically
		vkCmdSetViewport(commandBuffer, 0, 1, &vp);

		VkRect2D scissor{
			.extent = {.width = (uint32_t)wd->Width, .height = (uint32_t)wd->Height} };
		// Set scissor dynamically
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::EndScene(const Camera& camera)
	{

	}

	// same for camera but uses position instead of m_CubePosition
	void Renderer::RenderCube(const glm::vec3& position, const glm::vec3& rotation)
	{
		// apply cube position and rotation transforms
		m_PushConstants.Transform = glm::translate(glm::mat4(1.0f), position)
			* glm::eulerAngleXYZ(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z));

		VkCommandBuffer commandBuffer = Walnut::Application::GetActiveCommandBuffer();

		// Bind the graphics pipeline (no render pass in our renderer, so keep here)
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		// use push constants to move cube
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &m_PushConstants);

		// bind vertex buffer and index buffer
		VkDeviceSize offset{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer.Handle, &offset);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.Handle, offset, VK_INDEX_TYPE_UINT32);

		// Draw three vertices with one instance from the currently bound vertex bound.
		vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
	}

	void Renderer::RenderUI()
	{

	}


	void Renderer::InitBuffers()
	{
		VkDevice device = GetVulkanInfo()->Device;

		// create data to store in buffers
		std::array<Vertex, 24> vertexData;
		// front
		vertexData[0]  = { .Position = glm::vec3(-0.5f, -0.5f,  0.5f), .Normal = glm::vec3( 0.0f,  0.0f,  1.0f) };
		vertexData[1]  = { .Position = glm::vec3(-0.5f,  0.5f,  0.5f), .Normal = glm::vec3( 0.0f,  0.0f,  1.0f) };
		vertexData[2]  = { .Position = glm::vec3( 0.5f,  0.5f,  0.5f), .Normal = glm::vec3( 0.0f,  0.0f,  1.0f) };
		vertexData[3]  = { .Position = glm::vec3( 0.5f, -0.5f,  0.5f), .Normal = glm::vec3( 0.0f,  0.0f,  1.0f) };

		// right
		vertexData[4]  = { .Position = glm::vec3( 0.5f, -0.5f,  0.5f), .Normal = glm::vec3( 1.0f,  0.0f,  0.0f) };
		vertexData[5]  = { .Position = glm::vec3( 0.5f,  0.5f,  0.5f), .Normal = glm::vec3( 1.0f,  0.0f,  0.0f) };
		vertexData[6]  = { .Position = glm::vec3( 0.5f,  0.5f, -0.5f), .Normal = glm::vec3( 1.0f,  0.0f,  0.0f) };
		vertexData[7]  = { .Position = glm::vec3( 0.5f, -0.5f, -0.5f), .Normal = glm::vec3( 1.0f,  0.0f,  0.0f) };

		// back
		vertexData[8]  = { .Position = glm::vec3( 0.5f, -0.5f, -0.5f), .Normal = glm::vec3( 0.0f,  0.0f, -1.0f) };
		vertexData[9]  = { .Position = glm::vec3( 0.5f,  0.5f, -0.5f), .Normal = glm::vec3( 0.0f,  0.0f, -1.0f) };
		vertexData[10] = { .Position = glm::vec3(-0.5f,  0.5f, -0.5f), .Normal = glm::vec3( 0.0f,  0.0f, -1.0f) };
		vertexData[11] = { .Position = glm::vec3(-0.5f, -0.5f, -0.5f), .Normal = glm::vec3( 0.0f,  0.0f, -1.0f) };

		// left
		vertexData[12] = { .Position = glm::vec3(-0.5f, -0.5f, -0.5f), .Normal = glm::vec3(-1.0f,  0.0f,  0.0f) };
		vertexData[13] = { .Position = glm::vec3(-0.5f,  0.5f, -0.5f), .Normal = glm::vec3(-1.0f,  0.0f,  0.0f) };
		vertexData[14] = { .Position = glm::vec3(-0.5f,  0.5f,  0.5f), .Normal = glm::vec3(-1.0f,  0.0f,  0.0f) };
		vertexData[15] = { .Position = glm::vec3(-0.5f, -0.5f,  0.5f), .Normal = glm::vec3(-1.0f,  0.0f,  0.0f) };

		// top
		vertexData[16] = { .Position = glm::vec3(-0.5f,  0.5f,  0.5f), .Normal = glm::vec3( 0.0f,  1.0f,  0.0f) };
		vertexData[17] = { .Position = glm::vec3(-0.5f,  0.5f, -0.5f), .Normal = glm::vec3( 0.0f,  1.0f,  0.0f) };
		vertexData[18] = { .Position = glm::vec3( 0.5f,  0.5f, -0.5f), .Normal = glm::vec3( 0.0f,  1.0f,  0.0f) };
		vertexData[19] = { .Position = glm::vec3( 0.5f,  0.5f,  0.5f), .Normal = glm::vec3( 0.0f,  1.0f,  0.0f) };

		// top
		vertexData[20] = { .Position = glm::vec3(-0.5f, -0.5f, -0.5f), .Normal = glm::vec3( 0.0f, -1.0f,  0.0f) };
		vertexData[21] = { .Position = glm::vec3(-0.5f, -0.5f,  0.5f), .Normal = glm::vec3( 0.0f, -1.0f,  0.0f) };
		vertexData[22] = { .Position = glm::vec3( 0.5f, -0.5f,  0.5f), .Normal = glm::vec3( 0.0f, -1.0f,  0.0f) };
		vertexData[23] = { .Position = glm::vec3( 0.5f, -0.5f, -0.5f), .Normal = glm::vec3( 0.0f, -1.0f,  0.0f) };

		// create index array based on pattern
		std::array<uint32_t, 36> indicies;
		uint32_t offset = 0;
		for (int i = 0; i < 36; i += 6)
		{
			indicies[i + 0] = 0 + offset;
			indicies[i + 1] = 1 + offset;
			indicies[i + 2] = 2 + offset;
			indicies[i + 3] = 2 + offset;
			indicies[i + 4] = 3 + offset;
			indicies[i + 5] = 0 + offset;

			offset += 4;
		}

		// create buffers on cpu
		m_VertexBuffer.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		CreateOrResizeBuffer(m_VertexBuffer, vertexData.size() * sizeof(Vertex));

		m_IndexBuffer.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		CreateOrResizeBuffer(m_IndexBuffer, indicies.size() * sizeof(uint32_t));

		// create buffers on gpu and copy data to them
		// - vertex buffer
		glm::vec3* vbMemory = nullptr;
		VK_CHECK(vkMapMemory(device, m_VertexBuffer.Memory, 0, vertexData.size() * sizeof(Vertex), 0, (void**)&vbMemory));
		memcpy(vbMemory, vertexData.data(), vertexData.size() * sizeof(Vertex));

		// - index buffer
		uint32_t* ibMemory = nullptr;
		VK_CHECK(vkMapMemory(device, m_IndexBuffer.Memory, 0, indicies.size() * sizeof(uint32_t), 0, (void**)&ibMemory));
		memcpy(ibMemory, indicies.data(), indicies.size() * sizeof(uint32_t));

		// once the GPU gets the buffers, clear them on CPU side (flush then unmap)
		VkMappedMemoryRange range[2] = {};
		range[0] =
		{
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.memory = m_VertexBuffer.Memory,
			.size = VK_WHOLE_SIZE
		};
		range[1] =
		{
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.memory = m_IndexBuffer.Memory,
			.size = VK_WHOLE_SIZE
		};
		
		VK_CHECK(vkFlushMappedMemoryRanges(device, 2, range));
		vkUnmapMemory(device, m_VertexBuffer.Memory);
		vkUnmapMemory(device, m_IndexBuffer.Memory);

	}

	VkShaderModule Renderer::LoadShader(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream) // if not good or is not open
		{
			WL_ERROR("Could not open file! {}", path.string());
			return nullptr;
		}

		// determine length of file, then return to beginning
		stream.seekg(0, std::ios_base::end);
		std::streampos size = stream.tellg();
		stream.seekg(0, std::ios_base::beg);

		// size is in bytes so use char
		std::vector<char> buffer(size);

		// copy file to buffer
		if (!stream.read(buffer.data(), size))
		{
			WL_ERROR("Could not read file! {}", path.string());
			return nullptr;
		}
		 
		stream.close();

		VkShaderModuleCreateInfo shaderModuleCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shaderModuleCI.pCode = (uint32_t*)buffer.data();
		shaderModuleCI.codeSize = buffer.size();

		// create shader module and check if it worked
		VkDevice device = GetVulkanInfo()->Device;
		VkShaderModule result = nullptr;
		VK_CHECK(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &result));
		return result;
	}


	// based on Hello Triangle in Vulkan Samples - https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/api/hello_triangle/hello_triangle.cpp
	void Renderer::InitPipeline()
	{

		// get device from backend info in vulkan
		VkDevice device = GetVulkanInfo()->Device;
		VkRenderPass renderPass = Walnut::Application::GetMainWindowData()->RenderPass;

		// create pipeline with push constant information for viewprojection and transform matricies
		std::array<VkPushConstantRange, 1> pushConstantRanges;
		pushConstantRanges[0] = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(glm::mat4) * 2
		};

		VkPipelineLayoutCreateInfo layout_info{
			.flags = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pushConstantRangeCount = (uint32_t)pushConstantRanges.size(),
			.pPushConstantRanges = pushConstantRanges.data()
		};

		VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &m_PipelineLayout));

		// The Vertex input properties define the interface between the vertex buffer and the vertex shader.

		// Specify we will use triangle lists to draw geometry.
		VkPipelineInputAssemblyStateCreateInfo input_assembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		};

		// pulled from IMGUI's implementation of Vulkan - vertex input setup
		std::array<VkVertexInputBindingDescription, 1> binding_desc;
		binding_desc[0] = {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		// assign attributes to send to vertex shader
		std::array<VkVertexInputAttributeDescription, 2> attribute_desc;
		// vertex position
		attribute_desc[0] = {
			.location = 0,
			.binding = binding_desc[0].binding,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, Position)
		};
		// vertex normal
		attribute_desc[1] = {
			.location = 1,
			.binding = binding_desc[0].binding,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, Normal)
		};


		VkPipelineVertexInputStateCreateInfo vertex_input{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = (uint32_t)binding_desc.size(),
			.pVertexBindingDescriptions = binding_desc.data(),
			.vertexAttributeDescriptionCount = (uint32_t)attribute_desc.size(),
			.pVertexAttributeDescriptions = attribute_desc.data()
		};

		// Specify rasterization state.
		VkPipelineRasterizationStateCreateInfo raster{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.lineWidth = 1.0f };

		// Our attachment will write to all color channels, but no blending is enabled.
		VkPipelineColorBlendAttachmentState blend_attachment{
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };

		VkPipelineColorBlendStateCreateInfo blend{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blend_attachment };

		// We will have one viewport and scissor box.
		VkPipelineViewportStateCreateInfo viewport{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1 };

		// Disable all depth testing.
		VkPipelineDepthStencilStateCreateInfo depth_stencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		// No multisampling.
		VkPipelineMultisampleStateCreateInfo multisample{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };

		// Specify that these states will be dynamic, i.e. not part of pipeline state object.
		std::array<VkDynamicState, 2> dynamics{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamic{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamics.size()),
			.pDynamicStates = dynamics.data() };

		std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

		// Vertex stage of the pipeline
		shader_stages[0] = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = LoadShader("Assets/Shaders/bin/basic.vert.spirv"),
			.pName = "main" };

		// Fragment stage of the pipeline
		shader_stages[1] = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = LoadShader("Assets/Shaders/bin/basic.frag.spirv"),
			.pName = "main" };

		VkGraphicsPipelineCreateInfo pipe {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shader_stages.size()),
			.pStages = shader_stages.data(),
			.pVertexInputState = &vertex_input,
			.pInputAssemblyState = &input_assembly,
			.pViewportState = &viewport,
			.pRasterizationState = &raster,
			.pMultisampleState = &multisample,
			.pDepthStencilState = &depth_stencil,
			.pColorBlendState = &blend,
			.pDynamicState = &dynamic,
			.layout = m_PipelineLayout,        // We need to specify the pipeline layout up front
			.renderPass = renderPass             // We need to specify the render pass up front
		};

		VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, nullptr, &m_GraphicsPipeline));

		// Pipeline is baked, we can delete the shader modules now.
		vkDestroyShaderModule(device, shader_stages[0].module, nullptr);
		vkDestroyShaderModule(device, shader_stages[1].module, nullptr);
	}

	void Renderer::CreateOrResizeBuffer(Buffer& buffer, uint64_t newSize)
	{
		VkDevice device = GetVulkanInfo()->Device;

		if (buffer.Handle != VK_NULL_HANDLE)
			vkDestroyBuffer(device, buffer.Handle, nullptr);
		if (buffer.Memory != VK_NULL_HANDLE)
			vkFreeMemory(device, buffer.Memory, nullptr);

		// create buffer
		VkBufferCreateInfo bufferCI { 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = newSize,
			.usage = (VkBufferUsageFlags)buffer.Usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE

		};

		VK_CHECK(vkCreateBuffer(device, &bufferCI, nullptr, &buffer.Handle));

		//
		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(device, buffer.Handle, &req);
		//bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;
		VkMemoryAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = req.size,
			.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits)
		};
		
		VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &buffer.Memory));

		VK_CHECK(vkBindBufferMemory(device, buffer.Handle, buffer.Memory, 0));
		buffer.Size = req.size;
	}

}