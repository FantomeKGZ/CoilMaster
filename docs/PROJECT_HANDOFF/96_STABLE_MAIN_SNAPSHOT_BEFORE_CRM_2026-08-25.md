# CoilMaster — stable main snapshot before CRM redesign

Дата: **2026-08-25**  
Рабочая ветка после snapshot: **`cmp-protocol-v1`**

## Stable snapshot

Перед началом большого Web/CRM redesign ветка `main` была синхронизирована fast-forward с текущим `cmp-protocol-v1` без merge-конфликтов и без переписывания истории.

Зафиксированная стабильная точка:

```text
449570d47649d5f6336a31ee3eed491256e0fb1a
docs(handoff): transfer crm redesign phase
```

На момент snapshot:

```text
main                                  -> 449570d47649d5f6336a31ee3eed491256e0fb1a
stable-2026-08-25-pre-crm-redesign    -> 449570d47649d5f6336a31ee3eed491256e0fb1a
cmp-protocol-v1                       -> 449570d47649d5f6336a31ee3eed491256e0fb1a
```

До синхронизации `cmp-protocol-v1` был ahead of `main` на 583 commits и behind на 0, поэтому операция была чистым fast-forward.

## Meaning

- `main` теперь хранит стабильный pre-CRM snapshot.
- `stable-2026-08-25-pre-crm-redesign` — дополнительный immutable-style reference branch на ту же стабильную точку.
- вся дальнейшая разработка продолжается **только в `cmp-protocol-v1`**;
- `main` не использовать как source для последующих изменений;
- не двигать `main` снова до следующего явно согласованного stable checkpoint.

## Software/hardware note

Этот snapshot означает "последняя согласованная стабильная база перед CRM redesign", а не новое доказательство hardware acceptance. Hardware GREEN не выводится из branch synchronization.

Последний проверенный hardware-related operator-abort fix до snapshot имел GREEN software checks, но полный two-board acceptance всё ещё должен подтверждаться отдельно фактическими тестами.

## Next active phase

Следующая разработка начинается из:

```text
docs/PROJECT_HANDOFF/95_WEB_CRM_MOTOR_CLIENT_CASH_REDESIGN_2026-08-25.md
```

Порядок: schema/contracts -> motor pages/versioned windings -> client pages/profile -> repair/delivery -> weight-only wire migration -> cash/payments -> integration/backup/E2E.

## Working discipline

Перед каждым изменением existing file:

1. fetch exact current content из `cmp-protocol-v1`;
2. получить current blob SHA;
3. обновлять только `cmp-protocol-v1`;
4. после meaningful block обновлять PROJECT_HANDOFF;
5. не объявлять CI/build/hardware GREEN без фактического результата.
