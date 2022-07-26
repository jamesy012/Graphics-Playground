#pragma once

class Transform;
class btRigidBody;
class btTransform;

class PhysicsObject {
public:
	void AttachTransform(Transform* aTransform);
	void AttachRigidBody(btRigidBody* aRigidBody);

	void TranformUpdated(Transform* aTransform);

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
private:
	void GetPhysicsBtTransform(btTransform&) const;
	const btTransform GetPhysicsBtTransform() const;
	const btTransform CreateBtTransform() const;

	Transform* mTransformLink = nullptr;
	btRigidBody* mRigidBodyLink = nullptr;
};
