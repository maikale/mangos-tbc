#include "ProgressionMgr.h"

#include "Config/Config.h"
#include "Entities/Player.h"
#include "Log/Log.h"
#include "Server/WorldSession.h"
#include "World/World.h"

ProgressionMgr* sProgressionMgr = new ProgressionMgr();

ProgressionMgr::ProgressionMgr()
{
    m_enabled = false;
    m_phase = 1;
}

void ProgressionMgr::Load()
{

    m_enabled = sConfig.GetBoolDefault(
        "Progression.Enabled",
        true);

    m_phase = sConfig.GetIntDefault(
        "Progression.Phase",
        1);

    if (m_enabled)
    {

        sLog.outString("---------------------------------");
        sLog.outString(" Progression System Enabled");
        sLog.outString(" Phase %u : %s",
            m_phase,
            GetPhaseName());

        sLog.outString("---------------------------------");
    }
}

const char* ProgressionMgr::GetPhaseName() const
{

    switch (m_phase)
    {

        case 1:
            return "Karazhan Era";

        case 2:
            return "Tier 5 Era";

        case 3:
            return "Tier 6 Era";

        case 4:
            return "Zul'Aman Era";

        case 5:
            return "Sunwell Era";
    }

    return "Unknown";
}

bool ProgressionMgr::IsRaidUnlocked(uint32 mapId) const
{
    return m_phase >= GetRequiredPhase(mapId);
}

uint32 ProgressionMgr::GetRequiredPhase(uint32 mapId) const
{
    switch (mapId)
    {
        // Phase 1
        case 532: // Karazhan
        case 565: // Gruul's Lair
        case 544: // Magtheridon's Lair
            return 1;

        // Phase 2
        case 548: // Serpentshrine Cavern
        case 550: // Tempest Keep
            return 2;

        // Phase 3
        case 534: // Mount Hyjal
        case 564: // Black Temple
            return 3;

        // Phase 4
        case 568: // Zul'Aman
            return 4;

        // Phase 5
        case 580: // Sunwell Plateau
            return 5;
    }

    return 1;
}

const char* ProgressionMgr::GetRaidName(uint32 mapId) const
{
    switch (mapId)
    {
        case 532:
            return "Karazhan";

        case 565:
            return "Gruul's Lair";

        case 544:
            return "Magtheridon's Lair";

        case 548:
            return "Serpentshrine Cavern";

        case 550:
            return "Tempest Keep";

        case 534:
            return "Mount Hyjal";

        case 564:
            return "Black Temple";

        case 568:
            return "Zul'Aman";

        case 580:
            return "Sunwell Plateau";
    }

    return "Unknown Raid";
}

void ProgressionMgr::SendWelcomeMessage(Player* player)
{
    if (!player)
        return;

    if (!m_enabled)
        return;

    player->GetSession()->SendNotification(
        "Current Progression Phase: %u - %s",
        m_phase,
        GetPhaseName());
}

void ProgressionMgr::SetPhase(uint32 phase)
{
    if (phase < 1)
        phase = 1;

    if (phase > 5)
        phase = 5;

    m_phase = phase;

    sLog.outString(
        "Progression phase changed to %u (%s)",
        m_phase,
        GetPhaseName());

    AnnouncePhase();
}

void ProgressionMgr::AnnouncePhase()
{
    if (!m_enabled)
        return;

    char msg[256];

    snprintf(
        msg,
        sizeof(msg),
        "The realm has entered Progression Phase %u - %s!",
        m_phase,
        GetPhaseName());

    sWorld.SendServerMessage(
        SERVER_MSG_CUSTOM,
        msg);
}

bool ProgressionMgr::IsVendorItemUnlocked(uint32 vendorEntry, uint32 itemEntry) const
{
    /*
    =====================================================
        PHASE 1 - Karazhan Era
        T4 Content
    =====================================================
    */

    /*
    =====================================================
        PHASE 2 - Tier 5 Era
        SSC / TK Content
    =====================================================
    */

    /*
    =====================================================
        PHASE 3 - Tier 6 Era
        Hyjal / Black Temple Content
    =====================================================
    */

    // Example:
    // Ankh (17030)
    // Available only after Phase 3
    if (itemEntry == 17030)
    {
        return m_phase >= 3;
    }

    /*
    =====================================================
        PHASE 4 - Zul'Aman Era
    =====================================================
    */

    /*
    =====================================================
        PHASE 5 - Sunwell Era
    =====================================================
    */

    // Unknown items stay available
    return true;
}

bool ProgressionMgr::IsVendorUnlocked(uint32 vendorEntry) const
{
    /*
    =====================================
        TEST VENDOR UNLOCK
    =====================================
    */

    // Example vendor
    // Entry: 20001

    if (vendorEntry == 1275)
    {
        // Unlock on Phase 3
        return m_phase >= 1;
    }

    // All other vendors stay enabled
    return true;
}