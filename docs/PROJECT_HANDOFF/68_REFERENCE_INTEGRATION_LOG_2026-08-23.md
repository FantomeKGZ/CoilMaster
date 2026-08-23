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

Следовательно текущий reference migration baseline принимается как GREEN по operator confirmation.

## Следующий непосредственный шаг

Перейти от dry-build pipeline к публикации/интеграции generated reference content в storage-aware форме, сохраняя:

- 926 desktop страниц;
- 926 mobile страниц;
- 2769 общих ресурсов;
- общую навигацию CoilMaster;
- отсутствие верхнего legacy banner;
- case-safe ссылки;
- отсутствие `_vti_*`;
- отсутствие потерь таблиц/описаний/изображений.

Перед массовым добавлением generated content сначала определить фактический generated footprint и способ хранения, чтобы не раздувать firmware/repository без необходимости.
