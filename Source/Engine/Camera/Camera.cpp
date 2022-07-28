#include "Camera.h"

Camera::Camera() {
	mTransform.SetUpdateCallback(std::bind(&Camera::TranformUpdated, this, &mTransform));
	SetNearFar(0.1f, 10000.0f);
	SetFov(60, 1.0f);
}

glm::vec3 Camera::GetWorldDirFromScreen(const glm::vec2& aScreenpos, const glm::vec2& aScreenSize) {
	const glm::mat4 viewProjInv = glm::inverse(GetViewProjMatrix());

	const glm::vec2 screenPos = (aScreenpos / aScreenSize) * 2.0f - 1.0f;

	const glm::vec4 posInvNearV4 = viewProjInv * glm::vec4(screenPos, 0.0f, 1.0f);
	const glm::vec4 posInvFarV4 = viewProjInv * glm::vec4(screenPos, 1.0f, 1.0f);
	const glm::vec3 posInvNear = glm::vec3(posInvNearV4) / posInvNearV4.w;
	const glm::vec3 posInvFar = glm::vec3(posInvFarV4) / posInvFarV4.w;
	return glm::normalize(posInvFar - posInvNear);
}

void Camera::TranformUpdated(Transform* aTransform) {
	mViewMatrix = glm::inverse(mTransform.GetWorldMatrix());
	//mViewMatrix = mTransform.GetWorldMatrix();

	mViewProjMatrix = GetProjMatrix() * mViewMatrix;
}

void Camera::UpdateProjMatrix() {
	if(mProjDirty) {
		//zw is not set for perspective
		if(mFov.z == 0 && mFov.w == 0) {
			mProjMatrix = glm::perspective(mFov.x, mFov.y, mNearFar.x, mNearFar.y);
		} else {
			float scale = mNearFar.x;
			glm::vec4 fov = mFov * scale;
			mProjMatrix = glm::frustum(fov.x, fov.y, fov.z, fov.w, mNearFar.x, mNearFar.y);
		}

		//flip scene, vulkan only?
		mProjMatrix[1][1] *= -1.0f;

		mProjDirty = false;
	}
}