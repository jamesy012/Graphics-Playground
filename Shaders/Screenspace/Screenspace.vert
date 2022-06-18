#version 450 
//https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/


layout (location = 0) out vec3 outUV;

layout(push_constant) uniform uPushConstant {
    float arrayImage;
} pc;


void main() 
{
    vec2 location = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    outUV = vec3(location, pc.arrayImage);
    gl_Position = vec4(location * 2.0f + -1.0f, 0.0f, 1.0f);
}