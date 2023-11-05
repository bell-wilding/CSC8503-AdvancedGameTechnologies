#pragma once

#include "../CSC8503Common/GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class PositionConstraint;

		class SwingingBlock: public GameObject {
		public:
			SwingingBlock(Vector3 position, Vector3 f);
			~SwingingBlock();

			void Update(float dt) override;

		protected:
			void SwingLeft(float dt);
			void SwingRight(float dt);

			StateMachine* stateMachine;

			Vector3 force;
			float swingTimer;
		};
	}
}

