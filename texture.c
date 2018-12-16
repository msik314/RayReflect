#include "texture.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "context.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

int32_t textureCreate(Texture* texture, Context* context, void* data, int width, int height)
{
    VkImageCreateInfo textureInfo = {};
    VkMemoryRequirements memoryReqs = {};
    VkMemoryAllocateInfo allocInfo = {};
    VkMemoryType memoryType = {};
    void* mappedImage;
    VkMappedMemoryRange mappedRange = {};
    VkCommandBufferBeginInfo beginInfo = {};
    VkImageMemoryBarrier layoutBarrier = {};
    VkImageSubresourceRange resourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkSubmitInfo submitInfo = {};
    VkImageViewCreateInfo viewInfo = {};
    VkFence fence;
    VkFenceCreateInfo fenceInfo = {};
    VkSamplerCreateInfo samplerInfo = {};
    VkPipelineStageFlags waitStageMask = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    uint32_t memoryTypeBits;
    VkMemoryPropertyFlags desiredFlags = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
    VkResult result;
    
    textureInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.extent.width = width;
    textureInfo.extent.height = height;
    textureInfo.extent.depth = 1;
    textureInfo.mipLevels = 1;
    textureInfo.arrayLayers = 1;
    textureInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.tiling = VK_IMAGE_TILING_LINEAR;
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    
    result = vkCreateImage(context->device, &textureInfo, NULL, &texture->_image);
    if(result != VK_SUCCESS) return -1;
    
    vkGetImageMemoryRequirements(context->device, texture->_image, &memoryReqs);
    
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryReqs.size;
    memoryTypeBits = memoryReqs.memoryTypeBits;
    
    for(int i = 0; i < 32; ++i)
    {
        memoryType = context->physicalDeviceMemory.memoryTypes[i];
        if(memoryTypeBits & 1 && (memoryType.propertyFlags & desiredFlags) == desiredFlags)
        {
            allocInfo.memoryTypeIndex = i;
            break;
        }
        
        memoryTypeBits >>= 1;
    }
    
    result = vkAllocateMemory(context->device, &allocInfo, NULL, &texture->_memory);
    if(result != VK_SUCCESS) return -2;
    
    result = vkBindImageMemory(context->device, texture->_image, texture->_memory, 0);
    if(result != VK_SUCCESS) return -3;
    
    result = vkMapMemory(context->device, texture->_memory, 0, VK_WHOLE_SIZE, 0, &mappedImage);
    if(result != VK_SUCCESS) return -4;
    
    memcpy(mappedImage, data, width * height * 4);
    
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = texture->_memory;
    mappedRange.offset = 0;
    mappedRange.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(context->device, 1, &mappedRange);
    
    vkUnmapMemory(context->device, texture->_memory);
    
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    layoutBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layoutBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    layoutBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    layoutBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    layoutBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    layoutBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutBarrier.image = texture->_image;
    layoutBarrier.subresourceRange = resourceRange;
    
    vkBeginCommandBuffer(context->setupCmdBuffer, &beginInfo);
    vkCmdPipelineBarrier(context->setupCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &layoutBarrier);
    
    vkEndCommandBuffer(context->setupCmdBuffer);
    
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    result = vkCreateFence(context->device, &fenceInfo, NULL, &fence);
    if(result != VK_SUCCESS) return -5;
    
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context->setupCmdBuffer;
    
    result = vkQueueSubmit(context->gfxQueue, 1, &submitInfo, fence);
    if(result != VK_SUCCESS) return -6;
    
    vkWaitForFences(context->device, 1, &fence, VK_TRUE, 0xffffffffffffffff);
    vkResetFences(context->device, 1, &fence);
    vkResetCommandBuffer(context->setupCmdBuffer, 0);
    vkDestroyFence(context->device, fence, NULL);
    
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->_image;
    viewInfo.viewType = VK_IMAGE_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    result = vkCreateImageView(context->device, &viewInfo, NULL, &texture->_view);
    if(result != VK_SUCCESS) return -7;
    
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = 5;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    
    result = vkCreateSampler(context->device, &samplerInfo, NULL, &texture->sampler);
    if(result != VK_SUCCESS) return -8;
    
    return 0;
}

int32_t textureCreateFromFile(Texture* texture, Context* context, const char* file)
{
    int32_t width, height, nc, res;
    uint8_t* data;
    
    data = stbi_load(file, &width, &height, &nc, 4);
    
    res = textureCreate(texture, context, data, width, height);
    
    stbi_image_free(data);
    
    return res;
}

void textureUpdateDescriptor(Texture* texture, Context* context, VkDescriptorSet descriptorSet, uint32_t index, uint32_t baseIndex)
{
    VkDescriptorImageInfo descriptorInfo = {};
    VkWriteDescriptorSet writeDescriptor = {};
    
    descriptorInfo.sampler = texture->sampler;
    descriptorInfo.imageView = texture->_view;
    descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptor.dstSet = descriptorSet;
    writeDescriptor.dstBinding = baseIndex;
    writeDescriptor.dstArrayElement = index;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptor.pImageInfo = &descriptorInfo;
    writeDescriptor.pBufferInfo = NULL;
    writeDescriptor.pTexelBufferView = NULL;

    vkUpdateDescriptorSets(context->device, 1, &writeDescriptor, 0, NULL);
}

void textureDestroy(Texture* texture, Context* context)
{
    if(texture->sampler == VK_NULL_HANDLE) return;
    
    vkDestroySampler(context->device, texture->sampler, NULL);
    vkDestroyImageView(context->device, texture->_view, NULL);
    vkFreeMemory(context->device, texture->_memory, NULL);
    vkDestroyImage(context->device, texture->_image, NULL);
    
    texture->sampler = VK_NULL_HANDLE;
    texture->_view = VK_NULL_HANDLE;
    texture->_memory = VK_NULL_HANDLE;
    texture->_image = VK_NULL_HANDLE;
}
