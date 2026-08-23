# CoilMaster — статические сайты с microSD

## Назначение

`CM_StaticSiteServer` обслуживает мобильную и ПК-версии CoilMaster, а также дополнительные справочные сайты непосредственно с microSD.

Корень сайтов на карте:

```text
/web/
├── web-bundle-manifest.json
├── index.html
├── mobile/
│   └── index.html
├── desktop/
│   └── index.html
└── sites/
    └── reference/
        ├── mobile/
        │   ├── index.html
        │   ├── pages/
        │   └── assets/
        ├── desktop/
        │   ├── index.html
        │   ├── pages/
        │   └── assets/
        └── shared/
            ├── reference.css
            ├── reference.js
            └── assets/
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

1. проверяется общий reference-контент;
2. затем проверяется альтернативная версия интерфейса там, где это разрешено текущим серверным контрактом;
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

## Справочник обмотчика и размер репозитория

Полный legacy-справочник содержит сотни HTML-страниц и тысячи изображений. Generated-файлы не хранятся постоянно в git-ветке CoilMaster. Вместо этого workflow

```text
.github/workflows/reference-legacy-import.yml
```

checkout-ит `FantomeKGZ/motor-winding-reference`, строит desktop/mobile reference, дедуплицирует все повторяющиеся byte-identical non-CSS ресурсы с одинаковым расширением в `sites/reference/shared/assets/`, запускает integrity checker и формирует полный SD-ready `/web` каталог.

Это позволяет:

- не дублировать тысячи generated-файлов в истории CoilMaster;
- не увеличивать размер firmware flash — справочник обслуживается с microSD;
- получать один проверенный комплект, в котором основной CoilMaster Web и справочник имеют согласованные версии;
- сохранять общий shell CSS/JS и повторяющиеся byte-identical non-CSS ресурсы справочника только в одном экземпляре, даже если legacy-файлы имели разные имена или повторялись внутри одного UI mode;
- сохранять legacy CSS mode-specific, конвертировать его в UTF-8 и переписывать каждый локальный `url(...)` на фактический shared/mode asset, чтобы перенос ресурса не ломал относительные ссылки.

## Готовый artifact для microSD

Успешный `Reference Legacy Import Check` загружает artifact вида:

```text
coilmaster-web-sd-bundle-<commit-sha>
```

Тот же GREEN workflow публикует отдельный report-only artifact:

```text
coilmaster-reference-migration-audit-<commit-sha>
```

Он содержит JSON со страницами без legacy incoming links, неиспользуемыми assets, группировкой по расширению/count/bytes и оставшимися byte-identical группами. Audit artifact не является содержимым microSD и сам ничего не удаляет.

Содержимое artifact — готовое содержимое каталога `/web` на microSD. После распаковки структура должна выглядеть как:

```text
/web/web-bundle-manifest.json
/web/index.html
/web/mobile/...
/web/desktop/...
/web/shared/...
/web/sites/reference/mobile/...
/web/sites/reference/desktop/...
/web/sites/reference/shared/...
```

Копировать нужно именно содержимое artifact как каталог `/web` карты, а не помещать дополнительный уровень `coilmaster-web` внутрь `/web`.

Каждый новый artifact содержит /web/web-bundle-manifest.json с точной provenance:

- coilmaster_commit — commit CoilMaster, из которого собран Web;
- legacy_commit — commit исходного legacy-справочника;
- workflow_run_id;
- reference_catalog_entries;
- generated_utc.

Общий app shell читает manifest и показывает короткий SD <commit> рядом с firmware/Web version. Если установлена старая карта без manifest, интерфейс остаётся работоспособным и показывает SD web unknown; это fail-soft диагностика, а не блокировка производства.

Перед использованием artifact убедиться, что соответствующий workflow завершился GREEN. CI GREEN подтверждает сборку/целостность файлов, но не заменяет физическую проверку microSD на реальном ESP32.

## Ручное размещение

Без artifact исходные файлы `firmware/esp32/web/` можно копировать в `/web/` microSD вручную. Однако такой способ не содержит полного generated legacy-reference, если импортёр отдельно не запускался. Для полного справочника предпочтителен SD-ready artifact workflow.

В дальнейшем обновление сайтов также может выполняться через предусмотренный сервисный/FTP путь; это не меняет production safety boundary и не даёт Web прямого управления SSR.
