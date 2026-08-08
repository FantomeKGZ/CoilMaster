#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct JobSpoolSelection
{
    uint32_t jobId;
    uint32_t sessionId;
    uint32_t repairId;
    uint32_t motorId;
    uint32_t spoolId;
    uint16_t diameterHundredthsMm;
    uint32_t weightAtSelectionGrams;
    String wireType;

    JobSpoolSelection()
        : jobId(0UL), sessionId(0UL), repairId(0UL), motorId(0UL),
          spoolId(0UL), diameterHundredthsMm(0U),
          weightAtSelectionGrams(0UL) {}

    bool isValid() const
    {
        return jobId != 0UL && sessionId != 0UL &&
               repairId != 0UL && motorId != 0UL && spoolId != 0UL &&
               diameterHundredthsMm != 0U && weightAtSelectionGrams != 0UL &&
               (wireType == "CU" || wireType == "AL");
    }
};

class JobSpoolSelectionStore
{
public:
    explicit JobSpoolSelectionStore(fs::FS& storage);

    bool begin();
    bool isReady() const;
    bool create(const JobSpoolSelection& selection);
    bool load(uint32_t sessionId, JobSpoolSelection& selection) const;
    bool load(uint32_t sessionId, JobSpoolSelection& selection, bool& found) const;
    static bool loadReadOnly(fs::FS& storage,
                             uint32_t sessionId,
                             JobSpoolSelection& selection,
                             bool& found);
    bool validateIdentity(uint32_t jobId, uint32_t sessionId) const;

private:
    static constexpr const char* RootDirectory = "/data/winding-jobs";
    static constexpr const char* SelectionDirectory = "/data/winding-jobs/spool-selection";

    bool ensureDirectories();
    String selectionPath(uint32_t sessionId) const;
    String temporaryPath(uint32_t sessionId) const;
    static bool serialize(const JobSpoolSelection& selection, String& output);
    static bool parse(const String& input, JobSpoolSelection& selection);
    static bool findUnsigned(const String& input, const char* key, uint32_t& value);
    static bool findString(const String& input, const char* key, String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}
