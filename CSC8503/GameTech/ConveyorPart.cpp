#include "ConveyorPart.h"
#include "../CSC8503Common/CapsuleVolume.h"

NCL::CSC8503::ConveyorPart::ConveyorPart(Vector3 position, Quaternion rotation, float halfHeight, float radius, Vector3 f) {
	force = f;

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	SetBoundingVolume((CollisionVolume*)volume);

	GetTransform()
		.SetScale(Vector3(radius * 2, halfHeight, radius * 2))
		.SetPosition(position)
		.SetOrientation(rotation);
}
void NCL::CSC8503::ConveyorPart::Update(float dt) {
	physicsObject->SetAngularVelocity(force * dt);
}
