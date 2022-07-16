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
   outFragPos = vec3(pc.mWorld * vec4(inPos, 1.0f));
   outNormal = mat3(transpose(inverse(pc.mWorld))) * inNormal;  
   outColor = inColor;
   outUV = inUV;
   gl_Position = sceneData.mViewProj[gl_ViewIndex] * vec4(outFragPos,1);
}