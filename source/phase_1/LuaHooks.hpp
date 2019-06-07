#ifndef RZ_GAME1_LUAHOOKS_HPP
#define RZ_GAME1_LUAHOOKS_HPP

#include "Planner.hpp"

#pragma warning(push)
#pragma warning(disable : 4996)

#include <glm/glm.hpp>
#include <sol.hpp>

#pragma warning(pop)

namespace rz::lua::planner
{
    inline void hook(sol::state_view lua)
    {
        using namespace rz::planner;

        lua["OPERATION"] = sol::new_table();
        lua["OPERATION"]["NONE"] = Operation::NONE;
        lua["OPERATION"]["EQUAL"] = Operation::EQUAL;
        lua["OPERATION"]["LESS"] = Operation::LESS;
        lua["OPERATION"]["LESS_EQUAL"] = Operation::LESS_EQUAL;
        lua["OPERATION"]["GREATER"] = Operation::GREATER;
        lua["OPERATION"]["GREATER_EQUAL"] = Operation::GREATER_EQUAL;
        lua["OPERATION"]["ASSIGN"] = Operation::ASSIGN;
        lua["OPERATION"]["PLUS"] = Operation::PLUS;
        lua["OPERATION"]["MINUS"] = Operation::MINUS;
        lua["OPERATION"]["TIMES"] = Operation::TIMES;
        lua["OPERATION"]["DIVIDE"] = Operation::DIVIDE;

        // TODO make OPERATION immutable

        lua["ActionSet"] = sol::overload(
            []() { return ActionSet{}; },
            [](Action a) { return ActionSet{ a }; },
            [](Action a, Action b) { return ActionSet{ a, b }; },
            [](Action a, Action b, Action c) { return ActionSet{ a, b, c }; });

        lua["ConditionSet"] = sol::overload(
            []() { return ConditionSet{}; },
            [](Condition a) { return ConditionSet{ a }; },
            [](Condition a, Condition b) { return ConditionSet{ a, b }; },
            [](Condition a, Condition b, Condition c) { return ConditionSet{ a, b, c }; });

        lua["ProceduralConditionSet"] = sol::overload(
            []() { return ProceduralConditionSet{}; },
            [](ProceduralCondition a) { return ProceduralConditionSet{ a }; },
            [](ProceduralCondition a, ProceduralCondition b) { return ProceduralConditionSet{ a, b }; },
            [](ProceduralCondition a, ProceduralCondition b, ProceduralCondition c) { return ProceduralConditionSet{ a, b, c }; });

        lua.new_usertype<Condition>("Condition",
                                    sol::constructors<
                                        Condition(std::string, std::string, bool),
                                        Condition(std::string, std::string, bool, Operation),
                                        Condition(std::string, std::string, bool, Operation, int),
                                        Condition(std::string, std::string, int),
                                        Condition(std::string, std::string, int, Operation),
                                        Condition(std::string, std::string, int, Operation, int)>(),
                                    "id", &Condition::debugID,
                                    "type", &Condition::debugType,
                                    "value", &Condition::value,
                                    "op", &Condition::op,
                                    "weight", &Condition::weight);

        lua.new_usertype<Action>("Action",
                                 sol::constructors<Action(std::string, unsigned), Action(std::string, unsigned, ConditionSet, ConditionSet), Action(std::string, unsigned, ConditionSet, ConditionSet, ProceduralConditionSet, ProceduralConditionSet)>(),
                                 "name", &Action::name,
                                 "cost", &Action::cost,
                                 "preconditions", &Action::preconditions,
                                 "postconditions", &Action::postconditions);

        lua.new_usertype<Planner>("Planner",
                                  sol::constructors<Planner(), Planner(ConditionSet)>(),
                                  "plan", &Planner::plan,
                                  "savePlan", &Planner::savePlan,
                                  "worldState", sol::property(&Planner::getWorldState, &Planner::setWorldState));
    }
}

namespace rz::lua::maths
{
    template <typename Vector>
    inline Vector limit(const Vector &vec, float max)
    {
        if (glm::length(vec) > max)
            return glm::normalize(vec) * max;
        else
            return vec;
    }

    template <typename Vector>
    inline Vector normalize(const Vector &vec)
    {
        return glm::normalize(vec);
    }

    template <typename Vector>
    inline typename Vector::length_type length(const Vector &vec)
    {
        return glm::length(vec);
    }

    template <typename Vector>
    inline typename Vector::value_type dot(const Vector &u, const Vector &v)
    {
        return glm::dot(u, v);
    }

    template <typename Vector>
    inline float angle(const Vector &u, const Vector &v)
    {
        float theta = std::acos(glm::dot(u, v) / glm::length(u) * glm::length(v));

        if (u.y > v.y)
            return theta;
        else
            return 2.f * 3.14159f - theta;
    }

    inline void hook(sol::state_view lua)
    {
        // GLM hooks
        lua["glm"] = sol::new_table{};
        sol::table table = lua["glm"];

        // vec2
        table.new_usertype<glm::vec2>("vec2",
                                      sol::constructors<glm::vec2(float, float), glm::vec2(const glm::vec2 &)>(),
                                      sol::meta_function::addition, [](const glm::vec2 &lhs, const glm::vec2 &rhs) { return lhs + rhs; },
                                      sol::meta_function::subtraction, [](const glm::vec2 &lhs, const glm::vec2 &rhs) { return lhs - rhs; },
                                      sol::meta_function::multiplication, sol::overload([](const glm::vec2 &lhs, const glm::vec2 &rhs) { return lhs * rhs; }, [](const glm::vec2 &lhs, const float rhs) { return lhs * rhs; }),
                                      sol::meta_function::division, sol::overload([](const glm::vec2 &lhs, const glm::vec2 &rhs) { return lhs / rhs; }, [](const glm::vec2 &lhs, const float rhs) { return lhs / rhs; }),
                                      "x", &glm::vec2::x,
                                      "y", &glm::vec2::y);

        // u8vec3
        table.new_usertype<glm::u8vec3>("u8vec3", sol::constructors<glm::u8vec3(unsigned char, unsigned char, unsigned char), glm::u8vec3(const glm::u8vec3 &)>());

        table["normalize"] = sol::overload(&normalize<glm::vec2>);
        table["length"] = sol::overload(&length<glm::vec2>);
        table["limit"] = sol::overload(&limit<glm::vec2>);
        table["dot"] = sol::overload(&dot<glm::vec2>);
        table["angle"] = sol::overload(&angle<glm::vec2>);
    }
}

#endif //RZ_GAME1_LUAHOOKS_HPP
