# RawCodEt — Remote Code Editor

Редактор кода с удалённым компилятором. Клиент работает на Qt6 + C++ с Monaco Editor (через `QWebEngineView`), сервер — headless Qt-приложение. Связь по TCP с собственным протоколом на базе length-prefixed JSON.

## Возможности

- Файловое дерево с навигацией по локальной файловой системе
- Редактор Monaco с подсветкой синтаксиса (C++, Python, JS, HTML)
- Удалённый доступ к файлам на сервере через консольные команды
- Сохранение/загрузка файлов на удалённой машине
- Подключение/отключение через popup-меню или консоль
- Отслеживание изменений кода (флаг `*` в заголовке)

## Системные требования

### Для сборки из исходников:
- **CMake** 3.16 или выше
- **qmake6** всё подготовлено, но не рекомендуется к использованию
- **Qt 6.7.x** (или Qt 6.5+) со следующими модулями:
  - `qtbase` (Core, Gui, Widgets, Network, Sql)
  - `qtwebengine` (WebEngine, WebEngineWidgets)
  - `qtwebchannel`
  - `qtpositioning`
- **C++ компилятор** с поддержкой C++17:
  - Windows: MSVC 2019/2022 или MinGW-w64
  - Linux: GCC 9+ или Clang 10+
- **Ninja** (рекомендуется) или Make

### Для запуска готовой сборки:
- Windows 10/11 (x64)
- Linux (x64, glibc 2.31+)

## Сборка из исходников

### Windows

#### 1. Установка зависимостей

**Вариант А: Через vcpkg (рекомендуется)**
```powershell
# Установка vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Установка Qt6 с необходимыми модулями
.\vcpkg.exe install qt6-webengine:x64-windows
.\vcpkg.exe install qt6-webchannel:x64-windows
.\vcpkg.exe install qt6-positioning:x64-windows
```

**Вариант Б: Через официальный установщик Qt**
1. Скачайте [Qt Online Installer](https://www.qt.io/download-qt-installer-oss)
2. Установите Qt 6.7.x с компонентами:
   - MSVC 2019/2022 64-bit
   - Qt WebEngine
   - Qt WebChannel
   - Qt Positioning

#### 2. Сборка проекта

```powershell
# Клонируем репозиторий
git clone https://github.com/Ravanaizer/RawCodEt.git
cd RawCodEt

# Создаем папку сборки
mkdir build
cd build

# Конфигурируем CMake
# Если использовали vcpkg:
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release

# Если использовали официальный Qt (укажите свой путь):
cmake .. -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/msvc2022_64" -DCMAKE_BUILD_TYPE=Release

# Собираем
cmake --build . --config Release
```

#### 3. Деплой зависимостей

После сборки нужно скопировать DLL Qt рядом с исполняемыми файлами (если не настроены зависимости Qt):

```powershell
cd ..\bin

# Используем windeployqt для автоматического копирования
# Путь зависит от способа установки Qt:
# Для vcpkg:
C:\vcpkg\installed\x64-windows\tools\Qt6\bin\windeployqt.exe --webengine client.exe
C:\vcpkg\installed\x64-windows\tools\Qt6\bin\windeployqt.exe server.exe

# Для официального Qt:
C:\Qt\6.7.2\msvc2022_64\bin\windeployqt.exe --webengine client.exe
C:\Qt\6.7.2\msvc2022_64\bin\windeployqt.exe server.exe
```

Также нужно скопировать Monaco Editor и файлы проекта:
```powershell
# Скопируйте editor.html из папки client/
copy ..\client\editor.html .

# Скачайте Monaco Editor (версия 0.52.0 на момент работы)
# https://cdn.jsdelivr.net/npm/monaco-editor@0.52.0/min/vs
# https://github.com/microsoft/monaco-editor
# Распакуйте в папку monaco-editor/vs/
```

### Linux

#### 1. Установка зависимостей

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
sudo apt-get install -y qt6-base-dev qt6-webengine-dev qt6-webchannel-dev qt6-positioning-dev
```

**Arch Linux/Manjaro:**
```bash
sudo pacman -S base-devel cmake ninja
sudo pacman -S qt6-base qt6-webengine qt6-webchannel qt6-positioning
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake ninja-build
sudo dnf install qt6-qtbase-devel qt6-qtwebengine-devel qt6-qtwebchannel-devel qt6-qtpositioning-devel
```

#### 2. Сборка проекта

```bash
# Клонируем репозиторий
git clone https://github.com/Ravanaizer/RawCodEt.git
cd RawCodEt

# Создаем папку сборки
mkdir build
cd build

# Конфигурируем CMake
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# Собираем
cmake --build . --config Release
```

#### 3. Деплой зависимостей

```bash
cd ../bin

# Копируем необходимые библиотеки Qt
# Находим путь к Qt (обычно /usr/lib/x86_64-linux-gnu/qt6 или /usr/lib/qt6)
QT_LIB_PATH=$(pkg-config --variable=libdir Qt6Core)

# Копируем основные библиотеки
for lib in Qt6Core Qt6Gui Qt6Widgets Qt6Network Qt6Sql Qt6WebEngineCore Qt6WebEngineWidgets Qt6WebChannel Qt6Positioning Qt6OpenGL Qt6Quick Qt6Qml; do
    cp -v "$QT_LIB_PATH/lib${lib}.so"* . 2>/dev/null || true
done

# Копируем плагины
QT_PLUGINS_PATH=$(pkg-config --variable=plugindir Qt6Core)
for dir in platforms imageformats networkinformation tls; do
    cp -rv "$QT_PLUGINS_PATH/$dir" . 2>/dev/null || true
done

# Копируем ресурсы WebEngine
cp -rv /usr/share/qt6/resources ./resources 2>/dev/null || true
cp -v /usr/lib/qt6/libexec/QtWebEngineProcess . 2>/dev/null || true

# Исправляем RPATH
patchelf --set-rpath '$ORIGIN' client
patchelf --set-rpath '$ORIGIN' server

# Создаем qt.conf
cat > qt.conf << 'EOF'
[Paths]
Plugins = .
Libraries = .
Imports = .
Qml2Imports = .
EOF

# Копируем файлы проекта
cp ../client/editor.html .

# Скачиваем Monaco Editor
mkdir -p monaco-editor/vs
cd monaco-editor/vs
wget https://cdn.jsdelivr.net/npm/monaco-editor@0.52.0/min/vs/loader.js
wget https://cdn.jsdelivr.net/npm/monaco-editor@0.52.0/min/vs/editor/editor.main.js
wget https://cdn.jsdelivr.net/npm/monaco-editor@0.52.0/min/vs/editor/editor.main.css
# ... и остальные файлы

# Или скачать с гитхаба по ссылке
# https://github.com/microsoft/monaco-editor
```

## Сборка через CI/CD (GitHub Actions)

Проект настроен на автоматическую сборку через GitHub Actions. При каждом push в ветку `main` создаются готовые бинарники для Windows и Linux.

### Получение готовой сборки:

1. Перейдите на страницу [Releases](https://github.com/Ravanaizer/RawCodEt/releases)
2. Скачайте последний релиз:
   - **Windows**: `RawCodEt-Windows.zip`
   - **Linux**: `RawCodEt-Linux.tar.gz`
3. Распакуйте архив

**Windows:**
```powershell
# Распакуйте ZIP и запустите
.\bin\client.exe
```

**Linux:**
```bash
tar -xzf RawCodEt-Linux.tar.gz
chmod +x bin/client bin/server
./bin/client
```

## Запуск

### 1. Запуск сервера

```bash
# Windows
.\bin\server.exe

# Linux
./bin/server
```

Сервер запустится и будет слушать порт 5000.

### 2. Запуск клиента

```bash
# Windows
.\bin\client.exe

# Linux
./bin/client
```

### 3. Подключение к серверу

В клиенте используйте консольные команды:
```
/connect localhost:5000
```

Или через GUI: меню **Network** → **Connect**.

## Протокол

Формат пакета:
```
[4 байта: длина JSON в big-endian][JSON-тело]
```

JSON-сообщение:
```json
{
  "flag": "load|save|ls|error",
  "payload": { ... }
}
```

| Флаг | Направление | Описание |
|------|-------------|----------|
| `load` | bidirectional | Загрузка файла |
| `save` | bidirectional | Сохранение файла |
| `ls` | client → server | Список директории |
| `error` | server → client | Ошибка |

## Консольные команды клиента

| Команда | Описание |
|---------|----------|
| `ls [path]` | Список файлов на сервере |
| `load /path` | Загрузить файл с сервера в редактор |
| `save /path` | Сохранить текущий код на сервер |
| `connect [host:port]` | Подключение к серверу |
| `disconnect` | Отключение |
| `help` | Справка |

## Горячие клавиши

- `Ctrl+O` — открыть локальный файл
- `Ctrl+S` — сохранить текущий файл
- `Ctrl+Shift+S` — сохранить как

## Структура проекта

```
RawCodEt/
├── protocol/          # Общая библиотека протокола
│   ├── message.h
│   ├── message.cpp
│   └── CMakeLists.txt
├── client/            # GUI-клиент (Qt6 + WebEngine)
│   ├── mainwindow.h
│   ├── mainwindow.cpp
│   ├── main.cpp
│   ├── editor.html    # Отрисовка для Monaco Editor
│   └── CMakeLists.txt
├── server/            # Headless-сервер
│   ├── compilerserver.h
│   ├── compilerserver.cpp
│   ├── main.cpp
│   └── CMakeLists.txt
├── CMakeLists.txt     # Корневой CMake проект
├── .github/
│   └── workflows/
│       └── build.yml  # CI/CD конфигурация
└── README.md
```

## Безопасность

**Учебный проект. Не запускать в production без дополнительной защиты!**

Сервер выполняет операции с файлами без ограничений. Для production использования необходимо добавить:

- **Sandbox**: `firejail` / `bubblewrap` для изоляции сервера
- **White-list**: ограничение разрешённых директорий
- **Таймауты**: ограничение времени выполнения команд
- **Авторизация**: аутентификация клиентов

## Лицензия

MIT License

## Разработка

Проект разработан в рамках летней практики в ВУЗе.

**Технологии:**
- C++17
- Qt 6.7
- CMake 3.16+
- Monaco Editor 0.52.0
- GitHub Actions (CI/CD)
