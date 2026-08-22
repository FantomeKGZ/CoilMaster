#include "CM_WarehouseStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool WarehouseStore::validateSpoolsFile(const char* path) const
{
    if (path == nullptr || !m_storage.exists(path)) return false;
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousSpoolId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;
        String wireType;
        String optional;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            spoolId <= previousSpoolId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status) || status.length() == 0U ||
            (hasWireType &&
             (!findString(line, "wire_type", wireType) ||
              (wireType != "CU" && wireType != "AL"))))
        {
            file.close();
            return false;
        }
        previousSpoolId = spoolId;

        const char* optionalKeys[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        for (uint8_t keyIndex = 0U;
             keyIndex < sizeof(optionalKeys) / sizeof(optionalKeys[0]);
             ++keyIndex)
        {
            const String marker = String("\"") + optionalKeys[keyIndex] + F("\":");
            if (line.indexOf(marker) >= 0 &&
                !findString(line, optionalKeys[keyIndex], optional))
            {
                file.close();
                return false;
            }
        }
    }

    file.close();
    return true;
}

bool WarehouseStore::recoverSpoolFileSwap()
{
    const bool hasMain = m_storage.exists(SpoolsPath);
    const bool hasTemp = m_storage.exists(SpoolsTempPath);
    const bool hasBackup = m_storage.exists(SpoolsBackupPath);

    if (hasBackup)
    {
        // Backup is the last known committed inventory. Never discard it unless
        // its own contents are valid and any replacement main is independently
        // proven valid after the rename boundary.
        if (!validateSpoolsFile(SpoolsBackupPath)) return false;

        if (!hasMain)
        {
            if (!m_storage.rename(SpoolsBackupPath, SpoolsPath)) return false;
            if (!validateSpoolsFile(SpoolsPath)) return false;
            if (hasTemp && !m_storage.remove(SpoolsTempPath)) return false;
            return true;
        }

        if (hasTemp)
        {
            // main + backup + temp cannot be mapped to one unambiguous phase.
            return false;
        }

        if (validateSpoolsFile(SpoolsPath))
            return m_storage.remove(SpoolsBackupPath);

        // A rename is not proof of a valid committed file. If the replacement
        // main is damaged, restore the last validated committed backup instead
        // of deleting it and accepting corrupted spool weights.
        if (!m_storage.remove(SpoolsPath) ||
            !m_storage.rename(SpoolsBackupPath, SpoolsPath) ||
            !validateSpoolsFile(SpoolsPath))
        {
            return false;
        }
        return true;
    }

    if (hasTemp)
    {
        if (!hasMain || !validateSpoolsFile(SpoolsPath)) return false;
        // Temp was prepared but the replacement never started. The committed
        // main wins; do not promote an uncommitted inventory after reboot.
        return m_storage.remove(SpoolsTempPath);
    }

    // begin() must not report warehouse readiness while the authoritative spool
    // file is already corrupt even when no transaction residue is present.
    return !hasMain || validateSpoolsFile(SpoolsPath);
}

bool WarehouseStore::replaceSpoolsFileFromTemp()
{
    if (!m_storage.exists(SpoolsPath) ||
        !m_storage.exists(SpoolsTempPath) ||
        m_storage.exists(SpoolsBackupPath) ||
        !validateSpoolsFile(SpoolsPath) ||
        !validateSpoolsFile(SpoolsTempPath))
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

    // Verify the committed pathname, not just the prepared temp. A media or
    // rename anomaly must not cause the last valid inventory backup to be lost.
    if (!validateSpoolsFile(SpoolsPath))
    {
        bool restored = false;
        if (m_storage.remove(SpoolsPath) &&
            m_storage.rename(SpoolsBackupPath, SpoolsPath))
        {
            restored = validateSpoolsFile(SpoolsPath);
        }
        if (!restored) m_ready = false;
        return false;
    }

    if (m_storage.exists(SpoolsBackupPath) &&
        !m_storage.remove(SpoolsBackupPath))
    {
        m_ready = false;
        return false;
    }

    return true;
}
}
