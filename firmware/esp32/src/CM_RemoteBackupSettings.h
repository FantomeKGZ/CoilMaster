#ifndef CM_REMOTE_BACKUP_SETTINGS_H
#define CM_REMOTE_BACKUP_SETTINGS_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct RemoteBackupSettings
{
    bool enabled = false;
    String host;
    uint16_t port = 21U;
    String username;
    String password;
    String remoteDirectory = "/CoilMaster";
    uint8_t retentionCount = 7U;
};

class RemoteBackupSettingsStore
{
public:
    explicit RemoteBackupSettingsStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool load(RemoteBackupSettings& settings, bool& configured) const;
    bool save(const RemoteBackupSettings& settings);

    static bool valid(const RemoteBackupSettings& settings);

private:
    static constexpr const char* Directory = "/data/settings";
    static constexpr const char* SettingsPath =
        "/data/settings/remote-backup.json";
    static constexpr const char* TempPath =
        "/data/settings/remote-backup.tmp";
    static constexpr const char* BackupPath =
        "/data/settings/remote-backup.bak";

    bool recoverFileSwap();
    bool loadFromPath(const char* path, RemoteBackupSettings& settings) const;

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_REMOTE_BACKUP_SETTINGS_H
