#pragma once

#include <vector>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;

class PhysicsObject;
class Mesh;

class Physics {
public:
	void Startup();
	void Shutdown();

	void Update();

    void ImGuiWindow();

	void AddingObjectsTestGround(PhysicsObject* aObject);
	void AddingObjectsTestSphere(PhysicsObject* aObject);
	void AddingObjectsTestMesh(PhysicsObject* aObject, Mesh* aMesh);

    void JoinTwoObject(const PhysicsObject* aObject1, const PhysicsObject* aObject2);

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
};
extern Physics* gPhysics;
