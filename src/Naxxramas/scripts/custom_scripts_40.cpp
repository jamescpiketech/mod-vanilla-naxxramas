#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "InstanceScript.h"
#include "naxxramas.h"
#include "Map.h"
#include <cmath>

namespace
{
    // Eastern Plaguelands return location
    constexpr uint32 EPL_MAP_ID = 0;
    constexpr float EPL_X = 3119.318f;
    constexpr float EPL_Y = -3755.455f;
    constexpr float EPL_Z = 133.56865f;
    constexpr float EPL_O = 1.0722394f;

    // Frozen Halls destination (Sapphiron antechamber)
    constexpr uint32 NAXX_MAP_ID = 533;
    constexpr float FH_X = 3498.300049f;
    constexpr float FH_Y = -5349.490234f;
    constexpr float FH_Z = 144.968002f;
    constexpr float FH_O = 1.3698910f;

    // Hub portal location (where players land when entering)
    constexpr float HUB_X = 3005.5f;
    constexpr float HUB_Y = -3434.6f;
    constexpr float HUB_Z = 304.2f;
}

class NaxxPlayerScript : public PlayerScript
{
public:
    NaxxPlayerScript() : PlayerScript("NaxxPlayerScript") { }

    void OnPlayerBeforeChooseGraveyard(Player* player, TeamId /*teamId*/, bool /*nearCorpse*/, uint32& graveyardOverride) override
    {
        if (player->GetMapId() == MAP_NAXX && player->GetMap()->GetSpawnMode() == RAID_DIFFICULTY_10MAN_HEROIC)
        {
            graveyardOverride = NAXX40_GRAVEYARD;
        }
    }
};

class Naxx40ReleaseTeleport_PlayerScript : public PlayerScript
{
public:
    Naxx40ReleaseTeleport_PlayerScript() : PlayerScript("Naxx40ReleaseTeleport_PlayerScript") { }

    bool OnPlayerCanRepopAtGraveyard(Player* player) override
    {
        return true;
    }
};

class Naxx40GhostReturn_AllMapScript : public AllMapScript
{
public:
    Naxx40GhostReturn_AllMapScript() : AllMapScript("Naxx40GhostReturn_AllMapScript") { }

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        if (!map || map->GetId() != MAP_NAXX || map->GetSpawnMode() != RAID_DIFFICULTY_10MAN_HEROIC)
            return;

        if (_checkTimer > diff)
        {
            _checkTimer -= diff;
            return;
        }

        _checkTimer = 1000;

        Map::PlayerList const& players = map->GetPlayers();
        for (auto const& ref : players)
        {
            if (Player* player = ref.GetSource())
            {
                if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
                {
                    // Only teleport if not already at the target spot (avoid repeated teleports)
                    if (player->GetDistance2d(EPL_X, EPL_Y) > 5.0f || std::fabs(player->GetPositionZ() - EPL_Z) > 5.0f)
                    {
                        player->TeleportTo(EPL_MAP_ID, EPL_X, EPL_Y, EPL_Z, EPL_O);
                        player->RemovePlayerFlag(PLAYER_FLAGS_IS_OUT_OF_BOUNDS);
                    }
                }
            }
        }
    }

private:
    uint32 _checkTimer = 0;
};

class Naxx40HubPortal_AllMapScript : public AllMapScript
{
public:
    Naxx40HubPortal_AllMapScript() : AllMapScript("Naxx40HubPortal_AllMapScript") { }

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        if (!map || map->GetId() != MAP_NAXX || map->GetSpawnMode() != RAID_DIFFICULTY_10MAN_HEROIC)
            return;

        if (_checkTimer > diff)
        {
            _checkTimer -= diff;
            return;
        }

        _checkTimer = 500;

        InstanceScript* instance = nullptr;
        if (InstanceMap* iMap = map->ToInstanceMap())
            instance = iMap->GetInstanceScript();

        if (!instance)
            return;

        // Require the four wing bosses to be DONE
        if (instance->GetBossState(BOSS_MAEXXNA)  != DONE ||
            instance->GetBossState(BOSS_LOATHEB)  != DONE ||
            instance->GetBossState(BOSS_THADDIUS) != DONE ||
            instance->GetBossState(BOSS_HORSEMAN) != DONE)
            return;

        Map::PlayerList const& players = map->GetPlayers();
        for (auto const& ref : players)
        {
            if (Player* player = ref.GetSource())
            {
                if (!player->IsAlive() || player->IsInCombat())
                    continue;

                // Only fire when player is on/near the hub portal pad
                if (player->GetDistance2d(HUB_X, HUB_Y) < 6.0f && std::fabs(player->GetPositionZ() - HUB_Z) < 6.0f)
                {
                    player->TeleportTo(NAXX_MAP_ID, FH_X, FH_Y, FH_Z, FH_O);
                }
            }
        }
    }

private:
    uint32 _checkTimer = 0;
};

class naxx_northrend_entrance : public AreaTriggerScript
{
public:
    naxx_northrend_entrance() : AreaTriggerScript("naxx_northrend_entrance") { }

    bool OnTrigger(Player* player, AreaTrigger const* areaTrigger) override
    {
        // Do not allow entrance to Naxx 40 from Northrend
        // Change 10 man heroic to regular 10 man, as when 10 man heroic is not available
        Difficulty diff = player->GetGroup() ? player->GetGroup()->GetDifficulty(true) : player->GetDifficulty(true);
        if (diff == RAID_DIFFICULTY_10MAN_HEROIC)
        {
            player->SetRaidDifficulty(RAID_DIFFICULTY_10MAN_NORMAL);
        }
        switch (areaTrigger->entry)
        {
            // Naxx 10 and 25 entrances
            case 5191:
                player->TeleportTo(533, 3005.68f, -3447.77f, 293.93f, 4.65f);
                break;
            case 5192:
                player->TeleportTo(533, 3019.34f, -3434.36f, 293.99f, 6.27f);
                break;
            case 5193:
                player->TeleportTo(533, 3005.9f, -3420.58f, 294.11f, 1.58f);
                break;
            case 5194:
                player->TeleportTo(533, 2992.5f, -3434.42f, 293.94f, 3.13f);
                break;
        }
        return true;
    }
};

class naxx_exit_trigger : public AreaTriggerScript
{
public:
    naxx_exit_trigger() : AreaTriggerScript("naxx_exit_trigger") { }

    bool OnTrigger(Player* player, AreaTrigger const* areaTrigger) override
    {
        if (player->GetMap()->GetSpawnMode() == RAID_DIFFICULTY_10MAN_HEROIC)
        {
            // Only handle known exit triggers/orbs for Naxx40
            if (!areaTrigger)
                return true;

            switch (areaTrigger->entry)
            {
                case 5196:
                case 5197:
                case 5198:
                case 5199:
                case 4156:
                    // Avoid bouncing right after zoning in at the entrance platform
                    if (player->GetDistance2d(3005.5f, -3434.6f) < 20.0f && std::fabs(player->GetPositionZ() - 304.2f) < 10.0f)
                        return true;

                    player->TeleportTo(EPL_MAP_ID, EPL_X, EPL_Y, EPL_Z, EPL_O);
                    break;
                default:
                    // Ignore other triggers to avoid bounce-on-entry
                    break;
            }
            return true;
        }
        switch (areaTrigger->entry)
        {
            // Naxx 10 and 25 exits
            case 5196:
                player->TeleportTo(571, 3679.25f, -1278.58f, 243.55f, 2.39f);
                break;
            case 5197:
                player->TeleportTo(571, 3679.03f, -1259.68f, 243.55f, 3.98f);
                break;
            case 5198:
                player->TeleportTo(571, 3661.14f, -1279.55f, 243.55f, 0.82f);
                break;
            case 5199:
                player->TeleportTo(571, 3660.01f, -1260.99f, 243.55f, 5.51f);
                break;
            // Frozen Halls exit trigger
            case 4156:
                player->TeleportTo(571, 3679.25f, -1278.58f, 243.55f, 2.39f);
                break;
        }
        return true;
    }
};

class NaxxEntryFlag_AllMapScript : public AllMapScript
{
public:
    NaxxEntryFlag_AllMapScript() : AllMapScript("NaxxEntryFlag_AllMapScript") { }

    void OnPlayerEnterAll(Map* map, Player* player) override
    {
        if (player->IsGameMaster())
            return;

        // Check if mapId equals to Naxxramas (mapId: 533)
        if (map->GetId() != 533)
            return;

        // Cast on player Naxxramas Entry Flag Trigger DND - Classic (spellID: 29296)
        if (player->GetQuestStatus(NAXX40_ENTRANCE_FLAG) != QUEST_STATUS_REWARDED)
        {
            // Mark player as having entered
            Quest const* quest = sObjectMgr->GetQuestTemplate(NAXX40_ENTRANCE_FLAG);
            player->AddQuest(quest, nullptr);
            player->CompleteQuest(NAXX40_ENTRANCE_FLAG);
            player->RewardQuest(quest, 0, player, false, false);
            // Cast on player Naxxramas Entry Flag Trigger DND - Classic (spellID: 29296)
            player->CastSpell(player, 29296, true); // for visual effect only, possible crash if cast on login
        }
    }
};

void AddSC_custom_scripts_40()
{
    new NaxxPlayerScript();
    new Naxx40ReleaseTeleport_PlayerScript();
    new Naxx40GhostReturn_AllMapScript();
    new Naxx40HubPortal_AllMapScript();
    new naxx_exit_trigger();
    new naxx_northrend_entrance();
    new NaxxEntryFlag_AllMapScript();
}
