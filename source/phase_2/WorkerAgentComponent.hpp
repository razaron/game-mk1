#ifndef RZ_GAME2_WORKER_AGENTCOMPONENT_HPP
#define RZ_GAME2_WORKER_AGENTCOMPONENT_HPP

#include "AgentComponent.hpp"

namespace rz::game::components
{
    class WorkerAgentComponent : public rz::game::components::AgentComponent
    {
      public:
        WorkerAgentComponent(Team team, rz::planner::ActionSet actions) : AgentComponent{team, actions} {}
        WorkerAgentComponent() {}
        ~WorkerAgentComponent() {}

        int metal;
        double elapsed;
    };
} // namespace rz::game::components

#endif //RZ_GAME2_WORKER_AGENTCOMPONENT_HPP