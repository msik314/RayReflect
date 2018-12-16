#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "context.h"
#include "texture.h"
#include "shaderStorageBuffer.hpp"
#include "uniformBuffer.hpp"


typedef struct 
{
    Context* context;
    
    VkDeviceMemory _bufferMemory;
    VkImage _depthBuffer;
    VkImageView _depthView;
    VkImage _colorBuffer;
    VkImageView _colorView;
    VkImage _positionBuffer;
    VkImageView _positionView;
    VkImage _normalBuffer;
    VkImageView _normalView;
    VkRenderPass _renderPass;
    
    VkFramebuffer* _frameBuffers;
    
    VkDeviceMemory _vertexMemory;
    VkBuffer _vertexBuffer;
    
    VkShaderModule _vertexShader1;
    VkShaderModule _fragmentShader1;
    VkShaderModule _vertexShader2;
    VkShaderModule _fragmentShader2;
    
    VkPipelineLayout _pipelineLayoutPass1;
    VkPipeline _pipelinePass1;
    VkPipelineLayout _pipelineLayoutPass2;
    VkPipeline _pipelinePass2;
    
    VkSemaphore _renderCompleteSemaphore;
    VkFence _renderFence;
    
    VkCommandBuffer* _drawBuffers;
    
    VkDescriptorPool _descriptorPool;
    VkDescriptorSetLayout _descriptorLayout;
    VkDescriptorSet _descriptorSet;
    VkDescriptorSetLayout _secondPassDescLayout;
    VkDescriptorSet _secondPassDescSet;
    VkDescriptorSetLayout _sharedDescLayout;
    VkDescriptorSet _sharedDescSet;
    
    
    UniformBuffer _uniformBuffer;
    UniformBuffer _camPosBuffer;
    ShaderStorageBuffer _shaderVertexBuffer;
    ShaderStorageBuffer _shaderIndexBuffer;
    Texture _textures[8];
    glm::vec3 _camPos;
}
Renderer;

typedef struct
{
    const char* src;
    uint32_t len;
}
ShaderSrc;

int32_t rendererCreate(Renderer* renderer, Context* context);
int32_t createPipeline(Renderer* renderer, ShaderSrc p1Vertex, ShaderSrc p1Fragment, ShaderSrc p2Vertex, ShaderSrc p2Fragment);
int32_t createComputePipeline(Renderer* renderer, ShaderSrc compute);
int32_t createRenderCommands(Renderer* renderer);


void destroyRenderCommands(Renderer* renderer);
void destroyComputePipeline(Renderer* renderer);
void destroyPipeline(Renderer* renderer);
void rendererDestroy(Renderer* renderer);

static inline int32_t updateRendererUniforms(Renderer* renderer, StandardUniforms* data)
{
    return updateUniforms(&renderer->_uniformBuffer, renderer->context, data);
}

static inline int32_t updateSingleUniform(Renderer* renderer, StandardUniforms* data, uint32_t index)
{
    return updateUniformSingle(&renderer->_uniformBuffer, renderer->context, data, index);
}

static inline int32_t createTexture(Renderer* renderer, void* data, int width, int height, uint32_t index)
{
    textureDestroy(&renderer->_textures[index], renderer->context);
    return textureCreate(&renderer->_textures[index], renderer->context, data, width, height);
}

static inline createTextureFromFile(Renderer* renderer, const char* file, uint32_t index)
{
    textureDestroy(&renderer->_textures[index], renderer->context);
    return textureCreateFromFile(&renderer->_textures[index], renderer->context, file);
}

static inline void updateTexture(Renderer* renderer, uint32_t index)
{
    textureUpdateDescriptor(&renderer->_textures[index], renderer->context, renderer->_descriptorSet, index, 1);
    textureUpdateDescriptor(&renderer->_textures[index], renderer->context, renderer->_secondPassDescSet, index, 4);
}

static inline void destroyTexture(Renderer* renderer, uint32_t index)
{
    textureDestroy(&renderer->_textures[index], renderer->context);
}

static inline glm::vec3 getCamPos(Renderer* renderer)
{
    return renderer->_camPos;
}

static inline void setCamPos(Renderer* renderer, glm::vec3 camPos)
{
    renderer->_camPos = camPos;
    updateUniforms(&renderer->_camPosBuffer, renderer->context, &camPos);
}

static inline void render(Renderer* renderer)
{
    VkSubmitInfo submitInfo = {};
    VkPipelineStageFlags waitStageMask = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
    volatile VkResult eventStatus;    
    
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &renderer->context->presentSemaphore;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderer->_renderCompleteSemaphore;
    
    uint32_t nextImage = getNextImage(renderer->context, renderer->context->presentSemaphore);
    
    submitInfo.pCommandBuffers = &renderer->_drawBuffers[nextImage];
    
    vkQueueSubmit(renderer->context->gfxQueue, 1, &submitInfo, renderer->_renderFence);
    
    vkWaitForFences(renderer->context->device, 1, &renderer->_renderFence, VK_TRUE, 0xffffffffffffffff);
    vkResetFences(renderer->context->device, 1, &renderer->_renderFence);

    swapBuffers(renderer->context, nextImage, &renderer->_renderCompleteSemaphore, 1);
}

#endif //RENDERER_H
