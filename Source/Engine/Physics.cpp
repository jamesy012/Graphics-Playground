#include "Physics.h"

#include <vector>

#include <btBulletDynamicsCommon.h>
#include <imgui.h>

#include "Engine.h"
#include "PlatformDebug.h"
#include "PhysicsObject.h"
#include "Transform.h"

Physics* gPhysics = nullptr;

void Physics::Startup() {
	ASSERT(gPhysics == nullptr);
	gPhysics = this;

	mCollisionConfiguration = new btDefaultCollisionConfiguration();

	mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);

	mOverlappingPairCache = new btDbvtBroadphase();

	mSolver = new btSequentialImpulseConstraintSolver();

	mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mOverlappingPairCache, mSolver, mCollisionConfiguration);

	mDynamicsWorld->setGravity(btVector3(0, -10, 0));
}

void Physics::Shutdown() {
	ASSERT(gPhysics != nullptr);

	delete mDynamicsWorld;

	delete mSolver;

	delete mOverlappingPairCache;

	delete mDispatcher;

	delete mCollisionConfiguration;

	gPhysics = nullptr;
}

void Physics::Update() {
	ASSERT(gPhysics != nullptr);

	//for(int j = mDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
	//	btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[j];
	//	btRigidBody* body = btRigidBody::upcast(obj);
	//	Transform* transform = ((Transform*)body->getUserPointer());
	//	if(transform) {
	//		transform->UpdateToPhysics();
	//	}
	//}

	mActiveObjects = 0;

	//
	int output = mDynamicsWorld->stepSimulation(gEngine->GetDeltaTime());

	for(int j = mDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
		btCollisionObject* colObj = mDynamicsWorld->getCollisionObjectArray()[j];
		if(colObj->isActive()) {
			mActiveObjects++;
			btRigidBody* body = btRigidBody::upcast(colObj);
			PhysicsObject* object = ((PhysicsObject*)body->getUserPointer());
			if(object) {
				object->UpdateFromPhysics();
			}
		}
	}
}

void Physics::ImGuiWindow() {
	if(ImGui::Begin("Physics")) {
		ImGui::Text("Num Collision Objects: %i", mDynamicsWorld->getNumCollisionObjects());
		ImGui::Text("Num Active Objects: %i", mActiveObjects);
		ImGui::Text("Num RigidBodies: %i", mDynamicsWorld->getNonStaticRigidBodies().size());
	}
	ImGui::End();
}

void Physics::AddingObjectsTestGround(PhysicsObject* aObject) {
	glm::vec3 pos = aObject->GetTransform()->GetLocalPosition();
	glm::vec3 scale = aObject->GetTransform()->GetLocalScale();
	btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(scale.x), btScalar(scale.y), btScalar(scale.z)));

	mCollisionShapes.push_back(groundShape);

	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));

	btScalar mass(0.);

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if(isDynamic)
		groundShape->calculateLocalInertia(mass, localInertia);

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	body->setRollingFriction(0.5f);
	body->setAngularFactor(0.5f);
	body->setFriction(10.0f);

	aObject->AttachRigidBody(body);
	aObject->UpdateToPhysics();

	//add the body to the dynamics world
	mDynamicsWorld->addRigidBody(body);
}

void Physics::AddingObjectsTestSphere(PhysicsObject* aObject) {
	btCollisionShape* colShape = new btSphereShape(btScalar(aObject->GetTransform()->GetLocalScale().x / 2));
	mCollisionShapes.push_back(colShape);

	/// Create Dynamic Objects
	btTransform startTransform;
	startTransform.setIdentity();

	btScalar mass(1.f);

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if(isDynamic)
		colShape->calculateLocalInertia(mass, localInertia);

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	body->setSpinningFriction(0.5f);

	aObject->AttachRigidBody(body);
	aObject->UpdateToPhysics();

	mDynamicsWorld->addRigidBody(body);
}

//from the example
void Physics::Test() {

	//keep track of the shapes, we release memory at exit.
	//make sure to re-use collision shapes among rigid bodies whenever possible!
	btAlignedObjectArray<btCollisionShape*> collisionShapes;

	std::vector<Transform> transforms(2);
	std::vector<PhysicsObject> phyObjects(2);

	///create a few basic rigid bodies

	//the ground is a cube of side 100 at position y = -56.
	//the sphere will hit it at y = -6, with center at -5
	{
		btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

		collisionShapes.push_back(groundShape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -56, 0));

		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if(isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		phyObjects[0].AttachTransform(&transforms[0]);
		phyObjects[0].AttachRigidBody(body);

		//add the body to the dynamics world
		mDynamicsWorld->addRigidBody(body);
	}

	{
		//create a dynamic rigidbody

		//btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
		btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar mass(1.f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if(isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		startTransform.setOrigin(btVector3(2, 10, 0));

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		phyObjects[1].AttachTransform(&transforms[1]);
		phyObjects[1].AttachRigidBody(body);

		mDynamicsWorld->addRigidBody(body);
	}

	/// Do some simulation

	///-----stepsimulation_start-----
	for(int i = 0; i < 150; i++) {

		for(int j = mDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
			btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[j];
			btRigidBody* body = btRigidBody::upcast(obj);
			((PhysicsObject*)body->getUserPointer())->UpdateToPhysics();
		}

		mDynamicsWorld->stepSimulation(1.f / 60.f, 10);

		//print positions of all objects
		for(int j = mDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
			btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[j];
			btRigidBody* body = btRigidBody::upcast(obj);
			btTransform trans;
			if(body && body->getMotionState()) {
				body->getMotionState()->getWorldTransform(trans);
			} else {
				trans = obj->getWorldTransform();
				ASSERT(false);
			}
			((PhysicsObject*)body->getUserPointer())->UpdateFromPhysics();

			printf("world pos object %d = %f,%f,%f\n",
				   j,
				   float(trans.getOrigin().getX()),
				   float(trans.getOrigin().getY()),
				   float(trans.getOrigin().getZ()));
		}
	}

	///-----stepsimulation_end-----

	//cleanup in the reverse order of creation/initialization

	///-----cleanup_start-----

	//remove the rigidbodies from the dynamics world and delete them
	for(int i = mDynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if(body && body->getMotionState()) {
			delete body->getMotionState();
		}
		mDynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	//delete collision shapes
	for(int j = 0; j < collisionShapes.size(); j++) {
		btCollisionShape* shape = collisionShapes[j];
		collisionShapes[j] = 0;
		delete shape;
	}

	//next line is optional: it will be cleared by the destructor when the array goes out of scope
	collisionShapes.clear();
}