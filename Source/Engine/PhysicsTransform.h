#pragma once

#include "Transform.h"

class btRigidBody;
class btTransform;

class PhysicsTransform : Transform {
public:
	void UpdateFromPhysics();
	void UpdateToPhysics() const;

	void SetPhysicsLink(btRigidBody* aRigidBody);

private:
	void GetPhysicsBtTransform(btTransform&) const;
	const btTransform GetPhysicsBtTransform() const;
	const btTransform CreateBtTransform() const;


	btRigidBody* mLinkedRB;
};
