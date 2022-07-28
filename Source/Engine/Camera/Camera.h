#pragma once

#include <glm/glm.hpp>

#include "Engine/Transform.h"

class Camera {
public:
	Camera();

	virtual void Update() = 0;

	void SetFov(float aLeft, float aRight, float aUp, float aDown) {
		mFov = glm::tan(glm::vec4(aLeft, aRight, aUp, aDown));
		mProjDirty = true;
	}
	void SetFov(float aFovYDegrees, float aAspect) {
		mFov = glm::vec4(glm::radians(aFovYDegrees), aAspect, 0, 0);
		mProjDirty = true;
	}
	void SetNearFar(float aNear, float aFar) {
		mNearFar = glm::vec2(aNear, aFar);
		mProjDirty = true;
	}

	glm::vec3 GetWorldDirFromScreen(const glm::vec2& aScreenpos, const glm::vec2& aScreenSize);

	float GetFovDegrees() {
		//todo get degrees of uneven fov?
		ASSERT(mFov.z == 0 && mFov.w == 0);
		return glm::degrees(mFov.x);
	}

	glm::mat4 GetViewMatrix() {
		mTransform.CheckUpdate();
		return mViewMatrix;
	}
	glm::mat4 GetProjMatrix() {
		UpdateProjMatrix();
		return mProjMatrix;
	}
	glm::mat4 GetViewProjMatrix() {
		mTransform.CheckUpdate();
		return mViewProjMatrix;
	}

	Transform& GetTransform() {
		return mTransform;
	}

	Transform mTransform;

protected:
	void TranformUpdated(Transform* aTransform);

private:
	void UpdateProjMatrix();

	glm::mat4 mViewMatrix;
	glm::mat4 mProjMatrix;
	glm::mat4 mViewProjMatrix;

	glm::vec4 mFov;
	glm::vec2 mNearFar;

	bool mProjDirty = true;
};