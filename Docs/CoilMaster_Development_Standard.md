\# CoilMaster OS



Development Standard



Version: 1.0



Release: 0.1.0



Build: 002A



Status:

Project Standard



============================================================



\# 1. Purpose



This document defines mandatory development rules for the

entire CoilMaster project.



Every source file shall comply with this standard.



============================================================



\# 2. Project Philosophy



Specification First



↓



Architecture



↓



Implementation



↓



Review



↓



Testing



↓



Release



Code shall never define architecture.



Architecture defines code.



============================================================



\# 3. General Principles



• Keep It Simple (KISS)



• Single Responsibility Principle (SRP)



• DRY (Don't Repeat Yourself)



• Explicit is better than implicit



• Readability before optimization



• Safety before performance



============================================================



\# 4. Directory Structure



CoilMaster/



Docs/



Firmware/



Shared/



Hardware/



Tools/



Web/



Tests/



Every directory has a single responsibility.



============================================================



\# 5. Naming Convention



Files



CMP\_Protocol.cpp



CM\_Settings.h



HAL\_LCD.cpp



Classes



PascalCase



CM\_Settings



CMP\_Protocol



Variables



camelCase



packetCounter



payloadLength



Functions



camelCase



sendPacket()



validateCRC()



Constants



UPPER\_CASE



or



inline constexpr



Namespaces



PascalCase



============================================================



\# 6. Header File Layout



License / Banner



↓



Include Guard



↓



Includes



↓



Constants



↓



Types



↓



Class Declaration



↓



End



============================================================



\# 7. Source File Layout



Banner



↓



Includes



↓



Static Variables



↓



Public Methods



↓



Private Methods



↓



End



============================================================



\# 8. Comments



Every public class



must contain



Purpose



Responsibilities



Usage



Complex algorithms



must be documented.



Obvious code



must not be over-commented.



============================================================



\# 9. Memory Rules



Dynamic allocation is prohibited.



Forbidden:



malloc()



calloc()



realloc()



new



delete



Memory shall be deterministic.



============================================================



\# 10. Error Handling



No exceptions.



Return values only.



CMP\_Result



bool



enum class



============================================================



\# 11. Coding Rules



Avoid macros whenever possible.



Prefer



inline constexpr



Prefer



enum class



Avoid



reinterpret\_cast



unless absolutely required.



Avoid hidden side effects.



============================================================



\# 12. Hardware Access



Application layer



shall never



access hardware directly.



Application



↓



Core



↓



HAL



↓



Driver



↓



Hardware



============================================================



\# 13. Communication



UART



only through



CMP\_Protocol



No module may access UART directly.



============================================================



\# 14. Logging



Logging shall be performed



only through



CM\_Logger



No direct Serial.print()



outside CM\_Logger.



============================================================



\# 15. Configuration



Hardware configuration



CM\_Config



User configuration



CM\_Settings



Never mix them.



============================================================



\# 16. Versioning



Release



Major.Minor.Patch



Build



Numeric



Package



Engineering package



Protocol



Independent version



============================================================



\# 17. Documentation



Every module shall contain



Specification



API



Implementation



Future Notes



============================================================



\# 18. Git Rules



One logical change



=



One commit



Commit message format



Module:



Description



Example



Protocol:

Add CRC validation



============================================================



\# 19. Testing



Every module



must be



Compile Tested



Integration Tested



Regression Tested



before release.



============================================================



\# 20. Review Checklist



Architecture



Compilation



Memory



Performance



Naming



Documentation



Testing



============================================================



\# 21. Release Process



Specification



↓



Implementation



↓



Code Review



↓



Testing



↓



Tag Release



============================================================



\# 22. Project Rule



If implementation



conflicts



with specification



implementation shall be changed.



Specification is authoritative.



============================================================



END OF DOCUMENT

