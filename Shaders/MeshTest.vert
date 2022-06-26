#version 450 core

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_multiview : enable

#include "test.inc"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out struct {
    vec4 Color;
    vec2 UV;
} Out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Out.Color = inColor;
    Out.UV = inUV;
    gl_Position = sceneData.viewProj[gl_ViewIndex] * pc.world  * vec4(inPos,1);
}