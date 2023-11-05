#pragma once

namespace NCL {
	namespace CSC8503 {
		enum class CollisionLayer {
			DEFAULT,
			IGNORE_RAYCAST,
			RAY,
			IGNORE_ENVIRONMENT,
			IGNORE_ENV_AND_RAY,
			PLAYER,
			ENEMY,
			INTERACTABLE
		};
	}
}