#include <iostream>
#include <SFML/Graphics.hpp>
#include <list>
#include "engine/window_context_handler.hpp"
#include "engine/physics/physics.hpp"
#include "renderer.hpp"
#include "wind.hpp"


bool isInRadius(const Particle& p, sf::Vector2f center, float radius)
{
    const sf::Vector2f v = center - p.position;
    return v.x * v.x + v.y * v.y < radius * radius;
}


void applyForceOnCloth(sf::Vector2f position, float radius, sf::Vector2f force, PhysicSolver& solver)
{
    for (Particle& p : solver.objects) {
        if (isInRadius(p, position, radius)) {
            p.forces += force;
        }
    }
}


int main()
{
	const int32_t window_width = 1920;
	const int32_t window_height = 1080;
	WindowContextHandler app("Cloth", {window_width, window_height}, sf::Style::Default);

    PhysicSolver solver;
    Renderer renderer(solver);

    const uint32_t cloth_width = 150;
    const uint32_t cloth_height = 100;
    const float links_length = 10.0f;
    const float start_x = (window_width - (cloth_width - 1) * links_length) * 0.5f;
    // Initialize the cloth
    for (uint32_t y(0); y < cloth_height; ++y) {
        const float max_elongation = 1.5f * (2.0f - y / float(cloth_height));
        for (uint32_t x(0); x < cloth_width; ++x) {
            const uint32_t id = solver.addParticle(
                sf::Vector2f(start_x + x * links_length, y * links_length)
            );
            // Add left link if there is a particle on the left
            if (x > 0) {
                solver.addLink(id-1, id, max_elongation * 0.9f);
            }
            // Add top link if there is a particle on the top
            if (y > 0) {
                solver.addLink(id-cloth_width, id, max_elongation);
            } else {
                // If not, pin the particle
                solver.objects[id].moving = false;
            }
        }
    }

    sf::Vector2f last_mouse_position;
    bool dragging = false;
    bool erasing = false;
    // Add events callback for mouse control
    app.getEventManager().addMousePressedCallback(sf::Mouse::Right, [&](sfev::CstEv) {
        dragging = true;
        last_mouse_position = app.getWorldMousePosition();
    });
    app.getEventManager().addMouseReleasedCallback(sf::Mouse::Right, [&](sfev::CstEv) {
        dragging = false;
    });
    app.getEventManager().addMousePressedCallback(sf::Mouse::Middle, [&](sfev::CstEv) {
        erasing = true;
    });
    app.getEventManager().addMouseReleasedCallback(sf::Mouse::Middle, [&](sfev::CstEv) {
        erasing = false;
    });
    // Main loop
    const float dt = 1.0f / 60.0f;
    while (app.run()) {
        // Get the mouse coord in the world space, to allow proper control even with modified viewport
        const sf::Vector2f mouse_position = app.getWorldMousePosition();

        if (dragging) {
            // Apply a force on the particles in the direction of the mouse's movement
            const sf::Vector2f mouse_speed = mouse_position - last_mouse_position;
            last_mouse_position = mouse_position;
            applyForceOnCloth(mouse_position, 100.0f, mouse_speed * 8000.0f, solver);
        }

        if (erasing) {
            // Delete all nodes that are in the range of the mouse
            std::list<uint32_t> to_remove;
            for (Particle& p : solver.objects) {
                if (isInRadius(p, mouse_position, 20.0f)) {
                    to_remove.push_back(p.id);
                }
            }
            for (uint32_t id : to_remove) {
                solver.objects.erase(id);
            }
        }
        // Update physics
        solver.update(dt);
        // Render the scene
        RenderContext& render_context = app.getRenderContext();
        render_context.clear();
        renderer.render(render_context);
        render_context.display();
	}

	return 0;
}
