#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "context.h"

typedef struct
{
    glm::mat4 model;
    glm::mat4 vp;
    glm::vec3 camPos;
    uint32_t texture;
}
StandardUniforms;

typedef struct
{
    VkBuffer buffer;
    VkDeviceMemory _memory;
    uint32_t _dataSize;
    uint32_t _totalSize;
}
UniformBuffer;

template <typename T>
static inline int32_t uniformBufferCreate(UniformBuffer* uniforms, Context* context, uint32_t numUniforms)
{
    VkBufferCreateInfo bufferInfo = {};
    VkMemoryRequirements memoryReqs = {};
    VkMemoryAllocateInfo allocInfo = {};
    VkMemoryType memoryType = {};
    uint32_t memoryTypeBits;
    VkMemoryPropertyFlags desiredFlags = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
    VkResult result;
    
    uniforms->_dataSize = numUniforms * sizeof(T);
    
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = numUniforms * sizeof(T);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    result = vkCreateBuffer(context->device, &bufferInfo, NULL, &uniforms->buffer);
    if(result != VK_SUCCESS) return -1;
    
    vkGetBufferMemoryRequirements(context->device, uniforms->buffer, &memoryReqs);
    
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
    
    uniforms->_totalSize = allocInfo.allocationSize;
    
    result = vkAllocateMemory(context->device, &allocInfo, NULL, &uniforms->_memory);
    if(result != VK_SUCCESS) return -2;
    
    result = vkBindBufferMemory(context->device, uniforms->buffer, uniforms->_memory, 0);
    if(result != VK_SUCCESS) return -3;
    
    return 0;
}

template <typename T>
static inline int32_t updateUniformSingle(UniformBuffer* uniforms, Context* context, T* data, uint32_t index)
{
    void* mapped;
    VkResult result;
    VkMappedMemoryRange memoryRange = {};
    
    uint32_t atomSize = getAtomSize(context);
    uint32_t baseOffset = sizeof(T) * index;
    uint32_t actualOffset = atomSize * (baseOffset / atomSize);
    uint32_t endOffset = baseOffset + sizeof(T);
    uint32_t actualEnd = atomSize * (endOffset % atomSize ? endOffset / atomSize + 1 : endOffset / atomSize);
    uint32_t actualSize = actualEnd - actualOffset;
    
    result = vkMapMemory(context->device, uniforms->_memory, actualOffset, actualSize, 0, &mapped);
    if(result != VK_SUCCESS) return -1;
    
    memcpy((uint8_t*)mapped + (baseOffset - actualOffset), data, sizeof(T));
    
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = uniforms->_memory;
    memoryRange.offset = actualOffset;
    memoryRange.size = actualSize;
    vkFlushMappedMemoryRanges(context->device, 1, &memoryRange);
    
    vkUnmapMemory(context->device, uniforms->_memory);
    
    return 0;
}

template <typename T>
static inline int32_t updateUniforms(UniformBuffer* uniforms, Context* context, T* data)
{
    void* mapped;
    VkResult result;
    VkMappedMemoryRange memoryRange = {};
    
    result = vkMapMemory(context->device, uniforms->_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    if(result != VK_SUCCESS) return -1;
    
    memcpy(mapped, data, uniforms->_dataSize);
    
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = uniforms->_memory;
    memoryRange.offset = 0;
    memoryRange.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(context->device, 1, &memoryRange);
    
    vkUnmapMemory(context->device, uniforms->_memory);
    
    return 0;
}

static inline void uniformBufferDestroy(UniformBuffer* uniforms, Context* context)
{
    vkFreeMemory(context->device, uniforms->_memory, NULL);
    vkDestroyBuffer(context->device, uniforms->buffer, NULL);
}

#endif //UNIFORM_BUFFER_H
