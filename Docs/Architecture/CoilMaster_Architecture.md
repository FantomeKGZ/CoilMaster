\# CoilMaster OS



System Architecture



Version: 1.0 (Draft)



Release: 0.1.0



Build: 002A



Status:

Engineering Architecture



\------------------------------------------------------------



\# 1. Project Purpose



CoilMaster OS is a distributed embedded system

designed for professional electric motor winding.



The system consists of two independent processors

working together.



Arduino UNO



↓



Real-time controller



ESP32



↓



System controller



\------------------------------------------------------------



\# 2. System Components



Arduino UNO



Hall Sensor



LCD1602



Keypad



SSR



Buzzer



Real-time winding control



ESP32



WiFi



Web Server



FTP Server



REST API



Motor Database



Logging



Backup



OTA



Communication with Arduino



\------------------------------------------------------------



\# 3. Architecture



&#x20;                   User



&#x20;                    │



&#x20;       Browser / Mobile App



&#x20;                    │



&#x20;             HTTP / REST



&#x20;                    │



&#x20;                ESP32 Core



&#x20;                    │



&#x20;        CoilMaster Protocol



&#x20;                    │



&#x20;              Arduino UNO



&#x20;                    │



&#x20;             Hardware Layer



\------------------------------------------------------------



\# 4. Responsibilities



Arduino UNO



Responsible for



• Hall sensor



• Turn counter



• LCD



• Keypad



• Buzzer



• Relay



• Real-time execution



Arduino never stores user database.



Arduino never serves web pages.



\------------------------------------------------------------



ESP32



Responsible for



• WiFi



• Web UI



• SD Card



• FTP



• Database



• Backup



• Restore



• OTA



• REST API



ESP32 never performs

real-time winding control.



\------------------------------------------------------------



\# 5. Communication



Only CMP is used.



No text commands.



No JSON over UART.



No ASCII protocol.



Binary protocol only.



\------------------------------------------------------------



\# 6. Layer Model



Application



↓



Dispatcher



↓



CMP



↓



UART



↓



Arduino



\------------------------------------------------------------



ESP32



↓



CMP



↓



UART



↓



Arduino



\------------------------------------------------------------



\# 7. Software Modules



Shared



Protocol



Core



Firmware



HAL



Drivers



Storage



Network



Web



Docs



\------------------------------------------------------------



\# 8. Hardware Layers



HAL



↓



Drivers



↓



Hardware



No application code may access hardware directly.



\------------------------------------------------------------



\# 9. Storage



Motor database



↓



SD Card



Logs



↓



SD Card



Settings



↓



ESP32 Flash (NVS)



Backups



↓



ZIP Archive



\------------------------------------------------------------



\# 10. Backup



Backup contains



Motor Database



Settings



Logs



Web Files



Protocol Configuration



System Configuration



Backup is portable.



\------------------------------------------------------------



\# 11. Web



ESP32 hosts



HTML



CSS



JavaScript



REST API



WebSocket



\------------------------------------------------------------



\# 12. Logging



Arduino



↓



CMP



↓



ESP32 Logger



↓



SD Card



↓



FTP



\------------------------------------------------------------



\# 13. Security



Protocol Version



CRC16



Authentication (future)



Encryption (future)



\------------------------------------------------------------



\# 14. Coding Rules



Single Responsibility



No dynamic allocation



No blocking communication



No global mutable data

except controlled singleton modules



Every module has



Header



Implementation



Documentation



\------------------------------------------------------------



\# 15. Directory Structure



CoilMaster



Docs



Firmware



Shared



Tools



Hardware



Web



\------------------------------------------------------------



\# 16. Release Policy



Specification



↓



Implementation



↓



Review



↓



Testing



↓



Release



No implementation without approved specification.



\------------------------------------------------------------



END OF DOCUMENT

