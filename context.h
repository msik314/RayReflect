#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C"
{
#else /*__cplusplus*/
#include <stdbool.h>    
#endif /*__cplusplus*/

/*Do not modify any of the values in here.  Values not prefixed with an underscore may be read externally*/
typedef struct
{
    uint32_t width;
    uint32_t height;
    
    union
    {
        struct
        {
            uint8_t _gfxFamily;
            uint8_t _tfrFamily;
            uint8_t _cmpFamily;
            uint8_t _numFamilies;
        } __attribute((packed));
        uint8_t _queueFamilies[4];
        uint32_t _queueFamiliesValue;
    };
    
    VkQueue gfxQueue;
    VkQueue tfrQueue;
    VkQueue cmpQueue;
    
    VkCommandPool gfxCmdPool;
    VkCommandPool tfrCmdPool;
    VkCommandPool cmpCmdPool;
    
    VkCommandBuffer setupCmdBuffer;
    VkCommandBuffer transferCmdBuffer;
    VkCommandBuffer computeCmdBuffer;
    
    VkInstance _instance;
    VkDevice device;
    VkPhysicalDevice _physicalDevice;
    VkPhysicalDeviceProperties _physicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties physicalDeviceMemory;
    VkPhysicalDeviceFeatures _physicalDeviceFeatures;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat colorFormat;
    
    VkSemaphore presentSemaphore;
    
    VkImage* presentImages;
    VkImageView* presentViews;
    uint32_t numImages;
#ifndef NDEBUG
    VkDebugReportCallbackEXT _callback;
#endif /*NDEUBG*/
}
Context;

int32_t createContext(Context* context);
int32_t bindWindowContext(Context* context, void* window);
int32_t setupRender(Context* context);

void cleanupRender(Context* context);
void unbindWindowContext(Context* context);
void destroyContext(Context* context);

static inline uint32_t getFamilies(Context* context, uint32_t* families)
{
    families[0] = context->_gfxFamily;
    families[1] = context->_tfrFamily == context->_gfxFamily ? context->_cmpFamily : context->_tfrFamily;
    families[2] = context->_cmpFamily;
    
    return context->_numFamilies;
}

static inline void waitIdle(Context* context){vkDeviceWaitIdle(context->device);}

static inline uint32_t getNextImage(Context* context, VkSemaphore semaphore)
{
    uint32_t nextImage;
    vkAcquireNextImageKHR(context->device, context->_swapchain, 0xffffffffffffffff, semaphore, VK_NULL_HANDLE, &nextImage);
    
    return nextImage;
}

static inline void swapBuffers(Context* context, uint32_t nextImage, VkSemaphore* waitSemaphores, uint32_t numWaitSemaphores)
{
    VkPresentInfoKHR presentInfo = {};    
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.waitSemaphoreCount = numWaitSemaphores;
    presentInfo.pWaitSemaphores = waitSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &context->_swapchain;
    presentInfo.pImageIndices = &nextImage;
    presentInfo.pResults = NULL;
    vkQueuePresentKHR(context->gfxQueue, &presentInfo);
}

static inline uint32_t getAtomSize(Context* context)
{
    return context->_physicalDeviceProperties.limits.nonCoherentAtomSize;
}

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*CONTEXT_H*/
