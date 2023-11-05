#include "PlayerBall.h"
#include "Game.h"

NCL::CSC8503::PlayerBall::PlayerBall(Game* g) {
	gameInstance = g;
	name = "Player";
	powerUpTimer = 11;
	moveSpeed = 8;
	moveSpeed *= 1.5f;
}

void NCL::CSC8503::PlayerBall::Update(float dt) {
	powerUpTimer += dt;
	if (powerUpTimer > 10 && poweredUp) {
		moveSpeed /= 1.5f;
		poweredUp = false;
	}
}

void NCL::CSC8503::PlayerBall::OnCollisionBegin(GameObject* otherObject) {
	if (otherObject->GetName() == "Coin") {
		gameInstance->CollectCoin(otherObject);
	}

	if (otherObject->GetName() == "Power Up") {
		gameInstance->CollectPowerUp(otherObject);
		powerUpTimer = 0;
		moveSpeed *= 1.5f;
		poweredUp = true;
	}

	if (otherObject->GetName() == "Maze Enemy") {
		gameInstance->ResetLevelTwo();
	}
}
