#include "FlyCamera.h"

Camera::Camera() {
	mTransform.SetUpdateCallback(std::bind(&Camera::TranformUpdated, this, &mTransform));
    SetNearFar(0.1f, 100.0f);
    SetFov(60, 1.0f);
}

void Camera::TranformUpdated(Transform* aTransform) {
	mViewMatrix = glm::inverse(mTransform.GetWorldMatrix());
	//mViewMatrix = mTransform.GetWorldMatrix();

	mViewProjMatrix =  GetProjMatrix() * mViewMatrix;
}

void Camera::UpdateProjMatrix() {
	if(mProjDirty) {
        //zw is not set for perspective 
		if(mFov.z == 0 && mFov.w == 0) {
			mProjMatrix = glm::perspective(mFov.x, mFov.y, mNearFar.x, mNearFar.y);
		} else {
			float scale	  = mNearFar.x;
			glm::vec4 fov = mFov * scale;
			mProjMatrix	  = glm::frustum(fov.x, fov.y, fov.z, fov.w, mNearFar.x, mNearFar.y);
		}

        //flip scene, vulkan only?
		mProjMatrix[1][1] *= -1.0f;

		mProjDirty = false;
	}
}