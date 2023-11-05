#include "../../Common/Window.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourSelector.h"

#include "../CSC8503Common/NavigationGrid.h"

#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"

#include "TutorialGame.h"
#include "Game.h"

using namespace NCL;
using namespace CSC8503;

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/

class GameStatePushdown : public PushdownMachine {
public:
	GameStatePushdown(PushdownState* initialState) : PushdownMachine(initialState) { g = new Game(); }
	~GameStatePushdown() { delete g; }

protected:
	Game* g;
};

class GameState : public PushdownState {
public:
	GameState(Game* game) { g = game; }
	~GameState() {};
protected:
	Game* g;
};

class PauseScreen : public GameState {
public:
	PauseScreen(Game* game) : GameState(game) {};

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
			return PushdownResult::Pop;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		g->ChangeLevel(LevelState::PAUSED);
	}
};

class LevelOne : public GameState {
public:
	LevelOne(Game* game) : GameState(game) {};

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
			*newState = new PauseScreen(g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
			return PushdownResult::Pop;
		}
		g->UpdateGame(dt);
		return PushdownResult::NoChange;
	};
	void OnAwake() override {
		g->ChangeLevel(LevelState::LEVEL_ONE);
	}
};

class LevelTwo : public GameState {
public:
	LevelTwo(Game* game) : GameState(game) {};

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
			*newState = new PauseScreen(g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
			return PushdownResult::Pop;
		}
		g->UpdateGame(dt);
		return PushdownResult::NoChange;
	};
	void OnAwake() override {
		g->ChangeLevel(LevelState::LEVEL_TWO);
	}
};

class TestLevel : public GameState {
public:
	TestLevel(Game* game) : GameState(game) {};

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
			*newState = new PauseScreen(g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
			return PushdownResult::Pop;
		}
		g->UpdateGame(dt);
		return PushdownResult::NoChange;
	};
	void OnAwake() override {
		g->ChangeLevel(LevelState::TEST_LEVEL);
	}
};

class MenuScreen : public GameState {
public:
	MenuScreen(Game* game) : GameState(game) {};

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM1)) {
			*newState = new LevelOne(g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM2)) {
			*newState = new LevelTwo(g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM3)) {
			*newState = new TestLevel(g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
			return PushdownResult::Pop;
		}
		g->UpdateGame(dt);
		return PushdownResult::NoChange;
	};
	void OnAwake() override {
		g->ChangeLevel(LevelState::MENU);
	}
};

vector<Vector3> testNodes;

void TestPathfinding() {
	NavigationGrid grid("Maze.txt");

	NavigationPath outPath;

	Vector3 startPos(10, 0, 10);
	Vector3 endPos(90, 0, 130);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		pos.y += 20;
		testNodes.push_back(pos);
	}
}

void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Debug::RED);
	}
}

int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);

	//TestBehaviourTree();

	if (!w->HasInitialised()) {
		return -1;
	}	
	srand(time(0));
	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	//TestPathfinding();

	PushdownMachine machine(new MenuScreen(new Game()));
	w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		//TestPushdownAutomata(w);
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			w->SetWindowPosition(0, 0);
		}

		if (!machine.Update(dt)) {
			return -1;
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		DisplayPathfinding();

	}
	Window::DestroyGameWindow();
}