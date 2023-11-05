#pragma once
#include <vector>
#include "Ray.h"
#include "CollisionDetection.h"
#include "Octree.h"
#include "CollisionLayer.h"
#include <algorithm>
#include <set>

namespace NCL {
	class Camera;
	using Maths::Ray;
	namespace CSC8503 {
		class GameObject;
		class Constraint;

		typedef std::function<void(GameObject*)> GameObjectFunc;
		typedef std::vector<GameObject*>::const_iterator GameObjectIterator;

		class GameWorld {
		public:
			GameWorld();
			~GameWorld();

			void Clear();
			void ClearAndErase();

			void AddGameObject(GameObject* o);
			void RemoveGameObject(GameObject* o, bool andDelete = false);

			void AddConstraint(Constraint* c);
			void RemoveConstraint(Constraint* c, bool andDelete = false);

			void BuildStaticTree();

			Octree<GameObject*>* GetStaticTree() const {
				return staticTree;
			}

			bool CollisionAllowed(CollisionLayer layerA, CollisionLayer layerB) const {
				return std::find(ignoreCollisionMatrix.begin(), ignoreCollisionMatrix.end(), std::make_pair(layerA, layerB)) == ignoreCollisionMatrix.end();
			}

			void AddCollisionIgnore(CollisionLayer a, CollisionLayer b) {
				ignoreCollisionMatrix.insert(std::make_pair(a, b));
				ignoreCollisionMatrix.insert(std::make_pair(b, a));
			}

			Camera* GetMainCamera() const {
				return mainCamera;
			}

			void ShuffleConstraints(bool state) {
				shuffleConstraints = state;
			}

			void ShuffleObjects(bool state) {
				shuffleObjects = state;
			}

			bool Raycast(Ray& r, RayCollision& closestCollision, bool closestObject = false) const;

			virtual void UpdateWorld(float dt);

			void OperateOnContents(GameObjectFunc f);

			void GetAwakeObjectIterators(
				GameObjectIterator& first,
				GameObjectIterator& last) const;

			void GetObjectIterators(
				GameObjectIterator& first,
				GameObjectIterator& last) const;

			void GetConstraintIterators(
				std::vector<Constraint*>::const_iterator& first,
				std::vector<Constraint*>::const_iterator& last) const;

		protected:
			std::vector<GameObject*> gameObjects;
			std::vector<GameObject*> awakeObjects;
			std::vector<Constraint*> constraints;

			Camera* mainCamera;

			std::set<std::pair<CollisionLayer, CollisionLayer>> ignoreCollisionMatrix;

			bool	shuffleConstraints;
			bool	shuffleObjects;
			int		worldIDCounter;

			bool	debugMode;

			Octree<GameObject*>* staticTree;
		};
	}
}
