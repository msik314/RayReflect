#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inColor;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput posNY;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput nXZ;

layout (set = 0, binding = 3) uniform CamPos
{
    vec3 cameraPosition;
};

layout(location = 0)out vec4 color;

void main()
{
    vec4 pos = subpassLoad(posNY);
    vec2 norm = subpassLoad(nXZ).xy;
    vec3 l = normalize(cameraPosition.xyz - pos.xyz);
    float nDotL = max(dot(vec3(norm.x, pos.w, norm.y), l), 0.0f);
    color = nDotL * subpassLoad(inColor);
}
