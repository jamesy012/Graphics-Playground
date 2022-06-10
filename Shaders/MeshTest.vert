#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(push_constant) uniform uPushConstant {
    mat4 world;
} pc;

layout(set = 0, binding = 0) uniform SceneBuffer{   
	mat4 viewProj; 
} sceneData;


layout(location = 0) out struct {
    vec2 UV;
} Out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Out.UV = inUV;
    gl_Position = sceneData.viewProj * pc.world  * vec4(inPos,1);
}