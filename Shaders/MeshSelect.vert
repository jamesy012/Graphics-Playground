#version 450 core

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_multiview : enable

#include "test.inc"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inUV;

//todo move this into combined include with a #if SHADER_VERT
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outFragPos;
layout(location = 3) out vec3 outNormal;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 fragPos = pc.mWorld * vec4(inPos, 1.0f);
    outFragPos = vec3(fragPos);
    outNormal = mat3(transpose(inverse(pc.mWorld))) * inNormal;
    outColor = inColor * 10;
    outUV = inUV;
    gl_Position = sceneData.mViewProj[gl_ViewIndex] * fragPos;
    //gl_Position *= vec4(vec2(1.4f), 1.0f, 1.0f);
    //gl_Position -= vec4(vec2(0.2f), 0.0f, 0.0f);
    //gl_Position *= vec4(1.4f, 1.0f, 1.4f, 1.0f);
    //gl_Position -= vec4(0.2f, 0.0f, 0.2f, 0.0f);
}