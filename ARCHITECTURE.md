\# CoilMaster OS



\# ARCHITECTURE



Документ: DOC-001



Версия: 1.0



Release: 0.1.0



Build: 002A



\---



\# Назначение



Настоящий документ определяет архитектуру программной платформы CoilMaster OS.



Все программные модули проекта должны соответствовать данному документу.



\---



\# Общая архитектура



Система состоит из двух независимых контроллеров.



```

&#x20;               CoilMaster OS

&#x20;                     │

&#x20;     ┌───────────────┴───────────────┐

&#x20;     │                               │

&#x20;Arduino UNO                     ESP32 DevKit

Realtime Controller           Service Controller

```



\---



\# Arduino UNO



Назначение:



Контроллер реального времени.



Отвечает исключительно за управление оборудованием.



Функции:



\- Датчик Холла

\- LCD1602

\- Клавиатура

\- SSR

\- Звуковой сигнал

\- Управление намоткой

\- Выполнение команд

\- Обмен данными по CMP



Arduino не выполняет:



\- Wi-Fi

\- FTP

\- Web Server

\- Database

\- Backup



\---



\# ESP32



Назначение:



Сервисный контроллер.



Функции:



\- Wi-Fi

\- HTTP Server

\- FTP Server

\- OTA

\- RTC

\- microSD

\- База данных

\- Web Interface

\- Handbook

\- Backup

\- Logging



ESP32 не управляет оборудованием намотки.



\---



\# Протокол связи



Взаимодействие осуществляется только через CMP (CoilMaster Protocol).



Любой обмен между платами проходит через UART.



Запрещается прямое обращение одной платы к аппаратным модулям другой.



\---



\# Структура проекта



```

CoilMaster/



Firmware/

&#x20;   UNO/

&#x20;   ESP32/



Shared/



Documentation/



Web/



SD\_Image/



Tests/



Tools/



Releases/

```



\---



\# Shared



Папка Shared содержит общий код.



Используется одновременно Arduino и ESP32.



Содержит:



\- Общие типы данных

\- Общие структуры

\- Общие константы

\- Протокол CMP

\- Версии



\---



\# Firmware UNO



Содержит:



Core



HAL



App



Tests



\---



\# Firmware ESP32



Содержит:



Core



Network



Storage



Services



App



Tests



\---



\# HAL



Все аппаратные устройства работают только через Hardware Abstraction Layer.



Например:



HAL\_LCD



HAL\_Hall



HAL\_Keypad



HAL\_SSR



HAL\_Buzzer



Запрещается использование digitalWrite(), analogRead() и других функций Arduino напрямую из прикладного кода.



\---



\# Core



Ядро системы.



Модули:



CM\_System



CM\_Version



CM\_Config



CM\_Settings



CM\_Logger



CM\_Event



\---



\# Storage



Работает только на ESP32.



Модули:



STG\_SD



STG\_DB



STG\_Backup



\---



\# Network



Работает только на ESP32.



Модули:



NET\_WiFi



NET\_HTTP



NET\_FTP



NET\_OTA



\---



\# Application



Прикладная логика.



APP\_Winding



APP\_Handbook



APP\_MotorDB



\---



\# Документация



Каждый программный модуль обязан иметь:



описание



версию



дату изменения



автора



историю изменений



\---



\# Правило разработки



Любой новый модуль должен:



✔ соответствовать архитектуре;



✔ иметь документацию;



✔ успешно компилироваться;



✔ пройти тестирование;



✔ быть внесён в CHANGELOG.



\---



Конец документа.

