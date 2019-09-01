#ifndef RZ_GAME2_WORKER_AGENTCOMPONENT_HPP
#define RZ_GAME2_WORKER_AGENTCOMPONENT_HPP

#include "AgentComponent.hpp"

namespace rz::game::components
{
    class WorkerComponent
    {
      public:
        WorkerComponent() {}
        ~WorkerComponent() {}

        int metal{};
        double elapsed{};
    };
} // namespace rz::game::components

#endif //RZ_GAME2_WORKER_AGENTCOMPONENT_HPP