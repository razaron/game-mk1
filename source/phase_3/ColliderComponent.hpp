#ifndef RZ_GAME2_COLLIDERCOMPONENT_HPP
#define RZ_GAME2_COLLIDERCOMPONENT_HPP

#include "Component.hpp"

#include <glm/glm.hpp>

namespace rz::game::components
{
    class ColliderComponent : public rz::core::Component
	{
	public:
		ColliderComponent(float radius, int group) :radius{ radius }, group{ group } {}
		ColliderComponent() {}
		~ColliderComponent() {}

		float radius;
		int group;
	};
}

#endif //RZ_GAME2_COLLIDERCOMPONENT_HPP