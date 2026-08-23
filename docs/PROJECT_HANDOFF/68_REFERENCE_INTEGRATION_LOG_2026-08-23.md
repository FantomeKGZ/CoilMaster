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

2bdd4fb91a64436812fc96cd0341057852efc89c
fix(reference): include inline asset references in audit
```

Reference workflow теперь публикует отдельный artifact:

```text
coilmaster-reference-migration-audit-<commit-sha>
```

Полный SD-ready artifact остаётся неизменным по политике сохранения контента. Новый аудит добавлен в Actions summary и запускается при изменении самого аудитора или его contract test.

Синтетический contract test локально пройден: HTML/CSS references, unreferenced bytes, duplicate hashes и legacy incoming-link counts проверены. Фактические числа полного справочника будут зафиксированы только после GREEN нового Reference workflow.


### Actions failures 32643425748–32643578042

Проверены полные job logs четырёх Reference workflow runs:

- `32643425748` — ранняя версия аудитора падала при compile regex: `missing ), unterminated subpattern`;
- `32643450801` — тот же regex defect остановил contract test;
- `32643491396` — regex и contract test уже исправлены, импорт/checker прошли, но full-tree CSS scan встретил Windows-1251 stylesheet и получил `UnicodeDecodeError: byte 0xCC`;
- `32643578042` — подтверждение той же единственной оставшейся причины после успешного contract test и полного импорта.

Последняя ошибка исправлена:

```text
d7de669309abbc42589f84f35e440b0f32fcf57f
fix(reference): decode legacy CSS during migration audit

3603b7a20b8ce05fc41c863e97934300b4b1912d
test(reference): cover Windows-1251 stylesheet audit
```

Audit CSS reader теперь использует контролируемый decode `utf-8-sig -> cp1251`. Требование UTF-8 для generated HTML не ослаблено: его раньше аудитора проверяет authoritative reference checker.

Contract test дополнен реальным CP1251 stylesheet с кириллическим байтом и asset URL. Локально compile + synthetic contract test GREEN. Новый Actions результат после `3603b7a2...` ещё не зафиксирован как GREEN.


### Operator GREEN и точность cleanup-кандидатов

Оператор 2026-08-23 явно подтвердил: Actions после CP1251 CSS correction GREEN. Исправления `d7de6693...` + regression `3603b7a2...` приняты как текущая verified GREEN baseline.

Следующий report-only batch повышает точность, но не удаляет контент:

```text
8e42b306c8e791d0de30026d76d14c11136ae890
fix(reference): resolve relative legacy asset URLs

c0ba06776415e765d0f88b2036fd9fdbaae95454
fix(reference): normalize single Windows path separators

a751d23bdbbe1e5468d752298d705e69a57c9188
test(reference): cover relative legacy asset URLs

f33726909432658cccaf4f51bfa5bf2a6c12e635
feat(reference): classify unused assets in audit

a46e78056e34139ace22ff869a26ba1da893ca7e
test(reference): cover unused asset classification

f29d573c0c693991516729a163c74c9ddee15ad3
ci(reference): summarize unused asset types

6e87c5f914ab8f584b63e30db5d9500bee7e1f1b
test(reference): exercise single runtime backslashes
```

Аудитор теперь разрешает absolute, POSIX-relative и Windows-backslash URLs относительно реального HTML/CSS файла. Это уменьшает ложные `unreferenced` результаты. Неиспользуемые assets дополнительно группируются по расширению с count/bytes в JSON artifact и Actions summary. Synthetic regression локально GREEN; новый Actions batch пока не перенесён в verified baseline.


---

## 2026-08-23 — глобальная content-addressed дедупликация assets

Оператор явно подтвердил GREEN предыдущего URL-accuracy/audit-classification batch до начала этого изменения.

Проверка importer показала ограничение старой схемы: ресурс становился shared только при совпадении desktop/mobile, а shared filename включал legacy basename. Поэтому одинаковые байты с разными именами либо повтор внутри одного mode могли физически сохраняться больше одного раза.

Новая схема группирует все assets по:

```text
SHA-256 bytes + lowercase suffix
```

Если группа содержит два и более файла, все исходные ссылки переписываются на единственный content-addressed target:

```text
/sites/reference/shared/assets/<full-sha256><suffix>
```

Одинаковые байты с разными расширениями намеренно не объединяются, чтобы сохранить корректное MIME-разрешение static server.

Commits:

```text
70134c05b81476cf435c034f31fa3b5350838abe
perf(reference): deduplicate all repeated assets

9d615beec8ad3732d6f00479dbf7ec630e32fd19
test(reference): require global asset deduplication

c6d9879aaa3914a2c6b139423e8ff9a936d6c74b
test(reference): cover global content-addressed dedup

2cae0ebdf6a6dc59f5373f2608db4b72d4e86b92
ci(reference): run global asset dedup contracts

7ac48473de54167b3a5ddbd85be16755c41522f7
fix(reference): keep cross-suffix assets distinct

cc09993f8bfdf561357467166032efe5e5d22be7
test(reference): preserve identical cross-suffix assets

ab75e081cdd1ca3a1597b8ed5a6224ea22aef8d2
docs(sd): document global reference asset dedup
```

Checker теперь ищет duplicate `SHA-256 + suffix` во всех shared/desktop/mobile asset trees, а не только совпадения между mode-specific desktop/mobile. Synthetic importer regression проверяет разные legacy-имена, повтор внутри desktop, совпадение desktop/mobile, переписанные URLs, mode-unique файл и одинаковые bytes с другим suffix. Локальный end-to-end synthetic import прошёл GREEN: один physical shared file обслуживает три исходных `.bin` URL, `.dat` с теми же bytes сохранён отдельно.

Generated pages, catalog coverage и safety/runtime не изменяются. Новый full Reference Actions результат для этого batch ещё не объявлен GREEN.


---

## 2026-08-23 — mode-safe legacy CSS import

Оператор подтвердил GREEN global content-addressed asset dedup batch.

Следующий аудит выявил correctness boundary: CSS нельзя перемещать в shared как неизменяемый binary asset, потому что относительные `url(...)` разрешаются от нового местоположения и могут стать broken. Кроме того, одинаковый исходный CSS desktop/mobile после переписывания может требовать разные mode-specific targets.

Реализовано:

- legacy `.css` исключён из raw SHA asset dedup;
- CSS остаётся в `desktop/assets` или `mobile/assets`;
- Windows-1251/UTF-8 legacy CSS декодируется и записывается как UTF-8;
- локальные `url(...)` переписываются importer-ом через тот же canonical source mapping, что и HTML `href/src`;
- target может вести на content-addressed shared asset либо mode-specific asset;
- checker требует UTF-8, absolute rewritten local URLs, существующий target и отсутствие legacy CSS в `shared/assets`;
- intentional mode-specific CSS исключён из duplicate-savings отчёта.

Commits:

```text
426c2a56df90721a3f215ffade4847f1e8463a6e
fix(reference): preserve and rewrite legacy CSS assets

ab5a81e209273beed84e60cd8597d960ce08e210
test(reference): validate rewritten legacy CSS URLs

9fb5ecc945ec6837e909d3b2ec6e24fe7b668dca
fix(reference): exclude mode-specific CSS from dedup savings

464527ad878ae7c9640dd1947d52f34871df820f
test(reference): cover mode-safe legacy CSS rewriting

56a9037f13ebc16e781646df9cfe1ef39c2030cc
fix(reference): declare legacy CSS URL matcher

80c79ca81869c067277ab7d759a9c1687eb658df
fix(reference): correct CSS URL matcher escaping

2df1764a747bc8b05a65d073b356261c4897f865
fix(reference): declare checker CSS URL matcher

3503493f783bb18c755469463c8c4061bceeb5f9
docs(sd): document mode-safe legacy CSS handling
```

Локальный end-to-end synthetic import + authoritative checker GREEN: CP1251 CSS преобразован в UTF-8, desktop/mobile CSS сохранён mode-specific, оба `url(...)` ведут на один реальный shared binary asset, source gaps = 0. Новый full Reference Actions результат для этого batch пока не объявлен GREEN.


### Actions failures 32644626781–32644735339

Проверены jobs и полные logs шести Reference runs:

- `32644626781`, `32644645590`, `32644651617` — full importer/checker path достиг CSS validation и упал на отсутствующей декларации checker `CSS_URL_RE`;
- `32644667575` — промежуточный importer commit вызвал `NameError: CSS_URL_RE` уже в synthetic import contract;
- `32644700553` — matcher объявлен, но переэкранирован, поэтому CSS не переписывался и regression завершился `AssertionError`;
- `32644735339` — importer contract GREEN, full source импортирован полностью (`926 + 926`, catalog `926`, shared maps `2768 + 2768`), затем checker остановился на своей отсутствующей декларации `CSS_URL_RE`.

Все перечисленные runs относятся к superseded intermediate commits. Актуальные исправления:

```text
56a9037f13ebc16e781646df9cfe1ef39c2030cc
fix(reference): declare legacy CSS URL matcher

80c79ca81869c067277ab7d759a9c1687eb658df
fix(reference): correct CSS URL matcher escaping

2df1764a747bc8b05a65d073b356261c4897f865
fix(reference): declare checker CSS URL matcher

ef7798742900ee0e85035941cba15a87b4afb0f6
test(reference): run CSS checker in import contract
```

После `2df1764a...` локальный synthetic full import + authoritative checker GREEN. Новый regression теперь вызывает `validate_legacy_stylesheets()` непосредственно в раннем import contract step, чтобы missing matcher/broken CSS URL обнаруживался до checkout/full import 1852 страниц. Новый Actions результат после `ef779874...` ещё не объявлен GREEN.


---

## 2026-08-23 — embedded CSS и nested imports

Оператор подтвердил GREEN CSS matcher/checker correction batch.

Расширен rewrite/check contract:

- `url(...)` переписывается не только в отдельных CSS-файлах, но также в inline `style` и блоках `<style>` legacy HTML;
- quoted `@import "..."` / `@import '...'` переписывается на absolute generated CSS target;
- checker обходит оба вида CSS references в generated pages и mode-specific stylesheets;
- external/data/fragment URLs сохраняются;
- missing source target остаётся явным SOURCE WARNING, а importer-created broken/non-rewritten target — fatal error.

Commits:

```text
3fb5fc5d8a02868dfe916269dd3aaf534706879a
fix(reference): rewrite embedded CSS and imports

1a44fc09520ccb7dd05102673fce0254f967e58b
test(reference): validate embedded CSS and imports

5cf33c7c8909ece2f92d64f143a6ff796ce7c451
test(reference): cover inline CSS and imports

cc23af3504a6f24088f4ea55b93644d1da776885
docs(sd): document embedded CSS rewriting
```

Exact repository contract test локально GREEN: desktop inline `url` (style attribute + style block), mobile inline `url`, CP1251 main CSS, nested `@import`, nested CSS asset URL, shared binary dedup и checker validation пройдены. Новый Actions batch ещё не объявлен GREEN.


---

## 2026-08-23 — legacy media attributes

Оператор подтвердил GREEN embedded CSS/import batch.

Проведён аудит legacy HTML resource attributes. Importer/checker/auditor ранее обрабатывали `href/src`, но старый FrontPage HTML также может использовать `background` на body/table/cell и `poster` на media.

Добавлено единое покрытие:

```text
href | src | poster | background
```

Commits:

```text
b9784321a912fe19547122ddb406281ba6d951fd
fix(reference): rewrite legacy media attributes

1f8f8363c7f4955cafd5ecfe735fce049bfdaaf0
test(reference): validate legacy media attributes

fa4c7c3fdb2a14097349bda5248e28d40e0a3789
fix(reference): audit legacy media attributes

60149a5ba6b8d3624d790e57b08d48d5b08e38ec
test(reference): cover background and poster attributes

203057ef0c6faefacd93f82742ea8c602832fdb9
test(reference): cover media attribute audit

2b30eaa88bc970d58e97c10b2307666a166ed3d1
docs(sd): document legacy media attribute rewrite
```

Exact importer + checker + audit contract tests локально GREEN. Referenced `poster` asset не попадает в unreferenced candidates, а background/poster URLs переписываются на единственный shared target.

Code search в legacy repository не обнаружил `srcset=` или `poster=`; `poster` сохранён как недорогой defensive contract, а отдельный srcset parser без фактического source usage не добавлялся. Новый full Actions batch ещё не объявлен GREEN.

---

## 2026-08-23 — full content fidelity gate

Оператор подтвердил GREEN предыдущего batch с legacy media attributes.

Добавлена проверка полноты переноса содержимого для всех generated desktop/mobile страниц:

- source и generated HTML декодируются по тем же UTF-8/Windows-1251 правилам, что importer;
- из сравнения исключаются только служебные `script/style`, comments, shell и удаляемый legacy top banner;
- normalized visible text source должен полностью совпадать с generated page;
- дополнительно сравниваются structural counts: `table`, `tr`, `th`, `td`, `img`, `a`;
- CSS/URL rewrite не создаёт ложных несовпадений, потому что невидимое содержимое стилей не считается пользовательским текстом.

Commits:

```text
e89813f2feb2ad73f47aff6a6ae68f2b7dc41c1a
test(reference): validate full content fidelity

fe2aeed161b30886f3057df8cab24bda7d5ef932
test(reference): cover content fidelity regression
```

Exact synthetic importer/checker contract локально GREEN, включая отрицательный regression с подменой видимого текста. Новый полный workflow должен проверить этот контракт на реальных 926 desktop + 926 mobile страницах; до фактического результата он не записывается как CI GREEN.

Текущая оценка готовности:

```text
перенос контента:                         100%
автоматизация/целостность миграции:       98% после GREEN полного fidelity run
UX и offline SD bundle:                   92%
общая готовность сейчас:                  94%
общая готовность после GREEN fidelity:    около 96%
```

Оставшийся release scope: снять метрики нового artifact, проверить representative tables/images/transitions на последнем bundle и выполнить physical microSD/ESP32 smoke именно свежего artifact. Отдельный ESP32<->Arduino UART hardware gate не входит в процент готовности reference-сайта.

