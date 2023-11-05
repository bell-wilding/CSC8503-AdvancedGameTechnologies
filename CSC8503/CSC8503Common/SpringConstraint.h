#pragma once

#include "Constraint.h"
#include "Spring.h"
#include "../../Common/Vector3.h"
#include "../../Common/Quaternion.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class SpringConstraint : public Constraint {
		public:
			SpringConstraint(GameObject* block, GameObject* point, Spring* s, float forceAmount = 10.0f, float maxDist = 10.0f);

			~SpringConstraint() {
				delete spring;
			};

			void UpdateConstraint(float dt) override;

			void SetActive(bool a) {
				active = a;
			}

		protected:
			GameObject* springBlock;
			GameObject* staticPoint;

			Quaternion orientation;

			Spring* spring;

			bool active;

			float force;

			//float distanceFromStaticPoint
		};
	}
}