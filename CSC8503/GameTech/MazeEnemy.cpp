#include "MazeEnemy.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/NavigationGrid.h"

#include "../CSC8503Common/Debug.h"
#include "../CSC8503Common/CollisionDetection.h"
#include "Game.h"

#include "../CSC8503Common/Ray.h"

NCL::CSC8503::MazeEnemy::MazeEnemy(Game* g, NavigationGrid* n, GameObject* p, const Vector3& startPos, vector<GameObject*> world) {
	navGrid = n;
	player = p;
	gameInstance = g;
	world.push_back(player);
	worldObjects = world;
	name = "Maze Enemy";

	transform.SetPosition(startPos);

	patrolSpeed = 3.0f;
	chaseSpeed = 6.0f;
	pathNodeTolerance = 100.0f;
	turningDist = 200.0f;
	hearingDistance = 15000.0f;
	visionDistance = 1000.0f;
	huntingPathChangeTime = 8.0f;
	huntingPathChangeCountdown = 0;
	powerUpTimer = 11;
	patrolSpeed *= 1.5f;
	chaseSpeed *= 1.5;
	poweredUp = false;

	availablePowerUp = nullptr;

	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)->void
		{
			this->Patrol(dt);
		}
	);

	State* stateB = new State([&](float dt)->void
		{
			this->HuntPlayer(dt);
		}
	);

	State* stateC = new State([&](float dt)->void
		{
			this->ChasePlayer(dt);
		}
	);

	State* stateD = new State([&](float dt)->void
		{
			this->GetPowerUp(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);
	stateMachine->AddState(stateC);
	stateMachine->AddState(stateD);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool
		{
			float playerDistance = (player->GetTransform().GetPosition() - transform.GetPosition()).LengthSquared();
			return playerDistance < hearingDistance;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool
		{
			float playerDistance = (player->GetTransform().GetPosition() - transform.GetPosition()).LengthSquared();
			return playerDistance > hearingDistance + 4000;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateA, stateC, [&]()->bool
		{
			return CanSeePlayer();
		}
	));

	stateMachine->AddTransition(new StateTransition(stateC, stateA, [&]()->bool
		{
			return !CanSeePlayer();
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateC, [&]()->bool
		{
			return CanSeePlayer();
		}
	));

	stateMachine->AddTransition(new StateTransition(stateC, stateB, [&]()->bool
		{
			return !CanSeePlayer();
		}
	));

	stateMachine->AddTransition(new StateTransition(stateA, stateD, [&]()->bool
		{
			return availablePowerUp;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateD, stateA, [&]()->bool
		{
			return !availablePowerUp;
		}
	));

	FindNewPath(navGrid->GetRandomValidPosition());
	currentPathNode.x += 10;
	currentPathNode.y = 0;
	currentPathNode.z += 10;
}

NCL::CSC8503::MazeEnemy::~MazeEnemy() {
	delete stateMachine;
}

void NCL::CSC8503::MazeEnemy::Update(float dt) {
	stateMachine->Update(dt);
	powerUpTimer += dt;
	if (powerUpTimer > 10 && poweredUp) {
		patrolSpeed /= 1.5f;
		chaseSpeed /= 1.5f;
		poweredUp = false;
	}
}

void NCL::CSC8503::MazeEnemy::TargetPowerUp(GameObject* powerUp) {
	availablePowerUp = powerUp;
}

void NCL::CSC8503::MazeEnemy::OnCollisionBegin(GameObject* otherObject) {
	if (otherObject->GetName() == "Power Up") {
		target = nullptr;
		availablePowerUp = false;
		gameInstance->CollectPowerUp(otherObject, false);
		powerUpTimer = 0;
		patrolSpeed *= 1.5f;
		chaseSpeed *= 1.5;
		poweredUp = true;
	}
}

void NCL::CSC8503::MazeEnemy::Reset() {
	FindNewPath(navGrid->GetRandomValidPosition());
	currentPathNode.x += 10;
	currentPathNode.y = 0;
	currentPathNode.z += 10;
}

void NCL::CSC8503::MazeEnemy::Patrol(float dt) {
	if (target) {
		target = nullptr;
		renderObject->SetColour(Debug::GREEN);
	}
	FollowPath(patrolSpeed);
}

void NCL::CSC8503::MazeEnemy::HuntPlayer(float dt) {
	if (target != player) {
		target = player;
		renderObject->SetColour(Vector4(1, 0.84f, 0, 1));
		FindNewPath(target->GetTransform().GetPosition());
	}

	huntingPathChangeCountdown += dt;

	if (huntingPathChangeCountdown >= huntingPathChangeTime) {
		FindNewPath(target->GetTransform().GetPosition());
		huntingPathChangeCountdown = 0;
	}

	FollowPath(chaseSpeed);
}

void NCL::CSC8503::MazeEnemy::GetPowerUp(float dt) {
	if (availablePowerUp && target != availablePowerUp) {
		target = availablePowerUp;
		FindNewPath(target->GetTransform().GetPosition());
	}
	FollowPath(chaseSpeed);
}

void NCL::CSC8503::MazeEnemy::ChasePlayer(float dt) {
	if (target) {
		target = nullptr;
		renderObject->SetColour(Debug::RED);
	}
	Vector3 direction = player->GetTransform().GetPosition() - transform.GetPosition();
	physicsObject->AddForce(direction.Normalised() * chaseSpeed);
}

bool NCL::CSC8503::MazeEnemy::CanSeePlayer() {
	RayCollision collision;
	Ray r(transform.GetPosition(), (player->GetTransform().GetPosition() - transform.GetPosition()).Normalised());

	GameObject* closest = nullptr;

	for (GameObject* i : worldObjects) {
		if (!i->GetBoundingVolume()) { 
			continue;
		}
		RayCollision thisCollision;
		if (CollisionDetection::RayIntersection(r, *i, thisCollision)) {
			if (thisCollision.rayDistance < collision.rayDistance) {
				thisCollision.node = i;
				collision = thisCollision;
				closest = i;
			}
		}
	}
	//Debug::DrawLine(transform.GetPosition(), collision.collidedAt, Debug::CYAN);
	return closest && closest->GetName() == "Player";
}

bool NCL::CSC8503::MazeEnemy::FindNewPath(const Vector3& targetPosition) {
	NavigationPath pathToTarget;
	bool pathFound = navGrid->FindPath(transform.GetPosition(), targetPosition, pathToTarget);

	currentPathNode = Vector3(0, 0, 0);

	if (pathFound) {
		currentPath.Clear();
		currentPath = pathToTarget;
		//DisplayPath();
		currentPath.PopWaypoint(currentPathNode);
		currentPathNode.x += 10;
		currentPathNode.y = 0;
		currentPathNode.z += 10;
	}

	return pathFound;
}

void NCL::CSC8503::MazeEnemy::FollowPath(float speed) {
	Vector3 lineToNode = currentPathNode - transform.GetPosition();

	float squaredDist = abs(lineToNode.LengthSquared());

	if (squaredDist < pathNodeTolerance) {
		bool pathFinished = !currentPath.PopWaypoint(currentPathNode);
		if (pathFinished) {
			FindNewPath(target ? target->GetTransform().GetPosition() : navGrid->GetRandomValidPosition());
		}
		currentPathNode.x += 10;
		currentPathNode.y = 0;
		currentPathNode.z += 10;
		lineToNode = currentPathNode - transform.GetPosition();
		squaredDist = abs(lineToNode.LengthSquared());
	}

	physicsObject->AddForce(lineToNode.Normalised() * speed * min(1, squaredDist/turningDist));
}

void NCL::CSC8503::MazeEnemy::DisplayPath() {
	path.clear();
	NavigationPath p = currentPath;
	Vector3 pos;
	while (p.PopWaypoint(pos)) {
		pos.x += 10;
		pos.z += 10;
		pos.y += 5;
		path.push_back(pos);
	}

	for (int i = 1; i < path.size(); ++i) {
		Vector3 a = path[i - 1];
		Vector3 b = path[i];

		Debug::DrawLine(a, b, Debug::RED, 30);
	}
}
