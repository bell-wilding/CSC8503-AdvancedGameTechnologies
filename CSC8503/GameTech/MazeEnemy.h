#pragma once

#include "..\CSC8503Common\GameObject.h"
#include "..\CSC8503Common\NavigationPath.h"

namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class NavigationGrid;
		class PlayerBall;
		class Game;

		class MazeEnemy : public GameObject {
		public:
			MazeEnemy(Game* g, NavigationGrid* n, GameObject* p, const Vector3& startPos, vector<GameObject*> world);
			~MazeEnemy();

			virtual void Update(float dt);

			void TargetPowerUp(GameObject* powerUp);

			void OnCollisionBegin(GameObject* otherObject) override;

			void Reset();

			bool IsPoweredUp() { return poweredUp; }

		protected:
			void Patrol(float dt);
			void HuntPlayer(float dt);
			void GetPowerUp(float dt);
			void ChasePlayer(float dt);

			bool CanSeePlayer();

			bool FindNewPath(const Vector3& targetPosition);
			void FollowPath(float speed);

			void DisplayPath();

			Game* gameInstance;
			NavigationGrid* navGrid;
			StateMachine* stateMachine;
			GameObject* player;
			vector<GameObject*> worldObjects;

			vector<Vector3> path;

			GameObject* target;
			GameObject* availablePowerUp;

			NavigationPath currentPath;
			Vector3 currentPathNode;

			float huntingPathChangeTime;
			float huntingPathChangeCountdown;

			float hearingDistance;
			float visionDistance;

			float patrolSpeed;
			float chaseSpeed;

			float pathNodeTolerance;
			float turningDist;

			float powerUpTimer;
			bool poweredUp;
		};
	}
}