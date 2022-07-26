#include "PhysicsObject.h"

#include <btBulletDynamicsCommon.h>

#include "Transform.h"

#pragma region conversions
static glm::vec3 BulletToGlm(const btVector3& aOther) {
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

void PhysicsObject::AttachTransform(Transform* aTransform) {
	ASSERT(mTransformLink == nullptr);
	ASSERT(aTransform != nullptr);
	mTransformLink = aTransform;
	mTransformLink->SetUpdateCallback(std::bind(&PhysicsObject::TranformUpdated, this, aTransform));
}

void PhysicsObject::SetMass(float aNewMass) {
	ASSERT(IsValid());
	mRigidBodyLink->setMassProps(aNewMass, btVector3(0.0f, 0.0f, 0.0f));
	//if(aNewMass == 0) {
	//	mRigidBodyLink->setCollisionFlags(mRigidBodyLink->getCollisionFlags() | CF_STATIC_OBJECT);
	//} else {
	//	mRigidBodyLink->setCollisionFlags(mRigidBodyLink->getCollisionFlags() & ~CF_STATIC_OBJECT);
	//}
}

void PhysicsObject::UpdateFromPhysics() {
	ASSERT(IsValid());
	ASSERT(false);

	btTransform trans = mRigidBodyLink->getWorldTransform();
	//GetPhysicsBtTransform(trans);

	mTransformLink->SetPosition(BulletToGlm(trans.getOrigin()));
	mTransformLink->SetRotation(BulletToGlm(trans.getRotation()));
	switch(mRigidBodyLink->getCollisionShape()->getShapeType()) {
		default:
			//ASSERT(false);//new shape
			break;
	}
}

void PhysicsObject::ResetPhysics() const {
	ASSERT(IsValid());

	mRigidBodyLink->setLinearVelocity(btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f)));
	mRigidBodyLink->setAngularVelocity(btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f)));
	mRigidBodyLink->clearForces();
	mRigidBodyLink->activate();
	UpdateToPhysics();
}

void PhysicsObject::UpdateToPhysics() const {
	ASSERT(IsValid());

	const btTransform trans = CreateBtTransform();
	mRigidBodyLink->setWorldTransform(trans);
}

void PhysicsObject::AttachRigidBody(btRigidBody* aRigidBody) {
	ASSERT(mRigidBodyLink == nullptr);
	ASSERT(aRigidBody != nullptr);
	mRigidBodyLink = aRigidBody;

	ASSERT(mRigidBodyLink->getUserPointer() == nullptr);
	mRigidBodyLink->setUserPointer(this);
}

void PhysicsObject::AddAttachment(btTypedConstraint* aNewAttachment) {
	mAttachments.push_back(aNewAttachment);
}

#pragma region btMotionState overrides
void PhysicsObject::getWorldTransform(btTransform& worldTrans) const {
	worldTrans = CreateBtTransform();
}

//Bullet only calls the update of worldtransform for active objects
void PhysicsObject::setWorldTransform(const btTransform& worldTrans) {
	mTransformLink->SetPosition(BulletToGlm(worldTrans.getOrigin()));
	mTransformLink->SetRotation(BulletToGlm(worldTrans.getRotation()));
	switch(mRigidBodyLink->getCollisionShape()->getShapeType()) {
		default:
			//ASSERT(false);//new shape
			break;
	}
}
#pragma endregion

void PhysicsObject::TranformUpdated(Transform* aTransform) {
	//mViewMatrix = glm::inverse(mTransform.GetWorldMatrix());
	////mViewMatrix = mTransform.GetWorldMatrix();
	//
	//mViewProjMatrix =  GetProjMatrix() * mViewMatrix;
}

const btTransform PhysicsObject::CreateBtTransform() const {
	btTransform out;
	out.setRotation(GlmToBullet(mTransformLink->GetLocalRotation()));
	out.setOrigin(GlmToBullet(mTransformLink->GetLocalPosition()));
	return out;
}
