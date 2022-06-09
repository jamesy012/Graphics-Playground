#pragma once

#include <glm/vec3.hpp>

struct AABB {
public:
	glm::vec3 mMin;
	glm::vec3 mMax;

	AABB() : mMin(glm::vec3(999)), mMax(glm::vec3(-999)) {};
	AABB(const glm::vec3 aMin, const glm::vec3 aMax) : mMin(aMin), mMax(aMax) {};
	AABB(const glm::vec3& aMin, const glm::vec3& aMax) : mMin(aMin), mMax(aMax) {};

	void Expand(const glm::vec3& aMin, const glm::vec3& aMax) {
#define EXPAND_MIN(val)       \
	if(mMin.val > aMin.val) { \
		mMin.val = aMin.val;  \
	}

#define EXPAND_MAX(val)       \
	if(mMax.val < aMax.val) { \
		mMax.val = aMax.val;  \
	}

		EXPAND_MIN(x);
		EXPAND_MIN(y);
		EXPAND_MIN(z);
		EXPAND_MAX(x);
		EXPAND_MAX(y);
		EXPAND_MAX(z);
	}
	//void Expand(const glm::vec3 aMin, const glm::vec3 aMax) {
	//	Expand(aMin, aMax);
	//}
	void Expand(const AABB& aAABB) {
		Expand(aAABB.mMin, aAABB.mMax);
	}

private:
};