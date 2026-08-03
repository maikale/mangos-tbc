#ifndef __PROGRESSIONMGR_H
#define __PROGRESSIONMGR_H

#include "Common.h"

class Player;

class ProgressionMgr
{
public:
    ProgressionMgr();

    void Load();

    void AnnouncePhase();

    void SendWelcomeMessage(Player* player);

    void SetPhase(uint32 phase);

    bool IsEnabled() const
    {
        return m_enabled;
    }

    uint32 GetCurrentPhase() const
    {
        return m_phase;
    }

    bool IsRaidUnlocked(uint32 mapId) const;

    bool IsVendorItemUnlocked(uint32 vendorEntry, uint32 itemEntry) const;

    bool IsVendorUnlocked(uint32 vendorEntry) const;

    uint32 GetRequiredPhase(uint32 mapId) const;

    const char* GetRaidName(uint32 mapId) const;

    const char* GetPhaseName() const;
private:
    bool m_enabled;
    uint32 m_phase;
};

extern ProgressionMgr* sProgressionMgr;

#endif