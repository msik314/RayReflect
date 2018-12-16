#include "context.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <GLFW/glfw3.h>

#include "debugUtils.h"

const char* _extensionNames[] = 
{
#ifndef NDEBUG
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif /*NDEBUG*/
};

const char* _deviceExtensions[] = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef NDEBUG
#include <stdio.h>

const char* _layerNames[] = 
{
    "VK_LAYER_LUNARG_standard_validation"
};

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, 
    uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    fprintf(stderr, "%s: %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}

PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback = NULL;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback = NULL;
PFN_vkDebugReportMessageEXT vkDebugReportMessage = NULL;

static inline void loadVulkanExtensions(Context *context)
{
    *(void **)&vkCreateDebugReportCallback = vkGetInstanceProcAddr(context->_instance, "vkCreateDebugReportCallbackEXT");
    *(void **)&vkDestroyDebugReportCallback = vkGetInstanceProcAddr(context->_instance, "vkDestroyDebugReportCallbackEXT");
    *(void **)&vkDebugReportMessage = vkGetInstanceProcAddr(context->_instance, "vkDebugReportMessageEXT");
}

static inline void createDebugCallback(Context* context)
{
    loadVulkanExtensions(context);
    VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
    callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackInfo.pfnCallback = vulkanDebugReportCallback;
    
    VkResult result = vkCreateDebugReportCallback(context->_instance, &callbackInfo, NULL, &context->_callback);
    
    ASSERT(result == VK_SUCCESS, "Failed to create debug callback.");
}

static inline void destroyDebugCallback(Context* context)
{
    vkDestroyDebugReportCallback(context->_instance, context->_callback, NULL);
}

#else /*NDEBUG*/
static inline void createDebugCallback(Context* context){}
static inline void destroyDebugCallback(Context* context){}
#endif /*NDEBUG*/

static inline VkResult createInstance(Context* context)
{
    VkInstanceCreateInfo createInfo = {};
    VkApplicationInfo appInfo = {};
    int32_t glfwExtensionCount;
    VkResult result;
    uint32_t extensionCount = sizeof(_extensionNames)/sizeof(const char*);
    
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    uint32_t numRequiredExtensions = glfwExtensionCount + extensionCount;
    
    if(!glfwExtensions) return -1;
    
    char* extensions[glfwExtensionCount + extensionCount];
    memcpy(extensions, glfwExtensions, glfwExtensionCount * sizeof(char*));
    memcpy(extensions + glfwExtensionCount, _extensionNames, sizeof(_extensionNames));
    
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = numRequiredExtensions;
    createInfo.ppEnabledExtensionNames = (const char**)extensions;
    
#ifndef NDEBUG
    createInfo.enabledLayerCount = sizeof(_layerNames)/sizeof(const char*);
    createInfo.ppEnabledLayerNames = _layerNames;
#endif /*NDEBUG*/
    
    uint32_t supportedExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, NULL);
    VkExtensionProperties extensionsAvailable[supportedExtensionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, extensionsAvailable);

    uint32_t foundExtensions = 0;
    for(uint32_t i = 0; i < supportedExtensionCount; ++i)
    {
        for(int j = 0; j < numRequiredExtensions; ++j)
        {
            if(strcmp(extensionsAvailable[i].extensionName, extensions[j])== 0)
            {
                foundExtensions++;
                break;
            }
        }
    }
    
    ASSERT(foundExtensions == numRequiredExtensions, "Could not find required extensions");
    if(foundExtensions != numRequiredExtensions) return VK_ERROR_EXTENSION_NOT_PRESENT;
    
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Ray";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    
    return vkCreateInstance(&createInfo, NULL, &context->_instance);
}

static inline int32_t pickPhysicalDevice(VkPhysicalDevice* devices, int32_t numDevices)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    uint64_t deviceMemory = 0;
    int isDiscrete = 0;
    int32_t physicalDeviceIndex = -1;
    
    for(int i = 0; i < numDevices; ++i)
    {
        bool hasGraphics = false;
        bool hasTransfer = false;
        bool hasCompute = false;
        uint32_t numQueueFamilies = 0;
        VkBool32 supportsPresent;
        
        
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            isDiscrete = 1;
        }
        else if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            if(isDiscrete) continue;
        }
        else
        {
            continue;
        }
        
        
        
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &numQueueFamilies, NULL);
        VkQueueFamilyProperties queueFamilyProperties[numQueueFamilies];
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &numQueueFamilies, queueFamilyProperties);
        
        for(int j = 0; j < numQueueFamilies; ++j)
        {
            if(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) hasGraphics = true;
            if(queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) hasCompute = true;
            if(queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) hasTransfer = true;
            if(hasCompute && hasCompute && hasTransfer) break;
        }
        
        if(!hasCompute || !hasCompute || !hasTransfer) continue;
        
        vkGetPhysicalDeviceMemoryProperties(devices[i], &memoryProperties);
        uint64_t totalMem = 0;
        for(int j = 0; j < memoryProperties.memoryHeapCount; ++j)
        {
            if(memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                totalMem += memoryProperties.memoryHeaps[j].size;
            }
        }
        
        if(totalMem >= deviceMemory)
        {
            deviceMemory = totalMem;
            physicalDeviceIndex = i;
        }
    }
    
    return physicalDeviceIndex;
}

static inline void pickQueueFamilies(Context* context)
{
    uint32_t numQueueFamilies;
    
    vkGetPhysicalDeviceQueueFamilyProperties(context->_physicalDevice, &numQueueFamilies, NULL);
    VkQueueFamilyProperties queueFamilyProperties[numQueueFamilies];
    vkGetPhysicalDeviceQueueFamilyProperties(context->_physicalDevice, &numQueueFamilies, queueFamilyProperties);
    
    context->_queueFamiliesValue = 0;
    
    for(uint8_t i = 0; i < numQueueFamilies; ++i)
    {
        if(!(queueFamilyProperties[i].queueFlags  ^ VK_QUEUE_COMPUTE_BIT))
        {
            context->_cmpFamily = i;
            context->_numFamilies += 1;
            break;
        }
    }
    
    for(uint8_t i = 0; i < numQueueFamilies; ++i)
    {
        if(!(queueFamilyProperties[i].queueFlags  ^ VK_QUEUE_TRANSFER_BIT))
        {
            context->_tfrFamily = i;
            context->_numFamilies += 1;
            break;
        }
    }
    
    for(uint8_t i = 0; i < numQueueFamilies; ++i)
    {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) context->_gfxFamily = i;
        if(!context->_cmpFamily && queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) context->_cmpFamily = i;
        if(!context->_tfrFamily && queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) context->_tfrFamily = i;
        if(context->_gfxFamily && context->_cmpFamily && context->_tfrFamily) break;
    }
}

static inline VkResult createDevice(Context* context)
{
    uint32_t numPhysicalDevices;
    int32_t physicalDeviceIndex;
    VkDeviceCreateInfo deviceInfo = {};
    VkPhysicalDeviceFeatures features = {};
    VkDeviceQueueCreateInfo queueInfos[3] = {};
    VkResult result;
    float queuePriorities[3] = {1.0f, 1.0f, 1.0f};
    int32_t queueIndices[3] = {};
    
    
    
    vkEnumeratePhysicalDevices(context->_instance, &numPhysicalDevices, NULL);
    VkPhysicalDevice devices[numPhysicalDevices];
    vkEnumeratePhysicalDevices(context->_instance, &numPhysicalDevices, devices);
    
    physicalDeviceIndex = pickPhysicalDevice(devices, numPhysicalDevices);
    
    if(physicalDeviceIndex < 0)
    {
        return -1;
    }
    
    vkGetPhysicalDeviceProperties(devices[physicalDeviceIndex], &context->_physicalDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(devices[physicalDeviceIndex], &context->physicalDeviceMemory);
    vkGetPhysicalDeviceFeatures(devices[physicalDeviceIndex], &context->_physicalDeviceFeatures);
    context->_physicalDevice = devices[physicalDeviceIndex];
    
    DEBUG_PRINT("GPU selected: %d\nIs discrete GPU: %d\n", physicalDeviceIndex, context->_physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    
    pickQueueFamilies(context);
    
    DEBUG_PRINT("Graphics queue family: %d\nCompute queue family: %d\nTransfer queue family: %d\n", (int)(context->_gfxFamily), (int)(context->_cmpFamily), (int)(context->_tfrFamily));
    
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = queueInfos;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = _deviceExtensions;
    features.multiDrawIndirect = VK_TRUE;
    features.drawIndirectFirstInstance = VK_TRUE;
    features.vertexPipelineStoresAndAtomics = VK_TRUE;
    features.fragmentStoresAndAtomics = VK_TRUE;
    deviceInfo.pEnabledFeatures = &features;
    
    features.shaderClipDistance = VK_TRUE;
    
    queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfos[0].queueFamilyIndex = context->_gfxFamily;
    queueInfos[0].queueCount = 1;
    queueInfos[0].pQueuePriorities = queuePriorities;
    
    
    uint8_t queueCreateIndex = 1;
    for(uint8_t i = 1; i < 3; ++i)
    {
        if(context->_queueFamilies[i] == context->_gfxFamily)
        {
            queueInfos[0].queueCount += 1;
            continue;
        }
        
        queueInfos[queueCreateIndex].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[queueCreateIndex].queueFamilyIndex = context->_queueFamilies[i];
        queueInfos[queueCreateIndex].queueCount = 1;
        queueInfos[queueCreateIndex].pQueuePriorities = queuePriorities;
        deviceInfo.queueCreateInfoCount += 1;
        ++queueCreateIndex;
    }
    
    result = vkCreateDevice(context->_physicalDevice, &deviceInfo, NULL, &context->device);
    
    if(result == VK_SUCCESS)
    {
        vkGetDeviceQueue(context->device, context->_gfxFamily, queueIndices[context->_gfxFamily]++, &context->gfxQueue);
        vkGetDeviceQueue(context->device, context->_tfrFamily, queueIndices[context->_tfrFamily]++, &context->tfrQueue);
        vkGetDeviceQueue(context->device, context->_cmpFamily, queueIndices[context->_cmpFamily]++, &context->cmpQueue);
    }
    
    return result;
}

int32_t createContext(Context* context)
{
    if(createInstance(context) != VK_SUCCESS) return -1;
    createDebugCallback(context);
    if(createDevice(context) != VK_SUCCESS) return -2;
    
    return 0;
}

int32_t bindWindowContext(Context* context, void* window)
{
    uint32_t numFormats;
    int32_t formatIndex = -1;
    uint32_t numPresentModes;
    VkBool32 surfaceSupported;
    VkPresentModeKHR presentMode;
    VkColorSpaceKHR colorSpace;
    VkExtent2D surfaceResolution;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    VkSwapchainCreateInfoKHR swapchainInfo = {};
    
    glfwGetWindowSize((GLFWwindow*)window, &context->width, &context->height);
    
    if(glfwCreateWindowSurface(context->_instance, (GLFWwindow*)window, NULL, &context->_surface) != VK_SUCCESS) return -1;
    
    vkGetPhysicalDeviceSurfaceSupportKHR(context->_physicalDevice, (uint32_t)(context->_gfxFamily), context->_surface, &surfaceSupported);
    
    if(!surfaceSupported) return -2;
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->_physicalDevice, context->_surface, &numFormats, NULL);
    VkSurfaceFormatKHR supportedFormats[numFormats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->_physicalDevice, context->_surface, &numFormats, supportedFormats);
    
    if(numFormats == 1 && supportedFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        context->colorFormat = VK_FORMAT_B8G8R8_UNORM;
        colorSpace = supportedFormats[0].colorSpace;
        formatIndex = 0;
    }
    
    if(formatIndex < 0)
    {
        for(int i = 0; i < numFormats; ++i)
        {
            if(supportedFormats[i].format == VK_FORMAT_B8G8R8_UNORM || supportedFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                context->colorFormat = supportedFormats[i].format;
                colorSpace = supportedFormats[i].colorSpace;
                formatIndex = i;
            }
        }
    }
    
    if(formatIndex < 0)
    {
        context->colorFormat = supportedFormats[0].format;
        colorSpace = supportedFormats[0].colorSpace;
        formatIndex = 0;
    }
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->_physicalDevice, context->_surface, &surfaceCapabilities);
    
    uint32_t numDesiredImages = 2;
    if(numDesiredImages < surfaceCapabilities.minImageCount)
    {
        numDesiredImages = surfaceCapabilities.minImageCount;
    }
    else if(surfaceCapabilities.maxImageCount != 0 && numDesiredImages > surfaceCapabilities.maxImageCount)
    {
        numDesiredImages = surfaceCapabilities.maxImageCount;
    }

    surfaceResolution = surfaceCapabilities.currentExtent;
    if(surfaceResolution.width == -1)
    {
        surfaceResolution.width = context->width;
        surfaceResolution.height = context->height;
    }
    else
    {
        context->width = surfaceResolution.width;
        context->height = surfaceResolution.height;
    }

    preTransform = surfaceCapabilities.currentTransform;
    if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->_physicalDevice, context->_surface, &numPresentModes, NULL);
    VkPresentModeKHR presentModes[numPresentModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->_physicalDevice, context->_surface, &numPresentModes, presentModes);
    
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    
    for(int i = 0; i < numPresentModes; ++i)
    {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = context->_surface;
    swapchainInfo.minImageCount = numDesiredImages;
    swapchainInfo.imageFormat = context->colorFormat;
    swapchainInfo.imageColorSpace = colorSpace;
    swapchainInfo.imageExtent = surfaceResolution;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = preTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = true;
    
    if(vkCreateSwapchainKHR(context->device, &swapchainInfo, NULL, &context->_swapchain) != VK_SUCCESS) return -3;
    return 0;
}

int32_t setupRender(Context* context)
{
    VkFence submitFence;
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    VkCommandBufferAllocateInfo cmdBufferInfo = {};
    VkImageViewCreateInfo viewInfo = {};
    VkFenceCreateInfo fenceInfo = {};
    VkCommandBufferBeginInfo beginInfo = {};
    VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {};
    VkSemaphoreCreateInfo semaphoreInfo = {};
    
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = context->_gfxFamily;
    
    if(vkCreateCommandPool(context->device, &cmdPoolInfo, NULL, &context->gfxCmdPool) != VK_SUCCESS) return -1;
    
    cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferInfo.commandPool = context->gfxCmdPool;
    cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount = 1;
    
    if(vkAllocateCommandBuffers(context->device, &cmdBufferInfo, &context->setupCmdBuffer) != VK_SUCCESS) return -2;
    
    vkGetSwapchainImagesKHR(context->device, context->_swapchain, &context->numImages, NULL);
    context->presentImages = (VkImage*)malloc(context->numImages * (sizeof(VkImage) + sizeof(VkImageView)));
    context->presentViews = (VkImageView*)((uint8_t*)(context->presentImages) + context->numImages * sizeof(VkImage));
    vkGetSwapchainImagesKHR(context->device, context->_swapchain, &context->numImages, context->presentImages);
    
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = context->colorFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(context->device, &fenceInfo, NULL, &submitFence);
    
    for(int i = 0; i < context->numImages; ++i)
    {
        viewInfo.image = context->presentImages[i];
        
        vkBeginCommandBuffer(context->setupCmdBuffer, &beginInfo);
        {
            VkImageMemoryBarrier transitionBarrier = {};
            transitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            transitionBarrier.srcAccessMask = 0; 
            transitionBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            transitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            transitionBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            transitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transitionBarrier.image = context->presentImages[i];
            VkImageSubresourceRange resourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            transitionBarrier.subresourceRange = resourceRange;

            vkCmdPipelineBarrier(context->setupCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &transitionBarrier);

        }
        vkEndCommandBuffer(context->setupCmdBuffer);
        
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = NULL;
        submitInfo.pWaitDstStageMask = waitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &context->setupCmdBuffer;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = NULL;
        vkQueueSubmit(context->gfxQueue, 1, &submitInfo, submitFence);
        
        vkWaitForFences(context->device, 1, &submitFence, VK_TRUE, UINT64_MAX);
        vkResetFences(context->device, 1, &submitFence);
        vkResetCommandBuffer(context->setupCmdBuffer, 0);
        if(vkCreateImageView(context->device, &viewInfo, NULL, &context->presentViews[i]) != VK_SUCCESS) return -3;
    }
    
    vkDestroyFence(context->device, submitFence, NULL);
    
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(context->device, &semaphoreInfo, NULL, &context->presentSemaphore);
    
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = context->_tfrFamily;
    
    if(vkCreateCommandPool(context->device, &cmdPoolInfo, NULL, &context->tfrCmdPool) != VK_SUCCESS) return -4;
    
    cmdBufferInfo.commandPool = context->tfrCmdPool;
    cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount = 1;
    
    if(vkAllocateCommandBuffers(context->device, &cmdBufferInfo, &context->transferCmdBuffer) != VK_SUCCESS) return -5;
    
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = context->_cmpFamily;
    
    if(vkCreateCommandPool(context->device, &cmdPoolInfo, NULL, &context->cmpCmdPool) != VK_SUCCESS) return -6;
    
    return 0;
}

void cleanupRender(Context* context)
{
    waitIdle(context);
    
    vkDestroySemaphore(context->device, context->presentSemaphore, NULL);
    
    vkFreeCommandBuffers(context->device, context->gfxCmdPool, 1, &context->setupCmdBuffer);
    vkDestroyCommandPool(context->device, context->gfxCmdPool, NULL);
    
    vkFreeCommandBuffers(context->device, context->tfrCmdPool, 1, &context->transferCmdBuffer);
    vkDestroyCommandPool(context->device, context->tfrCmdPool, NULL);
    
    vkDestroyCommandPool(context->device, context->cmpCmdPool, NULL);
    
    for(int i = 0; i < context->numImages; ++i)
    {
        vkDestroyImageView(context->device, context->presentViews[i], NULL);
    }
    
    free(context->presentImages);
}

void unbindWindowContext(Context* context)
{
    waitIdle(context);
    vkDestroySwapchainKHR(context->device, context->_swapchain, NULL);
    vkDestroySurfaceKHR(context->_instance, context->_surface, NULL);
}

void destroyContext(Context* context)
{
    waitIdle(context);
    vkDestroyDevice(context->device, NULL);    
    destroyDebugCallback(context);
    vkDestroyInstance(context->_instance, NULL);
}
