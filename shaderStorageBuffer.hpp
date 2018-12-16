#ifndef SHADER_STORAGE_BUFFER_H
#define SHADER_STORAGE_BUFFER_H

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef struct
{
    VkBuffer buffer;
    VkDeviceMemory _memory;
    uint32_t size;
    uint32_t _totalSize;
}
ShaderStorageBuffer;

static inline int32_t createStagingBuffer(Context* context, uint32_t size, VkBuffer* buffer, VkDeviceMemory* mem)
{
    VkBufferCreateInfo bufferInfo = {};
    VkMemoryRequirements memoryReqs = {};
    VkMemoryAllocateInfo allocInfo = {};
    VkMemoryType memoryType = {};
    uint32_t memoryTypeBits;
    VkMemoryPropertyFlags desiredFlags = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    VkResult result;
    
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    result = vkCreateBuffer(context->device, &bufferInfo, NULL, buffer);
    if(result != VK_SUCCESS) return -1;
    
    vkGetBufferMemoryRequirements(context->device, *buffer, &memoryReqs);
    
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
    
    result = vkAllocateMemory(context->device, &allocInfo, NULL, mem);
    if(result != VK_SUCCESS) return -2;
    
    result = vkBindBufferMemory(context->device, *buffer, *mem, 0);
    if(result != VK_SUCCESS) return -3;
    
    return 0;
}

static inline int32_t shaderStorageBufferCreate(ShaderStorageBuffer* ssb, Context* context, uint32_t size)
{
    VkBufferCreateInfo bufferInfo = {};
    VkMemoryRequirements memoryReqs = {};
    VkMemoryAllocateInfo allocInfo = {};
    VkMemoryType memoryType = {};
    uint32_t memoryTypeBits;
    VkMemoryPropertyFlags desiredFlags = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    VkResult result;
    uint32_t familyIndices[3];
    
    ssb->size = size;
    
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferInfo.queueFamilyIndexCount = getFamilies(context, familyIndices);
    bufferInfo.pQueueFamilyIndices = familyIndices;
    
    result = vkCreateBuffer(context->device, &bufferInfo, NULL, &ssb->buffer);
    if(result != VK_SUCCESS) return -1;
    
    vkGetBufferMemoryRequirements(context->device, ssb->buffer, &memoryReqs);
    
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
    
    ssb->_totalSize = allocInfo.allocationSize;
    
    result = vkAllocateMemory(context->device, &allocInfo, NULL, &ssb->_memory);
    if(result != VK_SUCCESS) return -2;
    
    result = vkBindBufferMemory(context->device, ssb->buffer, ssb->_memory, 0);
    if(result != VK_SUCCESS) return -3;
    
    return 0;
}

static inline int32_t shaderStorageBufferWrite(ShaderStorageBuffer* ssb, Context* context, void* data, uint32_t size)
{
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    VkFenceCreateInfo fenceInfo = {};
    VkBuffer temp;
    VkDeviceMemory mem;
    VkFence fence;
    VkBufferCopy copy;
    VkSubmitInfo submitInfo = {};
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    void* mapped;
    VkResult vkRes;
    int32_t res;
    
    res = createStagingBuffer(context, size, &temp, &mem);
    
    if(res == -3)
    {
        vkFreeMemory(context->device, mem, NULL);
    }
    
    if(res < 0)
    {
        vkDestroyBuffer(context->device, temp, NULL);
        return -1;
    }
    
    vkRes = vkMapMemory(context->device, mem, 0, VK_WHOLE_SIZE, 0, &mapped);
    if(vkRes != VK_SUCCESS) return -2;
    
    memcpy(mapped, data, size);
    
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkRes = vkCreateFence(context->device, &fenceInfo, NULL, &fence);
    if(vkRes != VK_SUCCESS)
    {
        vkFreeMemory(context->device, mem, NULL);
        vkDestroyBuffer(context->device, temp, NULL);
        return -3;
    }
    
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    
    vkBeginCommandBuffer(context->transferCmdBuffer, &cmdBeginInfo);
    
    vkCmdCopyBuffer(context->transferCmdBuffer, temp, ssb->buffer, 1, &copy);
    
    vkEndCommandBuffer(context->transferCmdBuffer);
    
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context->transferCmdBuffer;
    
    vkRes = vkQueueSubmit(context->tfrQueue, 1, &submitInfo, fence);
    
    vkWaitForFences(context->device, 1, &fence, VK_TRUE, 0xffffffffffffffff);
    vkResetCommandBuffer(context->transferCmdBuffer, 0);
    
    vkDestroyFence(context->device, fence, NULL);
    
    vkFreeMemory(context->device, mem, NULL);
    vkDestroyBuffer(context->device, temp, NULL);    
    return 0;
}

static inline void shaderStorageBufferDestroy(ShaderStorageBuffer* ssb, Context* context)
{
    vkFreeMemory(context->device, ssb->_memory, NULL);
    vkDestroyBuffer(context->device, ssb->buffer, NULL);
}

#endif //SHADER_STORAGE_BUFFER_H
