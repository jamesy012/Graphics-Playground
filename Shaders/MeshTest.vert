#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

layout(push_constant) uniform uPushConstant {
    mat4 uProjViewWorld;
} pc;

layout(location = 0) out struct {
    vec2 UV;
} Out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Out.UV = aUV;
    gl_Position = pc.uProjViewWorld * vec4(aPos,1);
}