#pragma once

#include "../../Common/Vector3.h"

namespace NCL {
	using namespace Maths;
	namespace CSC8503 {
		class Spring {
		public:
			Spring(float kVal = 1.0f, float dampFac = 0.5f) : k(kVal), dampingFactor(dampFac) {};
			~Spring() {};

			float CalcResolutionForce(float penetrationDepth, Vector3 relativeVel, Vector3 contactNormal) {
				return (-k * penetrationDepth) - (dampingFactor * Vector3::Dot(contactNormal, relativeVel));
			};

			Vector3 CalcSpringForce(Vector3 relativeDist, Vector3 relativeVel) {
				return relativeDist * -k - relativeVel * dampingFactor;
			};


		protected:
			float k;
			float dampingFactor;
		};
	}
}