# Журнал интеграции справочника обмотчика

Дата начала журнала: **2026-08-23**  
Рабочая ветка: **`cmp-protocol-v1`**

Этот файл — хронологический журнал текущей адаптации legacy-справочника обмотчика из `FantomeKGZ/motor-winding-reference` в CoilMaster. Его нужно обновлять по мере выполнения каждого завершённого блока работы.

## Правило ведения

Для каждого завершённого блока фиксировать:

- что именно сделано;
- какие файлы/контракты затронуты;
- commit SHA;
- результат CI/проверки, если он подтверждён;
- обнаруженные особенности legacy-source;
- следующий непосредственный шаг.

Не записывать CI как GREEN без фактического результата GitHub Actions или явного подтверждения оператора.

---

## 2026-08-23 — старт адаптации справочника

Цель:

- сохранить две пользовательские версии справочника: desktop и mobile;
- привести обе к единому дизайну CoilMaster;
- сохранить таблицы, изображения, описания и внутренние переходы;
- убрать верхний legacy-баннер/логотип;
- сохранить переходы из справочника в остальные разделы CoilMaster;
- уменьшить общий размер за счёт хранения byte-identical ресурсов только один раз.

Источник:

```text
FantomeKGZ/motor-winding-reference
sourse/desktop
sourse/mobile
```

Подтверждено, что папка исходного репозитория называется именно `sourse`.

### Общая оболочка desktop/mobile

Добавлены общие ресурсы интерфейса справочника:

```text
firmware/esp32/web/sites/reference/shared/reference.css
firmware/esp32/web/sites/reference/shared/reference.js
```

Desktop и mobile используют единую визуальную систему и общий переключатель версии через `cm-ui-version`.

Справочник встроен в навигацию CoilMaster: из него доступны рабочие разделы основного проекта. На mobile навигация остаётся доступной в компактной прокручиваемой форме.

### Legacy importer

Добавлен:

```text
tools/import_legacy_winding_reference.py
```

Контракт импортёра:

- legacy HTML Windows-1251 -> UTF-8;
- удалить только верхний `div.verh` / `images/verh.jpg`;
- сохранить тело страниц, таблицы, описания, картинки и внутренние ссылки;
- desktop/mobile HTML остаются раздельными;
- byte-identical desktop/mobile assets дедуплицируются по SHA-256 в `/sites/reference/shared/assets/`;
- уникальные ресурсы остаются в mode-specific assets;
- `_vti_*` Microsoft FrontPage metadata не публикуется;
- runtime ESP32/Arduino и safety-инварианты не затрагиваются.

### Integrity checker

Добавлен:

```text
tools/check_legacy_winding_reference.py
```

Проверяет:

- количество реальных HTML-страниц;
- UTF-8;
- отсутствие legacy top banner;
- наличие shared CSS/JS;
- внутренние ссылки;
- утечки `_vti_*`;
- дубли mode-specific assets;
- разницу между importer-created broken links и дефектами самого legacy-source.

### CI dry-build

Добавлен workflow:

```text
.github/workflows/reference-legacy-import.yml
```

Workflow checkout-ит CoilMaster и `motor-winding-reference`, строит полный справочник во временном каталоге runner-а и запускает integrity checker без предварительного коммита тысяч generated-файлов.

### Выявленные особенности legacy-source

Первичный полный обход показал:

```text
raw HTML before FrontPage filtering:
  1856 desktop
  1856 mobile

raw matching assets before FrontPage filtering:
  5547 per mode
```

После исключения `_vti_*` служебных копий подтверждён реальный объём:

```text
real HTML:
  926 desktop
  926 mobile

byte-identical shared resources:
  2769
```

Legacy export не содержит `index.html`, хотя многие старые страницы ссылаются на него. Такие home-ссылки перенаправляются на оболочку CoilMaster reference.

### Исправление FrontPage metadata

Commit:

```text
b071576bbe18c104eb7e71eb1ff55ee8602f2a09
fix(reference): exclude FrontPage metadata from import
```

Результат: `_vti_*` больше не считается реальным контентом и не должен попадать в generated site.

### Исправление проверки реального page count

После фильтрации `_vti_*` checker приведён к тому же правилу подсчёта, что и importer. Реальный baseline — 926 + 926 страниц.

### Case-insensitive legacy paths

На Windows legacy-сайт допускал несовпадение регистра в локальных путях. На Linux/ESP32 это приводит к битым ссылкам.

Пример:

```text
legacy link: images/sovmob/rsov/036.JPG
source file: тот же путь с другим регистром имени/расширения
```

Исправление:

```text
072fd3870fa6b9e36658a217d50d40cc4f589c60
fix(reference): normalize legacy path casing
```

Importer теперь разрешает старые локальные пути без учёта регистра, но записывает в generated HTML реальное canonical имя файла из source.

### CI status на текущей точке

Подтверждено через GitHub connector:

```text
CMP Protocol Tests run 32639455684: GREEN
```

Оператор после commit `072fd3870...` отдельно сообщил: **все текущие Actions зелёные**.

Следовательно reference migration baseline до SD-bundle шага принимается как GREEN по operator confirmation.

---

## 2026-08-23 — storage-aware публикация через microSD artifact

Проверена текущая архитектура `CM_StaticSiteServer` и документация SD static-site server. Справочник обслуживается непосредственно с microSD из:

```text
/web/sites/reference/
```

Поэтому принято решение **не коммитить 926 + 926 generated HTML и тысячи generated assets в историю CoilMaster**. Это не требуется для firmware flash и только раздувало бы репозиторий.

### Изменение workflow

Commit:

```text
46500480b19ca2e2f2cb5277f0e14db1f35d7735
ci(reference): publish SD-ready web bundle
```

`.github/workflows/reference-legacy-import.yml` теперь:

1. checkout-ит CoilMaster;
2. checkout-ит `FantomeKGZ/motor-winding-reference`;
3. копирует текущий `firmware/esp32/web/` в временный полный web-bundle;
4. генерирует legacy reference прямо в `web/sites/reference/`;
5. запускает integrity checker;
6. считает footprint самого reference и полного `/web`;
7. загружает artifact:

```text
coilmaster-web-sd-bundle-<commit-sha>
```

Artifact является готовым содержимым каталога `/web` microSD.

Также path-filter workflow теперь включает:

```text
firmware/esp32/web/sites/reference/**
```

поэтому изменения оболочки/стилей справочника автоматически запускают полный reference build.

### Документация SD deployment

Commit:

```text
d8661903b0732d0aa6d0e60b1a4c3ccc04a4e226
docs(reference): document SD-ready web artifact
```

Обновлён `docs/30_SD_STATIC_SITE_SERVER.md`:

- зафиксирована фактическая структура `mobile/pages`, `desktop/pages`, mode-specific assets и `shared/assets`;
- описан SD-ready artifact;
- указано, что содержимое artifact копируется как `/web` на microSD;
- отдельно указано, что CI GREEN не заменяет физическую проверку карты на ESP32.

### Причина выбранной схемы

Преимущества:

- CoilMaster git остаётся компактным;
- full reference не занимает flash ESP32;
- основной Web и справочник собираются одним согласованным bundle;
- одинаковые картинки desktop/mobile физически хранятся один раз;
- generated content воспроизводим из source + importer;
- checker остаётся обязательным gate перед публикацией artifact.

### CI status этого блока

На момент записи этого пункта новый workflow после `46500480...` ещё не объявлен GREEN в этом журнале. GREEN будет записан только после фактического результата Actions или явного подтверждения оператора.

---

## 2026-08-23 — полноценная главная и поиск по 926 страницам

Выявлено, что repository entry pages `desktop/index.html` и `mobile/index.html` всё ещё содержали временный текст о подготовке переноса, а importer давал только один прямой стартовый переход на `4A.html`. Поскольку legacy export не содержит своего `index.html`, такой вход не обеспечивал удобное обнаружение всех 926 страниц.

### Общий searchable catalog

Commit:

```text
590bd50c9ce4041f7cdb1915312a9837e70dd405
feat(reference): generate shared page catalog
```

Importer теперь генерирует:

```text
/sites/reference/shared/catalog.json
```

Каталог строится из всех реальных HTML после `_vti_*` фильтрации и содержит:

- canonical relative path;
- декодированный русский title;
- наличие страницы в desktop;
- наличие страницы в mobile.

Один общий JSON используется обеими версиями и не дублирует сами страницы.

### Проверка каталога

Commit:

```text
3b17ddb4bfff773672ca02ee16452ae6bdece5cb
test(reference): validate shared page catalog
```

Integrity checker теперь отклоняет build, если:

- `catalog.json` отсутствует или невалиден;
- есть duplicate path;
- title пустой;
- desktop/mobile availability расходится с source;
- catalog target отсутствует в generated pages;
- catalog не покрывает полный union реальных source HTML.

### Shared search UI

Commits:

```text
61fe5f8f9d935d1f4b653e6e708af8a37ed81f65
feat(reference): add shared catalog search

c1ae359f7eb4eeffd0c9cfdb76ab91e2012d0a14
style(reference): add catalog search layout
```

`reference.js` загружает один `catalog.json`, ищет по русскому title + filename и открывает найденную страницу в текущем `desktop` или `mobile` режиме. Ограничение отрисовки — первые 60 совпадений, при этом пользователю показывается полное число найденных записей.

### Новые entry pages

Commits:

```text
67bbaaafd754d45e922eb3b22d285695824318d4
feat(reference): make desktop landing searchable

873f67eb1ae7005f2e3b8020d86ada382da87411
feat(reference): make mobile landing searchable
```

Обе главные страницы теперь имеют одинаковую структуру:

- общий CoilMaster navigation shell;
- поиск по всему каталогу;
- статус количества найденных страниц;
- результаты поиска;
- быстрый переход на таблицу серии 4А;
- ручной переключатель desktop/mobile.

Временный текст «подготавливается перенос» удалён.

### CI status этого блока

На момент этой записи новые reference workflows после catalog/search commits ещё не занесены как GREEN. Не считать catalog/search batch подтверждённым до фактического Actions результата или явного подтверждения оператора.

## Следующий непосредственный шаг

1. Проверить последний `Reference Legacy Import Check` после `873f67e...`.
2. При GREEN зафиксировать artifact, фактический reference size, полный `/web` size, file counts и catalog entries.
3. Затем провести следующий UX-аудит generated legacy pages: ширина таблиц, картинки, верх/низ страницы, переход «на главную» и удобство поиска с нескольких типичных запросов.
4. После repo-reviewable UX закрытия перейти к физическому обновлению `/web` на microSD и короткому ESP32 smoke-test.
