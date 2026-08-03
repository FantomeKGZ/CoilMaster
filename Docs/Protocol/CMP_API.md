\# CoilMaster Protocol (CMP)



\# Public API Specification



Version: 1.0 (Draft)



Release: 0.1.0



Build: 002A



\------------------------------------------------------------



\# Purpose



This document defines the public API of every CMP module.



Only public interfaces are described here.



Implementation details are not part of this specification.



\------------------------------------------------------------



\# CMP\_Buffer



Purpose



Stores incoming byte stream.



Responsibilities



• receive bytes



• store bytes



• provide random access



• remove bytes



• report available bytes



Public API



clear()



push()



pop()



peek(index)



peek(buffer,length)



read()



discard()



available()



size()



empty()



full()



\------------------------------------------------------------



\# CMP\_CRC



Purpose



CRC16 calculation.



Public API



begin()



update()



calculate()



\------------------------------------------------------------



\# CMP\_Parser



Purpose



Convert byte stream into validated packet.



Public API



parse()



Returns



CMP\_Result



Input



CMP\_Buffer



Output



CMP\_Packet



\------------------------------------------------------------



\# CMP\_Protocol



Purpose



Communication engine.



Responsibilities



Read UART



Write UART



Feed Parser



Return validated packets



Public API



begin()



update()



send()



available()



read()



\------------------------------------------------------------



\# CMP\_Dispatcher



Purpose



Application command dispatcher.



Responsibilities



Receive packet



Decode command



Execute handler



Create response



Public API



dispatch()



registerHandler()



\------------------------------------------------------------



\# CMP\_Packet



Purpose



Binary packet.



Contains



Header



Payload



CRC



\------------------------------------------------------------



\# CMP\_Header



Purpose



Packet header.



Contains



StartWord



Version



Flags



Reserved



Command



Counter



PayloadLength



\------------------------------------------------------------



\# Error Handling



Every public function returns



CMP\_Result



or



bool



No exceptions.



\------------------------------------------------------------



\# Memory Rules



Dynamic allocation is prohibited.



No malloc()



No new



No delete



All buffers are static.



\------------------------------------------------------------



\# Thread Safety



CMP is single-threaded.



Synchronization is performed by caller.



\------------------------------------------------------------



\# Transport Independence



CMP never accesses hardware directly.



Transport layer supplies bytes.



Supported transports



UART



USB



TCP



Bluetooth



CAN



RS485



\------------------------------------------------------------



\# Platform Compatibility



Arduino UNO



ESP32



AVR



Xtensa



Future ARM



\------------------------------------------------------------



\# Coding Rules



One class



One responsibility.



No global mutable state except protocol instance.



Public interface must remain backward compatible.



\------------------------------------------------------------



END OF DOCUMENT

