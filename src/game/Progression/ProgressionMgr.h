#ifndef __PROGRESSIONMGR_H
#define __PROGRESSIONMGR_H

#define __PROGRESSIONMGR_H

#include "Common.h"

#include <map>
#include <string>
#include <vector>

class WorldObject;
class Creature;
class GameObject;
class Player;
class Map;
class Unit;
class CreatureGroup;

class ProgressionMgr
{
public:
    ProgressionMgr();

    void Load();

    // Load unlock data from database
    void LoadUnlocks();

    void LoadAttunements();

    void LoadArenaSeasons();

    void LoadArenaItems();

    // Load creature specific loot unlocks
    void LoadLootUnlocks();

    // Load phases from database
    void LoadPhases();

    // Load raids from database
    void LoadRaids();

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

    bool IsUnlocked(std::string type, uint32 entry) const;

    bool IsRaidUnlocked(uint32 mapId) const;

    // Dungeon progression
    bool IsDungeonUnlocked(uint32 mapId, uint32 heroic) const;

    bool IsVendorItemUnlocked(uint32 vendorEntry, uint32 itemEntry) const;

    bool IsVendorUnlocked(uint32 vendorEntry) const;

    bool IsGameObjectUnlocked(uint32 entry) const;

    bool IsQuestUnlocked(uint32 questId) const;

    bool HasUnlockedQuestGiver(Creature* creature) const;

    bool IsCreatureUnlocked(uint32 creatureEntry) const;

    bool IsSpellUnlocked(uint32 spellId) const;

    bool IsItemUnlocked(uint32 itemEntry) const;

    // Creature specific loot check
    bool IsLootItemUnlocked(uint32 creatureEntry, uint32 itemEntry) const;

    uint32 GetRequiredPhase(uint32 mapId) const;

    uint32 GetRequiredDungeonPhase(uint32 mapId, uint32 heroic) const;

    const char* GetRaidName(uint32 mapId) const;

    // Dungeon name
    const char* GetDungeonName(uint32 mapId, uint32 heroic) const;

    bool HasAttunement(Player* player, uint32 mapId) const;

    const char* GetPhaseName() const;

    uint32 GetCurrentArenaSeason() const;

    // Arena progression
    bool IsArenaItem(uint32 itemId) const;

    bool IsArenaItemUnlocked(uint32 itemId) const;

private:
    struct LootUnlock
    {
        uint32 creatureEntry;
        uint32 itemEntry;
        uint32 phase;
    };

    // NEW Phase database cache
    struct PhaseInfo
    {
        uint32 phase;

        std::string name;

        std::string expansion;

        std::string patch;

        uint32 startLevel;

        uint32 maxLevel;
    };

    struct ArenaSeasonInfo
    {
        uint32 seasonId;

        uint32 phase;

        std::string name;

        std::string patch;
    };

    struct ArenaItemInfo
    {
        uint32 itemEntry;

        uint32 seasonId;

        std::string name;
    };

    // NEW Raid database cache
    struct RaidInfo
    {
        uint32 mapId;

        uint32 phase;

        std::string name;
    };

    // NEW Dungeon database cache
    struct DungeonInfo
    {
        uint32 mapId;

        uint32 phase;

        uint32 heroic;

        std::string name;
    };

    struct AttunementInfo
    {
        uint32 mapId;
        std::string type;
        uint32 entry;
        std::string name;
    };

    bool m_enabled;

    uint32 m_phase;

    uint32 m_arenaSeason;

    /*
        Global progression unlock cache

        progression_unlocks

        TYPE -> ENTRY -> PHASE
    */

    std::map<std::string, std::map<uint32, uint32>> m_unlocks;

    /*
        Creature specific loot unlock cache

        progression_loot_unlocks

        Creature -> Item -> Phase
    */

    std::vector<LootUnlock> m_lootUnlocks;

    /*
        progression_phases

        Phase -> Info
    */

    std::map<uint32, PhaseInfo> m_phases;

    /*
        progression_raids

        MapID -> Raid Info
    */

    std::map<uint32, RaidInfo> m_raids;

    /*
        progression_dungeons

        MapID -> Dungeon Info
    */

    std::map<std::pair<uint32, uint32>, DungeonInfo> m_dungeons;

    std::vector<AttunementInfo> m_attunements;

    std::map<uint32, ArenaSeasonInfo> m_arenaSeasons;

    std::map<uint32, ArenaItemInfo> m_arenaItems;
};

extern ProgressionMgr* sProgressionMgr;

#endif