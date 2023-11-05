#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "CollisionLayer.h"

#include "PhysicsObject.h"
#include "RenderObject.h"

#include <vector>
using std::vector;

namespace NCL {
	namespace CSC8503 {

		class GameObject {
		public:
			GameObject(string name = "");
			~GameObject();

			void SetBoundingVolume(CollisionVolume* vol) {
				boundingVolume = vol;
			}

			const CollisionVolume* GetBoundingVolume() const {
				return boundingVolume;
			}

			bool IsActive() const {
				return isActive;
			}

			bool IsSleeping() const {
				return isSleeping;
			}

			void SetSleeping(bool s) {
				isSleeping = s;
			}

			Transform& GetTransform() {
				return transform;
			}

			RenderObject* GetRenderObject() const {
				return renderObject;
			}

			PhysicsObject* GetPhysicsObject() const {
				return physicsObject;
			}

			void SetRenderObject(RenderObject* newObject) {
				renderObject = newObject;
			}

			void SetPhysicsObject(PhysicsObject* newObject) {
				physicsObject = newObject;
			}

			void SetName(const string& n) {
				name = n;
			}

			const string& GetName() const {
				return name;
			}

			const CollisionLayer GetLayer() const {
				return layer;
			}

			void SetLayer(CollisionLayer l) {
				layer = l;
			}

			virtual void OnCollisionBegin(GameObject* otherObject) {
				//std::cout << "OnCollisionBegin event occured!\n";
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
				//std::cout << "OnCollisionEnd event occured!\n";
			}

			bool IsTrigger() const {
				return isTrigger;
			}

			void SetTrigger(bool t) {
				isTrigger = t;
			}

			virtual void Update(float dt) {};

			bool GetBroadphaseAABB(Vector3& outsize) const;

			void UpdateBroadphaseAABB();

			void SetWorldID(int newID) {
				worldID = newID;
			}

			int	GetWorldID() const {
				return worldID;
			}

			bool IsStatic() const {
				return physicsObject && physicsObject->GetInverseMass() == 0;
			}

		protected:
			Transform			transform;

			CollisionVolume* boundingVolume;
			PhysicsObject* physicsObject;
			RenderObject* renderObject;

			bool			isTrigger;
			bool			isActive;
			bool			isSleeping;
			int				worldID;
			string			name;
			CollisionLayer  layer;

			Vector3 broadphaseAABB;
		};
	}
}
