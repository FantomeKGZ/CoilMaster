# CHANGELOG

## Release 0.1.0

### Build 001

- Проверена связь Arduino ↔ ESP32
- Проверен UART
- Проверен TXS0108E

Статус:
PASS

---

### Build 002

Начало разработки ядра CoilMaster OS.

---

### Build 002A — CMP Core

- Утверждена матрица зависимостей CMP Core.
- `CMP_Defines.h` переведён на типобезопасные константы `constexpr` в пространстве имён `CMP`.
- `CMP_Result.h` переведён на `CMP::Result` с явными стабильными кодами.
- Добавлены `constexpr`-помощники `succeeded()`, `failed()` и `isPending()`.
- Сохранены требования: без динамического выделения памяти и без исключений.

Статус:
IN PROGRESS
