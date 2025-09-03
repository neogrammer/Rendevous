#ifndef PHYSICS_H__
#define PHYSICS_H__
#include "Colliders.h"

class Physics
{
public:
	/// <summary>
	/// Returns if a collision occurred during call
	/// </summary>
	static bool DetectAndResolve(Collider&, Collider&);
	static std::vector<int> HandleCollisions(Collider& collider_, std::vector<std::unique_ptr<Collider>>& collidedVec_);


};

#endif
