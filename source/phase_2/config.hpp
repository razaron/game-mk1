#ifndef RZ_GAME2_CONFIG_HPP
#define RZ_GAME2_CONFIG_HPP

#include <glm/glm.hpp>

#include "EventStream.hpp"
#include "TaskScheduler.hpp"

const rz::eventstream::EventType EVENTTYPE_MODEL = rz::eventstream::EventType::EVENT_1;
const rz::eventstream::EventType EVENTTYPE_STEERING = rz::eventstream::EventType::EVENT_2;
const rz::eventstream::EventType EVENTTYPE_COLLISION = rz::eventstream::EventType::EVENT_3;

const unsigned SCREEN_WIDTH = 1024u;
const unsigned SCREEN_HEIGHT = 1024u;

const int GROUP_ALL = 0;
const int GROUP_RED = 1;
const int GROUP_BLUE = 2;
const int GROUP_BULLETS = 3;
const int GROUP_DEPOSITS = 4;

enum class SteeringBehaviour
{
    SEEK,
    ARRIVE,
    MAINTAIN,
    WANDER,
    STOP
};

struct EVENTDATA_MODEL
{
    glm::mat4 model;

    EVENTDATA_MODEL(glm::mat4 model) : model{ model } {}
};

struct EVENTDATA_STEERING
{
    UUID64 target;
    SteeringBehaviour behaviour;

    EVENTDATA_STEERING(UUID64 target, SteeringBehaviour behaviour) : target{ target }, behaviour{ behaviour } {}
};

struct EVENTDATA_COLLISION
{
    UUID64 target;
    float distance;
    int group;

    EVENTDATA_COLLISION(UUID64 target, float distance, int group) : target{ target }, distance{ distance }, group{ group } {}
};

#endif //RZ_GAME2_CONFIG_HPP