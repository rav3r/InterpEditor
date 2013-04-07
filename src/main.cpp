#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>

#include "../IMGUI/src/gfxlib.h"

#include "../IMGUI/src/IMGUI.h"
#include "../IMGUI/src/gfxRenderer.h"

sf::RenderWindow* gWindow = 0;
sf::Font* gFont = 0;

enum InterpType
{
	SPLINE,
	LINEAR
};

struct InterpPoint
{
	sf::Vector2f pos;
	sf::Vector2f prevDir;
	sf::Vector2f nextDir;

	int* id;
};

bool operator<(const InterpPoint& a, const InterpPoint& b)
{
	if(a.pos.x == b.pos.x)
		return a.pos.y < a.pos.y;
	return a.pos.x < b.pos.x;
}

sf::Vector2f CalcSpline(InterpPoint prev, InterpPoint next, float t)
{
	sf::Vector2f a1 = prev.pos;
	sf::Vector2f a2 = prev.nextDir;
	sf::Vector2f a3 = next.prevDir;
	sf::Vector2f a4 = next.pos;

	sf::Vector2f b1 = a1*(1-t) + a2*t;
	sf::Vector2f b2 = a2*(1-t) + a3*t;
	sf::Vector2f b3 = a3*(1-t) + a4*t;

	sf::Vector2f c1 = b1*(1-t) + b2*t;
	sf::Vector2f c2 = b2*(1-t) + b3*t;

	return c1*(1-t) + c2*t;
}

void DrawPoints(const std::vector<InterpPoint>& points)
{
	sf::VertexArray array;
	array.setPrimitiveType(sf::LinesStrip);

	for(int i=0; i<points.size()-1; i++)
	{
		InterpPoint a = points[i];
		InterpPoint b = points[i+1];

		for(int j=0; j<=30; j++)
		{
			sf::Vertex v(CalcSpline(a, b, j/30.0f));
			v.color = sf::Color(255, 255, 128);
			array.append(v);
		}
	}

	gWindow->draw(array);

	sf::VertexArray dirs;
	dirs.setPrimitiveType(sf::PrimitiveType::Lines);

	for(int i=0; i<points.size(); i++)
	{
		if(i > 0)
		{
			dirs.append(points[i].pos);
			dirs.append(points[i].prevDir);
		}

		if(i < points.size()-1)
		{
			dirs.append(points[i].pos);
			dirs.append(points[i].nextDir);
		}
	}

	gWindow->draw(dirs);
}

int main()
{
	sf::RenderWindow window(sf::VideoMode(800, 600), "SFML works!");
	sf::View view;
	gWindow = &window;

	sf::Font font;
	gFont = &font;
	if(!font.loadFromFile("data/Cabin-Regular.otf"))
		return EXIT_FAILURE;

	gfxRenderer renderer;
	igContext gui(&renderer);

	std::vector<InterpPoint> points;

	InterpPoint tmp;
	
	tmp.pos = sf::Vector2f(100, 300);
	tmp.prevDir = sf::Vector2f(100, 50) + tmp.pos;
	tmp.nextDir = sf::Vector2f(-100, 50) + tmp.pos;

	tmp.id = new int;
	points.push_back(tmp);

	tmp.pos = sf::Vector2f(700, 300);
	tmp.prevDir = sf::Vector2f(100, 50) + tmp.pos;
	tmp.nextDir = sf::Vector2f(-100, 50) + tmp.pos;
	tmp.id = new int;
	points.push_back(tmp);

	int currVertex = 0;

	sf::CircleShape selected = sf::CircleShape(10);
	selected.setFillColor(sf::Color(255,0,0,255));
	selected.setOrigin(sf::Vector2f(10, 10));

	float vslider = 0.0f;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if(event.type == sf::Event::Closed)
				window.close();
			if(event.type == sf::Event::Resized)
			{
				view.reset(sf::FloatRect(0, 0, event.size.width, event.size.height));
				window.setView(view);
			}
			if(event.type == sf::Event::MouseMoved)
			{
				gui.mouseX = event.mouseMove.x;
				gui.mouseY = event.mouseMove.y;
			}
			if(event.type == sf::Event::MouseButtonPressed)
			{
				if(event.mouseButton.button == sf::Mouse::Left)
				{
					gui.leftDown = true;
				}
				if(event.mouseButton.button == sf::Mouse::Right)
				{
					InterpPoint newPoint;
					newPoint.pos = sf::Vector2f(gui.mouseX, gui.mouseY);
					newPoint.prevDir = sf::Vector2f(-50, 0) + newPoint.pos;
					newPoint.nextDir = sf::Vector2f(50, 0) + newPoint.pos;
					newPoint.id = new int;
					for(int i=0; i<points.size(); i++)
					{
						if(points[i].pos.x > newPoint.pos.x)
						{
							points.insert(points.begin() + i, newPoint);
							break;
						}
					}
				}
			}
			if(event.type == sf::Event::MouseButtonReleased)
			{
				if(event.mouseButton.button == sf::Mouse::Left)
					gui.leftDown = false;
			}
			if(event.type == sf::Event::TextEntered)
			{
				gui.charEntered = event.text.unicode;
			}
			if(event.type == sf::Event::KeyPressed)
			{
				if(event.key.code == sf::Keyboard::Back)
					gui.backspace = true;
				if(event.key.code == sf::Keyboard::Left)
					gui.textCharPos--;
				if(event.key.code == sf::Keyboard::Right)
					gui.textCharPos++;
				if(event.key.code == sf::Keyboard::Delete)
					gui.charEntered = 127;
			}
		}

		window.clear(sf::Color(128, 128, 128, 255));

		selected.setPosition(points[currVertex].pos);
		window.draw(selected);

		DrawPoints(points);

		gui.Begin();

		gui.VSlider(GEN_NULL_ID, 780, 0, 20, 600, 0.1f, vslider);


		for(int i=0; i<points.size(); i++)
		{
			float x = points[i].pos.x - 5;
			float y = points[i].pos.y - 5;
			if(gui.Move(GEN_ID(*points[i].id), x, y, 10, 10, 0))
			{
				sf::Vector2f tmp = points[i].pos;

				points[i].pos.x = x + 5;
				points[i].pos.y = y + 5;

				currVertex = i;
			}

			if(i > 0)
			{
				x = points[i].prevDir.x - 5;
				y = points[i].prevDir.y - 5;
				if(gui.Move(GEN_ID(*points[i].id), x, y, 10, 10, 0))
				{
					points[i].prevDir.x = x + 5;
					points[i].prevDir.y = y + 5;
				}
			}

			if(i < points.size() - 1)
			{
				x = points[i].nextDir.x - 5;
				y = points[i].nextDir.y - 5;
				if(gui.Move(GEN_ID(*points[i].id), x, y, 10, 10, 0))
				{
					points[i].nextDir.x = x + 5;
					points[i].nextDir.y = y + 5;
				}
			}
		}

		gui.End();

		std::sort(points.begin(), points.end());

		for(int i=0; i<points.size(); i++)
		{
			if(points[i].prevDir.x > points[i].pos.x) points[i].prevDir.x = points[i].pos.x;
			if(points[i].nextDir.x < points[i].pos.x) points[i].nextDir.x = points[i].pos.x;
			if(i > 0 && points[i].prevDir.x < points[i-1].pos.x) points[i].prevDir.x = points[i-1].pos.x;
			if(i < points.size()-1 && points[i].nextDir.x > points[i+1].pos.x) points[i].nextDir.x = points[i+1].pos.x;
		}

		window.display();
	}

	return 0;
}
