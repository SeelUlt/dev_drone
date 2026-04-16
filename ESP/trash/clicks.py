import machine
import time

class Button:
    # Состояния
    STATE_RELEASE = 0
    STATE_CLICK_VERIFICATION = 1
    STATE_PUSH = 2
    STATE_LONG_PUSH = 3

    # События
    EVENT_NONE = 0
    EVENT_SHORT_PRESS = 1
    EVENT_LONG_PRESS = 2

    # Тайминги
    LOOP_TIMEOUT = 10
    THRESHHOLD_TIMEOUT = 50
    LONG_PUSH_TIMEOUT = 1000  # 1 секунда для долгого нажатия

    def __init__(self, pin: int):
        self._pin = machine.Pin(pin, machine.Pin.IN, machine.Pin.PULL_UP)
        self._state = self.STATE_RELEASE
        self._last_event_time = time.ticks_ms()
        self._loop_time = time.ticks_ms()
        self._last_event = self.EVENT_NONE

    def times_up(self, ts, timeout):
        return time.ticks_diff(time.ticks_ms(), ts) >= timeout

    def getPinValue(self):
        return self._pin.value()

    def run(self):
        """
        Возвращает событие:
        - EVENT_NONE — ничего не произошло
        - EVENT_SHORT_PRESS — короткое нажатие
        - EVENT_LONG_PRESS — долгое нажатие
        """
        
        # Ограничение частоты обновления
        if not self.times_up(self._loop_time, self.LOOP_TIMEOUT):
            return self.EVENT_NONE
        self._loop_time = time.ticks_ms()

        pin_state = self.getPinValue()

        # Сброс события после обработки
        self._last_event = self.EVENT_NONE

        if self._state == self.STATE_RELEASE:
            if pin_state == 0:  # Кнопка нажата
                self._state = self.STATE_CLICK_VERIFICATION
                self._last_event_time = time.ticks_ms()
                print("Начало нажатия")

        elif self._state == self.STATE_CLICK_VERIFICATION:
            if pin_state == 0 and self.times_up(self._last_event_time, self.LONG_PUSH_TIMEOUT):
                self._state = self.STATE_LONG_PUSH
                print("	Долгое нажатие")
                self._last_event = self.EVENT_LONG_PRESS

            elif pin_state == 1 and self.times_up(self._last_event_time, self.THRESHHOLD_TIMEOUT):
                self._state = self.STATE_PUSH
                print("	Короткое нажатие")
                self._last_event = self.EVENT_SHORT_PRESS

            elif pin_state == 1:  # Кнопка отпущена
                self._state = self.STATE_RELEASE
                print("Дребезг")

        elif self._state == self.STATE_PUSH or self._state == self.STATE_LONG_PUSH:
            if pin_state == 1:  # Кнопка отпущена
                self._state = self.STATE_RELEASE
                print("Сброс состояния")

        return self._last_event


if __name__ == "__main__":
    button = Button(15)

    while True:
        event = button.run()