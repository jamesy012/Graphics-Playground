#pragma once

#include <vector>

#include <LinearMath/btMotionState.h>
#include <glm/glm.hpp>

class Transform;
class Physics;
class btRigidBody;
class btTransform;
class btTypedConstraint;

//as a motion state, it will be destroyed when the rigidbody gets destroyed
class PhysicsObject : public btMotionState {
	friend Physics;

public:
	void AttachTransform(Transform* aTransform);
	void AttachOther(void* aOther) {
		mOtherLink = aOther;
	}

	void SetMass(const float& aNewMass);
	void SetVelocity(const glm::vec3& aNewVelocity);
	void SetKinematic(bool aIsKinematic);
	void AddCollisionFlags(uint32_t aFlag);
	void RemoveCollisionFlags(uint32_t aFlag);

	//quick removing of attachements
	//we probably want to do some processing on the attachment to remove it from the other attached physics object..
	void RemoveAttachmentsTemp() {
		mAttachments.clear();
	}

	void UpdateFromPhysics();
	void ResetPhysics() const;
	void UpdateToPhysics() const;

	Transform* GetTransform() const {
		return mTransformLink;
	};
	btRigidBody* GetRigidBody() const {
		return mRigidBodyLink;
	}
	void* GetOther() {
		return mOtherLink;
	}

	const bool IsValid() const {
		return mTransformLink != nullptr && mRigidBodyLink != nullptr;
	}

protected:
	void AttachRigidBody(btRigidBody* aRigidBody);
	void AddAttachment(btTypedConstraint* aNewAttachment);

#pragma region btMotionState overrides
	void getWorldTransform(btTransform& worldTrans) const override;

	//Bullet only calls the update of worldtransform for active objects
	void setWorldTransform(const btTransform& worldTrans) override;
#pragma endregion

private:
	//called whenever the transform is updated
	void TranformUpdated(Transform* aTransform);

	Transform* mTransformLink = nullptr;
	btRigidBody* mRigidBodyLink = nullptr;
	void* mOtherLink = nullptr;

	std::vector<btTypedConstraint*> mAttachments;
};
