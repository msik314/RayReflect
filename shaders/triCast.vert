#version 450
#extension GL_ARB_separate_shader_objects : enable

const vec4 vertices[3] =
{
    vec4(-1, -1, 1, 1),
    vec4(-1, 3, 1, 1),
    vec4(3, -1, 1, 1)
};

void main()
{
    gl_Position = vertices[gl_VertexIndex];
}
