#!/bin/bash

# Цвета для красивого вывода
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${PURPLE}╔══════════════════════════════════════╗${NC}"
echo -e "${PURPLE}║      ЗАПУСК PYGAME ПРОЕКТА           ║${NC}"
echo -e "${PURPLE}╚══════════════════════════════════════╝${NC}"
echo ""

# Проверяем, существует ли venv
if [ ! -d "venv" ]; then
    echo -e "${YELLOW}📦 Виртуальное окружение не найдено. Создаю...${NC}"
    python3 -m venv venv
    echo -e "${GREEN}✅ Виртуальное окружение создано!${NC}"
else
    echo -e "${GREEN}✅ Виртуальное окружение уже существует${NC}"
fi

# Активируем окружение
echo -e "${CYAN}🚀 Активирую виртуальное окружение...${NC}"
source venv/bin/activate

# Проверяем, установлен ли pygame
if ! pip show pygame > /dev/null 2>&1; then
    echo -e "${YELLOW}📦 Pygame не установлен. Устанавливаю...${NC}"
    pip install pygame
    echo -e "${GREEN}✅ Pygame установлен!${NC}"
else
    echo -e "${GREEN}✅ Pygame уже установлен${NC}"
fi

# Запускаем main.py
echo -e "${CYAN}🎮 Запускаю main.py...${NC}"
echo ""
python main_linux.py

# Деактивируем окружение после завершения
deactivate
