#include "CM_Cmp1Crc.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(EXIT_FAILURE);
}
}

int main()
{
    const char* checkVector = "123456789";
    require(CM::Cmp1Crc::calculate(checkVector, std::strlen(checkVector)) ==
                0x4B37U,
            "CRC-16/MODBUS check vector");

    require(CM::Cmp1Crc::calculate(static_cast<const uint8_t*>(nullptr), 0U) ==
                CM::Cmp1Crc::InitialValue,
            "empty payload keeps the initial value");

    const char* eventPayload = "CMP1|EVT|RUN_COMPLETED|1|1|1";
    require(CM::Cmp1Crc::calculate(eventPayload, std::strlen(eventPayload)) ==
                0x78FEU,
            "CMP1 event frame CRC");

    const char* first = "CMP1|JOB_";
    const char* second = "CANCEL|42";
    uint16_t incremental = CM::Cmp1Crc::update(
        CM::Cmp1Crc::InitialValue,
        reinterpret_cast<const uint8_t*>(first),
        std::strlen(first));
    incremental = CM::Cmp1Crc::update(
        incremental,
        reinterpret_cast<const uint8_t*>(second),
        std::strlen(second));
    require(incremental == 0xE4D2U, "incremental CMP1 CRC");

    const char* acknowledgement = "CMP1|ACK|1|SAVED";
    require(CM::Cmp1Crc::calculate(acknowledgement,
                                   std::strlen(acknowledgement)) == 0x528FU,
            "CMP1 ACK frame CRC");

    const char* negativeAcknowledgement = "CMP1|NACK|1|WRITE_FAILED";
    require(CM::Cmp1Crc::calculate(negativeAcknowledgement,
                                   std::strlen(negativeAcknowledgement)) ==
                0x08DAU,
            "CMP1 NACK frame CRC");

    const char* jobWithReplyCrcCapability =
        "CMP1|JOB|1|1|WORKING|1|120|C";
    require(CM::Cmp1Crc::calculate(jobWithReplyCrcCapability,
                                   std::strlen(jobWithReplyCrcCapability)) ==
                0x980AU,
            "CMP1 job reply-CRC capability frame");

    const char* jobAcknowledgement =
        "CMP1|JOB_ACK|1|ACCEPTED|READY|C";
    require(CM::Cmp1Crc::calculate(jobAcknowledgement,
                                   std::strlen(jobAcknowledgement)) == 0xA112U,
            "CMP1 JOB_ACK frame CRC");

    const char* cancelAcknowledgement =
        "CMP1|JOB_CANCEL_ACK|1|CANCELLED|CANCELLED|C";
    require(CM::Cmp1Crc::calculate(cancelAcknowledgement,
                                   std::strlen(cancelAcknowledgement)) ==
                0xADB9U,
            "CMP1 JOB_CANCEL_ACK frame CRC");

    std::cout << "CMP1 text CRC tests passed\n";
    return EXIT_SUCCESS;
}
