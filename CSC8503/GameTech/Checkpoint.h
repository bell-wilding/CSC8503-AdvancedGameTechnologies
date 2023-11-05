#pragma once

#include "../CSC8503Common/GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Checkpoint : public GameObject {
		public:
			Checkpoint(Vector3 spawnPos) : spawnPosition(spawnPos), isActiveCheckpoint(false) { 
				isTrigger = true;
				SetLayer(CollisionLayer::IGNORE_RAYCAST);
			};
			~Checkpoint() {};

			void OnCollisionBegin(GameObject* otherObject) override {
				isActiveCheckpoint = otherObject->GetName() == "Player";
			}

			Vector3 GetSpawnPosition() const {
				return spawnPosition;
			}

			void SetActiveCheckpoint(bool a) {
				isActiveCheckpoint = a;
			}

			bool IsActiveCheckpoint() {
				return isActiveCheckpoint;
			}

		protected:
			bool isActiveCheckpoint;
			Vector3 spawnPosition;
		};
	}
}