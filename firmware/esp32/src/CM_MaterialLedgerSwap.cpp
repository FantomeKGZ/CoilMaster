#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::recoverMaterialFileSwap()
{
    const bool hasMain = m_storage.exists(MaterialsPath);
    const bool hasTemp = m_storage.exists(MaterialsTempPath);
    const bool hasBackup = m_storage.exists(MaterialsBackupPath);

    if (hasBackup)
    {
        if (!hasMain)
        {
            if (!m_storage.rename(MaterialsBackupPath, MaterialsPath)) return false;
            if (hasTemp && !m_storage.remove(MaterialsTempPath)) return false;
            return true;
        }

        if (hasTemp)
        {
            // main + backup + temp cannot be mapped to one unambiguous swap phase.
            return false;
        }

        // main + backup means the replacement reached the committed state and
        // only backup cleanup was interrupted.
        return m_storage.remove(MaterialsBackupPath);
    }

    if (hasTemp)
    {
        if (!hasMain)
        {
            // Without main or backup the temp file has no trustworthy predecessor.
            return false;
        }
        // The new file was prepared but the swap never started.
        return m_storage.remove(MaterialsTempPath);
    }

    return true;
}

bool MaterialLedger::replaceMaterialsFileFromTemp()
{
    if (!m_storage.exists(MaterialsPath) ||
        !m_storage.exists(MaterialsTempPath) ||
        m_storage.exists(MaterialsBackupPath))
    {
        return false;
    }

    if (!m_storage.rename(MaterialsPath, MaterialsBackupPath)) return false;

    if (!m_storage.rename(MaterialsTempPath, MaterialsPath))
    {
        if (!m_storage.rename(MaterialsBackupPath, MaterialsPath))
        {
            // The on-disk state is now ambiguous. Leave pending transaction
            // markers intact and require startup recovery/manual inspection.
            m_ready = false;
        }
        return false;
    }

    // The new main file is already committed. Failure to remove the backup is
    // recoverable on the next boot, so do not report the data mutation as failed.
    if (m_storage.exists(MaterialsBackupPath))
        m_storage.remove(MaterialsBackupPath);

    return true;
}
}
