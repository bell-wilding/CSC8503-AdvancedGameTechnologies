#include "SwingingBlock.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/PositionConstraint.h"

NCL::CSC8503::SwingingBlock::SwingingBlock(Vector3 position, Vector3 f) {

	transform.SetPosition(position);
	force = f;

	swingTimer = 0.0f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)->void
		{
			this->SwingLeft(dt);
		}
	);

	State* stateB = new State([&](float dt)->void
		{
			this->SwingRight(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool
		{
			return this->swingTimer > 1.0f;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool
		{
			return this->swingTimer < -1.0f;
		}
	));
}

NCL::CSC8503::SwingingBlock::~SwingingBlock() {
	delete stateMachine;
}

void NCL::CSC8503::SwingingBlock::Update(float dt) {
	stateMachine->Update(dt);
}

void NCL::CSC8503::SwingingBlock::SwingLeft(float dt) {
	physicsObject->AddForce(force);
	swingTimer += dt;
}

void NCL::CSC8503::SwingingBlock::SwingRight(float dt) {
	physicsObject->AddForce(-force);
	swingTimer -= dt;
}
