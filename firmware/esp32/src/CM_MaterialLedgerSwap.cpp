#include "CM_MaterialLedger.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool MaterialLedger::validateMaterialsFile(const char* path) const
{
    if (path == nullptr || !m_storage.exists(path)) return false;
    File file = m_storage.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }

        uint32_t materialId = 0UL;
        uint32_t stock = 0UL;
        uint32_t price = 0UL;
        String name, unit, currency, status;
        if (!findUnsigned(line, "material_id", materialId) || materialId == 0UL ||
            materialId <= previousId ||
            !findString(line, "name", name) || name.length() == 0U ||
            !findString(line, "unit", unit) || unit.length() == 0U ||
            !findUnsigned(line, "stock_quantity_milli", stock) ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "status", status) || status.length() == 0U)
        {
            file.close();
            return false;
        }
        previousId = materialId;

        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                return false;
            }
        }

        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        const bool hasDiameter = line.indexOf(F("\"diameter_hundredths_mm\":")) >= 0;
        if (hasWireType != hasDiameter)
        {
            file.close();
            return false;
        }
        if (hasWireType)
        {
            String wireType;
            uint32_t diameter = 0UL;
            if (unit != "GRAM" ||
                !findString(line, "wire_type", wireType) ||
                (wireType != "CU" && wireType != "AL") ||
                !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
                diameter == 0UL || diameter > 65535UL)
            {
                file.close();
                return false;
            }
        }
    }

    file.close();
    return true;
}

bool MaterialLedger::recoverMaterialFileSwap()
{
    const bool hasMain = m_storage.exists(MaterialsPath);
    const bool hasTemp = m_storage.exists(MaterialsTempPath);
    const bool hasBackup = m_storage.exists(MaterialsBackupPath);

    if (hasBackup)
    {
        if (!validateMaterialsFile(MaterialsBackupPath)) return false;

        if (!hasMain)
        {
            if (!m_storage.rename(MaterialsBackupPath, MaterialsPath)) return false;
            if (!validateMaterialsFile(MaterialsPath)) return false;
            if (hasTemp && !m_storage.remove(MaterialsTempPath)) return false;
            return true;
        }

        if (hasTemp)
        {
            // main + backup + temp cannot be mapped to one unambiguous swap phase.
            return false;
        }

        if (validateMaterialsFile(MaterialsPath))
            return m_storage.remove(MaterialsBackupPath);

        // Rename completion alone is not commit proof. Restore the last validated
        // material ledger when the promoted main is malformed or unreadable.
        if (!m_storage.remove(MaterialsPath) ||
            !m_storage.rename(MaterialsBackupPath, MaterialsPath) ||
            !validateMaterialsFile(MaterialsPath))
        {
            return false;
        }
        return true;
    }

    if (hasTemp)
    {
        if (!hasMain || !validateMaterialsFile(MaterialsPath)) return false;
        // Prepared temp never became authoritative; keep committed main.
        return m_storage.remove(MaterialsTempPath);
    }

    return !hasMain || validateMaterialsFile(MaterialsPath);
}

bool MaterialLedger::replaceMaterialsFileFromTemp()
{
    if (!m_storage.exists(MaterialsPath) ||
        !m_storage.exists(MaterialsTempPath) ||
        m_storage.exists(MaterialsBackupPath) ||
        !validateMaterialsFile(MaterialsPath) ||
        !validateMaterialsFile(MaterialsTempPath))
    {
        return false;
    }

    if (!m_storage.rename(MaterialsPath, MaterialsBackupPath)) return false;

    if (!m_storage.rename(MaterialsTempPath, MaterialsPath))
    {
        if (!m_storage.rename(MaterialsBackupPath, MaterialsPath))
        {
            m_ready = false;
        }
        return false;
    }

    if (!validateMaterialsFile(MaterialsPath))
    {
        bool restored = false;
        if (m_storage.remove(MaterialsPath) &&
            m_storage.rename(MaterialsBackupPath, MaterialsPath))
        {
            restored = validateMaterialsFile(MaterialsPath);
        }
        if (!restored) m_ready = false;
        return false;
    }

    if (m_storage.exists(MaterialsBackupPath) &&
        !m_storage.remove(MaterialsBackupPath))
    {
        m_ready = false;
        return false;
    }

    return true;
}
}
