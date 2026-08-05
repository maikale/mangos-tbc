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

    LoadAttunements();

    LoadArenaSeasons();

    LoadArenaItems();

    if (m_enabled)
    {
        sLog.outString("---------------------------------");
        sLog.outString(" Progression System Enabled");

        sLog.outString(" Phase %u : %s",
            m_phase,
            GetPhaseName());

        sLog.outString(
            " Arena Season: %u",
            GetCurrentArenaSeason());

        sLog.outString("---------------------------------");
    }
}

void ProgressionMgr::LoadAttunements()
{
    m_attunements.clear();

    auto query = WorldDatabase.PQuery(
        "SELECT map_id, type, entry, name FROM progression_attunements");

    if (!query)
    {
        sLog.outString("Progression: no attunement data found");
        return;
    }

    do
    {
        Field* fields = query->Fetch();

        AttunementInfo info;

        info.mapId = fields[0].GetUInt32();
        info.type = fields[1].GetString();
        info.entry = fields[2].GetUInt32();
        info.name = fields[3].GetString();

        m_attunements.push_back(info);

        sLog.outString(
            "Progression attunement loaded: Map %u Type %s Entry %u %s",
            info.mapId,
            info.type.c_str(),
            info.entry,
            info.name.c_str());

    } while (query->NextRow());
}

void ProgressionMgr::LoadUnlocks()
{
    m_unlocks.clear();
    m_lootUnlocks.clear();
    m_raids.clear();
    m_dungeons.clear();
    m_phases.clear();
    m_arenaSeasons.clear();
    m_arenaItems.clear();

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

    // Load creature specific loot unlocks
    auto lootQuery = WorldDatabase.PQuery(
        "SELECT creature_entry, item_entry, phase FROM progression_loot_unlocks");

    if (lootQuery)
    {
        do
        {
            Field* fields = lootQuery->Fetch();

            LootUnlock unlock;

            unlock.creatureEntry = fields[0].GetUInt32();
            unlock.itemEntry = fields[1].GetUInt32();
            unlock.phase = fields[2].GetUInt32();

            m_lootUnlocks.push_back(unlock);

            sLog.outString(
                "Progression loot loaded: Creature %u Item %u Phase %u",
                unlock.creatureEntry,
                unlock.itemEntry,
                unlock.phase);

        } while (lootQuery->NextRow());
    }

    //
    // Load raid progression
    //
    auto raidQuery = WorldDatabase.PQuery(
        "SELECT map_id, phase, name FROM progression_raids");

    if (raidQuery)
    {
        do
        {
            Field* fields = raidQuery->Fetch();

            RaidInfo raid;

            raid.mapId = fields[0].GetUInt32();
            raid.phase = fields[1].GetUInt32();
            raid.name = fields[2].GetString();

            m_raids[raid.mapId] = raid;

            sLog.outString(
                "Progression raid loaded: Map %u %s Phase %u",
                raid.mapId,
                raid.name.c_str(),
                raid.phase);

        } while (raidQuery->NextRow());
    }

    //
    // Load dungeon progression
    //
    auto dungeonQuery = WorldDatabase.PQuery(
        "SELECT map_id, phase, name, heroic FROM progression_dungeons");

    if (dungeonQuery)
    {
        do
        {
            Field* fields = dungeonQuery->Fetch();

            DungeonInfo dungeon;

            dungeon.mapId = fields[0].GetUInt32();
            dungeon.phase = fields[1].GetUInt32();
            dungeon.name = fields[2].GetString();
            dungeon.heroic = fields[3].GetUInt32();

            m_dungeons[std::make_pair(dungeon.mapId, dungeon.heroic)] = dungeon;

            sLog.outString(
                "Progression dungeon loaded: Map %u %s Phase %u Heroic %u",
                dungeon.mapId,
                dungeon.name.c_str(),
                dungeon.phase,
                dungeon.heroic);

        } while (dungeonQuery->NextRow());
    }

    auto phaseQuery = WorldDatabase.PQuery(
        "SELECT phase, name, expansion, patch, start_level, max_level FROM progression_phases");

    if (phaseQuery)
    {
        do
        {
            Field* fields = phaseQuery->Fetch();

            PhaseInfo info;

            info.phase = fields[0].GetUInt32();
            info.name = fields[1].GetString();
            info.expansion = fields[2].GetString();
            info.patch = fields[3].GetString();
            info.startLevel = fields[4].GetUInt32();
            info.maxLevel = fields[5].GetUInt32();

            m_phases[info.phase] = info;

            sLog.outString(
                "Progression phase loaded: %u %s",
                info.phase,
                info.name.c_str());

        } while (phaseQuery->NextRow());
    }
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
    auto itr = m_phases.find(m_phase);

    if (itr == m_phases.end())
        return "Unknown";

    return itr->second.name.c_str();
}

bool ProgressionMgr::IsRaidUnlocked(uint32 mapId) const
{
    return m_phase >= GetRequiredPhase(mapId);
}

uint32 ProgressionMgr::GetRequiredPhase(uint32 mapId) const
{
    auto itr = m_raids.find(mapId);

    if (itr == m_raids.end())
        return 1;

    return itr->second.phase;
}

uint32 ProgressionMgr::GetRequiredDungeonPhase(uint32 mapId, uint32 heroic) const
{
    auto itr = m_dungeons.find(
        std::make_pair(mapId, heroic));

    if (itr == m_dungeons.end())
        return 1;

    return itr->second.phase;
}

bool ProgressionMgr::IsDungeonUnlocked(uint32 mapId, uint32 heroic) const
{
    auto itr = m_dungeons.find(
        std::make_pair(mapId, heroic));

    if (itr == m_dungeons.end())
        return true;

    return m_phase >= itr->second.phase;
}

bool ProgressionMgr::IsArenaItemUnlocked(uint32 itemEntry) const
{
    auto itr = m_arenaItems.find(itemEntry);

    // item няма запис -> нормален item
    if (itr == m_arenaItems.end())
        return true;

    uint32 currentSeason = GetCurrentArenaSeason();

    return currentSeason >= itr->second.seasonId;
}

bool ProgressionMgr::HasAttunement(Player* player, uint32 mapId) const
{
    if (!player)
        return false;

    for (auto const& attune : m_attunements)
    {
        // Това attunement изискване не е за тази инстанция
        if (attune.mapId != mapId)
            continue;

        // ITEM requirement
        if (attune.type == "ITEM")
        {
            if (!player->HasItemCount(attune.entry, 1))
            {
                player->GetSession()->SendNotification(
                    "You need: %s",
                    attune.name.c_str());

                return false;
            }
        }

        // QUEST requirement
        else if (attune.type == "QUEST")
        {
            if (player->GetQuestStatus(attune.entry) != QUEST_STATUS_COMPLETE)
            {
                player->GetSession()->SendNotification(
                    "You have not completed: %s",
                    attune.name.c_str());

                return false;
            }
        }
    }

    return true;
}

const char* ProgressionMgr::GetDungeonName(uint32 mapId, uint32 heroic) const
{
    auto itr = m_dungeons.find(
        std::make_pair(mapId, heroic)
    );

    if (itr == m_dungeons.end())
        return "Unknown Dungeon";

    return itr->second.name.c_str();
}

const char* ProgressionMgr::GetRaidName(uint32 mapId) const
{
    auto itr = m_raids.find(mapId);

    if (itr == m_raids.end())
        return "Unknown Raid";

    return itr->second.name.c_str();
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

    if (phase > 6)
        phase = 6;

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
    if (IsArenaItem(itemEntry))
        return IsArenaItemUnlocked(itemEntry);

    return IsUnlocked("ITEM", itemEntry);
}

bool ProgressionMgr::IsArenaItem(uint32 itemId) const
{
    auto itr = m_arenaItems.find(itemId);

    return itr != m_arenaItems.end();
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

bool ProgressionMgr::IsSpellUnlocked(uint32 spellId) const
{
    return IsUnlocked("SPELL", spellId);
}

bool ProgressionMgr::IsItemUnlocked(uint32 itemEntry) const
{
    return IsUnlocked("ITEM", itemEntry);
}

bool ProgressionMgr::IsLootItemUnlocked(uint32 creatureEntry, uint32 itemEntry) const
{
    // First check boss/NPC specific loot
    for (auto const& unlock : m_lootUnlocks)
    {
        if (unlock.creatureEntry == creatureEntry &&
            unlock.itemEntry == itemEntry)
        {
            return m_phase >= unlock.phase;
        }
    }

    // Fallback to global item unlock
    return IsUnlocked("ITEM", itemEntry);
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

void ProgressionMgr::LoadArenaSeasons()
{
    auto query = WorldDatabase.PQuery(
        "SELECT season_id, phase, name, patch FROM progression_arena_seasons");

    if (!query)
    {
        sLog.outString(
            "Progression: no arena season data found");

        return;
    }

    do
    {
        Field* fields = query->Fetch();

        ArenaSeasonInfo info;

        info.seasonId = fields[0].GetUInt32();
        info.phase = fields[1].GetUInt32();
        info.name = fields[2].GetString();
        info.patch = fields[3].GetString();

        m_arenaSeasons[info.seasonId] = info;

        sLog.outString(
            "Progression arena season loaded: Season %u %s Phase %u Patch %s",
            info.seasonId,
            info.name.c_str(),
            info.phase,
            info.patch.c_str());

    } while (query->NextRow());
}

uint32 ProgressionMgr::GetCurrentArenaSeason() const
{
    uint32 season = 0;

    for (auto const& itr : m_arenaSeasons)
    {
        if (m_phase >= itr.second.phase)
            season = itr.second.seasonId;
    }

    return season;
}

void ProgressionMgr::LoadArenaItems()
{
    auto query = WorldDatabase.PQuery(
        "SELECT item_entry, season_id, name FROM progression_arena_items");

    if (!query)
    {
        sLog.outString(
            "Progression: no arena item data found");

        return;
    }

    do
    {
        Field* fields = query->Fetch();

        ArenaItemInfo info;

        info.itemEntry = fields[0].GetUInt32();
        info.seasonId = fields[1].GetUInt32();
        info.name = fields[2].GetString();

        m_arenaItems[info.itemEntry] = info;

        sLog.outString(
            "Progression arena item loaded: Item %u Season %u %s",
            info.itemEntry,
            info.seasonId,
            info.name.c_str());

    } while (query->NextRow());
}