#ifndef RZ_GAME2_AGENTCOMPONENT_HPP
#define RZ_GAME2_AGENTCOMPONENT_HPP

#include "Entity.hpp"
#include "Planner.hpp"
#include "config.hpp"

namespace rz::game::components
{
    class AgentComponent : public rz::core::Component
    {
        struct Blackboard
        {
            int threat;
        };

      public:
        AgentComponent(Team team, rz::planner::ActionSet actions) : team{team}, actions{ actions } {}
        AgentComponent() {}
        ~AgentComponent() {}

        Team team;

        rz::planner::Planner planner;
        rz::planner::ActionSet actions;

        rz::planner::ActionSet curPlan;
        rz::planner::Action curAction;

        rz::core::Entity target;

        bool isDead;
        Blackboard blackboard;
    };
} // namespace rz::game::components

#endif //RZ_GAME2_AGENTCOMPONENT_HPP