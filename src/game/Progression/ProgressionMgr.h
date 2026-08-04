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

    // Load creature specific loot unlocks
    void LoadLootUnlocks();

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

    /*
        Generic progression check

        TYPE examples:
        ITEM
        QUEST
        GAMEOBJECT
        CREATURE
        VENDOR
        RAID
        SPELL
    */

    bool IsUnlocked(std::string type, uint32 entry) const;

    bool IsRaidUnlocked(uint32 mapId) const;

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

    const char* GetRaidName(uint32 mapId) const;

    const char* GetPhaseName() const;
private:
    struct LootUnlock
    {
        uint32 creatureEntry;
        uint32 itemEntry;
        uint32 phase;
    };

    bool m_enabled;

    uint32 m_phase;

    /*
        Global progression unlock cache

        Database:

        progression_unlocks

        Example:

        ITEM        17030       3
        QUEST       783         3
        GAMEOBJECT  144131      3

        Stored as:

        TYPE -> ENTRY -> PHASE
    */

    std::map<std::string, std::map<uint32, uint32>> m_unlocks;

    /*
        Creature specific loot unlock cache

        Example:

        Creature 23035
        Item     32768
        Phase    3

        Stored:

        Anzu -> Raven Lord mount -> Phase 3
    */

    std::vector<LootUnlock> m_lootUnlocks;
};

extern ProgressionMgr* sProgressionMgr;

#endif