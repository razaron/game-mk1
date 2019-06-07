#include "PhysicsSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::components;
using namespace rz::game::systems;
using namespace rz::eventstream;

PhysicsSystem::PhysicsSystem(sol::state_view lua)
	:_lua{ lua }
{
	_interval = 0.01;

	_componentTypes.insert(ComponentType::TRANSFORM);
	_componentTypes.insert(ComponentType::MOTION);
	_componentTypes.insert(ComponentType::COLLIDER);

	_lua.new_usertype<TransformComponent>("TransformComponent",
		sol::constructors<TransformComponent()>(),
		"translation", &TransformComponent::translation,
		"scale", &TransformComponent::scale,
		"rotation", sol::property(&TransformComponent::getRotation, &TransformComponent::setRotation)
		);

	_lua.new_usertype<MotionComponent>("MotionComponent",
                                           sol::constructors<MotionComponent()>(),
                                           "velocity", &MotionComponent::velocity);

	lua["STEERING_BEHAVIOUR"] = sol::new_table();
	lua["STEERING_BEHAVIOUR"]["ARRIVE"] = SteeringBehaviour::ARRIVE;
	lua["STEERING_BEHAVIOUR"]["MAINTAIN"] = SteeringBehaviour::MAINTAIN;
	lua["STEERING_BEHAVIOUR"]["SEEK"] = SteeringBehaviour::SEEK;
	lua["STEERING_BEHAVIOUR"]["WANDER"] = SteeringBehaviour::WANDER;
	lua["STEERING_BEHAVIOUR"]["STOP"] = SteeringBehaviour::STOP;

	_lua["updateSteering"] = [&](const UUID64 &recipient, const UUID64 &target, SteeringBehaviour behaviour) {
		_behaviours[recipient] = std::make_pair(behaviour, target);
	};

	registerHandler(EVENTTYPE_STEERING, [&](const Event &e) {
		auto data = std::static_pointer_cast<EVENTDATA_STEERING>(e.data);

		_behaviours[e.id] = std::make_pair(data->behaviour, data->target);
		}
	);

	extendHandler(EventType::SYSTEM_DELETE_COMPONENT, [&](const Event &e) {
		_behaviours.erase(e.id);
		}
	);
}

PhysicsSystem::~PhysicsSystem()
{
}

Task PhysicsSystem::update(EntityMap &entities, double delta)
{
	using ColliderVec = std::vector<std::tuple<UUID64, TransformComponent*, ColliderComponent*>>;

	// Extract relavent components
	std::map<UUID64, std::pair<TransformComponent*, MotionComponent*>, UUID64Cmp> bodies;
	ColliderVec colliders;
	for (auto &[id, entity] : entities)
	{
		if (entity.has(ComponentType::MOTION, ComponentType::TRANSFORM))
		{
			auto t = getObject<TransformComponent>(entity[ComponentType::TRANSFORM]);
			auto m = getObject<MotionComponent>(entity[ComponentType::MOTION]);

			bodies[id] = std::make_pair(t, m);
		}

		if (entity.has(ComponentType::COLLIDER, ComponentType::TRANSFORM))
		{
			auto t = getObject<TransformComponent>(entity[ComponentType::TRANSFORM]);
			auto c = getObject<ColliderComponent>(entity[ComponentType::COLLIDER]);

			colliders.push_back(std::make_tuple(id, t, c));
		}
	}

	// Update positions
	for (auto &[id, comps] : bodies)
	{
		auto[t, m] = comps;

		auto it = _behaviours.find(id);
		if (it != _behaviours.end())
		{
			auto behaviour = it->second.first;
			TransformComponent* target;

			if (entities.find(it->second.second) != entities.end() && entities.find(it->second.second)->second.has(ComponentType::TRANSFORM))
			{
				target = getObject<TransformComponent>((entities[it->second.second])[ComponentType::TRANSFORM]);
			}
			else
			{
				_behaviours[id] = std::make_pair(SteeringBehaviour::STOP, id);

				behaviour = SteeringBehaviour::STOP;
				target = t;
			}

			glm::vec2 steering{};

			switch (behaviour)
			{
				case SteeringBehaviour::ARRIVE:
				{
					if (target->translation == t->translation) break;

					glm::vec2 desired = glm::normalize(target->translation - t->translation) * m->maxVelocity;

					if (glm::length(target->translation - t->translation) < 125.f)
						desired *= glm::length(target->translation - t->translation) / 125.f;

					steering = desired - m->velocity;

					if (glm::length(steering) > m->maxAcceleration)
						steering = glm::normalize(steering) * m->maxAcceleration;

					break;
				}
				case SteeringBehaviour::MAINTAIN:
				{
					glm::vec2 desired = glm::normalize(target->translation - t->translation) * m->maxVelocity;

					if (glm::length(target->translation - t->translation) < 32.f)
						desired *= -(1 - glm::length(target->translation - t->translation) / 32.f);

					steering = desired - m->velocity;

					if (glm::length(steering) > m->maxAcceleration)
						steering = glm::normalize(steering) * m->maxAcceleration;

					break;
				}
				case SteeringBehaviour::SEEK:
				{
					glm::vec2 desired = glm::normalize(target->translation - t->translation) * m->maxVelocity;

					steering = desired - m->velocity;

					if (glm::length(steering) > m->maxAcceleration)
						steering = glm::normalize(steering) * m->maxAcceleration;

					break;
				}
				case SteeringBehaviour::WANDER:
				{
					glm::vec2 center = glm::normalize(m->velocity)*16.f;

					if (m->velocity == glm::vec2{ 0.f,0.f })
						center = m->velocity;

					glm::vec2 displacement = glm::vec2{
						8.f*std::cos(m->wanderAngle),
						8.f*std::sin(m->wanderAngle)
					};

					std::random_device r;
					std::uniform_real_distribution<float> dist{ 0.f,1.f };

					float theta = 3.14159f * delta * 16.f;
					m->wanderAngle += dist(r) * theta - theta * 0.5;

					steering = glm::normalize(center + displacement) * m->maxAcceleration * 10.f;

					break;
				}
				case SteeringBehaviour::STOP:
				{
					steering = -m->velocity;

					if (glm::length(steering) > m->maxAcceleration)
						steering = glm::normalize(steering) * m->maxAcceleration;

					break;
				}
			}

			m->velocity += steering * static_cast<float>(delta);

			if (glm::length(m->velocity) > m->maxVelocity)
				m->velocity = glm::normalize(m->velocity) * m->maxVelocity;

			t->translation += m->velocity * static_cast<float>(delta);

			float theta = std::acos(glm::dot(m->velocity, glm::vec2{ 1,0 }) / glm::length(m->velocity)*glm::length(glm::vec2{ 1,0 }));

			if (m->velocity.y > 0)
				t->setRotation(theta);
			else
				t->setRotation(2.f * 3.14159f - theta);
		}
		else
		{
			t->translation += m->velocity * static_cast<float>(delta);
		}
	}

	std::vector<Event> events;

	// Calculate collisions
	for (auto &[id1, t1, c1] : colliders)
	{
		for (auto &[id2, t2, c2] : colliders)
		{
			float length = glm::length(t2->translation - t1->translation);
			if (length < c1->radius)
			{
				events.emplace_back(
					id1,
					EVENTTYPE_COLLISION,
					std::make_shared<EVENTDATA_COLLISION>(id2, length, c2->group)
				);
			}
		}
	}

	// Send model matrixes to RenderSystem
	for (auto &[id, entity] : entities)
	{
		if (!entity.has(ComponentType::TRANSFORM)) continue;

		auto transform = getObject<TransformComponent>(entity[ComponentType::TRANSFORM]);

		events.emplace_back(
			id,
			EVENTTYPE_MODEL,
			std::make_shared<EVENTDATA_MODEL>(transform->getModel())
		);
	}

	pushEvents(events, StreamType::OUTGOING);


	return Task{};
}

ComponentHandle PhysicsSystem::createComponent(ComponentType type, std::shared_ptr<void> tuplePtr)
{
	Handle h;

	switch (type)
	{
		case ComponentType::TRANSFORM:
		{
			TransformArgs args = *(std::static_pointer_cast<TransformArgs>(tuplePtr));

			h = emplaceObject<TransformComponent>(std::get<1>(args), std::get<2>(args), std::get<3>(args));

			sol::table obj = std::get<0>(args);
			obj["transform"] = getObject<TransformComponent>(h);

			break;
		}
		case ComponentType::MOTION:
		{
			MotionArgs	args = *(std::static_pointer_cast<MotionArgs>(tuplePtr));

			h = emplaceObject<MotionComponent>(std::get<1>(args), std::get<2>(args), std::get<3>(args), std::get<4>(args), std::get<5>(args));

			sol::table obj = std::get<0>(args);
			obj["motion"] = getObject<MotionComponent>(h);

			break;
		}
		case ComponentType::COLLIDER:
		{
			ColliderArgs args = *(std::static_pointer_cast<ColliderArgs>(tuplePtr));

			h = emplaceObject<ColliderComponent>(std::get<1>(args), std::get<2>(args));

			sol::table obj = std::get<0>(args);
			obj["collider"] = getObject<ColliderComponent>(h);

			break;
		}
		default:
		{
			h = Handle{};
			break;
		}
	}

	return ComponentHandle{ type, h };
}

bool PhysicsSystem::removeComponent(ComponentHandle ch)
{
	Handle h;

	switch (ch.first)
	{
		case ComponentType::TRANSFORM:
			removeObject<TransformComponent>(ch.second);
			break;
		case ComponentType::MOTION:
			removeObject<MotionComponent>(ch.second);
			break;
		case ComponentType::COLLIDER:
			removeObject<ColliderComponent>(ch.second);
			break;
		default:
			return false;
			break;
	}

	return true;
}
