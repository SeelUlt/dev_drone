import os
import pygame
import serial
import struct

packet_id = 0
HEADER = 0xAA
btn = 0

ser = serial.Serial('/dev/ttyUSB0', 115200)  # Настройка порта и скорости

# Запуск системы
# cd ~/drone_dev/python
# python3 -m venv venv
# source venv/bin/activate
# pip install pygame
# python main.py

# номера кнопок : 0 A, 1 B, 2 X, 3 Y, 4 LB, 5 RB, 6 Back, 7 Start, 8 Guide, 9 LS, 10 RS \

BUTTON_NAMES = {
    0: "A",
    1: "B",
    2: "X",
    3: "Y",
    4: "LB",
    5: "RB",
    6: "Back",
    7: "Start",
    8: "Guide",
    9: "LS",
    10: "RS",
}

os.environ.setdefault("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1")

WIDTH, HEIGHT = 1200, 760
FPS = 60

BG = (20, 22, 28)
FG = (235, 235, 235)
DIM = (120, 125, 135)
LINE = (80, 85, 95)
GREEN = (80, 210, 120)
RED = (220, 90, 90)
BLUE = (90, 150, 255)
YELLOW = (240, 210, 90)
PURPLE = (170, 120, 255)

# Часто для Xbox-подобных контроллеров на Linux/pygame это ближе к правде:
# left stick  : 0, 1
# right stick : 3, 4
# triggers    : 2, 5
#
# Если у твоего геймпада всё ещё не совпадёт — смотри блок "RAW AXES" на экране
# и просто поменяй эти номера.
LX_AXIS = 0
LY_AXIS = 1
RX_AXIS = 3
RY_AXIS = 4
LT_AXIS = 2
RT_AXIS = 5

DEADZONE = 0.12

pygame.init()
pygame.joystick.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Gamepad Viewer")
clock = pygame.time.Clock()
font = pygame.font.SysFont("consolas", 20)
small = pygame.font.SysFont("consolas", 16)

joystick = None
joy_instance_id = None

axis_values = {}
button_values = {}
hat_values = {}


def connect_first_joystick():
    global joystick, joy_instance_id, axis_values, button_values, hat_values

    if pygame.joystick.get_count() == 0:
        joystick = None
        joy_instance_id = None
        axis_values = {}
        button_values = {}
        hat_values = {}
        return

    joystick = pygame.joystick.Joystick(0)
    joystick.init()
    joy_instance_id = joystick.get_instance_id()

    axis_values = {i: 0.0 for i in range(joystick.get_numaxes())}
    button_values = {i: False for i in range(joystick.get_numbuttons())}
    hat_values = {i: (0, 0) for i in range(joystick.get_numhats())}


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def apply_deadzone(v, dz=DEADZONE):
    if abs(v) < dz:
        return 0.0
    if v > 0:
        return (v - dz) / (1.0 - dz)
    return (v + dz) / (1.0 - dz)


def draw_text(text, x, y, color=FG, f=font):
    surf = f.render(text, True, color)
    screen.blit(surf, (x, y))


def draw_stick(cx, cy, radius, x_val, y_val, title):
    pygame.draw.circle(screen, LINE, (cx, cy), radius, 2)
    pygame.draw.line(screen, LINE, (cx - radius, cy), (cx + radius, cy), 1)
    pygame.draw.line(screen, LINE, (cx, cy - radius), (cx, cy + radius), 1)

    x_val = clamp(x_val, -1.0, 1.0)
    y_val = clamp(y_val, -1.0, 1.0)

    px = cx + int(x_val * (radius - 10))
    py = cy + int(y_val * (radius - 10))
    pygame.draw.circle(screen, GREEN, (px, py), 10)

    draw_text(title, cx - radius, cy - radius - 30, YELLOW, font)
    draw_text(f"x: {x_val:+.2f}", cx - radius, cy + radius + 10, FG, small)
    draw_text(f"y: {y_val:+.2f}", cx - radius, cy + radius + 30, FG, small)


def draw_axis_bar(x, y, w, h, value, label):
    pygame.draw.rect(screen, LINE, (x, y, w, h), 2)
    mid = x + w // 2
    pygame.draw.line(screen, DIM, (mid, y), (mid, y + h), 1)

    value = clamp(value, -1.0, 1.0)
    fill_w = int((w // 2 - 4) * abs(value))
    if value >= 0:
        pygame.draw.rect(screen, BLUE, (mid + 2, y + 2, fill_w, h - 4))
    else:
        pygame.draw.rect(screen, RED, (mid - fill_w - 2, y + 2, fill_w, h - 4))

    draw_text(f"{label}: {value:+.2f}", x, y - 22, FG, small)


def draw_button(x, y, size, pressed, label):
    color = GREEN if pressed else LINE
    pygame.draw.rect(screen, color, (x, y, size, size), 0, border_radius=10)
    pygame.draw.rect(screen, FG, (x, y, size, size), 2, border_radius=10)
    text = small.render(label, True, BG if pressed else FG)
    rect = text.get_rect(center=(x + size // 2, y + size // 2))
    screen.blit(text, rect)


def draw_hat(x, y, size, hat, label):
    pygame.draw.rect(screen, LINE, (x, y, size, size), 2, border_radius=8)
    cx = x + size // 2
    cy = y + size // 2
    pygame.draw.line(screen, DIM, (cx, y + 8), (cx, y + size - 8), 1)
    pygame.draw.line(screen, DIM, (x + 8, cy), (x + size - 8, cy), 1)

    hx, hy = hat
    dot_x = cx + hx * (size // 4)
    dot_y = cy + hy * (size // 4)
    pygame.draw.circle(screen, PURPLE, (dot_x, dot_y), 8)

    draw_text(label, x, y - 20, FG, small)
    draw_text(f"{hat}", x, y + size + 4, FG, small)


connect_first_joystick()

running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        elif event.type == pygame.JOYDEVICEADDED:
            connect_first_joystick()

        elif event.type == pygame.JOYDEVICEREMOVED:
            if joystick is not None and event.instance_id == joy_instance_id:
                connect_first_joystick()

        elif joystick is not None and hasattr(event, "instance_id"):
            if event.instance_id == joy_instance_id:
                if event.type == pygame.JOYAXISMOTION:
                    axis_values[event.axis] = event.value

                elif event.type == pygame.JOYBUTTONDOWN:
                    button_values[event.button] = True

                elif event.type == pygame.JOYBUTTONUP:
                    button_values[event.button] = False

                elif event.type == pygame.JOYHATMOTION:
                    hat_values[event.hat] = event.value

    screen.fill(BG)

    draw_text("Gamepad viewer", 20, 18, YELLOW, font)

    if joystick is None:
        draw_text("Геймпад не найден. Подключи USB-контроллер.", 20, 60, RED, font)
    else:
        draw_text(f"Device: {joystick.get_name()}", 20, 52, FG, font)
        draw_text(
            f"Axes: {joystick.get_numaxes()}   Buttons: {joystick.get_numbuttons()}   Hats: {joystick.get_numhats()}",
            20, 80, FG, small
        )

        # Сырые оси
        raw_lx = axis_values.get(LX_AXIS, 0.0)
        raw_ly = axis_values.get(LY_AXIS, 0.0)
        raw_rx = axis_values.get(RX_AXIS, 0.0)
        raw_ry = axis_values.get(RY_AXIS, 0.0)

        # Правильная обработка стиков:
        # - deadzone
        # - инверсия Y для экрана
        lx = apply_deadzone(raw_lx)
        ly = apply_deadzone(raw_ly)
        rx = apply_deadzone(raw_rx)
        ry = apply_deadzone(raw_ry)

        draw_stick(260, 220, 90, lx, ly, "Left stick")
        draw_stick(560, 220, 90, rx, ry, "Right stick")

        draw_text("Raw mapped axes:", 20, 350, YELLOW, font)
        draw_axis_bar(20, 385+20, 320, 18, raw_lx, f"LX axis {LX_AXIS}")
        draw_axis_bar(20, 415+40, 320, 18, -raw_ly, f"LY axis {LY_AXIS}")
        draw_axis_bar(20, 445+60, 320, 18, raw_rx, f"RX axis {RX_AXIS}")
        draw_axis_bar(20, 475+80, 320, 18, -raw_ry, f"RY axis {RY_AXIS}")

        # Триггеры часто живут в отдельном диапазоне.
        # Если они у тебя действительно влияют на стик, значит раньше оси были перепутаны.
        lt = axis_values.get(LT_AXIS, 0.0)
        rt = axis_values.get(RT_AXIS, 0.0)
        draw_axis_bar(20, 535+80, 320, 18, lt, f"LT axis {LT_AXIS}")
        draw_axis_bar(20, 565+100, 320, 18, rt, f"RT axis {RT_AXIS}")

        draw_text("Buttons:", 380, 350, YELLOW, font)
        btn_count = joystick.get_numbuttons()
        cols = 6
        size = 48
        start_x = 380
        start_y = 385
        for i in range(btn_count):
            row = i // cols
            col = i % cols
            x = start_x + col * (size + 12)
            y = start_y + row * (size + 18)
            label = BUTTON_NAMES.get(i, str(i))
            draw_button(x, y, size, button_values.get(i, False), label)

        draw_text("Hats:", 520, 635, YELLOW, font)
        for i in range(joystick.get_numhats()):
            draw_hat(600 + i * 110, 625, 90, hat_values.get(i, (0, 0)), f"Hat {i}")

    if joystick is not None:
        lx_i = int(clamp(lx, -1, 1) * 127)
        ly_i = int(clamp(-ly, -1, 1) * 127)
        rx_i = int(clamp(rx, -1, 1) * 127)
        ry_i = int(clamp(-ry, -1, 1) * 127)
        tr = int(clamp(rt - lt, -1, 1) * 127)
        btn = 0
        btn |= button_values.get(0, False) << 0  # A
        btn |= button_values.get(1, False) << 1  # B
        btn |= button_values.get(2, False) << 2  # X
        btn |= button_values.get(3, False) << 3  # Y
        btn |= button_values.get(4, False) << 4  # LB
        btn |= button_values.get(5, False) << 5  # RB
        btn |= button_values.get(6, False) << 6  # Back
        btn |= button_values.get(7, False) << 7  # Start

        packet_id = (packet_id + 1) & 0xFF
        packet = struct.pack("BBbbbbbb", HEADER, packet_id, lx_i, ly_i, rx_i, ry_i, tr, btn)
        crc = 0
        for b in packet:
            crc ^= b

        packet += struct.pack("B", crc)
        ser.write(packet)

    pygame.display.flip()
    clock.tick(FPS)

pygame.quit()