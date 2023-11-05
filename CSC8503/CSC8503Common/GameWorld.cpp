#include "GameWorld.h"
#include "GameObject.h"
#include "Constraint.h"
#include "CollisionDetection.h"
#include "../../Common/Camera.h"
#include <algorithm>

using namespace NCL;
using namespace NCL::CSC8503;

GameWorld::GameWorld() {
	mainCamera = new Camera();

	debugMode = false;
	shuffleConstraints = false;
	shuffleObjects = false;
	worldIDCounter = 0;
}

GameWorld::~GameWorld() {
	delete staticTree;
}

void GameWorld::Clear() {
	gameObjects.clear();
	constraints.clear();
}

void GameWorld::ClearAndErase() {
	for (auto& i : gameObjects) {
		delete i;
	}
	for (auto& i : constraints) {
		delete i;
	}
	Clear();
}

void GameWorld::AddGameObject(GameObject* o) {
	gameObjects.emplace_back(o);
	o->SetWorldID(worldIDCounter++);
}

void GameWorld::RemoveGameObject(GameObject* o, bool andDelete) {
	gameObjects.erase(std::remove(gameObjects.begin(), gameObjects.end(), o), gameObjects.end());

	if (andDelete) {
		delete o;
	}
}

void GameWorld::GetObjectIterators(
	GameObjectIterator& first,
	GameObjectIterator& last) const {

	first = gameObjects.begin();
	last = gameObjects.end();
}

void GameWorld::OperateOnContents(GameObjectFunc f) {
	for (GameObject* g : gameObjects) {
		f(g);
	}
}

void NCL::CSC8503::GameWorld::GetAwakeObjectIterators(
	GameObjectIterator& first, 
	GameObjectIterator& last) const {

	first = awakeObjects.begin();
	last = awakeObjects.end();
}

void GameWorld::UpdateWorld(float dt) {
	awakeObjects.clear();
	for (GameObject* g : gameObjects) {
		if (!g->IsSleeping()) {
			awakeObjects.emplace_back(g);
			g->Update(dt);
		}
	}

	if (shuffleObjects) {
		std::random_shuffle(gameObjects.begin(), gameObjects.end());
	}

	if (shuffleConstraints) {
		std::random_shuffle(constraints.begin(), constraints.end());
	}
}

bool GameWorld::Raycast(Ray& r, RayCollision& closestCollision, bool closestObject) const {
	//The simplest raycast just goes through each object and sees if there's a collision
	RayCollision collision;
	CollisionLayer closestObjLayer = CollisionLayer::DEFAULT;

	for (auto& i : gameObjects) {
		if (!i->GetBoundingVolume()) { //objects might not be collideable etc...
			continue;
		}
		RayCollision thisCollision;
		if (CollisionDetection::RayIntersection(r, *i, thisCollision)) {
			bool colAllowed = CollisionAllowed(CollisionLayer::RAY, i->GetLayer());
			if (!closestObject) {
				closestCollision = collision;
				closestCollision.node = i;
				return colAllowed;
			}
			else {
				if (colAllowed && thisCollision.rayDistance < collision.rayDistance) {
					thisCollision.node = i;
					collision = thisCollision;
					closestObjLayer = i->GetLayer();
				}
			}
		}
	}
	if (collision.node) {
		closestCollision = collision;
		closestCollision.node = collision.node;
		if (debugMode) {
			Debug::DrawLine(mainCamera->GetPosition(), collision.collidedAt, Debug::CYAN, 5);
		}
		return CollisionAllowed(CollisionLayer::RAY, closestObjLayer);
	}
	return false;
}

void NCL::CSC8503::GameWorld::BuildStaticTree() {
	if (staticTree) {
		delete staticTree;
	}

	staticTree = new Octree<GameObject*>(Vector3(1024, 1024, 1024), 7, 6);
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	GetObjectIterators(first, last);
	for (auto i = first; i != last; ++i) {
		(*i)->UpdateBroadphaseAABB();
		Vector3 halfSizes;
		if (!(*i)->GetBroadphaseAABB(halfSizes) || !(*i)->IsStatic()) {
			continue;
		}
		Vector3 pos = (*i)->GetTransform().GetPosition();
		staticTree->Insert(*i, pos, halfSizes);
	}
}

/*
Constraint Tutorial Stuff
*/

void GameWorld::AddConstraint(Constraint* c) {
	constraints.emplace_back(c);
}

void GameWorld::RemoveConstraint(Constraint* c, bool andDelete) {
	constraints.erase(std::remove(constraints.begin(), constraints.end(), c), constraints.end());
	if (andDelete) {
		delete c;
	}
}

void GameWorld::GetConstraintIterators(
	std::vector<Constraint*>::const_iterator& first,
	std::vector<Constraint*>::const_iterator& last) const {
	first = constraints.begin();
	last = constraints.end();
}