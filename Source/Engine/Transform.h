#pragma once

//for Transform name
//if splitting SimpleTransform can remove this
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Callback.h"
#include "PlatformDebug.h"

namespace CONSTANTS {
	const glm::vec3 RIGHT = glm::vec3(1, 0, 0);
	const glm::vec3 UP = glm::vec3(0, 1, 0);
	//forward goes towards screen
	const glm::vec3 FORWARD = glm::vec3(0, 0, 1);
} // namespace CONSTANTS

class SimpleTransform {
public:
	enum Types
	{
		POSITION = 1 << 0,
		SCALE = 1 << 1,
		ROTATION = 1 << 2,
		ALL = POSITION | SCALE | ROTATION,
		//parents?
	};

	SimpleTransform() {};
	SimpleTransform(const SimpleTransform& aOther) {
		SetPosition(aOther.mPos);
		SetScale(aOther.mScale);
		SetRotation(aOther.mRot);
	};
	template<typename POS>
	SimpleTransform(const POS& aPos) {
		SetPosition(aPos);
	};
	template<typename POS, typename SCALE>
	SimpleTransform(const POS& aPos, const SCALE& aScale) {
		Set(aPos, aScale);
	};
	template<typename POS, typename SCALE, typename QUAT>
	SimpleTransform(const POS& aPos, const SCALE& aScale, const QUAT& aRot) {
		Set(aPos, aScale, aRot);
	};
	~SimpleTransform() {};

	template<typename POS, typename SCALE>
	void Set(const POS& aPos, const SCALE& aScale) {
		SetPosition(aPos);
		SetScale(aScale);
	}

	template<typename POS, typename SCALE, typename QUAT>
	void Set(const POS& aPos, const SCALE& aScale, const QUAT& aRot) {
		SetPosition(aPos);
		SetScale(aScale);
		SetRotation(aRot);
	}

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

	void CopyTransform(const SimpleTransform& aOther, const Types aTypes = Types::ALL) {
		if(Types::POSITION & aTypes) {
			SetPosition(aOther.GetLocalPosition());
		}
		if(Types::SCALE & aTypes) {
			SetScale(aOther.GetLocalScale());
		}
		if(Types::ROTATION & aTypes) {
			SetRotation(aOther.GetLocalRotation());
		}
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

	//updates matrixies/parents
	glm::mat4 GetLocalMatrix() {
		//update only local??
		CheckUpdate();
		return mLocalMatrix;
	}

	//does not use or set the cached Value
	glm::mat4 GetLocalMatrixSlow() const {
		glm::mat4 result;
		CreateLocalMatrix(result);
		return result;
	}

	glm::vec3 GetRight() const {
		return mRot * CONSTANTS::RIGHT;
	}
	glm::vec3 GetUp() const {
		return mRot * CONSTANTS::UP;
	}
	//towards screen
	glm::vec3 GetForward() const {
		return mRot * CONSTANTS::FORWARD;
	}

	bool IsUp() const {
		return glm::dot(GetUp(), CONSTANTS::UP) > 0;
	}

	void SetDirty() {
		mDirty = true;
	}

	//updates our world and local matrix if we are dirty
	void CheckUpdate() {
		if(IsDirty()) {
			UpdateMatrix();
		}
	}

	//typedef std::function<void(SimpleTransform*)> TransformUpdatecallback;
	//callback for when the matrix's are updated, so another system can respond to it
	Callback<void(SimpleTransform*)> mUpdateCallback;

protected:
	virtual bool IsDirty() const {
		return mDirty;
	}

	void CreateLocalMatrix(glm::mat4& aMatrix) const;

	//updates world and local matrix and sets dirty flag to false
	virtual void UpdateMatrix();

	glm::vec3 mPos = glm::vec3(0.0f);
	glm::vec3 mScale = glm::vec3(1.0f);
	glm::quat mRot = glm::identity<glm::quat>();

	glm::mat4 mLocalMatrix = glm::identity<glm::mat4>();

	bool mDirty = false;
};

class Transform : public SimpleTransform {
public:
	Transform() : SimpleTransform() {};
	Transform(const SimpleTransform& aOther) :
		SimpleTransform(aOther) {
			//
		};
	Transform(Transform* aParent) : SimpleTransform() {
		SetParent(aParent);
	};
	Transform(const SimpleTransform& aOther, Transform* aParent) : SimpleTransform(aOther) {
		SetParent(aParent);
	};
	template<typename POS>
	Transform(const POS& aPos, Transform* aParent) : SimpleTransform(aPos) {
		SetParent(aParent);
	};
	template<typename POS, typename SCALE>
	Transform(const POS& aPos, const SCALE& aScale, Transform* aParent) : SimpleTransform(aPos, aScale) {
		SetParent(aParent);
	};
	template<typename POS, typename SCALE, typename QUAT>
	Transform(const POS& aPos, const SCALE& aScale, const QUAT& aRot, Transform* aParent) : SimpleTransform(aPos, aScale, aRot) {
		SetParent(aParent);
	};
	//Transform(const glm::vec3& aPos, Transform* aParent) {
	//	Set(aPos, aParent);
	//};
	//Transform(const glm::vec3& aPos, const glm::vec3& aScale, Transform* aParent) {
	//	Set(aPos, aScale, aParent);
	//};
	//Transform(const glm::vec3& aPos, const float& aScale, Transform* aParent) {
	//	Set(aPos, aScale, aParent);
	//};
	//Transform(const glm::vec3& aPos, const glm::vec3& aScale, const glm::quat& aRot, Transform* aParent) {
	//	Set(aPos, aScale, aRot, aParent);
	//};
	//Transform(const glm::vec3& aPos, const float& aScale, const glm::quat& aRot, Transform* aParent) {
	//	Set(aPos, aScale, aRot, aParent);
	//};
	//Transform(const glm::vec3& aPos, const glm::vec3& aScale, const glm::vec3& aRot, Transform* aParent) {
	//	Set(aPos, aScale, aRot, aParent);
	//};
	//Transform(const glm::vec3& aPos, const float& aScale, const glm::vec3& aRot, Transform* aParent) {
	//	Set(aPos, aScale, aRot, aParent);
	//};
	~Transform();

	template<typename POS>
	void Set(const POS& aPos, Transform* aParent) {
		SetPosition(aPos);
		SetParent(aParent);
	}
	template<typename POS, typename SCALE>
	void Set(const POS& aPos, const SCALE& aScale, Transform* aParent) {
		SimpleTransform::Set(aPos, aScale);
		SetParent(aParent);
	}

	template<typename POS, typename SCALE, typename QUAT>
	void Set(const POS& aPos, const SCALE& aScale, const QUAT& aRot, Transform* aParent) {
		SimpleTransform::Set(aPos, aScale, aRot);
		SetParent(aParent);
	}

	//clears all references to this Transform
	//should it reparent the children to our parent?
	//or make their parents nullptr
	void Clear(bool aReparent = true);

	void SetWorldPosition(const glm::vec3& aPos);
	void SetWorldScale(const glm::vec3& aScale);
	void SetWorldRotation(const glm::quat& aRotation);
	//degrees
	void SetWorldRotation(const glm::vec3& aRotation) {
		SetWorldRotation(glm::quat(glm::radians(aRotation)));
	}

	//Updates matrixies
	glm::vec3 GetWorldPosition();
	//Updates matrixies
	glm::vec3 GetWorldScale();
	//Updates matrixies
	glm::quat GetWorldRotation();
	//Updates matrixies, converts quat to euler degrees
	glm::vec3 GetWorldRotationEuler() {
		return glm::degrees(glm::eulerAngles(GetWorldRotation()));
	}

	//updates matrixies/parents
	glm::mat4 GetWorldMatrix() {
		CheckUpdate();
		return mWorldMatrix;
	}

	glm::vec3 GetWorldRight() {
		return GetWorldRotation() * CONSTANTS::RIGHT;
	}
	glm::vec3 GetWorldUp() {
		return GetWorldRotation() * CONSTANTS::UP;
	}
	//towards screen
	glm::vec3 GetWorldForward() {
		return GetWorldRotation() * CONSTANTS::FORWARD;
	}

	SimpleTransform WorldToSimple() {
		SimpleTransform st;
		st.SetPosition(GetWorldPosition());
		st.SetScale(GetWorldScale());
		st.SetRotation(GetWorldRotation());
		return st;
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

private:
	virtual bool IsDirty() const override {
		const Transform* parent = this;
		while(parent != nullptr) {
			if(parent->mDirty) {
				return true;
			}
			parent = parent->mParent;
		}
		return false;
	}

	//updates world and local matrix and sets dirty flag to false
	virtual void UpdateMatrix() override;

	void AddChild(Transform* aChild);
	void RemoveChild(Transform* aChild);

	glm::mat4 mWorldMatrix = glm::identity<glm::mat4>();

	Transform* mParent = nullptr;
	std::vector<Transform*> mChildren;
};