#include "Texture.h"

#include "Renderer.h"
#include "Walnut/Application.h"

namespace Cubed {

	Texture::Texture(uint32_t width, uint32_t height, Walnut::Buffer data)
        : m_Width(width), m_Height(height)
    {
        Init(data);
	}

	Texture::~Texture()
	{
        VkDevice device = GetVulkanInfo()->Device;
        // destroy members
        vkDestroySampler(device, m_Sampler, nullptr);
        vkDestroyImageView(device, m_ImageView, nullptr);
        vkDestroyImage(device, m_Image, nullptr);
        vkFreeMemory(device, m_Memory, nullptr);
	}

    // heavily recycled from IMGUI's implementation of Vulkan
    void Texture::Init(Walnut::Buffer data)
	{
        VkDevice device = GetVulkanInfo()->Device;
        size_t size = m_Width * m_Height * 4; // 4 bytes per pixel - based on format
        if (size != data.Size) // they need to be the same size...
        {
            __debugbreak();
            return;
        }

        // Create the Image:
        {
            VkImageCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = {.width = m_Width, .height = m_Height, .depth = 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // image is transferred with gpu transfer queue here
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            };

            VK_CHECK(vkCreateImage(device, &info, nullptr, &m_Image));

            // allocate memory for image and bind memory to image
            VkMemoryRequirements req;
            vkGetImageMemoryRequirements(device, m_Image, &req);
            VkMemoryAllocateInfo alloc_info {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = req.size,
                .memoryTypeIndex = Renderer::GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits)
            };

            VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &m_Memory));
            VK_CHECK(vkBindImageMemory(device, m_Image, m_Memory, 0));
        }

        // Create the Image View:
        {
            VkImageViewCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_Image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
            };
            VK_CHECK(vkCreateImageView(device, &info, nullptr, &m_ImageView));
        }

        // Create the Upload Buffer:
        VkBuffer stagingBuffer = nullptr;
        VkDeviceMemory stagingBufferMemory = nullptr;
        {
            VkBufferCreateInfo buffer_info {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = size,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // image is transferred with gpu transfer queue from here
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE
            };
            VK_CHECK(vkCreateBuffer(device, &buffer_info, nullptr, &stagingBuffer));

            VkMemoryRequirements req;
            vkGetBufferMemoryRequirements(device, stagingBuffer, &req);
            VkMemoryAllocateInfo alloc_info {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = req.size,
                .memoryTypeIndex = Renderer::GetVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits)
            };
            
            VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &stagingBufferMemory));
            VK_CHECK(vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0));
            
        }

        // Upload to Buffer:
        {
            uint8_t* map = NULL;
            VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, size, 0, (void**)(&map)));
            memcpy(map, data.Data, size);
            VkMappedMemoryRange range{
                .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                .memory = stagingBufferMemory,
                .size = size
            };
            VK_CHECK(vkFlushMappedMemoryRanges(device, 1, &range));
            vkUnmapMemory(device, stagingBufferMemory);
        }

        // use a command buffer from Walnut's pool of command buffers
        VkCommandBuffer commandBuffer = Walnut::Application::GetCommandBuffer(true);

        // Copy to Image:
        {
            VkImageMemoryBarrier copy_barrier[1] = {};
            copy_barrier[0] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = m_Image,
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
            };
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

            VkBufferImageCopy region {
                .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
                .imageExtent = { .width = m_Width, .height = m_Height, .depth = 1 }
            };

            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier use_barrier[1] = {};
            use_barrier[0] =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = m_Image,
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
            };
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
        }

        Walnut::Application::FlushCommandBuffer(commandBuffer);

        // destroy data created within this function
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        // create sampler
        VkSamplerCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .maxAnisotropy = 1.0f,
            .minLod = -1000,
            .maxLod = 1000
        };

        VK_CHECK(vkCreateSampler(device, &info, nullptr, &m_Sampler));

        m_ImageInfo = {
            .sampler = m_Sampler,
            .imageView = m_ImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
	}

}