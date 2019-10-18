#ifndef RZ_GAME2_AGENTCOMPONENT_HPP
#define RZ_GAME2_AGENTCOMPONENT_HPP

#include "Entity.hpp"
#include "Planner.hpp"
#include "config.hpp"

namespace rz::game::components
{
    class AgentComponent : public rz::core::Component
    {
      public:
        struct Blackboard
        {
            int threat;
        };

        AgentComponent(std::string name, Team team, rz::planner::ActionSet actions) : name{name}, team{team}, actions{ actions } {}
        AgentComponent() {}
        ~AgentComponent() {}

        std::string name{"AGENT"};
        Team team{};

        rz::planner::Planner planner{};
        rz::planner::ActionSet actions;

        rz::planner::ActionSet curPlan;
        rz::planner::Action curAction{};

        rz::core::Entity target{};

        bool isDead{false};
        Blackboard blackboard{};
    };
} // namespace rz::game::components

#endif //RZ_GAME2_AGENTCOMPONENT_HPP