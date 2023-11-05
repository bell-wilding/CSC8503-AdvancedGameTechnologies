#pragma once

#include "../CSC8503Common/GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Game;
		class PlayerBall : public GameObject {
		public:
			PlayerBall(Game* g);
			~PlayerBall() {};

			void Update(float dt) override;

			void OnCollisionBegin(GameObject* otherObject) override;

			float GetMoveSpeed() { return moveSpeed; }
			bool IsPoweredUp() { return poweredUp; }

		protected:
			Game* gameInstance;

			bool poweredUp;
			float powerUpTimer;

			float moveSpeed;
		};
	}
}