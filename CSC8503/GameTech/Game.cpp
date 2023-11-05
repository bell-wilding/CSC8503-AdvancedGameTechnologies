#include "Game.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/SpringConstraint.h"
#include "../../Common/Quaternion.h"
#include "ConveyorPart.h"
#include "TwistingBlock.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "SwingingBlock.h"

#include "Checkpoint.h"
#include "PlayerBall.h"
#include "MazeEnemy.h"
#include "../CSC8503Common/NavigationGrid.h"

#include <fstream>
#include "../../Common/Assets.h"

using namespace NCL;
using namespace CSC8503;

Game::Game() {
	levelState = LevelState::MENU;

	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	world->AddCollisionIgnore(CollisionLayer::RAY, CollisionLayer::IGNORE_RAYCAST);
	world->AddCollisionIgnore(CollisionLayer::RAY, CollisionLayer::PLAYER);
	world->AddCollisionIgnore(CollisionLayer::IGNORE_ENV_AND_RAY, CollisionLayer::DEFAULT);
	world->AddCollisionIgnore(CollisionLayer::IGNORE_ENVIRONMENT, CollisionLayer::DEFAULT);
	world->AddCollisionIgnore(CollisionLayer::IGNORE_ENVIRONMENT, CollisionLayer::IGNORE_ENVIRONMENT);

	forceMagnitude = 5.0f;
	useGravity = false;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
	InitMenu();
}

void NCL::CSC8503::Game::UpdateLevel(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	if (selectionObject) {
		Vector3 rot = selectionObject->GetTransform().GetOrientation().ToEuler();
		Vector3 pos = selectionObject->GetTransform().GetPosition();
		string rotStr = "Rot [x: " + std::to_string(rot.x) + ", y: " + std::to_string(rot.y) + ", z: " + std::to_string(rot.z) + "]";
		string posStr = "Pos [x: " + std::to_string(pos.x) + ", y: " + std::to_string(pos.y) + ", z: " + std::to_string(pos.z) + "]";
		renderer->DrawString(rotStr, Vector2(1, 2), Debug::WHITE, 16.0f);
		renderer->DrawString(posStr, Vector2(1, 4), Debug::WHITE, 16.0f);
	}

	renderer->DrawString("Coins Collected: " + std::to_string(coinsCollected), Vector2(32, 94), Debug::MAGENTA);
	renderer->DrawString("Stopwatch: " + std::to_string(stopwatch).substr(0, 5), Vector2(34, 98), Debug::MAGENTA);
	stopwatch += dt;

	if (levelState == LevelState::LEVEL_ONE || levelState == LevelState::TEST_LEVEL) {
		SelectObject();
	}

	UpdateKeys();

	MoveSelectedObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}

	if (levelState == LevelState::LEVEL_ONE) {
		UpdateLevelOne(dt);
	}
	else if (levelState == LevelState::LEVEL_TWO) {
		UpdateLevelTwo(dt);
	}
	checkpointTextTimer += dt;
}

void NCL::CSC8503::Game::UpdateLevelOne(float dt) {
	for (int i = 0; i < 6; ++i) {
		if (checkpoints[i] && checkpoints[i]->IsActiveCheckpoint() && checkpoints[i] != activeCheckpoint) {
			activeCheckpoint->SetActiveCheckpoint(false);
			activeCheckpoint = checkpoints[i];
			checkpointTextTimer = 0;

			for (GameObject* g : activeZone) {
				g->SetSleeping(true);
			}
			if (i != 5) {
				activeZone = zoneObjects[i];
			}
			for (GameObject* g : activeZone) {
				g->SetSleeping(false);
			}
			break;
		}
	}

	if (playerBall->GetTransform().GetPosition().y < -20) {
		Respawn();
	}

	if (activeCheckpoint == checkpoints[5]) {
		ChangeLevel(LevelState::FINISHED);
	}
	else if (checkpointTextTimer < 2.0f) {
		renderer->DrawString("Checkpoint Reached!", Vector2(26, 50), Debug::MAGENTA, 26.0f);
	}
}

void NCL::CSC8503::Game::UpdateLevelTwo(float dt) {
	powerUpTimer += dt;
	powerUpTextTimer += dt;
	if (powerUpTimer > 20.0f) {
		if (!activePowerUp && !playerBall->IsPoweredUp() && !mazeEnemy->IsPoweredUp()) {
			Vector3 randomPowerUpPos = grid->GetRandomValidPosition();
			randomPowerUpPos.y = 3;
			randomPowerUpPos.x += 10;
			randomPowerUpPos.z += 10;
			AddPowerUpToWorld(randomPowerUpPos);
			powerUpTimer = 0;
		}
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		playerBall->GetPhysicsObject()->AddForce(Vector3(-playerBall->GetMoveSpeed(), 0, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		playerBall->GetPhysicsObject()->AddForce(Vector3(playerBall->GetMoveSpeed(), 0, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		playerBall->GetPhysicsObject()->AddForce(Vector3(0, 0, -playerBall->GetMoveSpeed()));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		playerBall->GetPhysicsObject()->AddForce(Vector3(0, 0, playerBall->GetMoveSpeed()));
	}

	if (powerUpTextTimer < 2) {
		if (playerPowerUp) {
			renderer->DrawString("Power Up Collected! Speed Increased!", Vector2(15, 50), Debug::MAGENTA, 24.0f);
		}
		else {
			renderer->DrawString("Enemy Power Up! Enemy Speed Increased!", Vector2(8, 50), Debug::MAGENTA, 24.0f);
		}
	}

	renderer->DrawString("Lives Left: " + std::to_string(lives), Vector2(37, 90), Debug::MAGENTA);

	if (lives == 0) {
		physics->Clear();
		ChangeLevel(LevelState::FINISHED);
	}
}

void NCL::CSC8503::Game::Respawn() {
	playerBall->GetPhysicsObject()->ClearForces();
	playerBall->GetPhysicsObject()->SetAngularVelocity(Vector3());
	playerBall->GetPhysicsObject()->SetLinearVelocity(Vector3());
	playerBall->GetTransform().SetOrientation(Quaternion());
	playerBall->GetTransform().SetPosition(activeCheckpoint->GetSpawnPosition());
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void Game::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh", &cubeMesh);
	loadFunc("sphere.msh", &sphereMesh);
	loadFunc("Male1.msh", &charMeshA);
	loadFunc("courier.msh", &charMeshB);
	loadFunc("security.msh", &enemyMesh);
	loadFunc("coin.msh", &bonusMesh);
	loadFunc("capsule.msh", &capsuleMesh);

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");
}

Game::~Game() {
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	delete playerBall;
	delete[] checkpoints;
}

void Game::UpdateGame(float dt) {
	if (levelState == LevelState::MENU) {
		renderer->DrawString("Welcome to my CSC8503 Project!", Vector2(13, 40), Debug::MAGENTA, 26);
		renderer->DrawString("> Press (1) to play the first level", Vector2(15, 60), Debug::WHITE);
		renderer->DrawString("> Press (2) to play the second level", Vector2(15, 65), Debug::WHITE);
		renderer->DrawString("[Press (P) to pause game when playing]", Vector2(13, 80), Debug::WHITE);
		renderer->DrawString("[Press (M) to return to menu when playing]", Vector2(10, 85), Debug::WHITE);
		renderer->DrawString("[Press (R) to restart from a checkpoint]", Vector2(11.5f, 90), Debug::WHITE);
	}
	else if (levelState == LevelState::PAUSED) {
		renderer->DrawString("Press (U) to unpause game", Vector2(27, 50), Debug::GREEN);
		renderer->DrawString("Coins Collected: " + std::to_string(coinsCollected), Vector2(32, 94), Debug::MAGENTA);
		renderer->DrawString("Stopwatch: " + std::to_string(stopwatch).substr(0, 5), Vector2(34, 98), Debug::MAGENTA);
	} 
	else if (levelState == LevelState::FINISHED) {
		renderer->DrawString("Game Over!", Vector2(36, 40), Debug::MAGENTA, 28);
		renderer->DrawString("Coins Collected: " + std::to_string(coinsCollected), Vector2(31, 70), Debug::WHITE);
		renderer->DrawString("Time Taken: " + std::to_string(stopwatch).substr(0, 5), Vector2(32, 75), Debug::WHITE);
		renderer->DrawString("Press (M) to return to menu", Vector2(24, 90), Debug::WHITE);
	}
	else {
		UpdateLevel(dt);
	}

	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();
}

void NCL::CSC8503::Game::ChangeLevel(LevelState l) {
	LevelState prevLevel = levelState;
	levelState = l;

	if (levelState == LevelState::PAUSED) {
		UpdateGame(0.001f);
	}

	if (levelState != prevLevel && prevLevel != LevelState::PAUSED) {
		switch (levelState) {
		case LevelState::MENU:
			InitMenu();
			break;
		case LevelState::LEVEL_ONE:
			InitLevelOne();
			break;
		case LevelState::LEVEL_TWO:
			InitLevelTwo();
			break;
		case LevelState::TEST_LEVEL:
			InitWorld();
			break;
		case LevelState::FINISHED:
			InitMenu();
			break;
		}
	}
}

void NCL::CSC8503::Game::CollectCoin(GameObject* coin) {
	if (coin->GetName() == "Coin") {
		world->RemoveGameObject(coin);
		++coinsCollected;
		if (levelState == LevelState::LEVEL_TWO) {
			Vector3 randomCoinPos = grid->GetRandomValidPosition();
			randomCoinPos.y = 3;
			randomCoinPos.x += 10;
			randomCoinPos.z += 10;
			AddCoinToWorld(randomCoinPos, 10)->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(90, 0, 0));
		}
	}
}

void NCL::CSC8503::Game::CollectPowerUp(GameObject* powerUp, bool playerCollection) {
	if (powerUp->GetName() == "Power Up") {
		world->RemoveGameObject(powerUp);
		playerPowerUp = playerCollection;
		powerUpTextTimer = 0;
		activePowerUp = nullptr;
		mazeEnemy->TargetPowerUp(nullptr);
	}
}

void NCL::CSC8503::Game::ResetLevelTwo() {
	playerBall->GetTransform().SetPosition(Vector3(30, -2, 30));
	playerBall->GetPhysicsObject()->ClearForces();
	playerBall->GetPhysicsObject()->SetAngularVelocity(Vector3());
	playerBall->GetPhysicsObject()->SetLinearVelocity(Vector3());
	playerBall->GetTransform().SetOrientation(Quaternion());

	mazeEnemy->GetTransform().SetPosition(Vector3(190, -2, 270));
	mazeEnemy->GetPhysicsObject()->ClearForces();
	mazeEnemy->GetPhysicsObject()->SetAngularVelocity(Vector3());
	mazeEnemy->GetPhysicsObject()->SetLinearVelocity(Vector3());
	mazeEnemy->GetTransform().SetOrientation(Quaternion());
	mazeEnemy->Reset();

	--lives;
}

void Game::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		//InitWorld(); //We can reset the simulation at any time with F1
		InitLevelOne();
		selectionObject = nullptr;
		lockedObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::R)) {
		if (levelState == LevelState::LEVEL_ONE) {
			Respawn();
		}
		else if (levelState == LevelState::LEVEL_TWO) {
			ResetLevelTwo();
		}
	}

	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void Game::LockedObjectMovement() {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	/*float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
	}*/
}

void Game::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void Game::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void Game::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	InitMixedGridWorld(5, 5, 3.5f, 3.5f);
	//BridgeConstraintTest();
	//InitGameExamples();
	InitDefaultFloor();
	InitCamera();

	useGravity = false;
	physics->UseGravity(false);
	selectionObject = nullptr;
	lockedObject = nullptr;

	world->BuildStaticTree();
	//testStateObject = AddStateObjectToWorld(Vector3(0, 10, 0));
}

void NCL::CSC8503::Game::InitMenu() {
	world->ClearAndErase();
	physics->Clear();
	InitCamera();
	world->GetMainCamera()->SetYaw(105.0f);
	world->GetMainCamera()->SetPitch(5.0f);

	for (int i = 0; i < 6; ++i) {
		checkpoints[i] = nullptr;
		if (i != 5) {
			zoneObjects[i].clear();
		}
	}
	activeZone.clear();
	activeCheckpoint = nullptr;
}

void NCL::CSC8503::Game::InitLevelOne() {
	world->ClearAndErase();
	physics->Clear();
	stopwatch = 0;
	checkpointTextTimer = 3;
	powerUpTextTimer = 3;

	useGravity = true;
	physics->UseGravity(useGravity);

	playerBall = AddPlayerBallToWorld(Vector3(0, 56, 0));
	//playerBall = AddPlayerBallToWorld(Vector3(0, 60, -350));
	//playerBall = AddPlayerBallToWorld(Vector3(0, 80, -45));

	checkpoints[0] = AddCatapult();
	checkpoints[1] = AddPinObstacle();
	checkpoints[2] = AddWaterArea();
	checkpoints[3] = AddConveyor();
	checkpoints[4] = AddIceSlimeObstacle();
	checkpoints[5] = AddCheckPoint(Vector3(0, -20, -460), Vector3(30, 16, 16), Vector3(0, 60, -295));

	activeCheckpoint = checkpoints[0];
	activeCheckpoint->SetActiveCheckpoint(true);
	activeZone = zoneObjects[0];

	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(5.0f);
	world->GetMainCamera()->SetYaw(330.0f);
	world->GetMainCamera()->SetPosition(Vector3(-30, 40, 60));
	selectionObject = nullptr;
	lockedObject = nullptr;

	world->BuildStaticTree();
	world->UpdateWorld(0.0001f);
	renderer->Update(0.0001f);
	renderer->Render();

	for (int i = 1; i < numZones; ++i) {
		for (GameObject* i : zoneObjects[i]) {
			i->SetSleeping(true);
		}
	}
}

void NCL::CSC8503::Game::InitLevelTwo() {
	world->ClearAndErase();
	physics->Clear();
	physics->SetLinearDamping(0.9f);
	useGravity = true;
	physics->UseGravity(useGravity);

	lives = 3;
	stopwatch = 0;
	checkpointTextTimer = 3;
	powerUpTimer = 0;
	powerUpTextTimer = 3;

	vector<GameObject*> objects;

	int gridHeight;
	int gridWidth;
	int size;
	std::ifstream infile(Assets::DATADIR + "Maze.txt");

	infile >> size;
	infile >> gridWidth;
	infile >> gridHeight;

	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			char type = 0;
			infile >> type;
			if (type == 'x') {
				objects.emplace_back(AddFloorToWorld(Vector3(x + 0.5f, 0, y + 0.5f) * size, Vector3(size, size, size) * 0.5f));
			}
		}
	}
	AddFloorToWorld(Vector3((gridWidth / 2.0f), -0.25f, (gridHeight / 2.0f)) * size, Vector3(gridWidth / 2.0f, 0.1f, gridHeight / 2.0f) * size);

	grid = new NavigationGrid("Maze.txt");

	playerBall = AddPlayerBallToWorld(Vector3(30, -2, 30), 5);
	playerBall->SetLayer(CollisionLayer::INTERACTABLE);
	mazeEnemy = AddMazeEnemyToWorld(Vector3(190, -2, 270), grid, objects);

	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(700.0f);
	world->GetMainCamera()->SetPitch(-90.0f);
	world->GetMainCamera()->SetYaw(0);
	world->GetMainCamera()->SetPosition(Vector3(200, 390, 150));

	Vector3 randomCoinPos = grid->GetRandomValidPosition();
	randomCoinPos.y = 3;
	randomCoinPos.x += 10;
	randomCoinPos.z += 10;
	AddCoinToWorld(randomCoinPos, 10)->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(90, 0, 0));

	forceMagnitude = playerBall->GetMoveSpeed();

	selectionObject = nullptr;
	lockedObject = nullptr;

	world->BuildStaticTree();

	selectionObject = playerBall;
}

Checkpoint* NCL::CSC8503::Game::AddCatapult() {
	vector<GameObject*> objects;

	objects.emplace_back(AddFloorToWorld(Vector3(0, 50, 0), Vector3(4, 0.5f, 40), Quaternion(0.25f, 0, 0, 1.0f)));

	GameObject* springBlock = AddOrientedCubeToWorld(Vector3(0, 37.75f, 26.5f), Vector3(4, 2, 0.5f), Quaternion(0.25f, 0, 0, 1.0f));
	springBlock->GetPhysicsObject()->SetUseGravity(false);
	springBlock->SetName("Spring Block");
	springBlock->SetLayer(CollisionLayer::INTERACTABLE);
	springBlock->GetRenderObject()->SetColour(Debug::RED);
	objects.emplace_back(springBlock);

	GameObject* springPoint = AddSphereToWorld(Vector3(0, 39.0f, 24.0f), 0.25f, Quaternion(0.25f, 0, 0, 1.0f));
	springPoint->GetPhysicsObject()->SetInverseMass(0);
	delete springPoint->GetBoundingVolume();
	springPoint->SetBoundingVolume(nullptr);
	delete springPoint->GetRenderObject();
	springPoint->SetRenderObject(nullptr);
	objects.emplace_back(springPoint);

	springDir = (springPoint->GetTransform().GetPosition() - springBlock->GetTransform().GetPosition()).Normalised();
	SpringConstraint* constraint = new SpringConstraint(springBlock, springPoint, new Spring(7.5f, 0.2f));
	constraint->SetActive(true);
	world->AddConstraint(constraint);

	objects.emplace_back(AddCoinToWorld(Vector3(0, 63, -20)));

	zoneObjects[0] = objects;

	return AddCheckPoint(Vector3(0, 56, 0), Vector3(10, 10, 10), Vector3(0, 56, 0));
}

Checkpoint* NCL::CSC8503::Game::AddPinObstacle() {
	vector<GameObject*> objects;

	objects.emplace_back(AddFloorToWorld(Vector3(0, 49.5f, -70.5), Vector3(18, 1.0f, 40), Quaternion(-0.25f, 0, 0, 1.0f)));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(-18.0f, 51.75f, -71.5f), Vector3(0.25f, 3.0f, 40), Quaternion(-0.25f, 0, 0, 1.0f), 0));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(18.0f, 51.75f, -71.5f), Vector3(0.25f, 3.0f, 40), Quaternion(-0.25f, 0, 0, 1.0f), 0));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(-12.5f, 38.0f, -100.5f), Vector3(0.25f, 3.0f, 10), Quaternion(-0.25f, -0.35f, 0, 1.0f), 0));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(12.5f, 38.0f, -100.5f), Vector3(0.25f, 3.0f, 10), Quaternion(-0.25f, 0.35f, 0, 1.0f), 0));

	GameObject* blocker = AddFloorToWorld(Vector3(0, 76, -42), Vector3(18, 0.5f, 8), Quaternion(-0.45f, 0, 0, 1.0f));
	blocker->GetPhysicsObject()->SetElasticity(0.0f);
	objects.emplace_back(blocker);

	float colSpacing = 8.0f;
	float rowSpacing = 12.0f;
	float xOffset = -15.95f;
	float zOffset = -90.0f;
	float height = 42.8f;

	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 5; ++z) {
			if (((x + z) % 2)) {
				Vector3 position = Vector3(xOffset + (z * colSpacing), height, zOffset + (x * rowSpacing));
				GameObject* capsule = AddCapsuleToWorld(position, 8.0f, 4.0f, Quaternion(-0.25f, 0, 0, 1.0f), 0);
				capsule->SetLayer(CollisionLayer::IGNORE_ENV_AND_RAY);
				capsule->GetPhysicsObject()->SetElasticity(2.2f);
				capsule->GetRenderObject()->SetColour(Vector4(0.8, 0, 0.8, 1.0));
				objects.emplace_back(capsule);
			}
		}
		height += 6.8f;
	}

	objects.emplace_back(AddCoinToWorld(Vector3(0, 57.5f, -66)));
	objects.emplace_back(AddCoinToWorld(Vector3(0, 42.5f, -90)));

	zoneObjects[1] = objects;

	return AddCheckPoint(Vector3(0, 76, -42), Vector3(20, 8, 8), Vector3(0, 80, -45));
}

Checkpoint* NCL::CSC8503::Game::AddWaterArea() {
	vector<GameObject*> objects;

	GameObject* waterPlane = AddOrientedCubeToWorld(Vector3(0, 28.0f, -145.5), Vector3(18, 4.0f, 40), Quaternion(), 0);
	waterPlane->GetPhysicsObject()->SetSpringCollisionResolve(true);
	waterPlane->GetRenderObject()->SetColour(Vector4(0, 0.8f, 0.9f, 1));
	waterPlane->GetRenderObject()->SetDefaultTexture(nullptr);
	objects.emplace_back(waterPlane);

	objects.emplace_back(AddOrientedCubeToWorld(Vector3(-18.0f, 29.5f, -145.5f), Vector3(0.25f, 6.0f, 40), Quaternion(0, 0, 0, 1.0f), 0));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(18.0f, 29.5f, -145.5f), Vector3(0.25f, 6.0f, 40), Quaternion(0, 0, 0, 1.0f), 0));

	GameObject* waterBlocker = AddAxisAlignedCubeToWorld(Vector3(0, 24.0f, -145.5), Vector3(18, 4.0f, 40), 0);
	waterBlocker->GetPhysicsObject()->SetElasticity(1.8f);
	waterBlocker->SetLayer(CollisionLayer::IGNORE_ENV_AND_RAY);
	waterBlocker->GetPhysicsObject()->SetApplyLinearFriction(false);
	objects.emplace_back(waterBlocker);

	GameObject* obj;
	for (int i = 0; i < 6; ++i) {
		float rotationAmount = RandomFloat(0.0f, 1.0f);
		if (rand() % 2) {
			obj = AddSphereToWorld(Vector3(RandomFloat(-17.0f, 17.0f), 35.0f, RandomFloat(-150, -110)), 2, Quaternion(rotationAmount, rotationAmount, rotationAmount, 1.0f), 10);
			obj->SetLayer(CollisionLayer::INTERACTABLE);
		}
		else {
			obj = AddCapsuleToWorld(Vector3(RandomFloat(-15.0f, 15.0f), 35.0f, RandomFloat(-150, -110)), 3, 1.5f, Quaternion(1.0f, rotationAmount, 1.0f, 1.0f), 10);
			obj->SetLayer(CollisionLayer::INTERACTABLE);
		}
		obj->GetRenderObject()->SetColour(Debug::RED);
		objects.emplace_back(obj);
	}

	objects.emplace_back(AddCoinToWorld(Vector3(0, 34.0f, -120)));
	objects.emplace_back(AddCoinToWorld(Vector3(-13, 34.0f, -145)));
	objects.emplace_back(AddCoinToWorld(Vector3(13, 34.0f, -145)));
	objects.emplace_back(AddCoinToWorld(Vector3(0, 34.0f, -170)));

	zoneObjects[2] = objects;

	return AddCheckPoint(Vector3(0, 34.5f, -110), Vector3(20, 8, 8), Vector3(0, 34.5f, -110));
}

GameObject* NCL::CSC8503::Game::AddConveyorPart(const Vector3& position, const Quaternion& rotation, const Vector3& force) {
	ConveyorPart* conveyor = new ConveyorPart(position, rotation, 40, 15, force);

	conveyor->SetRenderObject(new RenderObject(&conveyor->GetTransform(), capsuleMesh, basicTex, basicShader));
	conveyor->SetPhysicsObject(new PhysicsObject(&conveyor->GetTransform(), conveyor->GetBoundingVolume()));
	conveyor->SetLayer(CollisionLayer::IGNORE_ENV_AND_RAY);

	conveyor->GetPhysicsObject()->SetFriction(1.5f);
	conveyor->GetPhysicsObject()->InitCubeInertia();
	conveyor->GetPhysicsObject()->SetInverseMass(0);
	conveyor->GetPhysicsObject()->SetElasticity(1.4f);

	world->AddGameObject(conveyor);
	return conveyor;
}

Checkpoint* NCL::CSC8503::Game::AddConveyor() {
	vector<GameObject*> objects;

	float spacing = 20.0f;
	float ySpacing = 5.0f;
	for (int i = 0; i < 6; ++i) {
		objects.emplace_back(AddConveyorPart(Vector3(0, 18.0f + (i * ySpacing), -196 - (i * spacing)), Quaternion::EulerAnglesToQuaternion(0, 0, -90 + (i % 2 ? -5 : 5)), Vector3(-400, 0, 0)));
	}

	objects.emplace_back(AddCoinToWorld(Vector3(0, 46.0f, -235)));
	objects.emplace_back(AddCoinToWorld(Vector3(-14, 46.0f, -235)));
	objects.emplace_back(AddCoinToWorld(Vector3(14, 46.0f, -235)));
	objects.emplace_back(AddCoinToWorld(Vector3(0, 60.5f, -295)));
	objects.emplace_back(AddCoinToWorld(Vector3(-14, 60.5f, -295)));
	objects.emplace_back(AddCoinToWorld(Vector3(14, 60.5f, -295)));

	zoneObjects[3] = objects;

	return AddCheckPoint(Vector3(0, 34.5, -192), Vector3(20, 8, 8), Vector3(0, 38.5f, -196));
}

GameObject* NCL::CSC8503::Game::AddSlimeBlock(const Vector3& position, const Vector3& scale, const Quaternion& rotation) {
	GameObject* slime = new GameObject();

	OBBVolume* volume = new OBBVolume(scale);
	slime->SetBoundingVolume((CollisionVolume*)volume);
	slime->GetTransform()
		.SetScale(scale * 2)
		.SetPosition(position)
		.SetOrientation(rotation);

	slime->SetRenderObject(new RenderObject(&slime->GetTransform(), cubeMesh, basicTex, basicShader));
	slime->GetRenderObject()->SetColour(Vector4(0.8, 0, 0.8, 1.0));

	slime->SetPhysicsObject(new PhysicsObject(&slime->GetTransform(), slime->GetBoundingVolume()));
	slime->GetPhysicsObject()->SetFriction(2.5f);
	slime->GetPhysicsObject()->SetElasticity(0);
	slime->GetPhysicsObject()->SetApplyAngularFriction(false);
	slime->GetPhysicsObject()->SetInverseMass(0);

	slime->SetLayer(CollisionLayer::IGNORE_ENV_AND_RAY);

	world->AddGameObject(slime);

	return slime;
}

GameObject* NCL::CSC8503::Game::AddTwistingBlock(const Vector3& twistForce, const Vector3& position, const Vector3& dimensions, const Quaternion& rotation) {

	TwistingBlock* block = new TwistingBlock(twistForce);

	OBBVolume* volume = new OBBVolume(dimensions);

	block->SetBoundingVolume((CollisionVolume*)volume);

	block->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(rotation);

	block->SetRenderObject(new RenderObject(&block->GetTransform(), cubeMesh, basicTex, basicShader));
	block->GetRenderObject()->SetColour(Debug::RED);

	block->SetPhysicsObject(new PhysicsObject(&block->GetTransform(), block->GetBoundingVolume()));
	block->GetPhysicsObject()->SetInverseMass(0.01f);
	block->GetPhysicsObject()->InitCubeInertia();
	block->GetPhysicsObject()->SetApplyAngularFriction(false);
	block->GetPhysicsObject()->SetUseGravity(false);

	world->AddGameObject(block);

	return block;
}


Checkpoint* NCL::CSC8503::Game::AddIceSlimeObstacle() {
	vector<GameObject*> objects;

	GameObject* iceFloor = AddFloorToWorld(Vector3(0, 6, -383), Vector3(30, 1.0f, 80), Quaternion(-0.25f, 0, 0, 1.0f));
	iceFloor->GetPhysicsObject()->SetFriction(0.025f);
	iceFloor->GetPhysicsObject()->SetElasticity(0.0f);
	iceFloor->GetRenderObject()->SetColour(Vector4(0, 0.91, 0.96, 1));
	iceFloor->GetRenderObject()->SetDefaultTexture(nullptr);
	objects.emplace_back(iceFloor);

	float colSpacing = 10.0f;
	float rowSpacing = 20.0f;
	float xOffset = -18.95f;
	float zOffset = -350.0f;
	float height = 23.65f;

	for (int x = 0; x < 4; ++x) {
		for (int z = 0; z < 5; ++z) {
			if (((x + z) % 2)) {
				Vector3 position = Vector3(xOffset + (z * colSpacing), height, zOffset - (x * rowSpacing));
				objects.emplace_back(AddSlimeBlock(position, Vector3(5, 1.0f, 5), Quaternion(-0.25f, 0, 0, 1.0f)));
			}
		}
		height -= 10.65f;
	}

	objects.emplace_back(AddTwistingBlock(Vector3(0, 65000, 0), Vector3(0, 35, -335), Vector3(15, 2, 1), Quaternion(-0.25f, 0, 0, 1.0f)));
	objects.emplace_back(AddTwistingBlock(Vector3(0, 20000, 0), Vector3(-15, 20.5f, -362), Vector3(5, 2, 1), Quaternion(-0.25f, 0, 0, 1.0f)));
	objects.emplace_back(AddTwistingBlock(Vector3(0, 20000, 0), Vector3(15, 20.5f, -362), Vector3(5, 2, 1), Quaternion(-0.25f, 0, 0, 1.0f)));
	objects.emplace_back(AddTwistingBlock(Vector3(0, 30000, 0), Vector3(0, 10.5f, -381), Vector3(10, 2, 1), Quaternion(-0.25f, 0, 0, 1.0f)));
	objects.emplace_back(AddTwistingBlock(Vector3(0, 20000, 0), Vector3(-15, 0.5f, -400), Vector3(5, 2, 1), Quaternion(-0.25f, 0, 0, 1.0f)));
	objects.emplace_back(AddTwistingBlock(Vector3(0, 20000, 0), Vector3(15, 0.5f, -400), Vector3(5, 2, 1), Quaternion(-0.25f, 0, 0, 1.0f)));

	objects.emplace_back(AddCoinToWorld(Vector3(0, 20.0f, -362)));
	objects.emplace_back(AddCoinToWorld(Vector3(0, 1.0f, -400)));

	objects.emplace_back(AddFloorToWorld(Vector3(0, -20, -525), Vector3(30, 1.0f, 100)));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(-18.0f, -15.5f, -430.5f), Vector3(0.25f, 3.0f, 15), Quaternion(-0.25f, -0.45f, 0, 1.0f), 0));
	objects.emplace_back(AddOrientedCubeToWorld(Vector3(18.0f, -15.5f, -430.5f), Vector3(0.25f, 3.0f, 15), Quaternion(-0.25f, 0.45f, 0, 1.0f), 0));

	objects.emplace_back(AddSwingPoint(Vector3(0, -20.0f, -435.5f), 25));

	zoneObjects[4] = objects;

	return AddCheckPoint(Vector3(0, 50, -320), Vector3(30, 16, 16), Vector3(0, 60, -295));
}

void Game::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 8, 8);

	float	invCubeMass	 = 5;
	int		numLinks	 = 10;
	float	maxDistance  = 30;
	float	cubeDistance = 20;

	Vector3 startPos = Vector3(10, 150, 10);

	GameObject* start = AddAxisAlignedCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddAxisAlignedCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddAxisAlignedCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* Game::AddFloorToWorld(const Vector3& position, const Vector3& scale, const Quaternion& rotation) {
	GameObject* floor = new GameObject("Floor");

	OBBVolume* volume = new OBBVolume(scale);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(scale * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetTransform().SetOrientation(rotation);
	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* Game::AddSphereToWorld(const Vector3& position, float radius, const Quaternion& rotation, float inverseMass) {
	GameObject* sphere = new GameObject("Sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetTransform().SetOrientation(rotation);
	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* Game::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, const Quaternion& rotation, float inverseMass) {
	GameObject* capsule = new GameObject("Capsule");

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius * 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetTransform().SetOrientation(rotation);
	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

PlayerBall* NCL::CSC8503::Game::AddPlayerBallToWorld(const Vector3& position, float radius) {

	PlayerBall* playerBall = new PlayerBall(this);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	playerBall->SetBoundingVolume((CollisionVolume*)volume);

	playerBall->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	playerBall->SetRenderObject(new RenderObject(&playerBall->GetTransform(), sphereMesh, basicTex, basicShader));
	playerBall->SetPhysicsObject(new PhysicsObject(&playerBall->GetTransform(), playerBall->GetBoundingVolume()));

	playerBall->GetPhysicsObject()->SetInverseMass(10);
	playerBall->GetPhysicsObject()->InitSphereInertia();

	playerBall->SetLayer(CollisionLayer::PLAYER);
	playerBall->GetRenderObject()->SetColour(Debug::CYAN);

	world->AddGameObject(playerBall);

	return playerBall;
}

MazeEnemy* NCL::CSC8503::Game::AddMazeEnemyToWorld(const Vector3& position, NavigationGrid* grid, vector<GameObject*> objects) {

	MazeEnemy* mazeEnemy = new MazeEnemy(this, grid, playerBall, position, objects);

	Vector3 sphereSize = Vector3(5, 5, 5);
	SphereVolume* volume = new SphereVolume(5);
	mazeEnemy->SetBoundingVolume((CollisionVolume*)volume);

	mazeEnemy->GetTransform()
		.SetScale(sphereSize);

	mazeEnemy->SetRenderObject(new RenderObject(&mazeEnemy->GetTransform(), sphereMesh, basicTex, basicShader));
	mazeEnemy->SetPhysicsObject(new PhysicsObject(&mazeEnemy->GetTransform(), mazeEnemy->GetBoundingVolume()));

	mazeEnemy->GetPhysicsObject()->SetInverseMass(10);
	mazeEnemy->GetPhysicsObject()->InitSphereInertia();
	mazeEnemy->GetPhysicsObject()->SetFriction(0.95f);

	mazeEnemy->SetLayer(CollisionLayer::ENEMY);
	mazeEnemy->GetRenderObject()->SetColour(Debug::GREEN);

	world->AddGameObject(mazeEnemy);

	return mazeEnemy;
}

GameObject* Game::AddOrientedCubeToWorld(const Vector3& position, Vector3 dimensions, const Quaternion& rotation, float inverseMass) {
	GameObject* cube = new GameObject("Cube");

	OBBVolume* volume = new OBBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetTransform().SetOrientation(rotation);
	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* Game::AddAxisAlignedCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject("Cube");

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

void Game::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, Quaternion(0, 0, 0, 0), 1.0f);
		}
	}
	AddAxisAlignedCubeToWorld(Vector3(0, -2, 0), Vector3(100, 2, 100), 0);
}

void Game::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			int val = rand() % 3;
			if (val == 0) {
				AddSphereToWorld(position, sphereRadius);
			}
			if (val == 1) {
				AddCapsuleToWorld(position, 2.0f, sphereRadius);
			}
			if (val == 2) {
				AddOrientedCubeToWorld(position, cubeDims);
			}
		}
	}
}

void Game::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddOrientedCubeToWorld(position, cubeDims);
		}
	}
}

void Game::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void Game::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddCoinToWorld(Vector3(10, 5, 0));
}

GameObject* Game::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject("Character");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	//lockedObject = character;

	return character;
}

GameObject* Game::AddEnemyToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject("Enemy");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* Game::AddCoinToWorld(const Vector3& position, int scale) {
	GameObject* coin = new GameObject("Coin");

	SphereVolume* volume = new SphereVolume(scale);
	coin->SetBoundingVolume((CollisionVolume*)volume);
	coin->GetTransform()
		.SetScale(Vector3(scale/7.0f, scale / 7.0f, scale / 7.0f))
		.SetPosition(position);
	coin->SetTrigger(true);

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), bonusMesh, nullptr, basicShader));
	coin->GetRenderObject()->SetColour(Vector4(0.93f, 0.79f, 0, 1));

	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume()));
	coin->GetPhysicsObject()->SetInverseMass(10);
	coin->GetPhysicsObject()->SetUseGravity(false);
	coin->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(coin);

	return coin;
}

GameObject* NCL::CSC8503::Game::AddPowerUpToWorld(const Vector3& position)
{
	GameObject* powerUp = new GameObject("Power Up");

	CapsuleVolume* volume = new CapsuleVolume(15, 7.5f);
	powerUp->SetBoundingVolume((CollisionVolume*)volume);
	powerUp->GetTransform()
		.SetScale(Vector3(15, 15, 15))
		.SetPosition(position);
	powerUp->SetTrigger(true);

	powerUp->SetRenderObject(new RenderObject(&powerUp->GetTransform(), capsuleMesh, basicTex, basicShader));
	powerUp->GetRenderObject()->SetColour(Debug::MAGENTA);

	powerUp->SetPhysicsObject(new PhysicsObject(&powerUp->GetTransform(), powerUp->GetBoundingVolume()));
	powerUp->GetPhysicsObject()->SetInverseMass(10);
	powerUp->GetPhysicsObject()->SetUseGravity(false);
	powerUp->GetPhysicsObject()->InitCubeInertia();

	activePowerUp = powerUp;
	world->AddGameObject(powerUp);
	mazeEnemy->TargetPowerUp(powerUp);

	return powerUp;
}

GameObject* NCL::CSC8503::Game::AddSwingPoint(const Vector3& position, float distance)
{
	SwingingBlock* swing = new SwingingBlock(position, Vector3(50, 0, 0));

	AABBVolume* volume = new AABBVolume(Vector3(8, 5, 3));

	swing->SetBoundingVolume((CollisionVolume*)volume);

	swing->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(8, 5, 3) * 2);

	swing->SetRenderObject(new RenderObject(&swing->GetTransform(), cubeMesh, basicTex, basicShader));
	swing->SetPhysicsObject(new PhysicsObject(&swing->GetTransform(), swing->GetBoundingVolume()));

	swing->GetPhysicsObject()->SetInverseMass(10);
	swing->GetPhysicsObject()->InitCubeInertia();

	GameObject* swingPoint = new GameObject();
	Vector3 pos = position;
	pos.y += distance;
	swingPoint->GetTransform().SetPosition(pos);
	swingPoint->SetPhysicsObject(new PhysicsObject(&swingPoint->GetTransform(), nullptr));
	swingPoint->GetPhysicsObject()->SetInverseMass(0);

	PositionConstraint* constraint = new PositionConstraint(swing, swingPoint, distance);
	world->AddGameObject(swing);
	world->AddGameObject(swingPoint);

	world->AddConstraint(constraint);

	return swing;
}

Checkpoint* NCL::CSC8503::Game::AddCheckPoint(const Vector3& position, const Vector3& dimensions, const Vector3& spawnPos)
{
	Checkpoint* checkpoint = new Checkpoint(spawnPos);

	AABBVolume* volume = new AABBVolume(dimensions);

	checkpoint->SetBoundingVolume((CollisionVolume*)volume);

	checkpoint->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	//checkpoint->SetRenderObject(new RenderObject(&checkpoint->GetTransform(), cubeMesh, basicTex, basicShader));
	checkpoint->SetPhysicsObject(new PhysicsObject(&checkpoint->GetTransform(), checkpoint->GetBoundingVolume()));

	checkpoint->GetPhysicsObject()->SetInverseMass(0);

	world->AddGameObject(checkpoint);

	return checkpoint;
}

StateGameObject* NCL::CSC8503::Game::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* basicAI = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	basicAI->SetBoundingVolume((CollisionVolume*)volume);
	basicAI->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	basicAI->SetRenderObject(new RenderObject(&basicAI->GetTransform(), bonusMesh, nullptr, basicShader));
	basicAI->SetPhysicsObject(new PhysicsObject(&basicAI->GetTransform(), basicAI->GetBoundingVolume()));

	basicAI->GetPhysicsObject()->SetInverseMass(1.0f);
	basicAI->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(basicAI);

	return basicAI;
}

// Taken from: https://stackoverflow.com/questions/5289613/generate-random-float-between-two-floats/5289624
float NCL::CSC8503::Game::RandomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool Game::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}

	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				if (selectionObject->GetName() == "Twisting Block") {
					static_cast<TwistingBlock*>(selectionObject)->SetSelected(false);
				}
				selectionObject->GetRenderObject()->SetColour(selectedObjCol);
				selectionObject = nullptr;
				lockedObject = nullptr;

			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectedObjCol = selectionObject->GetRenderObject()->GetColour();
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));

				if (selectionObject->GetName() == "Twisting Block") {
					static_cast<TwistingBlock*>(selectionObject)->SetSelected(true);
				}
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if (selectionObject) {
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void Game::MoveSelectedObject() {
	if (levelState == LevelState::LEVEL_ONE) {
		renderer->DrawString("Interaction Force:" + std::to_string(forceMagnitude),
			Vector2(2, 20));
		forceMagnitude += Window::GetMouse()->GetWheelMovement() * 10.0f;

	}

	if (!selectionObject) {
		return;
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD4)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(-forceMagnitude, 0, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD6)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(forceMagnitude, 0, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD8)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -forceMagnitude));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD5)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, forceMagnitude));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD9)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, forceMagnitude, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD7)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -forceMagnitude, 0));
	}

	if (selectionObject->GetName() == "Spring Block" && Window::GetKeyboard()->KeyDown(KeyboardKeys::F)) {
		selectionObject->GetPhysicsObject()->AddForce(-springDir * forceMagnitude);
	}

	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(
			*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->
					AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}