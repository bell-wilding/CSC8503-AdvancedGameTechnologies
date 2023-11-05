#include "SpringConstraint.h"
#include "GameObject.h"
#include "Debug.cpp"

NCL::CSC8503::SpringConstraint::SpringConstraint(GameObject* block, GameObject* point, Spring* s, float forceAmount, float maxDist) {
	springBlock = block;
	staticPoint = point;
	spring = s;
	force = forceAmount;

	active = false;

	Vector3 travelDir = (springBlock->GetTransform().GetPosition() - staticPoint->GetTransform().GetPosition()).Normalised();

	orientation = springBlock->GetTransform().GetOrientation();
}

void NCL::CSC8503::SpringConstraint::UpdateConstraint(float dt) {
	if (active) {
		Vector3 d = staticPoint->GetTransform().GetPosition() - springBlock->GetTransform().GetPosition();
		Vector3 v = staticPoint->GetPhysicsObject()->GetLinearVelocity() - springBlock->GetPhysicsObject()->GetLinearVelocity();
		Vector3 force = spring->CalcSpringForce(d, v);
		springBlock->GetPhysicsObject()->AddForce(-force);
	}

	springBlock->GetTransform().SetOrientation(orientation);
}