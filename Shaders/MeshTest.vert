#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "test.inc"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out struct {
    vec2 UV;
} Out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Out.UV = inUV;
    gl_Position = sceneData.viewProj[0] * pc.world  * vec4(inPos,1);
}