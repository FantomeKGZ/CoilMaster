#include "CM_UartEventReceiver.h"

namespace CM
{
bool UartEventReceiver::cancelPendingJob(const char* detail)
{
    if (!m_hasPendingJob || m_hasJobDelivery)
    {
        return false;
    }

    const char* cancellationDetail =
        (detail != nullptr && detail[0] != '\0') ? detail : "CANCELLED";

    publishJobDelivery(JobDeliveryResult::Cancelled,
                       m_pendingJob.jobId,
                       m_jobSendAttempts,
                       cancellationDetail);

    m_hasPendingJob = false;
    m_waitingJobAck = false;
    m_jobSendAttempts = 0U;
    return true;
}
}
