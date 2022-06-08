#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <assimp/types.h>

glm::vec2 AssimpToGlm(aiVector2D& aAssimp) {
    return glm::vec2(aAssimp.x, aAssimp.y);
}

glm::vec3 AssimpToGlm(aiVector3D& aAssimp) {
    return glm::vec3(aAssimp.x, aAssimp.y, aAssimp.z);
}

glm::vec4 AssimpToGlm(aiColor4D& aAssimp) {
    return glm::vec4(aAssimp.r, aAssimp.g, aAssimp.b, aAssimp.a);
}