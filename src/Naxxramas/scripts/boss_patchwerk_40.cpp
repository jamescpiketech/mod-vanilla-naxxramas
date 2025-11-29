/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CreatureScript.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"

enum Yells
{
    SAY_AGGRO                       = 0,
    SAY_SLAY                        = 1,
    SAY_DEATH                       = 2,
    EMOTE_BERSERK                   = 3,
    EMOTE_ENRAGE                    = 4
};

enum Spells
{
    SPELL_HATEFUL_STRIKE_10         = 41926,
    SPELL_HATEFUL_STRIKE_25         = 59192,
    SPELL_FRENZY                    = 28131,
    SPELL_BERSERK                   = 26662,
    SPELL_SLIME_BOLT                = 32309
};

enum Events
{
    EVENT_HEALTH_CHECK              = 1,
    EVENT_HATEFUL_STRIKE            = 2,
    EVENT_SLIME_BOLT                = 3,
    EVENT_BERSERK                   = 4
};

enum Misc
{
    ACHIEV_TIMED_START_EVENT        = 10286
};

class boss_patchwerk_40 : public CreatureScript
{
public:
    boss_patchwerk_40() : CreatureScript("boss_patchwerk_40") { }

    CreatureAI* GetAI(Creature* pCreature) const override
    {
        return GetNaxxramasAI<boss_patchwerk_40AI>(pCreature);
    }

    struct boss_patchwerk_40AI : public BossAI
    {
        explicit boss_patchwerk_40AI(Creature* c) : BossAI(c, BOSS_PATCHWERK)
        {}

        EventMap events;

        void Reset() override
        {
            BossAI::Reset();
            events.Reset();
        }

        void KilledUnit(Unit* who) override
        {
            if (!who->IsPlayer())
                return;

            if (!urand(0, 3))
                Talk(SAY_SLAY);

            instance->StorePersistentData(PERSISTENT_DATA_IMMORTAL_FAIL, 1);
        }

        void JustDied(Unit*  killer) override
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(SAY_AGGRO);
            me->SetInCombatWithZone();
            events.ScheduleEvent(EVENT_HATEFUL_STRIKE, 2400ms);
            events.ScheduleEvent(EVENT_BERSERK, 7min); // 7 minutes enrange
            events.ScheduleEvent(EVENT_HEALTH_CHECK, 1s);
            // instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_HATEFUL_STRIKE:
                   {
                        // Hit the second highest threat target in melee range (fallback to victim if none)
                        std::vector<Unit*> meleeThreatOrder;
                        for (auto const& ref : me->GetThreatMgr().GetThreatList())
                        {
                            if (Unit* t = ref->getTarget())
                                if (me->IsWithinMeleeRange(t))
                                    meleeThreatOrder.push_back(t);
                        }

                        Unit* hatefulTarget = nullptr;
                        if (meleeThreatOrder.size() >= 2)
                            hatefulTarget = meleeThreatOrder[1];
                        else if (!meleeThreatOrder.empty())
                            hatefulTarget = meleeThreatOrder[0];
                        else if (me->GetVictim() && me->IsWithinMeleeRange(me->GetVictim()))
                            hatefulTarget = me->GetVictim();

                        if (hatefulTarget)
                        {
                            int32 dmg = urand(22100,22850);
                            me->CastCustomSpell(hatefulTarget, SPELL_HATEFUL_STRIKE_10, &dmg, 0, 0, false);
                        }
                        events.Repeat(2400ms);
                        break;
                    }
                case EVENT_BERSERK:
                    Talk(EMOTE_BERSERK);
                    me->CastSpell(me, SPELL_BERSERK, true);
                    events.ScheduleEvent(EVENT_SLIME_BOLT, 3s);
                    break;
                case EVENT_SLIME_BOLT:
                    me->CastSpell(me, SPELL_SLIME_BOLT, false);
                    events.Repeat(3s);
                    break;
                case EVENT_HEALTH_CHECK:
                    if (me->GetHealthPct() <= 5)
                    {
                        Talk(EMOTE_ENRAGE);
                        me->CastSpell(me, SPELL_FRENZY, true);
                        break;
                    }
                    events.Repeat(1s);
                    break;
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_patchwerk_40()
{
    new boss_patchwerk_40();
}
