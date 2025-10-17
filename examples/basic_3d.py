from pyray import *

def main():
    screen_width = 800
    screen_height = 600

    init_window(screen_width, screen_height, "raylib [python] - Basic 3D")

    camera = Camera3D(
        Vector3(10.0, 10.0, 10.0),
        Vector3(0.0, 0.0, 0.0),
        Vector3(0.0, 1.0, 0.0),
        45.0,
        CAMERA_PERSPECTIVE
    )

    cube_position = Vector3(0.0, 0.0, 0.0)

    set_target_fps(60)

    while not window_should_close():
        update_camera(camera, CAMERA_ORBITAL)

        begin_drawing()

        clear_background(RAYWHITE)

        begin_mode_3d(camera)

        draw_cube(cube_position, 2.0, 2.0, 2.0, RED)
        draw_cube_wires(cube_position, 2.0, 2.0, 2.0, MAROON)

        draw_grid(10, 1.0)

        end_mode_3d()

        draw_text("Use mouse to rotate camera", 10, 10, 20, DARKGRAY)
        draw_fps(10, 40)

        end_drawing()

    close_window()

if __name__ == "__main__":
    main()
