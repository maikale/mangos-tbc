#ifndef __PROGRESSIONMGR_H
#define __PROGRESSIONMGR_H

#include "Common.h"

#include <map>
#include <string>

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
*/
    bool IsUnlocked(std::string type, uint32 entry) const;

    bool IsRaidUnlocked(uint32 mapId) const;

    bool IsVendorItemUnlocked(uint32 vendorEntry, uint32 itemEntry) const;

    bool IsVendorUnlocked(uint32 vendorEntry) const;

    bool IsGameObjectUnlocked(uint32 entry) const;

    bool IsQuestUnlocked(uint32 questId) const;

    bool HasUnlockedQuestGiver(Creature* creature) const;

    bool IsCreatureUnlocked(uint32 creatureEntry) const;

    uint32 GetRequiredPhase(uint32 mapId) const;

    const char* GetRaidName(uint32 mapId) const;

    const char* GetPhaseName() const;
private:
    bool m_enabled;

    uint32 m_phase;

    /*
        Progression unlock cache

        Database:

        progression_unlocks

        Example:

        ITEM       17030     3
        QUEST      783       3
        GAMEOBJECT 144131    3

        Stored as:

        TYPE -> ENTRY -> PHASE
    */

    std::map<std::string, std::map<uint32, uint32>> m_unlocks;
};

extern ProgressionMgr* sProgressionMgr;

#endif