#pragma once

#include "../CSC8503Common/GameObject.h"
#include "GameTechRenderer.h"

namespace NCL {
	namespace CSC8503 {
		class TwistingBlock : public GameObject {
		public:
			TwistingBlock(Vector3 twistForce) : GameObject("Twisting Block") {
				layer = CollisionLayer::INTERACTABLE;
				selected = false;
				force = twistForce;
			};
			~TwistingBlock() {};

			void Update(float dt) override {
				if (selected && Window::GetKeyboard()->KeyDown(KeyboardKeys::X)) {
					physicsObject->ApplyAngularImpulse(transform.GetOrientation() * force * dt);
				}
				if (selected && Window::GetKeyboard()->KeyDown(KeyboardKeys::C)) {
					physicsObject->ApplyAngularImpulse(transform.GetOrientation() * -force * dt);
				}
			}

			void SetSelected(bool s) {
				selected = s;
			}

			bool IsSelected() const {
				return selected;
			}

		protected:
			bool selected;

			Vector3 force;
		};
	}
}