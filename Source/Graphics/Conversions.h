#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <Engine/Transform.h>

#pragma region Assimp Conversions
#include <assimp/types.h>

static glm::vec2 AssimpToGlm(const aiVector2D& aAssimp) {
	return glm::vec2(aAssimp.x, aAssimp.y);
}

static glm::vec3 AssimpToGlm(const aiVector3D& aAssimp) {
	return glm::vec3(aAssimp.x, aAssimp.y, aAssimp.z);
}

static glm::vec4 AssimpToGlm(const aiColor4D& aAssimp) {
	return glm::vec4(aAssimp.r, aAssimp.g, aAssimp.b, aAssimp.a);
}

#pragma endregion

#pragma region Bullet3 Conversions
#include <LinearMath/btTransform.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

static glm::vec3 BulletToGlm(const btVector3& aOther) {
	return glm::vec3(aOther.getX(), aOther.getY(), aOther.getZ());
}

static glm::vec4 BulletToGlm(const btVector4& aOther) {
	return glm::vec4(aOther.getX(), aOther.getY(), aOther.getZ(), aOther.getW());
}

static glm::quat BulletToGlm(const btQuaternion& aOther) {
	return glm::quat(aOther.getW(), aOther.getX(), aOther.getY(), aOther.getZ());
}

static SimpleTransform BulletToGlm(const btTransform& aOther) {
	const glm::vec3 position = BulletToGlm(aOther.getOrigin());
	const glm::quat rotation = BulletToGlm(aOther.getRotation());
	return SimpleTransform(position, glm::vec3(1.0f), rotation);
}

static btVector3 GlmToBullet(const glm::vec3& aOther) {
	return btVector3(aOther.x, aOther.y, aOther.z);
}

static btQuaternion GlmToBullet(const glm::quat& aOther) {
	return btQuaternion(aOther.x, aOther.y, aOther.z, aOther.w);
}

static btTransform TransformLocalToBullet(const SimpleTransform& aOther) {
	return btTransform(GlmToBullet(aOther.GetLocalRotation()), GlmToBullet(aOther.GetLocalPosition()));
}

static btTransform TransformWorldToBullet(Transform& aOther) {
	return btTransform(GlmToBullet(aOther.GetWorldRotation()), GlmToBullet(aOther.GetWorldPosition()));
}
#pragma endregion

#pragma region Vulkan Conversions
#include <vulkan/vulkan.h>

static glm::vec2 VulkanToGlm(const VkExtent2D& aVulkan) {
	return glm::vec2(aVulkan.width, aVulkan.height);
}
#pragma endregion

#pragma region std::Format
#if __cpp_lib_format
#	include <format>

//https://fmt.dev/7.0.2/api.html#formatting-user-defined-types

//enum class color {
//	red,
//	green,
//	blue
//};
//
//template<> struct std::formatter<color> : formatter<string_view> {
//	// parse is inherited from formatter<string_view>.
//	template<typename FormatContext> auto format(color c, FormatContext& ctx) {
//		string_view name = "unknown";
//		switch(c) {
//			case color::red:
//				name = "red";
//				break;
//			case color::green:
//				name = "green";
//				break;
//			case color::blue:
//				name = "blue";
//				break;
//		}
//		return formatter<string_view>::format(name, ctx);
//	}
//};

template<> struct std::formatter<glm::vec3> : std::formatter<std::string_view> {
	// parse is inherited from formatter<string_view>.
	template<typename FormatContext> auto format(const glm::vec3& c, FormatContext& ctx) {
		return format_to(ctx.out(), "({0}, {1}, {2})", c.x, c.y, c.z);
	}
};
#endif
#pragma endregion
