#include "RenderSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::systems;

RenderSystem::RenderSystem(sol::state_view lua, sf::RenderWindow *window)
	: _lua(lua), _window{ window }
{
	// Load game script
	auto result = _lua.script_file("renderer.lua", [](lua_State*, sol::protected_function_result pfr) {
		sol::error err = pfr;
		std::cerr << err.what() << std::endl;
		return pfr;
	});

	_lua["Render"] = sol::new_table();
	_lua["Render"]["draw"] = sol::new_table();

	_lua["Render"]["TileMap"] = []() { return std::array<int, 32 * 32>{}; };

	sf::Font font;
	if (!font.loadFromFile("res/arial.ttf"))
	{
		// error...
	}

	_lua["Render"]["draw"]["text"] = [window, font](std::string str, glm::vec2 pos, unsigned size, glm::u8vec3 col) {
		sf::Text text;

		text.setFont(font);
		text.setString(str);
		text.setPosition(pos.x, pos.y);
		text.setCharacterSize(size);
		text.setColor(sf::Color{ col.r, col.g, col.b });

		window->draw(text);
	};

	sf::Texture texture;
	if (!texture.loadFromFile("res/textures.png"))
	{
		// error...
	}

	_lua["Render"]["draw"]["sprite"] = [window, texture](glm::vec2 pos, int tex) {
		sf::IntRect rect{ (tex % 4) * 32, (tex / 4) * 32, 32, 32 };

		sf::Sprite tile;
		tile.setTexture(texture);
		tile.setTextureRect(rect);
		tile.setPosition(pos.x * 32, pos.y * 32);

		window->draw(tile);
	};

	_lua["Render"]["draw"]["polygon"] = [window](int sides, float scale, glm::vec2 pos, float rot, glm::u8vec3 col) {
		sf::ConvexShape shape;
		shape.setPointCount(sides);

		float theta = .0f;
		float delta = 2 * 3.14159 / sides;
		for (int i = 0; i < sides; i++)
		{
			float x = scale * std::cos(theta);
			float y = scale * std::sin(theta);

			shape.setPoint(i, sf::Vector2f{ x, y });

			theta += delta;
		}

		shape.setPosition(pos.x, pos.y);
		shape.setRotation(rot / 3.14159 * 180);
		shape.setFillColor(sf::Color(col.r, col.g, col.b));

		sf::RectangleShape line(sf::Vector2f(scale, 1));
		line.setPosition(pos.x, pos.y);
		line.setRotation(rot / 3.14159 * 180);
		line.setFillColor(sf::Color(255-col.r, 255-col.g, 255-col.b));
		
		window->draw(shape);
		window->draw(line);
	};

	_lua["Render"]["draw"]["tilemap"] = [window, texture](std::array<int, 32 * 32> tilemap) {
		for (int i = 0; i < tilemap.size(); ++i)
		{
			int tex = tilemap[i];
			sf::Sprite tile;

			int x = tex % 4;
			int y = std::floor(tex / 4);

			sf::IntRect rect{ x * 32, y * 32, 32, 32 };

			tile.setTexture(texture);
			tile.setTextureRect(rect);
			tile.setPosition((i % 32) * 32, (i / 32) * 32);

			window->draw(tile);
		}
	};

	// Initialize game script globals
	try
	{
		sol::function func = _lua["renderer"]["init"];
		func();
	}
	catch (const sol::error &err)
	{
		std::cerr << err.what() << std::endl;
	}
}

RenderSystem::~RenderSystem()
{
}

Task RenderSystem::update(EntityMap &, double delta)
{
	_window->clear();

	// Run game script
	try
	{
		sol::function func = _lua["renderer"]["update"];
		func(delta);
	}
	catch (const sol::error &err)
	{
		std::cerr << err.what() << std::endl;
		std::cin.get();
	}

	_window->display();

	return Task{};
}

ComponentHandle RenderSystem::createComponent(ComponentType type, std::shared_ptr<void>)
{
	Handle h;

	switch (type)
	{
		default:
			h = Handle{};
			break;
	}

	return ComponentHandle{ type, h };
}

bool RenderSystem::removeComponent(ComponentHandle ch)
{
	Handle h;

	switch (ch.first)
	{
		default:
			return false;
			break;
	}

	return true;
}
