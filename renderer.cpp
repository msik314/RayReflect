#include "renderer.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "context.h"
#include "shaderStorageBuffer.hpp"
#include "texture.h"
#include "uniformBuffer.hpp"
#include "vertex.hpp"
#include "utilMacros.h"
#include "debugUtils.h"

const Vertex _vertices[14] =
{
    {glm::vec4(-0.5f, 0, 0, 1), glm::vec4(-1, 0, 0, 0), glm::vec4(0, 0, 0, 0)},
    {glm::vec4(0, 0, -0.5f, 1), glm::vec4(0, 0, -1, 0), glm::vec4(0, 1, 0, 0)},
    {glm::vec4(0.5f, 0, 0, 1), glm::vec4(1, 0, 0, 0), glm::vec4(1, 1, 0, 0)},
    {glm::vec4(0, 0, 0.5f, 1), glm::vec4(0, 0, 1, 0), glm::vec4(1, 0, 0, 0)},
    {glm::vec4(0, -0.5f, 0, 1), glm::vec4(0, -1, 0, 0), glm::vec4(0.5f, 0.5f, 0, 0)},
    {glm::vec4(0, 0.5f, 0, 1), glm::vec4(0, 1, 0, 0), glm::vec4(0.5f, 0.5f, 0, 0)},
    {glm::vec4(-1, 1, -1, 1), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(0, 0, 0, 0)},
    {glm::vec4(1, 1, -1, 1), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(1, 0, 0, 0)},
    {glm::vec4(1, 1, 1, 1), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(1, 1, 0, 0)},
    {glm::vec4(-1, 1, 1, 1), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(0, 1, 0, 0)},
    {glm::vec4(-1, 1, 1, 1), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f), glm::vec4(0, 1, 0, 0)},
    {glm::vec4(1, 1, 1, 1), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f), glm::vec4(1, 1, 0, 0)},
    {glm::vec4(1, -1, 1, 1), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f), glm::vec4(1, 0, 0, 0)},
    {glm::vec4(-1, -1, 1, 1), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f), glm::vec4(0, 0, 0, 0)}
};

const uint16_t _indices[36] = 
{
    0, 1, 4,
    1, 2, 4,
    2, 3, 4,
    3, 0, 4,
    1, 0, 5,
    2, 1, 5,
    3, 2, 5,
    0, 3, 5,
    6, 7, 8,
    8, 9, 6,
    10, 11, 12,
    12, 13, 10,
};

const VkDrawIndexedIndirectCommand _indirect[2] =
{
    {24, 1, 0, 0, 0},
    {12, 1, 24, 0, 1}
};

static inline int32_t createRenderBuffers(Renderer* renderer, Context* context)
{
    VkResult result;
    uint32_t memoryTypeBits;
    VkMemoryPropertyFlags desiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageCreateInfo imageInfos[4] = {};
    VkMemoryRequirements memoryReqs = {};
    VkMemoryAllocateInfo allocInfo = {};
    VkMemoryType memoryType;
    VkCommandBufferBeginInfo beginInfo = {};
    VkSubmitInfo submitInfo = {};
    VkImageMemoryBarrier layoutBarriers[4] = {};
    VkImageSubresourceRange depthResourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    VkImageSubresourceRange colorResourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkImageViewCreateInfo viewInfos[4] = {};
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    VkImageAspectFlags colorAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkFenceCreateInfo fenceInfo = {};
    VkFence renderFence;
    int32_t colorOffset;
    int32_t positionOffset;
    int32_t normalOffset;
    
    imageInfos[0].sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfos[0].imageType = VK_IMAGE_TYPE_2D;
    imageInfos[0].format = VK_FORMAT_D32_SFLOAT;
    imageInfos[0].extent = {context->width, context->height, 1};
    imageInfos[0].mipLevels = 1;
    imageInfos[0].arrayLayers = 1;
    imageInfos[0].samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfos[0].tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfos[0].usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfos[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfos[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    for(int32_t i = 1; i < 4; ++i)
    {
        imageInfos[i].sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfos[i].imageType = VK_IMAGE_TYPE_2D;
        imageInfos[i].extent = {context->width, context->height, 1};
        imageInfos[i].mipLevels = 1;
        imageInfos[i].arrayLayers = 1;
        imageInfos[i].samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfos[i].tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfos[i].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageInfos[i].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfos[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    
    imageInfos[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfos[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfos[3].format = VK_FORMAT_R16G16_SFLOAT;
    
    result = vkCreateImage(context->device, &imageInfos[0], NULL, &renderer->_depthBuffer);
    if(result != VK_SUCCESS) return -1;
    
    result = vkCreateImage(context->device, &imageInfos[1], NULL, &renderer->_colorBuffer);
    if(result != VK_SUCCESS) return -1;
    
    result = vkCreateImage(context->device, &imageInfos[2], NULL, &renderer->_positionBuffer);
    if(result != VK_SUCCESS) return -1;
    
    result = vkCreateImage(context->device, &imageInfos[3], NULL, &renderer->_normalBuffer);
    if(result != VK_SUCCESS) return -1;
    
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    
    vkGetImageMemoryRequirements(context->device, renderer->_depthBuffer, &memoryReqs);
    allocInfo.allocationSize = memoryReqs.size;
    memoryTypeBits = memoryReqs.memoryTypeBits;
    
    colorOffset = allocInfo.allocationSize;
    vkGetImageMemoryRequirements(context->device, renderer->_colorBuffer, &memoryReqs);
    colorOffset = memoryReqs.alignment * (colorOffset % memoryReqs.alignment ?
        colorOffset / memoryReqs.alignment + 1 :
        colorOffset / memoryReqs.alignment);
    
    allocInfo.allocationSize = colorOffset;
    allocInfo.allocationSize += memoryReqs.size;
    memoryTypeBits |= memoryReqs.memoryTypeBits;
    
    positionOffset = allocInfo.allocationSize;
    vkGetImageMemoryRequirements(context->device, renderer->_positionBuffer, &memoryReqs);
    positionOffset = memoryReqs.alignment * (positionOffset % memoryReqs.alignment ?
        positionOffset / memoryReqs.alignment + 1 :
        positionOffset / memoryReqs.alignment);
    
    allocInfo.allocationSize = positionOffset;
    allocInfo.allocationSize += memoryReqs.size;
    memoryTypeBits |= memoryReqs.memoryTypeBits;
    
    normalOffset = allocInfo.allocationSize;
    vkGetImageMemoryRequirements(context->device, renderer->_normalBuffer, &memoryReqs);
    normalOffset = memoryReqs.alignment * (normalOffset % memoryReqs.alignment ?
        normalOffset / memoryReqs.alignment + 1 :
        normalOffset / memoryReqs.alignment);
    
    allocInfo.allocationSize = normalOffset;
    allocInfo.allocationSize += memoryReqs.size;
    memoryTypeBits |= memoryReqs.memoryTypeBits;
    
    
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
    
    result = vkAllocateMemory(context->device, &allocInfo, NULL, &renderer->_bufferMemory);
    if(result != VK_SUCCESS) return -2;
    
    result = vkBindImageMemory(context->device, renderer->_depthBuffer, renderer->_bufferMemory, 0);
    if(result != VK_SUCCESS) return -3;
    
    result = vkBindImageMemory(context->device, renderer->_colorBuffer, renderer->_bufferMemory, colorOffset);
    if(result != VK_SUCCESS) return -3;
    
    result = vkBindImageMemory(context->device, renderer->_positionBuffer, renderer->_bufferMemory, positionOffset);
    if(result != VK_SUCCESS) return -3;
    
    result = vkBindImageMemory(context->device, renderer->_normalBuffer, renderer->_bufferMemory, normalOffset);
    if(result != VK_SUCCESS) return -3;
    
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(context->setupCmdBuffer, &beginInfo);
    
    layoutBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layoutBarriers[0].srcAccessMask = 0;
    layoutBarriers[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    layoutBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    layoutBarriers[0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    layoutBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutBarriers[0].image = renderer->_depthBuffer;
    layoutBarriers[0].subresourceRange = depthResourceRange;
    
    for(int32_t i = 1; i < 4; ++i)
    {
        layoutBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        layoutBarriers[i].srcAccessMask = 0;
        layoutBarriers[i].dstAccessMask = 
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            
        layoutBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        layoutBarriers[i].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        layoutBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        layoutBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        layoutBarriers[i].subresourceRange = colorResourceRange;
    }
    layoutBarriers[1].image = renderer->_colorBuffer;
    layoutBarriers[2].image = renderer->_positionBuffer;
    layoutBarriers[3].image = renderer->_normalBuffer;
    
    vkCmdPipelineBarrier(context->setupCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &layoutBarriers[0]);
    
    for(int32_t i = 1; i < 4; ++i)
    {
        vkCmdPipelineBarrier(context->setupCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &layoutBarriers[i]);
    }
    vkEndCommandBuffer(context->setupCmdBuffer);
    
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context->setupCmdBuffer;
    
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(context->device, &fenceInfo, NULL, &renderFence);
    
    result = vkQueueSubmit(context->gfxQueue, 1, &submitInfo, renderFence);
    
    vkWaitForFences(context->device, 1, &renderFence, VK_TRUE, 0xffffffffffffffff);
    vkResetCommandBuffer(context->setupCmdBuffer, 0);
    
    vkDestroyFence(context->device, renderFence, NULL);
    
    for(int i = 0; i < 4; ++i)
    {
        viewInfos[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfos[i].viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfos[i].format = imageInfos[i].format;
        viewInfos[i].components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        viewInfos[i].subresourceRange.aspectMask = colorAspectMask;
        viewInfos[i].subresourceRange.baseMipLevel = 0;
        viewInfos[i].subresourceRange.levelCount = 1;
        viewInfos[i].subresourceRange.baseArrayLayer = 0;
        viewInfos[i].subresourceRange.layerCount = 1;
    }
    viewInfos[0].image = renderer->_depthBuffer;
    viewInfos[0].subresourceRange.aspectMask = aspectMask;
    viewInfos[1].image = renderer->_colorBuffer;
    viewInfos[2].image = renderer->_positionBuffer;
    viewInfos[3].image = renderer->_normalBuffer;
    
    result = vkCreateImageView(context->device, &viewInfos[0], NULL, &renderer->_depthView);
    result = vkCreateImageView(context->device, &viewInfos[1], NULL, &renderer->_colorView);
    result = vkCreateImageView(context->device, &viewInfos[2], NULL, &renderer->_positionView);
    result = vkCreateImageView(context->device, &viewInfos[3], NULL, &renderer->_normalView);
    if(result != VK_SUCCESS) return -3;
    return 0;
}

static inline int32_t createRenderPass(Renderer* renderer, Context* context)
{
    VkAttachmentDescription passAttachments[5] = {};
    VkAttachmentReference backReference = {};
    VkAttachmentReference depthReference = {};
    VkAttachmentReference writeReferences[3] = {};
    VkAttachmentReference readReferences[3] = {};
    VkSubpassDescription subpasses[2] = {};
    VkRenderPassCreateInfo renderPassInfo = {};
    VkFramebufferCreateInfo frameBufferInfo = {};
    VkImageView framebufferAttachements[5] = {};
    VkSubpassDependency subpassDeps = {};
    VkResult result;
    
    passAttachments[0].format = context->colorFormat;
    passAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    passAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    passAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    passAttachments[1].format = VK_FORMAT_D32_SFLOAT;
    passAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    passAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    passAttachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    passAttachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    passAttachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    passAttachments[3].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    passAttachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    passAttachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    passAttachments[4].format = VK_FORMAT_R16G16_SFLOAT;
    passAttachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[4].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    passAttachments[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    backReference.attachment = 0;
    backReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    for(int32_t i = 0; i < 3; ++i)
    {
        writeReferences[i].attachment = i + 2;
        writeReferences[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        readReferences[i].attachment = i + 2;
        readReferences[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 3;
    subpasses[0].pColorAttachments = writeReferences;
    subpasses[0].pDepthStencilAttachment = &depthReference;
    
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &backReference;
    subpasses[1].inputAttachmentCount = 3;
    subpasses[1].pInputAttachments = readReferences;
    
    subpassDeps.srcSubpass = 0;
    subpassDeps.dstSubpass = 1;
    subpassDeps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDeps.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDeps.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDeps.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassDeps.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 5;
    renderPassInfo.pAttachments = passAttachments;
    renderPassInfo.subpassCount = 2;
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDeps;
    result = vkCreateRenderPass(context->device, &renderPassInfo, NULL, &renderer->_renderPass);
    
    if(result != VK_SUCCESS) return -1;
    
    
    framebufferAttachements[1] = renderer->_depthView;
    framebufferAttachements[2] = renderer->_colorView;
    framebufferAttachements[3] = renderer->_positionView;
    framebufferAttachements[4] = renderer->_normalView;
    
    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferInfo.renderPass = renderer->_renderPass;
    frameBufferInfo.attachmentCount = 5;
    frameBufferInfo.pAttachments = framebufferAttachements;
    frameBufferInfo.width = context->width;
    frameBufferInfo.height = context->height;
    frameBufferInfo.layers = 1;
    
    renderer->_frameBuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * context->numImages);
    
    for(int i = 0; i < context->numImages; ++i)
    {
        framebufferAttachements[0] = context->presentViews[i];
        result = vkCreateFramebuffer(context->device, &frameBufferInfo, NULL, &renderer->_frameBuffers[i]);
        if(result != VK_SUCCESS) return -2;
    }
    
    return 0;
}

static inline int32_t createVertexBuffer(Renderer* renderer, Context* context)
{
    VkResult result;
    int32_t ssbRes;
    uint32_t vertexMemoryTypeBits;
    VkMemoryType memoryType;
    VkBufferCreateInfo vertexInfo = {};
    VkMemoryRequirements vertexMemoryReqs = {};
    VkMemoryAllocateInfo allocInfo = {};
    VkMemoryPropertyFlags desiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    void* mapped;
    uint32_t indices[(LENGTH_OF(_indices) * sizeof(uint32_t) * 4) / 3];
    vertexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexInfo.size = sizeof(_indirect) + LENGTH_OF(_vertices) * sizeof(Vertex) + LENGTH_OF(_indices) * sizeof(uint16_t);
    vertexInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    vertexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    result = vkCreateBuffer(context->device, &vertexInfo, NULL, &renderer->_vertexBuffer);
    if(result != VK_SUCCESS) return -1;
    
    vkGetBufferMemoryRequirements(context->device, renderer->_vertexBuffer, &vertexMemoryReqs);
    
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = vertexMemoryReqs.size;
    
    vertexMemoryTypeBits = vertexMemoryReqs.memoryTypeBits;
    
    for(int i = 0; i < 32; ++i)
    {
        memoryType = context->physicalDeviceMemory.memoryTypes[i];
        if(vertexMemoryTypeBits & 1 && (memoryType.propertyFlags & desiredFlags) == desiredFlags)
        {
            allocInfo.memoryTypeIndex = i;
            break;
        }
        
        vertexMemoryTypeBits >>= 1;
    }
    
    result = vkAllocateMemory(context->device, &allocInfo, NULL, &renderer->_vertexMemory);
    if(result != VK_SUCCESS) return -2;
    
    result = vkMapMemory(context->device, renderer->_vertexMemory, 0, VK_WHOLE_SIZE, 0, &mapped);
    if(result != VK_SUCCESS) return -3;
    
    memcpy(mapped, _indirect, sizeof(_indirect));
    memcpy((uint8_t*)mapped + sizeof(_indirect), _vertices, sizeof(_vertices));
    memcpy((uint8_t*)mapped + sizeof(_indirect) + sizeof(_vertices), _indices, sizeof(_indices));
    vkUnmapMemory(context->device, renderer->_vertexMemory);
    result = vkBindBufferMemory(context->device, 
        renderer->_vertexBuffer, renderer->_vertexMemory, 0);
    
    if(result != VK_SUCCESS) return -4;
    
    ssbRes = shaderStorageBufferCreate(&renderer->_shaderVertexBuffer, context, sizeof(_vertices));
    if(ssbRes != VK_SUCCESS) return -5;
    
    ssbRes = shaderStorageBufferCreate(&renderer->_shaderIndexBuffer, context, sizeof(indices));
    if(ssbRes != VK_SUCCESS) return -6;
    for(int32_t i = 0; i < LENGTH_OF(indices); i += 4)
    {
        indices[i] = _indices[(i/4) * 3];
        indices[i + 1] = _indices[(i/4) * 3 + 1];
        indices[i + 2] = _indices[(i/4) * 3 + 2];
        indices[i + 3] = 0;
    }
    shaderStorageBufferWrite(&renderer->_shaderIndexBuffer, context, indices, sizeof(indices));
    
    return 0;
}

int32_t rendererCreate(Renderer* renderer, Context* context)
{
    renderer->context = context;
    renderer->_camPos = glm::vec3();
    
    if(createRenderBuffers(renderer, context)) return -1;
    if(createRenderPass(renderer, context)) return -2;
    if(createVertexBuffer(renderer, context)) return -3;
    
    return 0;
}

static inline int32_t createShaders(Renderer* renderer, ShaderSrc vertexSrc1, ShaderSrc fragSrc1, ShaderSrc vertexSrc2, ShaderSrc fragSrc2)
{
    VkShaderModuleCreateInfo vertexInfos[2] = {};
    VkShaderModuleCreateInfo fragmentInfos[2] = {};
    VkResult result;
    
    vertexInfos[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexInfos[0].codeSize = vertexSrc1.len;
    vertexInfos[0].pCode = (uint32_t*)(vertexSrc1.src);
    
    fragmentInfos[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentInfos[0].codeSize = fragSrc1.len;
    fragmentInfos[0].pCode = (uint32_t *)(fragSrc1.src);
    
    vertexInfos[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexInfos[1].codeSize = vertexSrc2.len;
    vertexInfos[1].pCode = (uint32_t*)(vertexSrc2.src);
    
    fragmentInfos[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentInfos[1].codeSize = fragSrc2.len;
    fragmentInfos[1].pCode = (uint32_t *)(fragSrc2.src);
    
    result = vkCreateShaderModule(renderer->context->device, &vertexInfos[0], NULL, &renderer->_vertexShader1);
    if(result != VK_SUCCESS) return -1;
    
    result = vkCreateShaderModule(renderer->context->device, &fragmentInfos[0], NULL, &renderer->_fragmentShader1);
    if(result != VK_SUCCESS) return -2;
    
    result = vkCreateShaderModule(renderer->context->device, &vertexInfos[1], NULL, &renderer->_vertexShader2);
    if(result != VK_SUCCESS) return -1;
    
    result = vkCreateShaderModule(renderer->context->device, &fragmentInfos[1], NULL, &renderer->_fragmentShader2);
    if(result != VK_SUCCESS) return -2;
    
    return 0;
}

static inline int32_t createDescriptors(Renderer* renderer)
{
    VkDescriptorSetLayoutBinding bindings[9] = {};
    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    VkDescriptorPoolSize uniformBufferPoolSize[9] = {};
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    VkDescriptorSetAllocateInfo descriptorAllocInfo = {};
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    VkDescriptorImageInfo descriptorImageInfos[3] = {};
    VkWriteDescriptorSet writeDescriptor = {};
    VkResult result;
    
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = MAX_TEXTURES;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    for(int32_t i = 2; i < 5; ++i)
    {
        bindings[i].binding = i - 2;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    
    bindings[5].binding = 3;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    bindings[6].binding = 4;
    bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[6].descriptorCount = MAX_TEXTURES;
    bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    bindings[7].binding = 0;
    bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    bindings[8].binding = 1;
    bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 2;
    setLayoutInfo.pBindings = bindings;
    
    result = vkCreateDescriptorSetLayout(renderer->context->device, &setLayoutInfo, NULL, &renderer->_descriptorLayout);
    if(result != VK_SUCCESS) return -1;
    
    setLayoutInfo.bindingCount = 5;
    setLayoutInfo.pBindings = &bindings[2];
    
    result = vkCreateDescriptorSetLayout(renderer->context->device, &setLayoutInfo, NULL, &renderer->_secondPassDescLayout);
    if(result != VK_SUCCESS) return -1;
    
    setLayoutInfo.bindingCount = 2;
    setLayoutInfo.pBindings = &bindings[7];
    
    result = vkCreateDescriptorSetLayout(renderer->context->device, &setLayoutInfo, NULL, &renderer->_sharedDescLayout);
    if(result != VK_SUCCESS) return -1;
    
    uniformBufferPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferPoolSize[0].descriptorCount = 1;
    
    uniformBufferPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uniformBufferPoolSize[1].descriptorCount = MAX_TEXTURES;
    
    for(int32_t i = 2; i < 5; ++i)
    {
        uniformBufferPoolSize[i].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        uniformBufferPoolSize[i].descriptorCount = 1;
    }
    
    uniformBufferPoolSize[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferPoolSize[5].descriptorCount = 1;
    
    uniformBufferPoolSize[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uniformBufferPoolSize[6].descriptorCount = MAX_TEXTURES;
    
    uniformBufferPoolSize[7].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    uniformBufferPoolSize[7].descriptorCount = 1;
    
    uniformBufferPoolSize[8].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    uniformBufferPoolSize[8].descriptorCount = 1;
    
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.maxSets = 3;
    descriptorPoolInfo.poolSizeCount = 9;
    descriptorPoolInfo.pPoolSizes = uniformBufferPoolSize;
    
    result = vkCreateDescriptorPool(renderer->context->device, &descriptorPoolInfo, NULL, &renderer->_descriptorPool);
    if(result != VK_SUCCESS) return -2;
    
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.descriptorPool = renderer->_descriptorPool;
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.pSetLayouts = &renderer->_descriptorLayout;
    
    result = vkAllocateDescriptorSets(renderer->context->device, &descriptorAllocInfo, &renderer->_descriptorSet);
    if(result != VK_SUCCESS) return -3;
    
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.descriptorPool = renderer->_descriptorPool;
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.pSetLayouts = &renderer->_secondPassDescLayout;
    
    result = vkAllocateDescriptorSets(renderer->context->device, &descriptorAllocInfo, &renderer->_secondPassDescSet);
    if(result != VK_SUCCESS) return -3;
    
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.descriptorPool = renderer->_descriptorPool;
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.pSetLayouts = &renderer->_sharedDescLayout;
    
    result = vkAllocateDescriptorSets(renderer->context->device, &descriptorAllocInfo, &renderer->_sharedDescSet);
    if(result != VK_SUCCESS) return -3;
    
    descriptorBufferInfo.buffer = renderer->_uniformBuffer.buffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = VK_WHOLE_SIZE;
    
    writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptor.dstSet = renderer->_descriptorSet;
    writeDescriptor.dstBinding = 0;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptor.pBufferInfo = &descriptorBufferInfo;
    
    vkUpdateDescriptorSets(renderer->context->device, 1, &writeDescriptor, 0, NULL);
    
    descriptorBufferInfo.buffer = renderer->_camPosBuffer.buffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = VK_WHOLE_SIZE;
    
    writeDescriptor.dstSet = renderer->_secondPassDescSet;
    writeDescriptor.dstBinding = 3;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptor.pBufferInfo = &descriptorBufferInfo;
    vkUpdateDescriptorSets(renderer->context->device, 1, &writeDescriptor, 0, NULL);
    
    descriptorBufferInfo.buffer = renderer->_shaderVertexBuffer.buffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = VK_WHOLE_SIZE;
    
    writeDescriptor.dstSet = renderer->_sharedDescSet;
    writeDescriptor.dstBinding = 0;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptor.pBufferInfo = &descriptorBufferInfo;
    vkUpdateDescriptorSets(renderer->context->device, 1, &writeDescriptor, 0, NULL);
    
    descriptorBufferInfo.buffer = renderer->_shaderIndexBuffer.buffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = VK_WHOLE_SIZE;
    
    writeDescriptor.dstSet = renderer->_sharedDescSet;
    writeDescriptor.dstBinding = 1;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptor.pBufferInfo = &descriptorBufferInfo;
    vkUpdateDescriptorSets(renderer->context->device, 1, &writeDescriptor, 0, NULL);
    
    for(int32_t i = 0; i < 3; ++i)
    {
        descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    descriptorImageInfos[0].imageView = renderer->_colorView;
    descriptorImageInfos[1].imageView = renderer->_positionView;
    descriptorImageInfos[2].imageView = renderer->_normalView;
    
    writeDescriptor.dstSet = renderer->_secondPassDescSet;
    writeDescriptor.dstBinding = 0;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 3;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writeDescriptor.pBufferInfo = NULL;
    writeDescriptor.pImageInfo = descriptorImageInfos;
    vkUpdateDescriptorSets(renderer->context->device, 1, &writeDescriptor, 0, NULL);
    
    return 0;
}

int32_t createPipeline(Renderer* renderer, ShaderSrc p1Vertex, ShaderSrc p1Fragment, ShaderSrc p2Vertex, ShaderSrc p2Fragment)
{
    VkPipelineLayoutCreateInfo layoutInfo = {};
    VkPipelineShaderStageCreateInfo shaderInfos[4] = {};
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfos[2] = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {};
    VkPipelineViewportStateCreateInfo viewportStateInfo = {};
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
    VkPipelineDepthStencilStateCreateInfo depthStateInfos[2] = {};
    VkPipelineColorBlendStateCreateInfo blendStateInfos[2] = {};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    VkVertexInputBindingDescription bindingDescription = {};
    VkVertexInputAttributeDescription attributeDescriptions[3] = {};
    VkViewport viewport = {};
    VkRect2D scissors = {};
    VkStencilOpState nopStencilState = {};
    VkPipelineColorBlendAttachmentState blendAttachmentStates[4] = {};
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    VkResult result;
    VkDescriptorSetLayout descLayouts[2] = {};
    VkSpecializationInfo specMap = {};
    VkSpecializationMapEntry specEntries[3] = {};
    uint32_t specData[3] = {LENGTH_OF(_indirect), LENGTH_OF(_vertices), LENGTH_OF(_indices)/3};
    
    if(uniformBufferCreate<StandardUniforms>(&renderer->_uniformBuffer, renderer->context, 2)) return -1;
    if(uniformBufferCreate<glm::vec3>(&renderer->_camPosBuffer, renderer->context, 1)) return -1;
    
    if(createShaders(renderer, p1Vertex, p1Fragment, p2Vertex, p2Fragment)) return -2;
    
    if(createDescriptors(renderer)) return -3;
    
    descLayouts[0] = renderer->_descriptorLayout;
    descLayouts[1] = renderer->_sharedDescLayout;
    
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts = descLayouts;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = NULL;
    
    result = vkCreatePipelineLayout(renderer->context->device, &layoutInfo, NULL, &renderer->_pipelineLayoutPass1);
    if(result != VK_SUCCESS) return -4;
    
    descLayouts[0] = renderer->_secondPassDescLayout;
    
    result = vkCreatePipelineLayout(renderer->context->device, &layoutInfo, NULL, &renderer->_pipelineLayoutPass2);
    if(result != VK_SUCCESS) return -4;
    
    for(int32_t i = 0; i < 3; ++i)
    {
        specEntries[i].constantID = i;
        specEntries[i].offset = i * sizeof(uint32_t);
        specEntries[i].size = sizeof(uint32_t);
    }
    
    specMap.mapEntryCount = 3;
    specMap.pMapEntries = specEntries;
    specMap.dataSize = sizeof(specData);
    specMap.pData = specData;
    
    shaderInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfos[0].module = renderer->_vertexShader1;
    shaderInfos[0].pName = "main";
    shaderInfos[0].pSpecializationInfo = &specMap;
    
    shaderInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderInfos[1].module = renderer->_fragmentShader1;
    shaderInfos[1].pName = "main";
    shaderInfos[1].pSpecializationInfo = &specMap;
    
    shaderInfos[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfos[2].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfos[2].module = renderer->_vertexShader2;
    shaderInfos[2].pName = "main";
    shaderInfos[2].pSpecializationInfo = &specMap;
    
    shaderInfos[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfos[3].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderInfos[3].module = renderer->_fragmentShader2;
    shaderInfos[3].pName = "main";
    shaderInfos[3].pSpecializationInfo = &specMap;
    
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(glm::vec4);
    
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = 2 * sizeof(glm::vec4);
    
    vertexInputStateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfos[0].vertexBindingDescriptionCount = 1;
    vertexInputStateInfos[0].pVertexBindingDescriptions = &bindingDescription;
    vertexInputStateInfos[0].vertexAttributeDescriptionCount = 3;
    vertexInputStateInfos[0].pVertexAttributeDescriptions = attributeDescriptions;

    vertexInputStateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfos[1].vertexBindingDescriptionCount = 0;
    vertexInputStateInfos[1].vertexAttributeDescriptionCount = 0;
    
    inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = renderer->context->width;
    viewport.height = renderer->context->height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    
    scissors.offset = {0, 0};
    scissors.extent = {renderer->context->width, renderer->context->height};
    
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissors;
    
    rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateInfo.depthBiasConstantFactor = 0;
    rasterizationStateInfo.depthBiasClamp = 0;
    rasterizationStateInfo.depthBiasSlopeFactor = 0;
    rasterizationStateInfo.lineWidth = 1;
    
    multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateInfo.minSampleShading = 0;
    multisampleStateInfo.pSampleMask = NULL;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;
    
    nopStencilState.failOp = VK_STENCIL_OP_KEEP;
    nopStencilState.passOp = VK_STENCIL_OP_KEEP;
    nopStencilState.depthFailOp = VK_STENCIL_OP_KEEP;
    nopStencilState.compareOp = VK_COMPARE_OP_ALWAYS;
    nopStencilState.compareMask = 0;
    nopStencilState.writeMask = 0;
    nopStencilState.reference = 0;
    
    depthStateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStateInfos[0].depthTestEnable = VK_TRUE;
    depthStateInfos[0].depthWriteEnable = VK_TRUE;
    depthStateInfos[0].depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStateInfos[0].depthBoundsTestEnable = VK_FALSE;
    depthStateInfos[0].stencilTestEnable = VK_FALSE;
    depthStateInfos[0].front = nopStencilState;
    depthStateInfos[0].back = nopStencilState;
    depthStateInfos[0].minDepthBounds = 0;
    depthStateInfos[0].maxDepthBounds = 1;
    
    depthStateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStateInfos[1].depthTestEnable = VK_FALSE;
    depthStateInfos[1].depthWriteEnable = VK_FALSE;
    depthStateInfos[1].depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStateInfos[1].depthBoundsTestEnable = VK_FALSE;
    depthStateInfos[1].stencilTestEnable = VK_FALSE;
    depthStateInfos[1].front = nopStencilState;
    depthStateInfos[1].back = nopStencilState;
    depthStateInfos[1].minDepthBounds = 0;
    depthStateInfos[1].maxDepthBounds = 1;
    
    for(int32_t i = 0; i < 4; ++i)
    {
        blendAttachmentStates[i].blendEnable = VK_FALSE;
        blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
        blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
        blendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentStates[i].colorWriteMask = 0x0f;
    }
    
    blendStateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendStateInfos[0].logicOpEnable = VK_FALSE;
    blendStateInfos[0].logicOp = VK_LOGIC_OP_CLEAR;
    blendStateInfos[0].attachmentCount = 3;
    blendStateInfos[0].pAttachments = &blendAttachmentStates[0];
    blendStateInfos[0].blendConstants[0] = 1.0f;
    blendStateInfos[0].blendConstants[1] = 1.0f;
    blendStateInfos[0].blendConstants[2] = 1.0f;
    blendStateInfos[0].blendConstants[3] = 1.0f;
    
    blendStateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendStateInfos[1].logicOpEnable = VK_FALSE;
    blendStateInfos[1].logicOp = VK_LOGIC_OP_CLEAR;
    blendStateInfos[1].attachmentCount = 1;
    blendStateInfos[1].pAttachments = &blendAttachmentStates[3];
    blendStateInfos[1].blendConstants[0] = 1.0f;
    blendStateInfos[1].blendConstants[1] = 1.0f;
    blendStateInfos[1].blendConstants[2] = 1.0f;
    blendStateInfos[1].blendConstants[3] = 1.0f;
    
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;
    
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderInfos;
    pipelineInfo.pVertexInputState = &vertexInputStateInfos[0];
    pipelineInfo.pInputAssemblyState = &inputAssemblyStateInfo;
    pipelineInfo.pTessellationState = NULL;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizationStateInfo;
    pipelineInfo.pMultisampleState = &multisampleStateInfo;
    pipelineInfo.pDepthStencilState = &depthStateInfos[0];
    pipelineInfo.pColorBlendState = &blendStateInfos[0];
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = renderer->_pipelineLayoutPass1;
    pipelineInfo.renderPass = renderer->_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;
    
    result = vkCreateGraphicsPipelines(renderer->context->device,
        VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &renderer->_pipelinePass1);
    
    if(result != VK_SUCCESS) return -5;
    
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = &shaderInfos[2];
    pipelineInfo.pVertexInputState = &vertexInputStateInfos[1];
    pipelineInfo.pDepthStencilState = &depthStateInfos[1];
    pipelineInfo.pColorBlendState = &blendStateInfos[1];
    pipelineInfo.layout = renderer->_pipelineLayoutPass2;
    pipelineInfo.renderPass = renderer->_renderPass;
    pipelineInfo.subpass = 1;
    
    result = vkCreateGraphicsPipelines(renderer->context->device,
        VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &renderer->_pipelinePass2);
    
    if(result != VK_SUCCESS) return -5;
    
    for(int32_t i = 0; i < MAX_TEXTURES; ++i)
    {
        textureCreateFromFile(&renderer->_textures[i], renderer->context, "res/blank.png");
        textureUpdateDescriptor(&renderer->_textures[i], renderer->context, renderer->_descriptorSet, i, 1);
        textureUpdateDescriptor(&renderer->_textures[i], renderer->context, renderer->_secondPassDescSet, i, 4);
    }
    
    return 0;
}

int32_t createRenderCommands(Renderer* renderer)
{
    VkFenceCreateInfo fenceInfo = {};
    VkSemaphoreCreateInfo semaphoreInfo = {};
    VkEventCreateInfo eventInfo = {};
    VkCommandBufferAllocateInfo cmdBufferInfo = {};
    VkCommandBufferBeginInfo beginInfo = {};
    VkImageMemoryBarrier renderBarrier = {};
    VkImageMemoryBarrier presentBarrier = {};
    VkMemoryBarrier uniformBarrier = {};
    VkRenderPassBeginInfo renderPassInfo = {};
    VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkClearValue clearValues[] =
    {
        {0.0f, 0.0f, 0.0f, 1.0f}, 
        {1.0f, 0.0f}, 
        {0.0f, 0.0f, 0.0f, 0.0f}, 
        {0.0f, 0.0f, 0.0f, 0.0f}, 
        {0.0f, 0.0f}
    };
    
    VkViewport viewport = {0, 0, (float)(renderer->context->width), (float)(renderer->context->height), 0, 1};
    VkRect2D scissor = {0, 0, renderer->context->width, renderer->context->height};
    VkDeviceSize offsets = sizeof(_indirect);
    VkResult result;
    
    renderer->_drawBuffers = (VkCommandBuffer*)malloc(renderer->context->numImages * sizeof(VkCommandBuffer));
    
    cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferInfo.commandPool = renderer->context->gfxCmdPool;
    cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount = renderer->context->numImages;
    
    result = vkAllocateCommandBuffers(renderer->context->device, &cmdBufferInfo, renderer->_drawBuffers);
    if(result != VK_SUCCESS) return -1;
    
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    result = vkCreateFence(renderer->context->device, &fenceInfo, NULL, &renderer->_renderFence);
    if(result != VK_SUCCESS) return -2;
    
    semaphoreInfo.sType= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    result = vkCreateSemaphore(renderer->context->device, &semaphoreInfo, NULL, &renderer->_renderCompleteSemaphore);
    if(result != VK_SUCCESS) return -3;
    
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    
    renderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    renderBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    renderBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    renderBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    renderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    renderBarrier.subresourceRange = resourceRange;
    
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderer->_renderPass;
    renderPassInfo.renderArea = {0, 0, renderer->context->width, renderer->context->height};
    renderPassInfo.clearValueCount = 5;
    renderPassInfo.pClearValues = clearValues;
    
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.subresourceRange = resourceRange;
    
    uniformBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    uniformBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    uniformBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    
    eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    
    for(int32_t i = 0; i < renderer->context->numImages; ++i)
    {
        vkBeginCommandBuffer(renderer->_drawBuffers[i], &beginInfo);
        
        vkCmdPipelineBarrier(renderer->_drawBuffers[i], VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &uniformBarrier, 0, NULL, 0, NULL);
        
        renderBarrier.image = renderer->context->presentImages[i];
        vkCmdPipelineBarrier(renderer->_drawBuffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &renderBarrier);
        
        renderPassInfo.framebuffer = renderer->_frameBuffers[i];
        vkCmdBeginRenderPass(renderer->_drawBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(renderer->_drawBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->_pipelinePass1);
        
        vkCmdSetViewport(renderer->_drawBuffers[i], 0, 1, &viewport);
        vkCmdSetScissor(renderer->_drawBuffers[i], 0, 1, &scissor);
        
        vkCmdBindVertexBuffers(renderer->_drawBuffers[i], 0, 1, &renderer->_vertexBuffer, &offsets);
        vkCmdBindIndexBuffer(renderer->_drawBuffers[i], renderer->_vertexBuffer, offsets + sizeof(_vertices), VK_INDEX_TYPE_UINT16);
        
        vkCmdBindDescriptorSets(renderer->_drawBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->_pipelineLayoutPass1, 0, 1, &renderer->_descriptorSet, 0, NULL);
        vkCmdBindDescriptorSets(renderer->_drawBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->_pipelineLayoutPass1, 1, 1, &renderer->_sharedDescSet, 0, NULL);
        
        vkCmdDrawIndexedIndirect(renderer->_drawBuffers[i], renderer->_vertexBuffer, 0, LENGTH_OF(_indirect), sizeof(_indirect[0]));
        
        vkCmdNextSubpass(renderer->_drawBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(renderer->_drawBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->_pipelinePass2);
        vkCmdBindDescriptorSets(renderer->_drawBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->_pipelineLayoutPass2, 0, 1, &renderer->_secondPassDescSet, 0, NULL);
        vkCmdBindDescriptorSets(renderer->_drawBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->_pipelineLayoutPass2, 1, 1, &renderer->_sharedDescSet, 0, NULL);
        
        vkCmdDraw(renderer->_drawBuffers[i], 3, 1, 0, 0);
        
        vkCmdEndRenderPass(renderer->_drawBuffers[i]);
        
        presentBarrier.image = renderer->context->presentImages[i];
        vkCmdPipelineBarrier(renderer->_drawBuffers[i], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &presentBarrier);
        
        vkEndCommandBuffer(renderer->_drawBuffers[i]);
    }
    
    return 0;
}

void destroyRenderCommands(Renderer* renderer)
{
    waitIdle(renderer->context);
    
    vkDestroyFence(renderer->context->device, renderer->_renderFence, NULL);
    vkDestroySemaphore(renderer->context->device, renderer->_renderCompleteSemaphore, NULL);
    
    vkFreeCommandBuffers(renderer->context->device,
        renderer->context->gfxCmdPool, renderer->context->numImages, renderer->_drawBuffers);
    
    free(renderer->_drawBuffers);
}

static inline void destroyDescriptors(Renderer* renderer)
{
    for(int32_t i = 0; i < MAX_TEXTURES; ++i)
    {
        textureDestroy(&renderer->_textures[i], renderer->context);
    }
    
    vkResetDescriptorPool(renderer->context->device, renderer->_descriptorPool, 0);
    vkDestroyDescriptorPool(renderer->context->device, renderer->_descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(renderer->context->device, renderer->_descriptorLayout, NULL);
    vkDestroyDescriptorSetLayout(renderer->context->device, renderer->_secondPassDescLayout, NULL);
    vkDestroyDescriptorSetLayout(renderer->context->device, renderer->_sharedDescLayout, NULL);
}

void destroyPipeline(Renderer* renderer)
{
    waitIdle(renderer->context);
    
    vkDestroyPipeline(renderer->context->device, renderer->_pipelinePass1, NULL);
    vkDestroyPipelineLayout(renderer->context->device, renderer->_pipelineLayoutPass1, NULL);
    vkDestroyShaderModule(renderer->context->device, renderer->_vertexShader1, NULL);
    vkDestroyShaderModule(renderer->context->device, renderer->_fragmentShader1, NULL);
    
    vkDestroyPipeline(renderer->context->device, renderer->_pipelinePass2, NULL);
    vkDestroyPipelineLayout(renderer->context->device, renderer->_pipelineLayoutPass2, NULL);
    vkDestroyShaderModule(renderer->context->device, renderer->_vertexShader2, NULL);
    vkDestroyShaderModule(renderer->context->device, renderer->_fragmentShader2, NULL);
    
    destroyDescriptors(renderer);
    
    uniformBufferDestroy(&renderer->_uniformBuffer, renderer->context);
    uniformBufferDestroy(&renderer->_camPosBuffer, renderer->context);
}

void rendererDestroy(Renderer* renderer)
{
    waitIdle(renderer->context);
    
    shaderStorageBufferDestroy(&renderer->_shaderVertexBuffer, renderer->context);
    shaderStorageBufferDestroy(&renderer->_shaderIndexBuffer, renderer->context);
    
    vkFreeMemory(renderer->context->device, renderer->_vertexMemory, NULL);
    vkDestroyBuffer(renderer->context->device, renderer->_vertexBuffer, NULL);
    
    for(int i = 0; i < renderer->context->numImages; ++i)
    {
        vkDestroyFramebuffer(renderer->context->device, renderer->_frameBuffers[i], NULL);
    }
    
    vkDestroyRenderPass(renderer->context->device, renderer->_renderPass, NULL);
    
    vkFreeMemory(renderer->context->device, renderer->_bufferMemory, NULL);
    vkDestroyImageView(renderer->context->device, renderer->_depthView, NULL);
    vkDestroyImage(renderer->context->device, renderer->_depthBuffer, NULL);
    vkDestroyImageView(renderer->context->device, renderer->_colorView, NULL);
    vkDestroyImage(renderer->context->device, renderer->_colorBuffer, NULL);
    vkDestroyImageView(renderer->context->device, renderer->_positionView, NULL);
    vkDestroyImage(renderer->context->device, renderer->_positionBuffer, NULL);
    vkDestroyImageView(renderer->context->device, renderer->_normalView, NULL);
    vkDestroyImage(renderer->context->device, renderer->_normalBuffer, NULL);
    
    free(renderer->_frameBuffers);
}
