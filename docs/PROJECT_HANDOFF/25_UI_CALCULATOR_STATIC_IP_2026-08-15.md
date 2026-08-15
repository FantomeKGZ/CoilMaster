# CoilMaster — UI, calculator and static IP checkpoint

Дата: **2026-08-15**  
Ветка: **`cmp-protocol-v1`**  
Кодовый commit: **`b6b6a0594895c55547cfa8f46c59504f1de18e67`**

## Выполнено

- В меню desktop-склада удалены фиктивные `href="#"`:
  - «Главная» ведёт на `/desktop/`;
  - «История» ведёт на `/desktop/winding-history.html`;
  - «Статистика» ведёт на `/desktop/reports.html`.
- Пустые кнопки справочника заменены явным сообщением, что материал ещё не загружен.
- Оставшиеся динамические ссылки с `href="#"` скрываются до получения реального `repair_id`.
- Калькулятор проводника принимает до пяти групп исходных жил с отдельными диаметром и количеством.
- Старый single-bundle API сохранён; новый API использует `source_component_count` и numbered source fields.
- Wi-Fi profile schema обновлена с 1 до 2 с обратным чтением schema 1.
- Для каждого Wi-Fi-профиля доступны DHCP (default) или static STA settings:
  - local IP;
  - gateway;
  - subnet;
  - DNS 1 / DNS 2.
- Service AP `192.168.4.1` остаётся включённым независимо от STA settings.
- Пароли по-прежнему не возвращаются через API.

## Проверка

```text
CMP Protocol Tests: SUCCESS
ESP32 Build: SUCCESS
RAM:   15.5% (50688 / 327680 bytes)
Flash: 39.8% (1251797 / 3145728 bytes)
Web audit: 48 HTML files + shared/injected JavaScript SUCCESS
```

Huge APP / 3 MiB application partition остаётся обязательной конфигурацией для текущей 4 MiB ESP32 без PSRAM.

## Hardware smoke-test после прошивки

1. Проверить старый DHCP-профиль без пересохранения.
2. Создать static profile с адресом вне DHCP pool либо с reservation на роутере.
3. Проверить доступ по static STA IP и `coil.local`.
4. Переключить профиль обратно на DHCP и проверить получение адреса.
5. В калькуляторе проверить 3–5 разных алюминиевых диаметров на известном практическом примере.

## Оценка готовности

Общая готовность проекта: **около 88%**.

- Функциональная готовность кода: примерно **92%**.
- Подтверждённая эксплуатационная готовность: примерно **84%**.

Основные остатки: реальные hardware tests нового static-IP path, многоисточникового калькулятора и FTP/remote backup interruption/recovery; заполнение справочной базы; длительные fault/power-loss/SD-full tests.

Safety boundary не изменён: physical START только физический, ESP32 не управляет SSR, auto-resume отсутствует, `RUN_COMPLETED` не списывает провод автоматически.
