# CoilMaster — статические сайты с microSD

## Назначение

`CM_StaticSiteServer` обслуживает мобильную и ПК-версии CoilMaster, а также дополнительные справочные сайты непосредственно с microSD.

Корень сайтов на карте:

```text
/web/
├── index.html
├── mobile/
│   └── index.html
├── desktop/
│   └── index.html
└── sites/
    └── reference/
        ├── mobile/
        │   └── index.html
        ├── desktop/
        │   └── index.html
        └── common/          # необязательный резервный вариант
```

## Основные маршруты

```text
/                         → /web/index.html
/mobile/                  → /web/mobile/index.html
/desktop/                 → /web/desktop/index.html
/sites/reference/         → автоматический выбор варианта
/sites/reference/mobile/  → мобильный справочник
/sites/reference/desktop/ → справочник для ПК
```

Маршрут `/sites/reference/` читает сохранённый в браузере параметр:

```text
cm-ui-version = mobile | desktop
```

Поэтому мобильный CoilMaster открывает мобильный справочник, а ПК-интерфейс — ПК-справочник.

## Резервный выбор варианта

Если запрошенного файла нет:

1. проверяется папка `common`;
2. затем проверяется альтернативная версия интерфейса;
3. если файл не найден, сервер возвращает управление обычному обработчику 404.

## Безопасность путей

Сервер отклоняет:

- пути с `..`;
- обратные слеши;
- запросы без начального `/`;
- попытки обслуживать `/api/*` как файлы.

API всегда остаётся под контролем отдельных обработчиков ESP32.

## Типы содержимого

Поддерживаются HTML, CSS, JavaScript, JSON, SVG, PNG, JPEG, GIF, WebP, PDF, TXT, ICO, WOFF и WOFF2.

## Подключение к `main.cpp`

После инициализации microSD:

```cpp
#include "CM_StaticSiteServer.h"

CM::StaticSiteServer staticSites(webServer, SD);
```

При настройке веб-сервера:

```cpp
staticSites.begin("/web");

webServer.onNotFound([]()
{
    if (staticSites.serveCurrentRequest())
    {
        return;
    }

    webServer.send(404,
                   "application/json; charset=utf-8",
                   "{\"error\":\"not_found\"}");
});
```

Маршруты API необходимо регистрировать до `onNotFound()`.

## Размещение файлов на карте

На первом этапе файлы из репозитория `firmware/esp32/web/` копируются в `/web/` microSD вручную или скриптом подготовки карты. В дальнейшем загрузка и обновление сайтов будут доступны через защищённый сервисный интерфейс и FTP.
