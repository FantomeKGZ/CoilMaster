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

### Подтверждённый SD bundle и UX-навигация

Reference workflow:

```text
Reference Legacy Import Check run 32641366079 — GREEN
head_sha cea85a09dd4208bf88545284ae80a12edce22681
```

Фактический результат runner-а:

```text
desktop generated pages: 926
mobile generated pages: 926
catalog entries: 926
pre-existing source gaps: 0
reference footprint: 329M
full /web footprint: 331M
reference files: 4626
reference HTML files: 1854
shared assets: 2769
desktop unique assets: 0
mobile unique assets: 0
full /web files: 4693
```

Создан artifact:

```text
coilmaster-web-sd-bundle-cea85a09dd4208bf88545284ae80a12edce22681
artifact id: 9493687862
archive size: 266893049 bytes
digest: sha256:f161c871a39dfaa4ecbbd8d4e1971e464bba392d7260585d29d8161a93cacf1d
expires: 2026-09-06
```

Artifact содержит готовое содержимое каталога `/web` для microSD.

UX-аудит generated shell также закрыт repo-reviewable изменениями:

```text
e1437c45288916e17e8ead7870a8950710022849
feat(reference): add generated page navigation

229d26cddc6345a6e9c60f0fa68de6f455a924c2
style(reference): improve legacy page navigation

cea85a09dd4208bf88545284ae80a12edce22681
test(reference): require generated page navigation
```

На каждой generated desktop/mobile странице теперь есть верхний и нижний переход к общей странице поиска, переход наверх, адаптивные изображения и горизонтальная прокрутка широких таблиц. Integrity checker требует эту навигацию на всех generated страницах.

### Связанный CMP CI

Runs `32640743878` и `32640777993` выявили, что общий repository navigation audit не различал committed и generated reference targets. Контракт разделён commit-ом:

```text
116a65a6a9ca0d7a3e9454169ccd6418be358908
test(web): recognize generated reference links
```

Run `32641366058` затем выявил только синтаксическую ошибку переэкранированного JavaScript regex. Исправление:

```text
e5fc99e6f03c68e3e313fa5cc151a5ade0af74a4
fix(web): correct generated reference regex
```

Reference workflow подтверждён GREEN. После исправления `e5fc99e6...` оператор 2026-08-23 явно подтвердил: **все текущие Actions зелёные**. Repo-level reference integration принимается GREEN по сочетанию фактического reference run и operator confirmation текущего CMP результата.

## Следующий непосредственный шаг

1. Скачать подтверждённый artifact до истечения срока хранения.
2. С резервной копией текущего каталога заменить содержимое `/web` на microSD.
3. Выполнить короткий ESP32 smoke-test: главные desktop/mobile, поиск, типичные таблицы, изображения и переходы.
4. Hardware GREEN не выводить из CI; записать только после фактической проверки на ESP32.

---

## 2026-08-23 — physical web smoke GREEN и двусторонняя навигация

Оператор после обновления microSD явно подтвердил: **ошибок нет**. Физический smoke справочника на ESP32 принимается GREEN для проверенного bundle:

```text
coilmaster-web-sd-bundle-cea85a09dd4208bf88545284ae80a12edce22681
```

Подтверждены без ошибок основные desktop/mobile entry pages, поиск, generated страницы, таблицы, изображения и переходы. Это подтверждение относится к web/reference smoke; оно не заменяет отдельный двухплатный UART/repeat/cancel hardware gate.

После physical smoke найден и исправлен следующий repo-level UX-разрыв:

```text
327786a0372cb9dfc53126651f642ad71a7702b2
fix(web): restore anchor link audit

24822ededb3d7f079441bf7e55dc7e9ef40963fd
feat(web): link reference from shared shell

9033fbef1623c91282481ccca103914e516eb2b1
test(web): require reference shell entry

e955850bb65fbdd0a9881d0e1566b3d2404b852d
ci(reference): rebuild bundle for all web changes
```

Основной shared app shell теперь показывает «📚 Справочник» и ведёт в desktop/mobile reference согласно текущему UI mode. Общий anchor audit снова реально разбирает теги ссылок, а regression test требует новый вход.

Reference workflow path-filter расширен с одного reference subtree до полного firmware/esp32/web subtree. Любое изменение основного Web теперь пересобирает согласованный SD-ready bundle, а не оставляет artifact со старой версией остальных страниц.

### Следующий непосредственный шаг

Оператор 2026-08-23 явно подтвердил: **все текущие Actions зелёные**. Batch двустороннего входа и полного web-bundle trigger принимается repo-level GREEN.

Следующий UX-разрыв: переключатель desktop/mobile на generated legacy странице возвращал на главную справочника и терял открытую таблицу. Исправлено:

```text
60ae0f54db4339a6f3f452685d369e73214ed4eb
feat(reference): preserve page on mode switch

41717ae7ca79e4376dc79c1102fa3f9688ed5d67
test(reference): require same-page mode switch
```

Теперь обе кнопки переключения версии на каждой generated странице ведут на тот же relative page path в другой версии. Checker требует exact same-page target для всех 926 desktop и 926 mobile страниц.

1. Проверить новый Reference Legacy Import Check после 41717ae7.
2. При GREEN скачать новый согласованный SD artifact.
3. После обновления microSD проверить вход «📚 Справочник» из основного сайта и переключение desktop/mobile внутри одной найденной таблицы.

---

## 2026-08-23 — улучшение релевантности поиска

Search UX расширен без изменения generated catalog format:

```text
fff7e338c9a4d7516eb09458ae5e7c5c74eda2c3
feat(reference): improve catalog search relevance

62667b66dd575b6cb89a28d057bb9244a9bf2749
test(reference): cover catalog search relevance

7c595062c2303f57307330041e9f49a31f6fe040
ci(reference): run catalog search contracts
```

Теперь поиск:

- разбивает запрос на несколько слов и требует совпадение каждого слова независимо от порядка;
- ищет одновременно по title и legacy filename/path;
- ставит точные и начальные совпадения выше частичных;
- сохраняет запрос в URL-параметре q, поэтому результат можно повторно открыть и восстановить;
- сохраняет desktop/mobile availability filter.

Добавлен исполняемый Node regression test для нормализации, multi-token ranking, Latin legacy path и mode filtering. Он включён отдельным шагом в CMP Protocol Tests.

Следующий шаг: проверить CMP и Reference workflows после 7c595062, затем включить этот поиск в новый SD artifact.

---

## 2026-08-23 — быстрые фильтры, keyboard UX и SD provenance

Search entry pages получили одинаковые desktop/mobile controls:

```text
dec392cff396f927182710f4de3dd06567810c9d
feat(reference): add desktop search controls

4330a6219e6d220aecad57b772bcb5ca922c7797
feat(reference): add mobile search controls

a77f034d8995672a82acf9caad465b2408d086d3
feat(reference): add keyboard search controls

c2540add35f403cf038b6d15ba01e49953426874
style(reference): refine search controls

c8b0e92251591022c0e790c77512d1e6c0f8a046
test(reference): validate search entry controls

01f1acf6c39d8a5eabeefe00a9123e077c8f18ff
test(reference): cover search controls
```

Добавлены clear button, Enter для первого результата, Escape для очистки, live status и list semantics.

Для быстрого старта без ручного ввода добавлены chips 4А, АИР, АО2 и 5А:

```text
ec96ecb787c6ede53f486cf0d35e0320ec1bb7a8
67caf349e431865f92c54b91b42c4023fa222f4c
e420afd78e2c5d17be64f95a2debc7ac716db3e6
d2dbdc9a754a291277b042726711a54f4fead2f2
d03de9ec5e7404a0904192c6d36d7a78ef378cd3
17dfe1f4082e67d36bf39ef95d37eaff869d6432
```

### SD web provenance

Каждый новый bundle теперь содержит web-bundle-manifest.json с exact CoilMaster commit, exact legacy source commit, workflow run, catalog count и generation time:

```text
d1deb65456dbc269063a5db210c965647320ebfb
ci(web): add SD bundle provenance manifest

d130af8e7248c4f9ed9a37bde4a1958c702650c2
ci(web): keep manifest catalog count extensible

d5f22ad695c860a786753d10a349e4af43687f6b
feat(web): display SD bundle provenance

625bd081ddcb2861b8532495cf3fa98d0697853a
test(web): require SD bundle provenance

b4079ca0e277079f662d137e978fb7d475e5791a
test(ci): require SD bundle manifest

49fb6868f6bc1a458bbb40f58c4d0893250e2785
docs(web): document SD bundle provenance
```

Основной app shell и страницы справочника показывают SD <commit>. Старая карта без manifest остаётся работоспособной и явно показывает SD web unknown.

Reference-side badge и regression protection:

```text
0227d3d72e9e21eb907e5aab453b71039a769290
feat(reference): display SD bundle provenance

c6c5fca35869b01a5fff7a99d6c808e1993d5de1
style(reference): add SD provenance badge

be79e7c831bcf34293d6d7a38c7360b10a2392b6
test(reference): require SD provenance badge
```

Следующий шаг: проверить текущие CMP/Reference workflows и получить новый artifact с manifest. До фактического результата этот новый batch не объявлять GREEN.

---

## 2026-08-23 — performance/accessibility legacy content

Generated legacy pages получили runtime enhancement без изменения исходного содержимого:

```text
76d3ce658b0026dfbce0e300ac0d66e1ccec7a98
perf(reference): lazy-load legacy images

5f0543e26df8fcc32a6aacdce4207804699a8559
style(reference): expose scrollable tables

b129af369a7245f1ca51381092a90c0b42a31725
test(reference): cover legacy media UX
```

Все legacy images получают lazy loading и async decoding; если source не содержит alt, используется безопасный fallback по имени файла. Широкие таблицы помещаются в focusable horizontal-scroll region с role/aria-label и видимым keyboard focus.

Это уменьшает первоначальную загрузку тяжёлых generated страниц и делает таблицы доступными для touch и keyboard navigation.

Текущий combined web/reference batch ожидает фактического Actions результата; GREEN не заявлен.


---

## 2026-08-23 — report-only аудит миграционного мусора

После подтверждения оператором, что все текущие commits GREEN, начат следующий блок оптимизации legacy-миграции.

Добавлен отдельный read-only аудитор:

```text
39c21b75dcc6a669fbf4fb83c8d2c2e7787a1027
feat(reference): add report-only migration audit
```

`tools/audit_legacy_winding_reference.py` формирует JSON-отчёт и ничего не удаляет. Он считает:

- generated pages и legacy incoming links;
- страницы без входящих ссылок из других legacy pages отдельно для desktop/mobile;
- assets, на которые не ссылаются generated HTML или CSS;
- группы byte-identical assets и потенциальную экономию;
- группы полностью одинаковых generated HTML внутри одного UI mode.

Страница без legacy incoming link не считается автоматически мусором: она остаётся доступна через полный `catalog.json` и поиск. Любое удаление допускается только после анализа отчёта.

CI integration:

```text
06fc997c9932c70501decc823cfbab45fdc3fb71
ci(reference): publish migration cleanup audit

fea939e2b17cf3268292f41ee350c8fa2088c8cd
test(reference): cover report-only migration audit

4be50011a4ab85210b2225821802e8fbdcf2aa8b
ci(reference): run migration audit contracts

a50c1be52ef96e9784fcffdc29c08261a0105275
fix(reference): correct migration audit URL matching
```

Reference workflow теперь публикует отдельный artifact:

```text
coilmaster-reference-migration-audit-<commit-sha>
```

Полный SD-ready artifact остаётся неизменным по политике сохранения контента. Новый аудит добавлен в Actions summary и запускается при изменении самого аудитора или его contract test.

Синтетический contract test локально пройден: HTML/CSS references, unreferenced bytes, duplicate hashes и legacy incoming-link counts проверены. Фактические числа полного справочника будут зафиксированы только после GREEN нового Reference workflow.
