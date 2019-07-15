#include "TransformComponent.hpp"

using namespace rz::game::components;

TransformComponent::TransformComponent()
	: translation{ 0.f, 0.f }, scale{ 0.1f, 0.1f }, _rotation{ 0 }
{
}

TransformComponent::TransformComponent(glm::vec2 translation, glm::vec2 scale, float rotation)
	: translation{ translation }, scale{ scale }, _rotation{ rotation }
{
}

TransformComponent::~TransformComponent()
{
}

glm::mat4 TransformComponent::getModel()
{
	glm::mat4 translationMatrix = glm::translate(glm::mat4{1.f}, glm::vec3{ translation, 0.f });
	glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.f }, glm::vec3{ scale, 1.f });
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4{ 1.f }, _rotation, glm::vec3{ 0.f, 0.f, 1.f });

	return translationMatrix * rotationMatrix * scaleMatrix;
}

float TransformComponent::setRotation(float rotation)
{
	_rotation = rotation;

	if (_rotation >= 2 * glm::pi<float>())
		_rotation = std::fmod(_rotation, 2 * glm::pi<float>());
	else if (_rotation < 0)
		_rotation = 2 * glm::pi<float>() + std::fmod(_rotation, 2 * glm::pi<float>());

	return _rotation;
}
