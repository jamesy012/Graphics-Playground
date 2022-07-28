#include "Physics.h"

#include <vector>

#include <btBulletDynamicsCommon.h>
#include <imgui.h>

#include "Engine.h"
#include "PlatformDebug.h"
#include "PhysicsObject.h"
#include "Transform.h"
#include "Graphics/Mesh.h"

extern ContactStartedCallback gContactStartedCallback;

Physics* gPhysics = nullptr;

//callback on every collision
void CollisionCallback(btPersistentManifold* const& manifold) {
	//LOGGER::Log("Collision\n");
}

void Physics::Startup() {
	ASSERT(gPhysics == nullptr);
	gPhysics = this;

	gContactStartedCallback = &CollisionCallback;

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

	gContactStartedCallback = 0;

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

	mCollisionsLastFrame = mDispatcher->getNumManifolds();
	for(int j = mCollisionsLastFrame - 1; j >= 0; j--) {
		const btPersistentManifold* manifold = mDispatcher->getManifoldByIndexInternal(j);
		const btManifoldPoint& collisionPoint = manifold->getContactPoint(0);
		const btVector3 position = collisionPoint.getPositionWorldOnB();
	}

	for(int j = mDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
		btCollisionObject* colObj = mDynamicsWorld->getCollisionObjectArray()[j];
		if(colObj->isActive()) {
			mActiveObjects++;
			//btRigidBody* body = btRigidBody::upcast(colObj);
			//PhysicsObject* object = ((PhysicsObject*)body->getUserPointer());
			//if(object) {
			//	object->UpdateFromPhysics();
			//}
		}
	}
}

void Physics::ImGuiWindow() {
	if(ImGui::Begin("Physics")) {
		ImGui::Text("Num Collision Objects: %i", mDynamicsWorld->getNumCollisionObjects());
		ImGui::Text("Num Active Objects: %i", mActiveObjects);
		ImGui::Text("Num Collisions: %i", mCollisionsLastFrame);
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
	//btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, aObject, groundShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	//body->setRollingFriction(0.5f);
	//body->setAngularFactor(0.5f);
	body->setFriction(1.0f);

	aObject->AttachRigidBody(body);
	aObject->UpdateToPhysics();

	//add the body to the dynamics world
	mDynamicsWorld->addRigidBody(body);
}

void Physics::AddingObjectsTestSphere(PhysicsObject* aObject) {
	btCollisionShape* colShape = new btSphereShape(btScalar(aObject->GetTransform()->GetLocalScale().x / 2));
	mCollisionShapes.push_back(colShape);

	AddRigidBody(aObject, colShape, 1.0f);
}

void Physics::AddingObjectsTestBox(PhysicsObject* aObject) {
	const glm::vec3 scale = aObject->GetTransform()->GetLocalScale() / 2.0f;
	btCollisionShape* colShape = new btBoxShape(btVector3(scale.x, scale.y, scale.z));
	mCollisionShapes.push_back(colShape);

	AddRigidBody(aObject, colShape, 1.0f);
}

void Physics::AddingObjectsTestMesh(PhysicsObject* aObject, Mesh* aMesh) {
	btTriangleIndexVertexArray* meshInterface = new btTriangleIndexVertexArray();
	btIndexedMesh part;

	for(int i = 0; i < aMesh->GetNumMesh(); i++) {
		part.m_vertexBase = (const unsigned char*)&aMesh->GetMesh(i).mVertices[0].mPos.x;
		part.m_vertexStride = sizeof(MeshVert);
		part.m_numVertices = aMesh->GetMesh(i).mVertices.size();
		part.m_vertexType = PHY_FLOAT;

		part.m_triangleIndexBase = (const unsigned char*)aMesh->GetMesh(i).mIndices.data();
		part.m_triangleIndexStride = sizeof(MeshIndex) * 3;
		part.m_numTriangles = aMesh->GetMesh(i).mIndices.size() / 3;
		part.m_indexType = PHY_INTEGER;

		meshInterface->addIndexedMesh(part, PHY_INTEGER);
	}

	bool useQuantizedAabbCompression = true;
	btCollisionShape* colShape = new btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression);

	glm::vec3 pos = aObject->GetTransform()->GetLocalPosition();
	glm::vec3 scale = aObject->GetTransform()->GetLocalScale();
	colShape->setLocalScaling(btVector3(btScalar(scale.x), btScalar(scale.y), btScalar(scale.z)));

	mCollisionShapes.push_back(colShape);

	AddRigidBody(aObject, colShape, 0.0f);
}

void Physics::AddRigidBody(PhysicsObject* aObject, btCollisionShape* aShape, float mass) {
	/// Create Dynamic Objects
	btTransform startTransform;
	startTransform.setIdentity();

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if(isDynamic)
		aShape->calculateLocalInertia(mass, localInertia);

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	//btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, aObject, aShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	body->setFriction(0.5f);
	body->setSpinningFriction(0.1f);
	body->setRollingFriction(0.1f);
	//body->setAngularFactor(btVector3(1, 0, 1));
	body->setDamping(0.0f, 0.2f);

	aObject->AttachRigidBody(body);
	aObject->UpdateToPhysics();

	mDynamicsWorld->addRigidBody(body);
}

void Physics::JoinTwoObject(PhysicsObject* aObject1, PhysicsObject* aObject2) {
	btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*aObject1->GetRigidBody(),
															   *aObject2->GetRigidBody(),
															   btVector3(0, aObject1->GetTransform()->GetLocalScale().y / 2.0f, 0),
															   btVector3(0, -aObject2->GetTransform()->GetLocalScale().y / 2.0f, 0));
	//p2p->m_setting.m_damping = 0.0625;
	//p2p->m_setting.m_impulseClamp = 0.95;
	//p2p->m_setting.m_tau = 0.5f;
	//p2p->m_setting.m_damping = 0.5f;
	mDynamicsWorld->addConstraint(p2p, true);
	aObject1->AddAttachment(p2p);
	aObject2->AddAttachment(p2p);
	//btTransform localA, localB;
	//float scale = 1.0f;
	//localA.setIdentity();
	//localB.setIdentity();
	//localA.getBasis().setEulerZYX(0, M_PI_2, 0);
	//localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.15), btScalar(0.)));
	//localB.getBasis().setEulerZYX(0, M_PI_2, 0);
	//localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.15), btScalar(0.)));
	//btHingeConstraint* hinge = new btHingeConstraint(*aObject1->GetRigidBody(), *aObject2->GetRigidBody(), localA, localB);
	//mDynamicsWorld->addConstraint(hinge);
	//mDynamicsWorld->addAction();
}

PhysicsObject* Physics::Raycast(const glm::vec3& aPosition, const glm::vec3& aDirection, const float aLength) const {
	const btVector3 rayFromWorld = btVector3(aPosition.x, aPosition.y, aPosition.z);
	const btVector3 rayToWorld = rayFromWorld + btVector3(aDirection.x, aDirection.y, aDirection.z) * aLength;
	btCollisionWorld::ClosestRayResultCallback result(rayFromWorld, rayToWorld);
	mDynamicsWorld->rayTest(rayFromWorld, rayToWorld, result);
	if(result.hasHit()) {
		const btRigidBody* body = btRigidBody::upcast(result.m_collisionObject);
		return (PhysicsObject*)body->getUserPointer();
	}
	return nullptr;
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