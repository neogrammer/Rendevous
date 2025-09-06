#include "Physics.h"

bool Physics::DetectAndResolve(Collider& r1, Collider& r2)
{
    Collider* collider1 = &r1;
    Collider* collider2 = &r2;
    bool collided = false;
    for (int shape = 0; shape < 2; shape++)
    {
        if (shape == 1)
        {
            collider1 = &r2;
            collider2 = &r1;
        }

        // Check diagonals of this polygon...
        for (int p = 0; p < 4; p++)
        {
            sf::Vector2f line_r1s = collider1->getPos();

            auto points = collider1->getPoints();

            sf::Vector2f line_r1e = points[p];

            sf::Vector2f displacement = { 0,0 };

            // ...against edges of this polygon
            for (int q = 0; q < 4; q++)
            {
                auto points2 = collider2->getPoints();

                sf::Vector2f line_r2s = points2[q];
                sf::Vector2f line_r2e = points2[(q + 1) % points2.size()];

                // Standard "off the shelf" line segment intersection
                float h = (line_r2e.x - line_r2s.x) * (line_r1s.y - line_r1e.y) - (line_r1s.x - line_r1e.x) * (line_r2e.y - line_r2s.y);
                float t1 = ((line_r2s.y - line_r2e.y) * (line_r1s.x - line_r2s.x) + (line_r2e.x - line_r2s.x) * (line_r1s.y - line_r2s.y)) / h;
                float t2 = ((line_r1s.y - line_r1e.y) * (line_r1s.x - line_r2s.x) + (line_r1e.x - line_r1s.x) * (line_r1s.y - line_r2s.y)) / h;

                if (t1 >= 0.0f && t1 < 1.0f && t2 >= 0.0f && t2 < 1.0f)
                {
                    collided = true;
                    displacement.x += (1.0f - (t1-0.001f)) * (line_r1e.x - line_r1s.x);
                    displacement.y += (1.0f - (t1-0.001f)) * (line_r1e.y - line_r1s.y);
                }
            }

            r1.getSprite().move({ displacement.x * (shape == 0 ? -1 : +1), displacement.y * (shape == 0 ? -1 : +1) });
        }
    }

    // Cant overlap if static collision is resolved
    return collided;

	return false;
}

std::vector<int> Physics::HandleCollisions(Collider& collider_, std::vector<std::unique_ptr<Collider>>& collidedVec_ )
{
	bool collisionOccurredAccum = false;
	std::vector<int> indicesCollided;
	indicesCollided.reserve(collidedVec_.size());
	for (int i = 0; i < collidedVec_.size(); i++)
	{
		bool collisionOccurred = DetectAndResolve(collider_, *collidedVec_[i]);
		collisionOccurredAccum |= collisionOccurred;

		if (collisionOccurred)
		{
			indicesCollided.emplace_back(i);
		}
	}

	if (!collisionOccurredAccum)
	{
		return std::vector<int>{};
	}
	else
	{
        indicesCollided.shrink_to_fit();
		return indicesCollided;
	}
}
