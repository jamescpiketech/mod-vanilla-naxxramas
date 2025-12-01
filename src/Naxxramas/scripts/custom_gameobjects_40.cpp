#include "ScriptMgr.h"
#include "SpellScript.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "VanillaNaxxramas.h"
#include "naxxramas_40.h"
#include "SharedDefines.h"
#include "Map.h"

namespace
{
    // Eastern Plaguelands return location
    constexpr uint32 EPL_MAP_ID = 0;
    constexpr float EPL_X = 3119.318f;
    constexpr float EPL_Y = -3755.455f;
    constexpr float EPL_Z = 133.56865f;
    constexpr float EPL_O = 1.0722394f;
}

class gobject_naxx40_tele : public GameObjectScript
{
public:
    gobject_naxx40_tele() : GameObjectScript("gobject_naxx40_tele") { }

    struct gobject_naxx40_teleAI: GameObjectAI
    {
        explicit gobject_naxx40_teleAI(GameObject* object) : GameObjectAI(object) { };
    };

    GameObjectAI* GetAI(GameObject* object) const override
    {
        return new gobject_naxx40_teleAI(object);
    }

    void HandleTeleport(Player* player) const
    {
        if ((!sVanillaNaxxramas->requireNaxxStrath || player->GetQuestStatus(NAXX40_ENTRANCE_FLAG) == QUEST_STATUS_REWARDED))
        {
            if (IsAttuned(player) || !sVanillaNaxxramas->requireAttunement)
            {
                // If the player is a ghost, resurrect before sending them in
                if (!player->IsAlive())
                {
                    player->ResurrectPlayer(1.0f);
                    player->SpawnCorpseBones();
                    player->SetHealth(player->GetMaxHealth());

                    // Refill all available power types (mana/energy/rage/runic)
                    for (uint32 powerIdx = 0; powerIdx < MAX_POWERS; ++powerIdx)
                    {
                        Powers powerType = static_cast<Powers>(powerIdx);
                        if (player->GetMaxPower(powerType) > 0)
                            player->SetPower(powerType, player->GetMaxPower(powerType));
                    }
                }

                player->SetRaidDifficulty(RAID_DIFFICULTY_10MAN_HEROIC);
                player->TeleportTo(533, 3005.51f, -3434.64f, 304.195f, 6.2831f);
            }
        }
    }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        HandleTeleport(player);
        return true;
    }

    // Fallback for cores without OnGameObjectUse hook on GameObjectScript
    bool OnGameObjectUse(Player* player, GameObject* /*go*/) 
    {
        HandleTeleport(player);
        return true;
    }
};

class gobject_naxx40_plaguewood_orb : public GameObjectScript
{
public:
    // Handles the Plaguewood teleporter GO (Entry: 181476) for Naxx40
    gobject_naxx40_plaguewood_orb() : GameObjectScript("gobject_naxx40_plaguewood_orb") { }

    void HandleTeleport(Player* player) const
    {
        if (!player)
            return;

        // Resurrect ghosts before sending them in
        if (!player->IsAlive())
        {
            player->ResurrectPlayer(1.0f);
            player->SpawnCorpseBones();
            player->SetHealth(player->GetMaxHealth());
            for (uint32 powerIdx = 0; powerIdx < MAX_POWERS; ++powerIdx)
            {
                Powers powerType = static_cast<Powers>(powerIdx);
                if (player->GetMaxPower(powerType) > 0)
                    player->SetPower(powerType, player->GetMaxPower(powerType));
            }
        }

        player->SetRaidDifficulty(RAID_DIFFICULTY_10MAN_HEROIC);
        player->TeleportTo(533, 3005.51f, -3434.64f, 304.195f, 6.2831f);
    }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        HandleTeleport(player);
        return true;
    }

    bool OnGameObjectUse(Player* player, GameObject* /*go*/)
    {
        HandleTeleport(player);
        return true;
    }

    struct gobject_naxx40_plaguewood_orbAI : GameObjectAI
    {
        explicit gobject_naxx40_plaguewood_orbAI(GameObject* object) : GameObjectAI(object) { }

        void UpdateAI(uint32 diff) override
        {
            if (_checkTimer <= diff)
            {
                TeleportNearbyPlayers();
                _checkTimer = 500;
            }
            else
            {
                _checkTimer -= diff;
            }
        }

        void TeleportNearbyPlayers()
        {
            // Auto-teleport only for the Plaguewood entrance orb (entry 181476) to let ghosts in
            if (!me || me->GetEntry() != 181476 || !me->GetMap())
                return;

            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            for (auto const& ref : players)
            {
                if (Player* player = ref.GetSource())
                {
                    if (player->IsWithinDistInMap(me, 4.0f, false))
                    {
                        // Inline teleport logic (for cores without GetScript)
                        if (!player->IsAlive())
                        {
                            player->ResurrectPlayer(1.0f);
                            player->SpawnCorpseBones();
                            player->SetHealth(player->GetMaxHealth());
                            for (uint32 powerIdx = 0; powerIdx < MAX_POWERS; ++powerIdx)
                            {
                                Powers powerType = static_cast<Powers>(powerIdx);
                                if (player->GetMaxPower(powerType) > 0)
                                    player->SetPower(powerType, player->GetMaxPower(powerType));
                            }
                        }

                        player->SetRaidDifficulty(RAID_DIFFICULTY_10MAN_HEROIC);
                        player->TeleportTo(533, 3005.51f, -3434.64f, 304.195f, 6.2831f);
                    }
                }
            }
        }

    private:
        uint32 _checkTimer = 500;
    };

    GameObjectAI* GetAI(GameObject* object) const override
    {
        return new gobject_naxx40_plaguewood_orbAI(object);
    }
};

class gobject_naxx40_inside_orb : public GameObjectScript
{
public:
    // Handles WotLK orbs when in Naxx40 mode: exit to EPL
    gobject_naxx40_inside_orb() : GameObjectScript("gobject_naxx40_inside_orb") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        // Only the Frozen Halls orb (202277) should port out
        if (player->GetEntry() == 202277 && player->GetMapId() == 533 && player->GetRaidDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
        {
            player->TeleportTo(0, EPL_X, EPL_Y, EPL_Z, EPL_O);
            return true;
        }
        return false;
    }

    bool OnGameObjectUse(Player* player, GameObject* /*go*/)
    {
        if (player->GetEntry() == 202277 && player->GetMapId() == 533 && player->GetRaidDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
        {
            player->TeleportTo(0, EPL_X, EPL_Y, EPL_Z, EPL_O);
            return true;
        }
        return false;
    }

    struct gobject_naxx40_inside_orbAI : GameObjectAI
    {
        explicit gobject_naxx40_inside_orbAI(GameObject* object) : GameObjectAI(object) { }

        void UpdateAI(uint32 diff) override
        {
            if (_checkTimer <= diff)
            {
                TeleportNearbyPlayers();
                _checkTimer = 500;
            }
            else
            {
                _checkTimer -= diff;
            }
        }

        void TeleportNearbyPlayers()
        {
            if (!me || me->GetEntry() != 202277 || !me->GetMap())
                return;

            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            for (auto const& ref : players)
            {
                if (Player* player = ref.GetSource())
                {
                    if (player->IsWithinDistInMap(me, 4.0f, false) && player->GetMapId() == 533 && player->GetRaidDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
                    {
                        player->TeleportTo(EPL_MAP_ID, EPL_X, EPL_Y, EPL_Z, EPL_O);
                    }
                }
            }
        }

    private:
        uint32 _checkTimer = 500;
    };

    GameObjectAI* GetAI(GameObject* object) const override
    {
        return new gobject_naxx40_inside_orbAI(object);
    }
};

void AddSC_custom_gameobjects_40()
{
    new gobject_naxx40_tele();
    new gobject_naxx40_plaguewood_orb();
    new gobject_naxx40_inside_orb();
}
