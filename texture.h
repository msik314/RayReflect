#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "context.h"

#ifdef __cplusplus
extern "C"
{ 
#endif /*__cplusplus*/

#define MAX_TEXTURES 8

typedef struct
{
    VkImage _image;
    VkImageView _view;
    VkDeviceMemory _memory;
    
    VkSampler sampler;
}
Texture;

int32_t textureCreate(Texture* texture, Context* context, void* data, int width, int height);
int32_t textureCreateFromFile(Texture* texture, Context* context, const char* file);

void textureUpdateDescriptor(Texture* texture, Context* context, VkDescriptorSet descriptorSet, uint32_t index, uint32_t baseIndex);

void textureDestroy(Texture* texture, Context* context);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*TEXTURE_H*/
