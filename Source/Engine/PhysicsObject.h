#pragma once

#include <vector>

#include "LinearMath/btMotionState.h"

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

	void SetMass(float aNewMass);

	void UpdateFromPhysics();
	void ResetPhysics() const;
	void UpdateToPhysics() const;

	Transform* GetTransform() const {
		return mTransformLink;
	};
	btRigidBody* GetRigidBody() const {
		return mRigidBodyLink;
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

	//creates a btTransform from the attached Transform class
	const btTransform CreateBtTransform() const;

	Transform* mTransformLink = nullptr;
	btRigidBody* mRigidBodyLink = nullptr;

	std::vector<btTypedConstraint*> mAttachments;
};