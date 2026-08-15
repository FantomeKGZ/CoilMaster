# CoilMaster — pricing history paging checkpoint

Дата: **2026-08-15**  
Ветка: **`cmp-protocol-v1`**  
Кодовый commit: **`ec998c85179a1918958a77c0fe1294049f994b4c`**

## Закрытый unbounded consumer

`GET /api/repairs/pricing-history` больше не формирует все редакции цены одного
ремонта в одном JSON.

```text
cursor default 0
limit default 20
limit max 32
count
total_count
has_more
next_cursor
max_page_size
```

Cursor является проверяемым byte offset на границе NDJSON-записи. Ответ страницы
ограничен, но backend продолжает строгий проход полного pricing ledger, чтобы
проверить total revision count, последнюю цену, валюту и timestamp относительно
активной калькуляции ремонта.

Desktop/mobile costing UI показывает по 20 редакций и кнопки «Назад/Далее».

## Проверка

```text
CMP Protocol Tests: SUCCESS
ESP32 Build: SUCCESS
RAM:   15.5% (50712 / 327680 bytes)
Flash: 39.9% (1255489 / 3145728 bytes)
```

Safety invariants не изменены. Это read-only paging истории цены; физический
START, SSR, UART и ручное exact-run списание провода не затронуты.
