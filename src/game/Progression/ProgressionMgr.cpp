#include "ProgressionMgr.h"

#include "Config/Config.h"
#include "Database/DatabaseEnv.h"
#include "Entities/Player.h"
#include "Log/Log.h"
#include "Server/WorldSession.h"
#include "World/World.h"
#include "Entities/Creature.h"
#include "Globals/ObjectMgr.h"

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

    LoadUnlocks();

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

void ProgressionMgr::LoadUnlocks()
{
    m_unlocks.clear();

    auto queryResult = WorldDatabase.PQuery(
        "SELECT type, entry, phase FROM progression_unlocks");

    if (!queryResult)
    {
        sLog.outString("Progression: no unlock data found");
        return;
    }

    do
    {
        Field* fields = queryResult->Fetch();

        std::string type = fields[0].GetString();

        uint32 entry = fields[1].GetUInt32();

        uint32 phase = fields[2].GetUInt32();

        m_unlocks[type][entry] = phase;

        sLog.outString(
            "Progression loaded: %s Entry %u Phase %u",
            type.c_str(),
            entry,
            phase);

    } while (queryResult->NextRow());
}

bool ProgressionMgr::IsUnlocked(std::string type, uint32 entry) const
{
    auto typeItr = m_unlocks.find(type);

    if (typeItr == m_unlocks.end())
        return true;

    auto entryItr = typeItr->second.find(entry);

    if (entryItr == typeItr->second.end())
        return true;

    return m_phase >= entryItr->second;
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
        case 532:
        case 565:
        case 544:
            return 1;

        case 548:
        case 550:
            return 2;

        case 534:
        case 564:
            return 3;

        case 568:
            return 4;

        case 580:
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
    return IsUnlocked("ITEM", itemEntry);
}

bool ProgressionMgr::IsVendorUnlocked(uint32 vendorEntry) const
{
    return IsUnlocked("VENDOR", vendorEntry);
}

bool ProgressionMgr::IsGameObjectUnlocked(uint32 entry) const
{
    return IsUnlocked("GAMEOBJECT", entry);
}

bool ProgressionMgr::IsQuestUnlocked(uint32 questId) const
{
    return IsUnlocked("QUEST", questId);
}

bool ProgressionMgr::IsCreatureUnlocked(uint32 creatureEntry) const
{
    return IsUnlocked("CREATURE", creatureEntry);
}

bool ProgressionMgr::HasUnlockedQuestGiver(Creature* creature) const
{
    if (!creature)
        return false;

    QuestRelationsMapBounds bounds =
        sObjectMgr.GetCreatureQuestRelationsMapBounds(creature->GetEntry());

    for (QuestRelationsMap::const_iterator itr = bounds.first;
        itr != bounds.second;
        ++itr)
    {
        if (IsQuestUnlocked(itr->second))
            return true;
    }

    return false;
}
