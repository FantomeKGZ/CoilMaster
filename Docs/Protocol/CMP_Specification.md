\# CoilMaster Protocol (CMP)



Version: 1.0 (Draft)



Status: Engineering Specification



Release: 0.1.0



Build: 002A



\------------------------------------------------------------



\# 1. Purpose



CMP (CoilMaster Protocol) is a binary communication protocol

used for communication between devices inside the CoilMaster

ecosystem.



Primary communication:



Arduino UNO  ⇄  ESP32



Future support:



ESP32 ⇄ PC



ESP32 ⇄ Web



ESP32 ⇄ Mobile



ESP32 ⇄ Other Controllers



\------------------------------------------------------------



\# 2. Design Goals



The protocol shall provide:



• deterministic packet format



• binary transport



• high reliability



• packet integrity verification



• low overhead



• protocol version compatibility



• future extensibility



\------------------------------------------------------------



\# 3. OSI Position



Application



↓



Dispatcher



↓



Protocol



↓



Parser



↓



Buffer



↓



Transport (UART)



\------------------------------------------------------------



CMP starts at the Parser layer.



UART is outside of CMP.



\------------------------------------------------------------



\# 4. Transport



Current transport:



UART



115200 baud



8N1



Future transports:



USB CDC



TCP



Bluetooth



CAN



RS485



without changing protocol implementation.



\------------------------------------------------------------



\# 5. Packet Structure



+------------+



Header



+------------+



Payload



+------------+



CRC16



+------------+



\------------------------------------------------------------



Header size:



12 bytes



Payload:



0...128 bytes



CRC:



2 bytes



\------------------------------------------------------------



Maximum packet:



142 bytes



\------------------------------------------------------------



\# 6. Header



Field



Size



StartWord          2



VersionMajor       1



VersionMinor       1



Flags              1



Reserved           1



Command            2



Counter            2



PayloadLength      2



\------------------------------------------------------------



Total:



12 bytes



\------------------------------------------------------------



\# 7. Packet Rules



Every packet begins with:



0xAA55



Little Endian.



Payload length is variable.



CRC is always present.



\------------------------------------------------------------



\# 8. Packet States



WAIT\_START



↓



READ\_HEADER



↓



READ\_PAYLOAD



↓



READ\_CRC



↓



VERIFY



↓



READY



\------------------------------------------------------------



\# 9. CRC



Algorithm:



CRC16 CCITT



Polynomial:



0x1021



Initial value:



0xFFFF



\------------------------------------------------------------



CRC is calculated over:



Header



\+



Payload



CRC field itself is excluded.



\------------------------------------------------------------



\# 10. Commands



Commands are grouped.



0x0000 System



0x0100 Configuration



0x0200 Winding



0x0300 Hall



0x0400 Display



0x0500 Logger



0x0600 Storage



0x0700 Network



0x0800 Diagnostics



\------------------------------------------------------------



\# 11. Flags



ACK Required



ACK



NACK



Response



Broadcast



Compressed



Encrypted



Reserved



\------------------------------------------------------------



\# 12. Parser Rules



The parser NEVER reads directly from UART.



The parser works only with CMP\_Buffer.



The parser SHALL NOT remove bytes from the buffer until the

entire packet has been validated.



If validation fails, synchronization is restored by searching

for the next StartWord.



\------------------------------------------------------------



\# 13. Buffer Rules



The buffer stores raw incoming bytes.



The parser reads from the buffer.



Only validated packets are removed from the buffer.



\------------------------------------------------------------



\# 14. Protocol Rules



Protocol is responsible only for:



Sending packets



Receiving packets



Calling parser



Returning validated packets



Protocol does not process commands.



\------------------------------------------------------------



\# 15. Dispatcher Rules



Dispatcher receives validated packets.



Dispatcher executes commands.



Dispatcher returns responses.



\------------------------------------------------------------



\# 16. Compatibility



Major version mismatch:



Packet rejected.



Minor version newer:



Packet rejected.



Older minor version:



Supported if compatible.



\------------------------------------------------------------



\# 17. Future Extensions



Timestamp



Device ID



Encryption



Compression



Authentication



Routing



Multiple devices



\------------------------------------------------------------



END OF SPECIFICATION

