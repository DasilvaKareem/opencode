from pyray import *

def main():
    screen_width = 800
    screen_height = 600

    init_window(screen_width, screen_height, "raylib [python] - Basic Window")

    set_target_fps(60)

    while not window_should_close():
        begin_drawing()

        clear_background(RAYWHITE)

        draw_text("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY)
        draw_fps(10, 10)

        end_drawing()

    close_window()

if __name__ == "__main__":
    main()
