#include "Transform.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "PlatformDebug.h"

void SimpleTransform::SetMatrix(const glm::mat4& aMat) {
	//possibly wrong?
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(aMat, scale, rotation, translation, skew, perspective);
	SetPosition(translation);
	SetScale(scale);
	//SetRotation(glm::conjugate(rotation));
	SetRotation(rotation);
}

void SimpleTransform::TranslateLocal(const glm::vec3& aTranslation) {
	mPos += mRot * aTranslation;
	SetDirty();
}

void SimpleTransform::RotateAxis(const float aAmount, const glm::vec3& aAxis) {
	const float amount = glm::radians(aAmount);
	mRot = mRot * glm::angleAxis(amount, aAxis);

	SetDirty();
}

void SimpleTransform::RotateAxis(const glm::vec2& aEulerAxisRotation) {
	const glm::vec2 axisRotation = glm::radians(aEulerAxisRotation);

	mRot = mRot * glm::angleAxis(axisRotation.x, glm::vec3(1, 0, 0));
	mRot = glm::angleAxis(axisRotation.y, glm::vec3(0, 1, 0)) * mRot;

	SetDirty();
}

void SimpleTransform::RotateAxis(const glm::vec3& aEulerAxisRotation) {
	const glm::vec3 axisRotation = glm::radians(aEulerAxisRotation);

	mRot = mRot * glm::angleAxis(axisRotation.x, glm::vec3(1, 0, 0));
	mRot = glm::angleAxis(axisRotation.y, glm::vec3(0, 1, 0)) * mRot;
	mRot = mRot * glm::angleAxis(axisRotation.z, glm::vec3(0, 0, 1));

	SetDirty();
}

void SimpleTransform::SetLookAt(const glm::vec3& aPos, const glm::vec3& aLookAt, const glm::vec3& aUp) {
	glm::mat4 lookAt = glm::lookAt(aPos, aLookAt, aUp);

	SetRotation(glm::quat(glm::inverse(lookAt)));
	SetPosition(aPos);
}

void SimpleTransform::Rotate(const glm::quat& aRotation) {
	mRot *= aRotation;
	SetDirty();
}

void SimpleTransform::CreateLocalMatrix(glm::mat4& aMatrix) const {
	aMatrix = glm::identity<glm::mat4>();

	aMatrix = glm::translate(mLocalMatrix, mPos);
	aMatrix *= glm::mat4_cast(mRot);
	aMatrix = glm::scale(mLocalMatrix, mScale);
}

void SimpleTransform::UpdateMatrix() {
	CreateLocalMatrix(mLocalMatrix);

	mDirty = false;

	mUpdateCallback.Call(this);
}

Transform::~Transform() {
	//reparent?
	ASSERT(mChildren.size() == 0);

	if(mParent != nullptr) {
		mParent->RemoveChild(this);
		mParent = nullptr;
	}
}

void Transform::Clear(bool aReparent /* = true*/) {
	Transform* newChildParent = aReparent ? mParent : nullptr;

	const int childCount = mChildren.size();
	for(int i = 0; i < childCount; i++) {
		mChildren[0]->SetParent(newChildParent);
	}

	if(mParent != nullptr) {
		mParent->RemoveChild(this);
		mParent = nullptr;
	}
}

void Transform::SetWorldPosition(const glm::vec3& aPos) {
	if(mParent) {
		//skip out matrix
		mPos = glm::inverse(mParent->GetWorldMatrix()) * glm::vec4(aPos, 1);
		SetDirty();
	} else {
		SetPosition(aPos);
	}
}

void Transform::SetWorldScale(const glm::vec3& aScale) {
	if(mParent) {
		//todo
		ASSERT(false);
	} else {
		SetScale(aScale);
	}
}

void Transform::SetWorldRotation(const glm::quat& aRot) {
	if(mParent) {
		mRot = glm::inverse(mParent->GetWorldMatrix()) * glm::mat4_cast(aRot);
		SetDirty();
	} else {
		SetRotation(aRot);
	}
}

glm::vec3 Transform::GetWorldPosition() {
	CheckUpdate();
	return glm::vec3(mWorldMatrix[3]);
}

glm::vec3 Transform::GetWorldScale() {
	CheckUpdate();
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(mWorldMatrix, scale, rotation, translation, skew, perspective);
	return scale;
	//https://stackoverflow.com/a/68323550
	//glm::vec3 scale;
	//scale[0] = glm::length(glm::vec3(mWorldMatrix[0]));
	//scale[1] = glm::length(glm::vec3(mWorldMatrix[1]));
	//scale[2] = glm::length(glm::vec3(mWorldMatrix[2]));
	//return scale;
}

glm::quat Transform::GetWorldRotation() {
	CheckUpdate();
	//glm::vec3 scale;
	//glm::quat rotation;
	//glm::vec3 translation;
	//glm::vec3 skew;
	//glm::vec4 perspective;
	//glm::decompose(mWorldMatrix, scale, rotation, translation, skew, perspective);
	//return glm::conjugate(rotation);
	//https://stackoverflow.com/a/68323550
	const glm::vec3 scale = GetWorldScale();
	const glm::mat3 rotMtx(glm::vec3(mWorldMatrix[0]) / scale[0], glm::vec3(mWorldMatrix[1]) / scale[1], glm::vec3(mWorldMatrix[2]) / scale[2]);
	return glm::quat_cast(rotMtx);
}

bool Transform::IsChild(const Transform* aChild) const {
	for(const Transform* child: mChildren) {
		if(child == aChild) {
			return true;
		}
	}
	return false;
}

void Transform::UpdateMatrix() {
	CreateLocalMatrix(mLocalMatrix);

	if(mParent != nullptr) {
		//recursive Get world of parent till we reach the top
		mWorldMatrix = mParent->GetWorldMatrix() * mLocalMatrix;
	} else {
		mWorldMatrix = mLocalMatrix;
	}

	mDirty = false;

	mUpdateCallback.Call(this);
}

void Transform::AddChild(Transform* aChild) {
	//debug only check?
	int count = std::count(mChildren.begin(), mChildren.end(), aChild);
	ASSERT(count == 0);
	if(count != 0) {
		return;
	}

	mChildren.push_back(aChild);
	aChild->SetDirty();
}

void Transform::RemoveChild(Transform* aChild) {

	int count = std::count(mChildren.begin(), mChildren.end(), aChild);
	ASSERT(count == 1);

	const int childCount = mChildren.size();
	for(int i = 0; i < childCount; ++i) {
		Transform* child = mChildren[i];
		if(child == aChild) {
			child->mParent = nullptr;
			child->SetDirty();
			mChildren.erase(mChildren.begin() + i);
			return;
		}
	}
	//they were not my child?
	ASSERT(false);
}
