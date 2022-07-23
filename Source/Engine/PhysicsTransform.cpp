#include "PhysicsTransform.h"

#include "btBulletDynamicsCommon.h"

#include "PlatformDebug.h"

#pragma region conversions
static glm::vec3 BulletoGlm(const btVector3& aOther) {
	return glm::vec3(aOther.getX(), aOther.getY(), aOther.getZ());
}

static glm::vec4 BulletToGlm(const btVector4& aOther) {
	return glm::vec4(aOther.getX(), aOther.getY(), aOther.getZ(), aOther.getW());
}

static glm::quat BulletToGlm(const btQuaternion& aOther) {
	return glm::quat(aOther.getW(), aOther.getX(), aOther.getY(), aOther.getZ());
}

static btVector3 GlmToBullet(const glm::vec3& aOther) {
	return btVector3(aOther.x, aOther.y, aOther.z);
}

static btQuaternion GlmToBullet(const glm::quat& aOther) {
	return btQuaternion(aOther.x, aOther.y, aOther.z, aOther.w);
}

#pragma endregion

void PhysicsTransform::UpdateFromPhysics() {
	if(mLinkedRB == nullptr) {
		return;
	}
	btTransform trans;
	GetPhysicsBtTransform(trans);

	SetPosition(BulletToGlm(trans.getOrigin()));
	SetRotation(BulletToGlm(trans.getRotation()));
}

void PhysicsTransform::UpdateToPhysics() const {
	if(mLinkedRB == nullptr) {
		return;
	}
	const btTransform trans = CreateBtTransform();
	mLinkedRB->setWorldTransform(trans);
}

void PhysicsTransform::SetPhysicsLink(btRigidBody* aRigidBody) {
	ASSERT(mLinkedRB == nullptr);
	ASSERT(aRigidBody->getUserPointer() == nullptr);
	aRigidBody->setUserPointer(this);
	mLinkedRB = aRigidBody;

	UpdateFromPhysics();
}

void PhysicsTransform::GetPhysicsBtTransform(btTransform& aTransform) const {
	const btMotionState* motionState = mLinkedRB->getMotionState();
	if(mLinkedRB && motionState) {
		motionState->getWorldTransform(aTransform);
	} else {
		ASSERT(false);
		aTransform = mLinkedRB->getWorldTransform();
	}
}
const btTransform PhysicsTransform::GetPhysicsBtTransform() const {
	const btMotionState* motionState = mLinkedRB->getMotionState();
	if(mLinkedRB && motionState) {
		btTransform transform;
		motionState->getWorldTransform(transform);
		return transform;
	} else {
		return mLinkedRB->getWorldTransform();
	}
	return btTransform();
}

const btTransform PhysicsTransform::CreateBtTransform() const {
	btTransform out;
	out.setRotation(GlmToBullet(GetLocalRotation()));
	out.setOrigin(GlmToBullet(GetLocalPosition()));
	return out;
}
