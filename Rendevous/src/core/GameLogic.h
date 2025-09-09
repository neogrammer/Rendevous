#ifndef GAMELOGIC_H__
#define GAMELOGIC_H__

class GameLogic
{
	// Resources* storage
	// Clients* players;
public:

	// GameObject* objFactory;
	// CollisionMgr collMgr;
	// GameStateMgr gstateMgr;
	// Commander commander; - 
	// Execute(Command cmd);  commander commands
	// std::vector<GameObject*> liveObjects; +
	// std::vector<GameObject*> bufferObjects; - // swap at end of frame
	// SwapBuffers(); +
	// EventManager evtMgr; -
	// PushEvent(GameEvent* evt); +
	// ScriptManager aiMgr; -
	// PushScript(Script* script);
	// AddPlayer(client& aClient);
	// RemovePlayer(client& aClient);
	// processFrame(std::vector<GameObject*> toAdd); + // process everything in this frame, then save any changed data instead to bufferObjects while updating
	// runPendingEvents(); -
	// runPendingScripts(); -
};

#endif