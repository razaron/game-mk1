#ifndef RZ_GAME2_LUAHOOKS_HPP
#define RZ_GAME2_LUAHOOKS_HPP

#include "Entity.hpp"
#include "EventsCore.hpp"
#include "EventStream.hpp"
#include "PhysicsSystem.hpp"
#include "Planner.hpp"
#include "RenderSystem.hpp"

#include <glm/glm.hpp>
#include <sol.hpp>

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
        table.new_usertype<glm::u8vec3>("u8vec3",
                                        sol::constructors<glm::u8vec3(unsigned char, unsigned char, unsigned char), glm::u8vec3(const glm::u8vec3 &)>());

        table["normalize"] = sol::overload(&normalize<glm::vec2>);
        table["length"] = sol::overload(&length<glm::vec2>);
        table["limit"] = sol::overload(&limit<glm::vec2>);
        table["dot"] = sol::overload(&dot<glm::vec2>);
        table["angle"] = sol::overload(&angle<glm::vec2>);
    }
}

#endif //RZ_GAME2_LUAHOOKS_HPP