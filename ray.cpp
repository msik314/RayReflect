#include <stdio.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <windows.h>

#include "context.h"
#include "debugUtils.h"
#include "renderer.hpp"
#include "uniformBuffer.hpp"

#define MOVE_SPEED 0.001f
#define CAM_SENSITIVITY -0.005f

long int readShaderFromFile(const char* fileName, char** shaderSrc)
{
    FILE* file;
    long int size;
    
    file = fopen(fileName, "rb");
    
    fseek(file, 0, SEEK_END);
    
    size = ftell(file);
    rewind(file);
    
    *shaderSrc = (char*)malloc(size);
    
    fread(*shaderSrc, sizeof(char), size, file);
    fclose(file);
    
    return size;
}

int main(int argc, char** argv)
{
    GLFWwindow* window;
    Context context;
    Renderer renderer;
    char* vert1Src;
    char* frag1Src;
    char* vert2Src;
    char* frag2Src;
    long int vert1Size;
    long int frag1Size;
    long int vert2Size;
    long int frag2Size;
    StandardUniforms uniforms[2];
    float fovy = glm::pi<float>()/2.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    double lastX = 0;
    double xPos, yPos;
    
    /*Apparently, using a lefthanded perspective perspective matrix results in a functional righthanded
      coordinate system and using a righthanded perspective matrix results in z > 1 every time.*/
    glm::mat4 projection = glm::perspectiveFovLH_ZO(fovy, 16.0f, 9.0f, nearClip, farClip);
    
    glm::quat camRot = glm::angleAxis(0.0f, glm::vec3(0.0f, -1.0f, 0.0f));
    
    glfwInit();
    
    int res = createContext(&context);
    ASSERT(res == 0, "Failed to create context");
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);
    
#ifdef NDEBUG
    window = glfwCreateWindow(1280, 720, "Ray", glfwGetPrimaryMonitor(), NULL);
#else //NDEBUG
    window = glfwCreateWindow(1280, 720, "Ray", NULL, NULL);
#endif //NDEBUG
    res = bindWindowContext(&context, window);
    ASSERT(res == 0, "Failed to bind context to window");
    
    res = setupRender(&context);
    ASSERT(res == 0, "Failed to setup render");
    
    res = rendererCreate(&renderer, &context);
    ASSERT(res == 0, "Failed to create render engine");
    
    vert1Size = readShaderFromFile("shaders/mpAttachVert.spv", &vert1Src);
    frag1Size = readShaderFromFile("shaders/mpAttachFrag.spv", &frag1Src);
    vert2Size = readShaderFromFile("shaders/triCastVert.spv", &vert2Src);
    frag2Size = readShaderFromFile("shaders/triCastFrag.spv", &frag2Src);
    
    res = createPipeline(&renderer, 
        {(const char*)vert1Src, (uint32_t)vert1Size}, 
        {(const char*)frag1Src, (uint32_t)frag1Size},
        {(const char*)vert2Src, (uint32_t)vert2Size}, 
        {(const char*)frag2Src, (uint32_t)frag2Size});
    
    ASSERT(res == 0, "Failed to create render pipeline");
    
    free(vert1Src);
    free(frag1Src);
    free(vert2Src);
    free(frag2Src);
    
    createTextureFromFile(&renderer, "res/brick.png", 0);
    updateTexture(&renderer, 0);
    createTextureFromFile(&renderer, "res/spinner.png", 1);
    updateTexture(&renderer, 1);
    
    res = createRenderCommands(&renderer);
    ASSERT(res == 0, "Failed to create render commands");
    
    setCamPos(&renderer, {0, 0, -1});

    glfwGetCursorPos(window, &xPos, &yPos);
    lastX = xPos;
    
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
        
        glm::vec3 camPos = getCamPos(&renderer);
        glm::vec3 movement = {};
        
        
        glfwGetCursorPos(window, &xPos, &yPos);
        
        float diffX = (float)(xPos - lastX);
        lastX = xPos;
        
        camRot *= glm::angleAxis(diffX * CAM_SENSITIVITY, glm::vec3(0, -1, 0));
        
        if(glfwGetKey(window, GLFW_KEY_W))
        {
            movement.z += 1;
        }
        
        if(glfwGetKey(window, GLFW_KEY_S))
        {
            movement.z -= 1;
        }
        
        if(glfwGetKey(window, GLFW_KEY_A))
        {
            movement.x -= 1;
        }
        
        if(glfwGetKey(window, GLFW_KEY_D))
        {
            movement.x += 1;
        }
        
        movement = camRot * movement;
        
        camPos += MOVE_SPEED * movement;
        
        setCamPos(&renderer, camPos);
        
        glm::mat4 camTransform = glm::translate(camPos) * glm::toMat4(camRot);
        glm::mat4 viewMat = glm::inverse(camTransform);
        
        uniforms[0].model = glm::translate(glm::vec3(glm::sin((float)glfwGetTime() * 0.5f), 0.0f, 0.0f));
        uniforms[0].camPos = getCamPos(&renderer);
        
        
        uniforms[0].vp = projection * viewMat;
        uniforms[0].texture = 1;
        
        updateSingleUniform(&renderer, &uniforms[0], 0);
        
        uniforms[1].model = glm::mat4(1.0f);
        uniforms[1].camPos = getCamPos(&renderer);
        
        uniforms[1].vp = projection * viewMat;
        uniforms[1].texture = 0;
        
        updateSingleUniform(&renderer, &uniforms[1], 1);
        
        render(&renderer);
    }
    
    destroyTexture(&renderer, 0);
    destroyTexture(&renderer, 1);
    
    destroyRenderCommands(&renderer);

    destroyPipeline(&renderer);
    
    rendererDestroy(&renderer);
    
    cleanupRender(&context);
    
    unbindWindowContext(&context);
    glfwDestroyWindow(window);
    
    destroyContext(&context);
    glfwTerminate();
    
#ifndef NDEBUG
    system("PAUSE");
#endif //NDEBUG
    
    return 0;
}
