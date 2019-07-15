#include "ShapeComponent.hpp"

using namespace rz::game::components;

ShapeComponent::ShapeComponent()
	:sides{4}, colour{glm::u8vec3{255, 255, 255}}
{
}

ShapeComponent::ShapeComponent(int sides, glm::u8vec3 colour)
	: sides{ sides }, colour{ colour }
{
}

ShapeComponent::~ShapeComponent()
{
}
