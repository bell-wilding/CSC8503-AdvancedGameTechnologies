#pragma once

#include "../CSC8503Common/GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class ConveyorPart : public GameObject {
		public:
			ConveyorPart(Vector3 position, Quaternion rotation, float halfHeight, float radius, Vector3 f);
			~ConveyorPart() {};

			void Update(float dt) override;

		protected:
			Vector3 force;
		};
	}
}

