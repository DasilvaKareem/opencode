from pyray import *

def main():
    screen_width = 800
    screen_height = 600

    init_window(screen_width, screen_height, "raylib [python] - 2D Player Movement")

    player_pos = Vector2(screen_width / 2.0, screen_height / 2.0)
    player_speed = 5.0
    player_radius = 20.0

    set_target_fps(60)

    while not window_should_close():
        if is_key_down(KEY_RIGHT):
            player_pos.x += player_speed
        if is_key_down(KEY_LEFT):
            player_pos.x -= player_speed
        if is_key_down(KEY_UP):
            player_pos.y -= player_speed
        if is_key_down(KEY_DOWN):
            player_pos.y += player_speed

        if player_pos.x < player_radius:
            player_pos.x = player_radius
        if player_pos.x > screen_width - player_radius:
            player_pos.x = screen_width - player_radius
        if player_pos.y < player_radius:
            player_pos.y = player_radius
        if player_pos.y > screen_height - player_radius:
            player_pos.y = screen_height - player_radius

        begin_drawing()

        clear_background(RAYWHITE)

        draw_circle_v(player_pos, player_radius, RED)

        draw_text("Move the ball with arrow keys", 10, 10, 20, DARKGRAY)
        draw_fps(10, 40)

        end_drawing()

    close_window()

if __name__ == "__main__":
    main()
