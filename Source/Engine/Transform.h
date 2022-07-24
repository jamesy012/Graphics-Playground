#pragma once

#include <vector>
#include <functional>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "PlatformDebug.h"

class btRigidBody;
class btTransform;

namespace CONSTANTS {
	const glm::vec3 RIGHT = glm::vec3(1, 0, 0);
	const glm::vec3 UP = glm::vec3(0, 1, 0);
	//forward goes towards screen
	const glm::vec3 FORWARD = glm::vec3(0, 0, 1);
} // namespace CONSTANTS

class Transform {
	typedef std::function<void(Transform*)> TransformUpdatecallback;

public:
	Transform(Transform* mParent = nullptr) {
		SetParent(mParent);
	};
	template<typename POS>
	Transform(const POS& aPos, Transform* aParent = nullptr) {
		Set(aPos, aParent);
	};
	template<typename POS, typename SCALE>
	Transform(const POS& aPos, const SCALE& aScale, Transform* aParent = nullptr) {
		Set(aPos, aScale, aParent);
	};
	template<typename POS, typename SCALE, typename QUAT>
	Transform(const POS& aPos, const SCALE& aScale, const QUAT& aRot, Transform* aParent = nullptr) {
		Set(aPos, aScale, aRot, aParent);
	};
	~Transform();

	template<typename POS, typename SCALE>
	void Set(const POS& aPos, const SCALE& aScale) {
		SetPosition(aPos);
		SetScale(aScale);
	}
	template<typename POS, typename SCALE>
	void Set(const POS& aPos, const SCALE& aScale, Transform* aParent) {
		Set(aPos, aScale);
		SetParent(aParent);
	}

	//template<typename POS, typename SCALE, typename QUAT>
	//void Set(const POS& aPos, const SCALE& aScale, const QUAT& aRot) {
	//	SetPosition(aPos);
	//	SetScale(aScale);
	//	SetRotation(aRot);
	//}

	template<typename POS, typename SCALE, typename QUAT>
	void Set(const POS& aPos, const SCALE& aScale, const QUAT& aRot, Transform* aParent) {
		Set(aPos, aScale, aRot);
		SetParent(aParent);
	}

	//clears all references to this Transform
	//should it reparent the children to our parent?
	//or make their parents nullptr
	void Clear(bool aReparent = true);

	void SetPosition(const glm::vec3& aPos) {
		mPos = aPos;
		SetDirty();
	}
	void SetScale(const glm::vec3& aScale) {
		mScale = aScale;
		SetDirty();
	}
	void SetScale(const float& aScale) {
		mScale = glm::vec3(aScale);
		SetDirty();
	}
	//degrees
	void SetRotation(const glm::vec3& aRot) {
		mRot = glm::radians(aRot);
		SetDirty();
	}
	void SetRotation(const glm::quat& aRot) {
		mRot = aRot;
		SetDirty();
	}
	void SetMatrix(const glm::mat4& aMat);

	void SetWorldPosition(const glm::vec3& aPos);
	void SetWorldScale(const glm::vec3& aScale);
	void SetWorldRotation(const glm::quat& aRotation);
	//degrees
	void SetWorldRotation(const glm::vec3& aRotation) {
		SetWorldRotation(glm::quat(glm::radians(aRotation)));
	}

	void TranslateLocal(const glm::vec3& aTranslation);
	void Rotate(const glm::quat& aRotation);
	void RotateAxis(const float aAmount, const glm::vec3& aAxis);
	//rotates transform on vec2 X then Y
	void RotateAxis(const glm::vec2& aEulerAxisRotation);
	//rotates transform on vec3 X then Y then Z
	void RotateAxis(const glm::vec3& aEulerAxisRotation);

	void SetLookAt(const glm::vec3& aPos, const glm::vec3& aLookAt, const glm::vec3& up);

	glm::vec3 GetLocalPosition() const {
		return mPos;
	}
	glm::vec3 GetLocalScale() const {
		return mScale;
	}
	glm::quat GetLocalRotation() const {
		return mRot;
	}
	//returns rotation in degreens
	glm::vec3 GetLocalRotationEuler() const {
		return glm::degrees(glm::eulerAngles(mRot));
	}

	//Updates matrixies
	glm::vec3 GetWorldPosition();
	//Updates matrixies
	glm::vec3 GetWorldScale();
	//Updates matrixies
	glm::quat GetWorldRotation();
	//Updates matrixies, converts quat to euler degrees
	glm::quat GetWorldRotationEuler() {
		return glm::degrees(glm::eulerAngles(GetWorldRotation()));
	}

	//updates matrixies/parents
	glm::mat4 GetLocalMatrix() {
		//update only local??
		CheckUpdate();
		return mLocalMatrix;
	}
	//updates matrixies/parents
	glm::mat4 GetWorldMatrix() {
		CheckUpdate();
		return mWorldMatrix;
	}

	glm::vec3 GetRight() const {
		return mRot * CONSTANTS::RIGHT;
	}
	glm::vec3 GetUp() const {
		return mRot * CONSTANTS::UP;
	}
	glm::vec3 GetForward() const {
		return mRot * CONSTANTS::FORWARD;
	}

	bool IsUp() const {
		return glm::dot(GetUp(), CONSTANTS::UP) > 0;
	}

	//todo give option to keep world position
	void SetParent(Transform* aParent) {
		if(aParent == mParent) {
			return;
		}
		if(mParent) {
			mParent->RemoveChild(this);
		}
		mParent = aParent;
		if(mParent != nullptr) {
			mParent->AddChild(this);
		}
		SetDirty();
	}

	Transform* GetParent() {
		return mParent;
	}

	const uint8_t GetNumChildren() const {
		return mChildren.size();
	}
	bool IsChild(const Transform* aChild) const;

	void SetDirty();

	//calls the callback after global matrix is set
	void SetUpdateCallback(TransformUpdatecallback aCallback) {
		ASSERT(mUpdateCallback == nullptr);
		mUpdateCallback = aCallback;
	}

	//updates our world and local matrix if we are dirty
	void CheckUpdate() {
		if(IsDirty()) {
			UpdateMatrix();
		}
	}

	//physics
	void UpdateFromPhysics();
	void ResetPhysics() const;
	void UpdateToPhysics() const;

	void SetPhysicsLink(btRigidBody* aRigidBody);

private:
	bool IsDirty() const {
		return mDirty;
	}

	//updates world and local matrix and sets dirty flag to false
	void UpdateMatrix();

	void AddChild(Transform* aChild);
	void RemoveChild(Transform* aChild);

	void GetPhysicsBtTransform(btTransform&) const;
	const btTransform GetPhysicsBtTransform() const;
	const btTransform CreateBtTransform() const;

	glm::vec3 mPos = glm::vec3(0.0f);
	glm::vec3 mScale = glm::vec3(1.0f);
	glm::quat mRot = glm::identity<glm::quat>();

	glm::mat4 mLocalMatrix = glm::identity<glm::mat4>();
	glm::mat4 mWorldMatrix = glm::identity<glm::mat4>();

	Transform* mParent = nullptr;
	std::vector<Transform*> mChildren;

	//callback for when the matrix's are updated, so another system can respond to it
	TransformUpdatecallback mUpdateCallback = nullptr;

	bool mDirty = false;

	btRigidBody* mLinkedRB = nullptr;
};