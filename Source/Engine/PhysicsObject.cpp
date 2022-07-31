#include "PhysicsObject.h"

#include <btBulletDynamicsCommon.h>

#include "Transform.h"
#include "Graphics\Conversions.h"

void PhysicsObject::AttachTransform(Transform* aTransform) {
	ASSERT(mTransformLink == nullptr);
	ASSERT(aTransform != nullptr);
	mTransformLink = aTransform;
	mTransformLink->SetUpdateCallback(std::bind(&PhysicsObject::TranformUpdated, this, aTransform));
}

void PhysicsObject::SetMass(const float& aNewMass) {
	ASSERT(IsValid());
	mRigidBodyLink->setMassProps(aNewMass, btVector3(0.0f, 0.0f, 0.0f));
	//if(aNewMass == 0) {
	//	mRigidBodyLink->setCollisionFlags(mRigidBodyLink->getCollisionFlags() | CF_STATIC_OBJECT);
	//} else {
	//	mRigidBodyLink->setCollisionFlags(mRigidBodyLink->getCollisionFlags() & ~CF_STATIC_OBJECT);
	//}
}

void PhysicsObject::SetVelocity(const glm::vec3& aNewVelocity) {
	ASSERT(IsValid());
	mRigidBodyLink->setLinearVelocity(GlmToBullet(aNewVelocity));
}

void PhysicsObject::SetKinematic(bool aIsKinematic) {
	ASSERT(IsValid());
	int flags = mRigidBodyLink->getCollisionFlags();
	if(aIsKinematic) {
		flags |= btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT;
	} else {
		flags &= ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);
	}
	mRigidBodyLink->setCollisionFlags(flags);
}

void PhysicsObject::AddCollisionFlags(uint32_t aFlag) {
	mRigidBodyLink->getBroadphaseProxy()->m_collisionFilterMask |= aFlag;
}
void PhysicsObject::RemoveCollisionFlags(uint32_t aFlag) {
	mRigidBodyLink->getBroadphaseProxy()->m_collisionFilterMask &= ~(aFlag);
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

	const btTransform trans = TransformWorldToBullet(*mTransformLink);
	mRigidBodyLink->setWorldTransform(trans);

	auto type = mRigidBodyLink->getCollisionShape()->getShapeType();
	switch(type) {
		case BOX_SHAPE_PROXYTYPE: {
			btBoxShape* shape = (btBoxShape*)mRigidBodyLink->getCollisionShape();
			const btVector3 scale = GlmToBullet(mTransformLink->GetLocalScale() / 2.0f);
			//const btScalar margin = shape->getMargin();
			//const btVector3 currMargin = (shape->getImplicitShapeDimensions() + btVector3(margin, margin, margin))*2;
			//const btVector3 currScale = shape->getLocalScaling();
			//shape->setLocalScaling(scale / (currScale * currMargin));
			//shape->setLocalScaling(scale);
			// 
			//m_implicitShapeDimensions = (boxHalfExtents * m_localScaling) - margin;
			shape->setImplicitShapeDimensions(scale);
			shape->setSafeMargin(scale);
			break;
		}
		case SPHERE_SHAPE_PROXYTYPE: {
			btSphereShape* shape = (btSphereShape*)mRigidBodyLink->getCollisionShape();
			const float scale = mTransformLink->GetLocalScale().x / 2.0f;
			shape->setUnscaledRadius(scale);
			break;
		}
		case COMPOUND_SHAPE_PROXYTYPE: {
			btCompoundShape* shape = (btCompoundShape*)mRigidBodyLink->getCollisionShape();
			const btVector3 scale = GlmToBullet(mTransformLink->GetLocalScale());
			const btVector3 currScale = shape->getLocalScaling();
			shape->setLocalScaling(scale / currScale);
			break;
		}
		default:
			ASSERT(false);
			break;
	}
	//mRigidBodyLink->getCollisionShape()->setLocalScaling(GlmToBullet(mTransformLink->GetLocalScale()));
	//mRigidBodyLink->getCollisionShape()->setLocalScaling(GlmToBullet(glm::vec3(0.9f)));
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
	worldTrans = TransformWorldToBullet(*mTransformLink);
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
