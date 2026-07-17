/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_PRIORITYMULTIPLIER_H
#define PLAYERBOTS_PRIORITYMULTIPLIER_H

#include "Multiplier.h"
#include "PlayerbotAI.h"

class PriorityMultiplier : public Multiplier
{
public:
    PriorityMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "priority") {}

    float GetValue(Action* action) override
    {
        std::string const strategy = botAI->GetPriorityStrategy();
        if (strategy.empty())
            return 1.0f;

        std::string const name = action->getName();
        float multiplier = 1.0f;

        if (strategy == "quest")
        {
            if (IsQuestAction(name))
                multiplier = 50.0f;
        }
        else if (strategy == "rpg")
        {
            if (IsRpgAction(name))
                multiplier = 50.0f;
        }
        else if (strategy == "grind")
        {
            if (IsGrindAction(name))
                multiplier = 50.0f;
        }
        else if (strategy == "travel")
        {
            if (IsTravelAction(name))
                multiplier = 50.0f;
        }

        if (multiplier > 1.0f)
            LOG_DEBUG("playerbots", "PriorityMultiplier: action={} strategy={} multiplier={}",
                      name, strategy, multiplier);

        return multiplier;
    }

private:
    static bool IsQuestAction(const std::string& name)
    {
        return name.find("quest") != std::string::npos ||
               name.find("accept") != std::string::npos ||
               name == "talk to quest giver" ||
               name == "complete quest";
    }

    static bool IsRpgAction(const std::string& name)
    {
        return name.find("rpg") != std::string::npos;
    }

    static bool IsGrindAction(const std::string& name)
    {
        return name.find("attack") != std::string::npos;
    }

    static bool IsTravelAction(const std::string& name)
    {
        return name.find("travel") != std::string::npos ||
               name.find("move to") != std::string::npos ||
               name == "move random";
    }
};

#endif
