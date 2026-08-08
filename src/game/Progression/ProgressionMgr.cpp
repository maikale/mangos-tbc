#include "ProgressionMgr.h"

#include "Config/Config.h"
#include "Database/DatabaseEnv.h"
#include "Entities/Creature.h"
#include "Entities/Player.h"
#include "Globals/ObjectMgr.h"
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

    uint32 attunementCount = 0;

    if (!query)
    {
        sLog.outString(
            "Progression attunements loaded: 0 entries");

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

        ++attunementCount;

    } while (query->NextRow());

    sLog.outString(
        "Progression attunements loaded: %u entries",
        attunementCount);
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

    //
    // Global progression unlocks
    //
    auto queryResult = WorldDatabase.PQuery(
        "SELECT type, entry, phase FROM progression_unlocks");

    if (!queryResult)
    {
        sLog.outString(
            "Progression unlocks loaded: 0 entries");

        return;
    }

    uint32 unlockCount = 0;

    do
    {
        Field* fields = queryResult->Fetch();

        std::string type = fields[0].GetString();

        uint32 entry = fields[1].GetUInt32();

        uint32 phase = fields[2].GetUInt32();

        m_unlocks[type][entry] = phase;

        ++unlockCount;

    } while (queryResult->NextRow());

    sLog.outString(
        "Progression unlocks loaded: %u entries",
        unlockCount);

    //
    // Creature specific loot unlocks
    //
    auto lootQuery = WorldDatabase.PQuery(
        "SELECT creature_entry, item_entry, phase FROM progression_loot_unlocks");

    uint32 lootCount = 0;

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

            ++lootCount;

        } while (lootQuery->NextRow());
    }

    sLog.outString(
        "Progression loot unlocks loaded: %u entries",
        lootCount);

    //
    // Load raid progression
    //
    auto raidQuery = WorldDatabase.PQuery(
        "SELECT map_id, phase, name FROM progression_raids");

    uint32 raidCount = 0;

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

            ++raidCount;

        } while (raidQuery->NextRow());
    }

    sLog.outString(
        "Progression raids loaded: %u entries",
        raidCount);

    //
    // Load dungeon progression
    //
    auto dungeonQuery = WorldDatabase.PQuery(
        "SELECT map_id, phase, name, heroic FROM progression_dungeons");

    uint32 dungeonCount = 0;

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

            m_dungeons[std::make_pair(
                dungeon.mapId,
                dungeon.heroic)] = dungeon;

            ++dungeonCount;

        } while (dungeonQuery->NextRow());
    }

    sLog.outString(
        "Progression dungeons loaded: %u entries",
        dungeonCount);

    //
    // Load phases
    //
    auto phaseQuery = WorldDatabase.PQuery(
        "SELECT phase, name, expansion, patch, start_level, max_level FROM progression_phases");

    uint32 phaseCount = 0;

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

            ++phaseCount;

        } while (phaseQuery->NextRow());
    }

    sLog.outString(
        "Progression phases loaded: %u entries",
        phaseCount);
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

uint32 ProgressionMgr::GetRequiredDungeonPhase(
    uint32 mapId,
    uint32 heroic) const
{
    auto itr = m_dungeons.find(
        std::make_pair(mapId, heroic));

    if (itr == m_dungeons.end())
        return 1;

    return itr->second.phase;
}

bool ProgressionMgr::IsDungeonUnlocked(
    uint32 mapId,
    uint32 heroic) const
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

bool ProgressionMgr::HasAttunement(
    Player* player,
    uint32 mapId) const
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

const char* ProgressionMgr::GetDungeonName(
    uint32 mapId,
    uint32 heroic) const
{
    auto itr = m_dungeons.find(
        std::make_pair(mapId, heroic));

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

bool ProgressionMgr::IsVendorItemUnlocked(
    uint32 vendorEntry,
    uint32 itemEntry) const
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

bool ProgressionMgr::IsLootItemUnlocked(
    uint32 creatureEntry,
    uint32 itemEntry) const
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

bool ProgressionMgr::HasUnlockedQuestGiver(
    Creature* creature) const
{
    if (!creature)
        return false;

    QuestRelationsMapBounds bounds =
        sObjectMgr.GetCreatureQuestRelationsMapBounds(
            creature->GetEntry());

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

    uint32 seasonCount = 0;

    if (!query)
    {
        sLog.outString(
            "Progression arena seasons loaded: 0 entries");

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

        ++seasonCount;

    } while (query->NextRow());

    sLog.outString(
        "Progression arena seasons loaded: %u entries",
        seasonCount);
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

    uint32 arenaItemCount = 0;

    if (!query)
    {
        sLog.outString(
            "Progression arena items loaded: 0 entries");

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

        ++arenaItemCount;

    } while (query->NextRow());

    sLog.outString(
        "Progression arena items loaded: %u entries",
        arenaItemCount);
}