#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <assimp/types.h>

#pragma region conversions
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

#pragma region std::Format
#if __cpp_lib_format
#include <format>

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
