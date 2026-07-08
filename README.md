# RawCodEt — Remote Code Editor

Редактор кода с удалённым компилятором. Клиент работает на **Qt6 + C++** с **Monaco Editor** (через `QWebEngineView`), сервер — headless Qt-приложение. Связь по **TCP** с собственным протоколом на базе **length-prefixed JSON**.

## Возможности

- Файловое дерево с навигацией по локальной файловой системе
- Редактор Monaco с подсветкой синтаксиса (C++, Python, JS, HTML)
- Удалённый доступ к файлам на сервере через консольные команды
- Сохранение/загрузка файлов на удалённой машине
- Подключение/отключение через popup-меню или консоль
- Отслеживание изменений кода (флаг `*` в заголовке)

## Структура проекта

```
summer_practice/
├── protocol/          # Общая библиотека протокола
│   ├── message.h
│   ├── message.cpp
│   └── protocol.pro
├── client/            # GUI-клиент (Qt6 + WebEngine)
│   ├── mainwindow.h
│   ├── mainwindow.cpp
│   ├── main.cpp
│   ├── editor.html    # Monaco Editor
│   └── client.pro
├── server/            # Headless-сервер
│   ├── compilerserver.h
│   ├── compilerserver.cpp
│   ├── main.cpp
│   └── server.pro
├── remote-compiler.pro  # Корневой subdirs-проект
├── Testing/           # Build-директория (out-of-source)
├── lib/               # libprotocol.a
└── bin/               # client, server
```

## Сборка

Требования: **Qt6**, **qmake6**, **g++**, **PostgreSQL** (опционально).

```bash
# На Manjaro:
sudo pacman -S qt6-base qt6-webengine qt6-tools postgresql

# Сборка:
mkdir -p Testing && cd Testing
qmake6 ..
make

# Бинарники появятся в bin/
```

## Запуск

```bash
# Терминал 1 — сервер
./bin/server
# Server listening on port 5000

# Терминал 2 — клиент
./bin/client
```

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

## Безопасность

**Учебный проект.** Сервер выполняет операции с файлами без ограничений. Не запускать в production без:
- sandbox'а (`firejail` / `bubblewrap`)
- white-list разрешённых директорий
- таймаутов на выполнение команд
- авторизации клиентов
