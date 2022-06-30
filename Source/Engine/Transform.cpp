#include "Transform.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "PlatformDebug.h"

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

void Transform::SetWorldPosition(glm::vec3 aPos) {
	if(mParent) {
		//skip out matrix
		mPos = glm::inverse(mParent->GetWorldMatrix()) * glm::vec4(aPos, 1);
		SetDirty();
	} else {
		SetPosition(aPos);
	}
}

void Transform::SetWorldScale(glm::vec3 aScale) {
	if(mParent) {
		//todo
		ASSERT(false);
	} else {
		SetScale(aScale);
	}
}

void Transform::SetWorldRotation(glm::quat aRot) {
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
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(mWorldMatrix, scale, rotation, translation, skew, perspective);
	return glm::conjugate(rotation);
	//https://stackoverflow.com/a/68323550
	//const glm::vec3 scale = GetWorldScale();
	//const glm::mat3 rotMtx(glm::vec3(mWorldMatrix[0]) / scale[0], glm::vec3(mWorldMatrix[1]) / scale[1], glm::vec3(mWorldMatrix[2]) / scale[2]);
	//return glm::quat_cast(rotMtx);
}

bool Transform::IsChild(Transform* aChild) const {
	for(const Transform* child: mChildren) {
		if(child == aChild) {
			return true;
		}
	}
	return false;
}

void Transform::SetDirty() {
	const int childCount = mChildren.size();
	for(int i = 0; i < childCount; ++i) {
		mChildren[i]->SetDirty();
	}
	mDirty = true;
}

void Transform::UpdateMatrix() {
	mLocalMatrix = glm::identity<glm::mat4>();

	mLocalMatrix = glm::translate(mLocalMatrix, mPos);
	mLocalMatrix *= glm::mat4_cast(mRot);
	mLocalMatrix = glm::scale(mLocalMatrix, mScale);

	if(mParent != nullptr) {
		//recursive Get world of parent till we reach the top
		mWorldMatrix = mParent->GetWorldMatrix() * mLocalMatrix;
	} else {
		mWorldMatrix = mLocalMatrix;
	}
	
	mDirty = false;

	if(mUpdateCallback) {
		mUpdateCallback(this);
	}

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