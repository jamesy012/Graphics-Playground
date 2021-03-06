#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Engine/Transform.h"

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
class btTypedConstraint;
class btRigidBody;

class PhysicsObject;
class Mesh;

enum PhysicsFlags
{
	//starting at 6 due to continuing CollisionFilterGroups from btBroadphaseProxy
	Raycastable = (1 << 6),
};

class Physics {
public:
	void Startup();
	void Shutdown();

	void Update();

	void ImGuiWindow();

	//todo need to work out a good method of setting these up
	//without letting everything know about btCollisionShape's

	void AddingObjectsTestGround(PhysicsObject* aObject);
	void AddingObjectsTestCompoundBoxs(PhysicsObject* aObject, const std::vector<SimpleTransform>& aObjects);
	void AddingObjectsTestSphere(PhysicsObject* aObject);
	void AddingObjectsTestBox(PhysicsObject* aObject);
	void AddingObjectsTestMesh(PhysicsObject* aObject, Mesh* aMesh);
	btRigidBody* AddRigidBody(PhysicsObject* aObject, btCollisionShape* aShape, float mass);

	btTypedConstraint* JoinTwoObject(PhysicsObject* aObject1, PhysicsObject* aObject2);
	void RemoveContraintTemp(btTypedConstraint* aConstraint);
	void RemovePhysicsObject(PhysicsObject* aObject);

	PhysicsObject* Raycast(const glm::vec3& aPosition, const glm::vec3& aDirection, const float aLength) const;

	//testing the physics in a standalone update loop
	void Test();

private:
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	btDefaultCollisionConfiguration* mCollisionConfiguration;

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	btCollisionDispatcher* mDispatcher;

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	btBroadphaseInterface* mOverlappingPairCache;

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* mSolver;

	btDiscreteDynamicsWorld* mDynamicsWorld;

	//collisionShapes.push_back(groundShape);
	std::vector<btCollisionShape*> mCollisionShapes;

	int mActiveObjects = 0;
	int mCollisionsLastFrame = 0;
};
extern Physics* gPhysics;
