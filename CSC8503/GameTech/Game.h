#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "StateGameObject.h"
#include "../CSC8503Common/Spring.h"

namespace NCL {
	namespace CSC8503 {
		class Checkpoint;
		class PlayerBall;
		class MazeEnemy;
		class NavigationGrid;

		enum LevelState {
			MENU = 0,
			LEVEL_ONE = 1,
			LEVEL_TWO = 2,
			TEST_LEVEL = 3,
			PAUSED = 4,
			FINISHED = 5
		};

		class Game {
		public:
			Game();
			~Game();

			virtual void UpdateGame(float dt);
			void ChangeLevel(LevelState l);

			void CollectCoin(GameObject* coin);
			void CollectPowerUp(GameObject* powerUp, bool playerCollection = true);
			void ResetLevelTwo();

		protected:
			void UpdateLevel(float dt);
			void UpdateLevelOne(float dt);
			void UpdateLevelTwo(float dt);

			void Respawn();

			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			void InitMenu();
			void InitLevelOne();
			void InitLevelTwo();

			Checkpoint* AddCatapult();
			Checkpoint* AddPinObstacle();
			Checkpoint* AddWaterArea();

			GameObject* AddConveyorPart(const Vector3& position, const Quaternion& rotation, const Vector3& force);
			Checkpoint* AddConveyor();

			GameObject* AddSlimeBlock(const Vector3& position, const Vector3& scale = Vector3(100, 2, 100), const Quaternion& rotation = Quaternion());
			GameObject* AddTwistingBlock(const Vector3& twistForce, const Vector3& position, const Vector3& scale = Vector3(100, 2, 100), const Quaternion& rotation = Quaternion());
			Checkpoint* AddIceSlimeObstacle();

			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void BridgeConstraintTest();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position, const Vector3& scale = Vector3(100, 2, 100), const Quaternion& rotation = Quaternion());
			GameObject* AddSphereToWorld(const Vector3& position, float radius, const Quaternion& rotation = Quaternion(), float inverseMass = 10.0f);
			GameObject* AddOrientedCubeToWorld(const Vector3& position, Vector3 dimensions = Vector3(1, 1, 1), const Quaternion& rotation = Quaternion(), float inverseMass = 10.0f);
			GameObject* AddAxisAlignedCubeToWorld(const Vector3& position, Vector3 dimensions = Vector3(1, 1, 1), float inverseMass = 10.0f);
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, const Quaternion& rotation = Quaternion(), float inverseMass = 10.0f);

			PlayerBall* AddPlayerBallToWorld(const Vector3& position, float radius = 1);
			MazeEnemy* AddMazeEnemyToWorld(const Vector3& position, NavigationGrid* grid, vector<GameObject*> objects);

			GameObject* AddCoinToWorld(const Vector3& position, int scale = 2.0f);
			GameObject* AddPowerUpToWorld(const Vector3& position);

			GameObject* AddSwingPoint(const Vector3& position, float distance);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);

			Checkpoint* AddCheckPoint(const Vector3& position, const Vector3& dimensions, const Vector3& spawnPos);

			StateGameObject* AddStateObjectToWorld(const Vector3& position);

			float RandomFloat(float a, float b);

			GameTechRenderer* renderer;
			PhysicsSystem* physics;
			GameWorld* world;
			NavigationGrid* grid;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;
			PlayerBall* playerBall = nullptr;
			MazeEnemy* mazeEnemy = nullptr;

			OGLMesh* capsuleMesh = nullptr;
			OGLMesh* cubeMesh = nullptr;
			OGLMesh* sphereMesh = nullptr;
			OGLTexture* basicTex = nullptr;
			OGLShader* basicShader = nullptr;

			//Coursework Meshes
			OGLMesh* charMeshA = nullptr;
			OGLMesh* charMeshB = nullptr;
			OGLMesh* enemyMesh = nullptr;
			OGLMesh* bonusMesh = nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject = nullptr;
			Vector3 lockedOffset = Vector3(0, 14, 20);
			Vector4 selectedObjCol = Vector4(1, 1, 1, 1);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			StateGameObject* testStateObject;

			Vector3 springDir;

			int numZones = 5;
			vector<GameObject*> zoneObjects[5];
			vector<GameObject*> activeZone;

			Checkpoint* activeCheckpoint;
			Checkpoint* checkpoints[6];
			float checkpointTextTimer;

			float stopwatch;
			int coinsCollected;
			int lives = 3;

			GameObject* activePowerUp;
			float powerUpTimer;
			float powerUpTextTimer;
			bool playerPowerUp;

			LevelState levelState;
		};
	}
}
