#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Transform {
public:
	Transform(Transform* mParent = nullptr) {
		SetParent(mParent);
	};
	template<typename POS>
	Transform(POS aPos, Transform* aParent = nullptr) {
		Set(aPos, aParent);
	};
	template<typename POS, typename SCALE>
	Transform(POS aPos, SCALE aScale, Transform* aParent = nullptr) {
		Set(aPos, aScale, aParent);
	};
	template<typename POS, typename SCALE, typename QUAT>
	Transform(POS aPos, SCALE aScale, QUAT aRot, Transform* aParent = nullptr) {
		Set(aPos, aScale, aRot, aParent);
	};
	~Transform();

	template<typename POS, typename SCALE>
	void Set(POS aPos, SCALE aScale) {
		SetPosition(aPos);
		SetScale(aScale);
	}
	template<typename POS, typename SCALE>
	void Set(POS aPos, SCALE aScale, Transform* aParent) {
		Set(aPos, aScale);
		SetParent(aParent);
	}

	template<typename POS, typename SCALE, typename QUAT>
	void Set(POS aPos, SCALE aScale, QUAT aRot) {
		SetPosition(aPos);
		SetScale(aScale);
		SetRotation(aRot);
	}

	template<typename POS, typename SCALE, typename QUAT>
	void Set(POS aPos, SCALE aScale, QUAT aRot, Transform* aParent) {
		Set(aPos, aScale, aRot);
		SetParent(aParent);
	}

	//clears all references to this Transform
	//should it reparent the children to our parent?
	//or make their parents nullptr
	void Clear(bool aReparent = true);

	void SetPosition(glm::vec3 aPos) {
		mPos = aPos;
		SetDirty();
	}
	void SetScale(glm::vec3 aScale) {
		mScale = aScale;
		SetDirty();
	}
	void SetScale(float aScale) {
		mScale = glm::vec3(aScale);
		SetDirty();
	}
	//degrees
	void SetRotation(glm::vec3 aRot) {
		mRot = glm::radians(aRot);
		SetDirty();
	}
	void SetRotation(glm::quat aRot) {
		mRot = aRot;
		SetDirty();
	}

	//todo
	void SetWorldPosition(glm::vec3 aPos);
	void SetWorldScale(glm::vec3 aScale);
	void SetWorldRotation(glm::quat aRotation);
	//degrees
	void SetWorldRotation(glm::vec3 aRotation){
		SetWorldRotation(glm::quat(glm::radians(aRotation)));
	}

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
	bool IsChild(Transform* aChild) const;

	void SetDirty();

private:
	bool IsDirty() const {
		return mDirty;
	}

	//updates world and local matrix and sets dirty flag to false
	void UpdateMatrix();

	//updates our world and local matrix if we are dirty
	void CheckUpdate() {
		if(IsDirty()) {
			UpdateMatrix();
		}
	}

	void AddChild(Transform* aChild);
	void RemoveChild(Transform* aChild);

	glm::vec3 mPos	 = glm::vec3(0.0f);
	glm::vec3 mScale = glm::vec3(1.0f);
	glm::quat mRot	 = glm::identity<glm::quat>();

	glm::mat4 mLocalMatrix = glm::identity<glm::mat4>();
	glm::mat4 mWorldMatrix = glm::identity<glm::mat4>();

	Transform* mParent = nullptr;
	std::vector<Transform*> mChildren;

	bool mDirty = false;
};