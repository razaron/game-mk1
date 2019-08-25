#ifndef RZ_GAME3_CONFIG_HPP
#define RZ_GAME3_CONFIG_HPP

#include <glm/glm.hpp>

#include "EventStream.hpp"
#include "TaskScheduler.hpp"

const unsigned SCREEN_WIDTH = 1024u;
const unsigned SCREEN_HEIGHT = 1024u;

const int GROUP_ALL = 0;
const int GROUP_RED = 1;
const int GROUP_GREEN = 2;
const int GROUP_BLUE = 3;
const int GROUP_YELLOW = 4;
const int GROUP_BULLET = 5;
const int GROUP_DEPOSIT = 6;

enum class SteeringBehaviour
{
    SEEK,
    ARRIVE,
    MAINTAIN,
    WANDER,
    STOP
};

namespace rz::game::event::type
{
    const eventstream::EventType MODEL{"MODEL"};
    const eventstream::EventType STEERING{"STEERING"};
    const eventstream::EventType COLLISION{"COLLISION"};
    const eventstream::EventType TEXT{"TEXT"};
}

namespace rz::game::event::data
{

    struct MODEL
    {
        glm::mat4 model;

        MODEL(glm::mat4 model) : model{ model } {}
    };

    struct STEERING
    {
        UUID64 target;
        SteeringBehaviour behaviour;

        STEERING(UUID64 target, SteeringBehaviour behaviour) : target{ target }, behaviour{ behaviour } {}
    };

    struct COLLISION
    {
        UUID64 target;
        float distance;
        int group;

        COLLISION(UUID64 target, float distance, int group) : target{ target }, distance{ distance }, group{ group } {}
    };

    struct TEXT
    {
        std::string str;
        glm::vec2 pos;
        glm::u8vec3 col;
        unsigned size;

        TEXT(std::string str, glm::vec2 pos, glm::u8vec3 col, unsigned size) : str{ str }, pos{ pos }, col{ col }, size{ size } {}
    };
} // namespace rz::game::event::data

#endif //RZ_GAME3_CONFIG_HPP