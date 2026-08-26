#ifndef CM_CASH_PAYMENT_INTEGRITY_AUDIT_H
#define CM_CASH_PAYMENT_INTEGRITY_AUDIT_H

#include <FS.h>
#include <stdint.h>

namespace CM
{
class CashPaymentIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, uint32_t& recordCount);
};
}

#endif // CM_CASH_PAYMENT_INTEGRITY_AUDIT_H
