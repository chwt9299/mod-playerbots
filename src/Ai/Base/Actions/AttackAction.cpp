/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AttackAction.h"

#include "CreatureAI.h"
#include "Event.h"
#include "LastMovementValue.h"
#include "LootObjectStack.h"
#include "PlayerbotAI.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "SharedDefines.h"
#include "Unit.h"
#include "WaitForAttackStrategy.h"

// Suppress TellError when bot is in full-auto AI mode to avoid spam
static inline void TellErrorIfVerbose(PlayerbotAI* botAI, std::string const& text)
{
    if (!botAI->IsBotAiMode())
        botAI->TellError(text);
}

bool AttackAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    if (!target->IsInWorld())
        return false;

    return Attack(target);
}

bool AttackMyTargetAction::Execute(Event /*event*/)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    ObjectGuid guid = master->GetTarget();
    if (!guid)
    {
        if (verbose)
            botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "pull_no_target_error", "You have no target", {}));

        return false;
    }

    botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({guid});
    bool result = Attack(botAI->GetUnit(guid));
    if (result)
        context->GetValue<ObjectGuid>("pull target")->Set(guid);

    return result;
}

bool AttackAction::Attack(Unit* target, bool /*with_pet*/ /*true*/)
{
    if (!target)
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_no_target_error", "I have no target", {}));

        return false;
    }

    // Cooldown after a failed attack attempt to prevent core-level error spam
    if (_lastAttackFailTime && getMSTimeDiff(_lastAttackFailTime, getMSTime()) < ATTACK_FAIL_COOLDOWN)
        return false;

    if (!target->IsInWorld())
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_target_not_in_world_error",
                "%target is no longer in the world.",
                {{"%target", target->GetName()}}));

        return false;
    }

    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE ||
        bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_in_flight_error", "I cannot attack in flight", {}));

        return false;
    }

    // Check if bot OR target is in prohibited zone/area (skip for duels)
    if ((target->IsPlayer() || target->IsPet()) &&
        (!bot->duel || bot->duel->Opponent != target) &&
        (sPlayerbotAIConfig.IsPvpProhibited(bot->GetZoneId(), bot->GetAreaId()) ||
        sPlayerbotAIConfig.IsPvpProhibited(target->GetZoneId(), target->GetAreaId())))
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_pvp_prohibited_error",
                "I cannot attack other players in PvP prohibited areas.",
                {}));

        return false;
    }

    if (bot->IsFriendlyTo(target))
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_target_friendly_error",
                "%target is friendly to me.",
                {{"%target", target->GetName()}}));

        return false;
    }

    if (target->isDead())
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_target_dead_error",
                "%target is dead.",
                {{"%target", target->GetName()}}));

        return false;
    }

    if (!bot->IsWithinLOSInMap(target))
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_target_not_in_sight_error",
                "%target is not in my sight.",
                {{"%target", target->GetName()}}));

        return false;
    }

    // Infantry attacks are not allowed from vehicles drivers.
    // Check is needed to stop some auto-attack situations.
    if (botAI->IsInVehicle() && !botAI->IsInVehicle(false, false, true))
        return false;

    Unit* oldTarget = context->GetValue<Unit*>("current target")->Get();
    bool shouldMelee = bot->IsWithinMeleeRange(target) || botAI->IsMelee(bot);

    bool sameTarget = oldTarget == target && bot->GetVictim() == target;
    bool inCombat = botAI->GetState() == BOT_STATE_COMBAT;
    bool sameAttackMode = bot->HasUnitState(UNIT_STATE_MELEE_ATTACKING) == shouldMelee;

    if (sameTarget && inCombat && sameAttackMode)
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_already_attacking_error",
                "I am already attacking %target.",
                {{"%target", target->GetName()}}));

        return false;
    }

    if (!bot->IsValidAttackTarget(target))
    {
        if (verbose)
            TellErrorIfVerbose(botAI, PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "attack_invalid_target_error", "I cannot attack an invalid target.", {}));

        return false;
    }

    // if (bot->IsMounted() && bot->IsWithinLOSInMap(target))
    // {
    //     WorldPacket emptyPacket;
    //     bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);
    // }

    ObjectGuid guid = target->GetGUID();
    bot->SetSelection(target->GetGUID());

    context->GetValue<Unit*>("old target")->Set(oldTarget);
    context->GetValue<Unit*>("current target")->Set(target);
    context->GetValue<LootObjectStack*>("available loot")->Get()->Add(guid);

    LastMovement& lastMovement = AI_VALUE(LastMovement&, "last movement");
    bool moveControlled = bot->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_CONTROLLED) != NULL_MOTION_TYPE;
    if (lastMovement.priority < MovementPriority::MOVEMENT_COMBAT && bot->isMoving() && !moveControlled)
    {
        AI_VALUE(LastMovement&, "last movement").clear();
        bot->GetMotionMaster()->Clear(false);
        bot->StopMoving();
    }

    if (botAI->CanMove() && !bot->HasInArc(CAST_ANGLE_IN_FRONT, target))
        ServerFacade::instance().SetFacingTo(bot, target);

    botAI->ChangeEngine(BOT_STATE_COMBAT);

    if (!WaitForAttackStrategy::ShouldWait(botAI))
    {
        // Final safety gate: re-check LOS immediately before calling core Attack()
        // Target may have moved out of LOS between initial checks (top of function) and now
        if (bot->IsWithinLOSInMap(target))
        {
            // Distance pre-check: prevent core Attack() from firing system errors
            // like "Target is too far away" when target is clearly out of range
            if (shouldMelee && !bot->IsWithinMeleeRange(target))
            {
                _lastAttackFailTime = getMSTime();
                return false;
            }
            bot->Attack(target, shouldMelee);
        }
        else
        {
            _lastAttackFailTime = getMSTime();
            return false;
        }
    }
    /* prevent pet dead immediately in group */
    // if (bot->GetMap()->IsDungeon() && bot->GetGroup() && !target->IsInCombat())
    // {
    //     with_pet = false;
    // }
    // if (Pet* pet = bot->GetPet())
    // {
    //     if (with_pet)
    //     {
    //         pet->SetReactState(REACT_DEFENSIVE);
    //         pet->SetTarget(target->GetGUID());
    //         pet->GetCharmInfo()->SetIsCommandAttack(true);
    //         pet->AI()->AttackStart(target);
    //     }
    //     else
    //     {
    //         pet->SetReactState(REACT_PASSIVE);
    //         pet->GetCharmInfo()->SetIsCommandFollow(true);
    //         pet->GetCharmInfo()->IsReturning();
    //     }
    // }
    return true;
}

bool AttackDuelOpponentAction::isUseful() { return AI_VALUE(Unit*, "duel target"); }

bool AttackDuelOpponentAction::Execute(Event /*event*/) { return Attack(AI_VALUE(Unit*, "duel target")); }
