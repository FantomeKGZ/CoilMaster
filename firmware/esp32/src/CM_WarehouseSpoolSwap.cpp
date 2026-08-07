#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::recoverSpoolFileSwap()
{
    const bool hasMain = m_storage.exists(SpoolsPath);
    const bool hasTemp = m_storage.exists(SpoolsTempPath);
    const bool hasBackup = m_storage.exists(SpoolsBackupPath);

    if (hasBackup)
    {
        if (!hasMain)
        {
            if (!m_storage.rename(SpoolsBackupPath, SpoolsPath)) return false;
            if (hasTemp && !m_storage.remove(SpoolsTempPath)) return false;
            return true;
        }

        if (hasTemp)
        {
            // main + backup + temp cannot be mapped to one unambiguous phase.
            return false;
        }

        // The new main file is already committed; only cleanup was interrupted.
        return m_storage.remove(SpoolsBackupPath);
    }

    if (hasTemp)
    {
        if (!hasMain) return false;
        // Temp was prepared but the replacement never started.
        return m_storage.remove(SpoolsTempPath);
    }

    return true;
}

bool WarehouseStore::replaceSpoolsFileFromTemp()
{
    if (!m_storage.exists(SpoolsPath) ||
        !m_storage.exists(SpoolsTempPath) ||
        m_storage.exists(SpoolsBackupPath))
    {
        return false;
    }

    if (!m_storage.rename(SpoolsPath, SpoolsBackupPath)) return false;

    if (!m_storage.rename(SpoolsTempPath, SpoolsPath))
    {
        if (!m_storage.rename(SpoolsBackupPath, SpoolsPath))
            m_ready = false;
        return false;
    }

    // New main is durable. A stale backup is safe and will be removed at boot.
    if (m_storage.exists(SpoolsBackupPath))
        m_storage.remove(SpoolsBackupPath);

    return true;
}
}
