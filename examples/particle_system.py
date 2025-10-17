from pyray import *
import random

class Particle:
    def __init__(self, x, y):
        self.position = Vector2(x, y)
        self.velocity = Vector2(random.uniform(-2, 2), random.uniform(-5, -1))
        self.lifetime = random.uniform(1.0, 3.0)
        self.age = 0.0
        self.size = random.uniform(2, 6)
        self.color = Color(
            random.randint(200, 255),
            random.randint(100, 200),
            random.randint(0, 100),
            255
        )

    def update(self, dt):
        self.age += dt
        self.position.x += self.velocity.x
        self.position.y += self.velocity.y
        self.velocity.y += 0.2

    def is_alive(self):
        return self.age < self.lifetime

    def draw(self):
        alpha = int(255 * (1.0 - self.age / self.lifetime))
        color = Color(self.color.r, self.color.g, self.color.b, alpha)
        draw_circle_v(self.position, self.size, color)

def main():
    screen_width = 800
    screen_height = 600

    init_window(screen_width, screen_height, "raylib [python] - Particle System")

    particles = []

    set_target_fps(60)

    while not window_should_close():
        dt = get_frame_time()

        if is_mouse_button_down(MOUSE_LEFT_BUTTON):
            mouse_pos = get_mouse_position()
            for _ in range(5):
                particles.append(Particle(mouse_pos.x, mouse_pos.y))

        for particle in particles[:]:
            particle.update(dt)
            if not particle.is_alive():
                particles.remove(particle)

        begin_drawing()

        clear_background(BLACK)

        for particle in particles:
            particle.draw()

        draw_text("Click and hold left mouse button", 10, 10, 20, RAYWHITE)
        draw_text(f"Particles: {len(particles)}", 10, 40, 20, RAYWHITE)
        draw_fps(10, 70)

        end_drawing()

    close_window()

if __name__ == "__main__":
    main()
