/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "EmoteAction.h"

#include <algorithm>

#include "Event.h"
#include "Playerbots.h"
#include "ServerFacade.h"

std::map<std::string, uint32> EmoteActionBase::emotes;
std::map<std::string, uint32> EmoteActionBase::textEmotes;
char* strstri(char const* haystack, char const* needle);

EmoteActionBase::EmoteActionBase(PlayerbotAI* botAI, std::string const name) : Action(botAI, name)
{
    if (emotes.empty())
        InitEmotes();
}

EmoteAction::EmoteAction(PlayerbotAI* botAI) : EmoteActionBase(botAI, "emote"), Qualified() {}

void EmoteActionBase::InitEmotes()
{
    emotes["dance"] = EMOTE_ONESHOT_DANCE;
    emotes["drown"] = EMOTE_ONESHOT_DROWN;
    emotes["land"] = EMOTE_ONESHOT_LAND;
    emotes["liftoff"] = EMOTE_ONESHOT_LIFTOFF;
    emotes["loot"] = EMOTE_ONESHOT_LOOT;
    emotes["no"] = EMOTE_ONESHOT_NO;
    emotes["roar"] = EMOTE_STATE_ROAR;
    emotes["salute"] = EMOTE_ONESHOT_SALUTE;
    emotes["stomp"] = EMOTE_ONESHOT_STOMP;
    emotes["train"] = EMOTE_ONESHOT_TRAIN;
    emotes["yes"] = EMOTE_ONESHOT_YES;
    emotes["applaud"] = EMOTE_ONESHOT_APPLAUD;
    emotes["beg"] = EMOTE_ONESHOT_BEG;
    emotes["bow"] = EMOTE_ONESHOT_BOW;
    emotes["cheer"] = EMOTE_ONESHOT_CHEER;
    emotes["chicken"] = EMOTE_ONESHOT_CHICKEN;
    emotes["cry"] = EMOTE_ONESHOT_CRY;
    emotes["dance"] = EMOTE_STATE_DANCE;
    emotes["eat"] = EMOTE_ONESHOT_EAT;
    emotes["exclamation"] = EMOTE_ONESHOT_EXCLAMATION;
    emotes["flex"] = EMOTE_ONESHOT_FLEX;
    emotes["kick"] = EMOTE_ONESHOT_KICK;
    emotes["kiss"] = EMOTE_ONESHOT_KISS;
    emotes["kneel"] = EMOTE_ONESHOT_KNEEL;
    emotes["laugh"] = EMOTE_ONESHOT_LAUGH;
    emotes["point"] = EMOTE_ONESHOT_POINT;
    emotes["question"] = EMOTE_ONESHOT_QUESTION;
    emotes["ready1h"] = EMOTE_ONESHOT_READY1H;
    emotes["roar"] = EMOTE_ONESHOT_ROAR;
    emotes["rude"] = EMOTE_ONESHOT_RUDE;
    emotes["shout"] = EMOTE_ONESHOT_SHOUT;
    emotes["shy"] = EMOTE_ONESHOT_SHY;
    emotes["sleep"] = EMOTE_STATE_SLEEP;
    emotes["talk"] = EMOTE_ONESHOT_TALK;
    emotes["wave"] = EMOTE_ONESHOT_WAVE;
    emotes["wound"] = EMOTE_ONESHOT_WOUND;

    textEmotes["bored"] = TEXT_EMOTE_BORED;
    textEmotes["bye"] = TEXT_EMOTE_BYE;
    textEmotes["cheer"] = TEXT_EMOTE_CHEER;
    textEmotes["congratulate"] = TEXT_EMOTE_CONGRATULATE;
    textEmotes["hello"] = TEXT_EMOTE_HELLO;
    textEmotes["no"] = TEXT_EMOTE_NO;
    textEmotes["nod"] = TEXT_EMOTE_NOD;  // yes
    textEmotes["sigh"] = TEXT_EMOTE_SIGH;
    textEmotes["thank"] = TEXT_EMOTE_THANK;
    textEmotes["welcome"] = TEXT_EMOTE_WELCOME;  // you are welcome
    textEmotes["whistle"] = TEXT_EMOTE_WHISTLE;
    textEmotes["yawn"] = TEXT_EMOTE_YAWN;
    textEmotes["oom"] = 323;
    textEmotes["follow"] = 324;
    textEmotes["wait"] = 325;
    textEmotes["healme"] = 326;
    textEmotes["openfire"] = 327;
    textEmotes["helpme"] = 303;
    textEmotes["flee"] = 306;
    textEmotes["danger"] = 304;
    textEmotes["charge"] = 305;
    textEmotes["help"] = 307;
    textEmotes["train"] = 264;
}

bool EmoteActionBase::Emote(Unit* target, uint32 type, bool textEmote)
{
    if (target && !bot->HasInArc(static_cast<float>(M_PI), target, sPlayerbotAIConfig.sightDistance))
        bot->SetFacingToObject(target);

    ObjectGuid oldSelection = bot->GetTarget();
    if (target)
    {
        bot->SetSelection(target->GetGUID());

        Player* player = dynamic_cast<Player*>(target);
        if (player)
        {
            PlayerbotAI* playerBotAI = GET_PLAYERBOT_AI(player);
            if (playerBotAI && !player->HasInArc(static_cast<float>(M_PI), bot, sPlayerbotAIConfig.sightDistance))
            {
                player->SetFacingToObject(bot);
            }
        }
    }

    if (textEmote)
    {
        WorldPacket data(SMSG_TEXT_EMOTE);
        data << type;
        data << GetNumberOfEmoteVariants((TextEmotes)type, bot->getRace(), bot->getGender());
        data << ((bot->GetTarget() && urand(0, 1)) ? bot->GetTarget() : ObjectGuid::Empty);
        bot->GetSession()->HandleTextEmoteOpcode(data);
    }
    else
        bot->HandleEmoteCommand(type);

    if (oldSelection)
        bot->SetTarget(oldSelection);

    return true;
}

Unit* EmoteActionBase::GetTarget()
{
    Unit* target = nullptr;

    GuidVector nfp = *context->GetValue<GuidVector>("nearest friendly players");
    std::vector<Unit*> targets;
    for (GuidVector::iterator i = nfp.begin(); i != nfp.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && ServerFacade::instance().GetDistance2d(bot, unit) < sPlayerbotAIConfig.tooCloseDistance)
            targets.push_back(unit);
    }

    if (!targets.empty())
        target = targets[urand(0, targets.size() - 1)];

    return target;
}

// 辅助函数：从 | 分隔的字符串池中随机选取一项
static std::string PickRandom(std::string const& pool)
{
    if (pool.empty()) return "";
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = pool.find('|');
    while (end != std::string::npos)
    {
        parts.push_back(pool.substr(start, end - start));
        start = end + 1;
        end = pool.find('|', start);
    }
    parts.push_back(pool.substr(start));
    if (parts.empty()) return "";
    return parts[urand(0, parts.size() - 1)];
}

// 辅助函数：转义花括号（避免 LOG_INFO 格式化崩溃）
static std::string EscapeFmt(std::string const& s)
{
    std::string result;
    result.reserve(s.size() + 4);
    for (char c : s)
    {
        if (c == '{' || c == '}') result += c;
        result += c;
    }
    return result;
}

// 辅助函数：替换占位符
static std::string ReplacePlaceholders(std::string const& tmpl, std::string const& targetName, uint32 zoneId, std::string const& zoneName)
{
    std::string result = tmpl;
    // {target} -> 目标玩家名字
    if (!targetName.empty())
    {
        size_t pos = 0;
        while ((pos = result.find("{target}", pos)) != std::string::npos)
        {
            result.replace(pos, 8, targetName);
            pos += targetName.size();
        }
    }
    // {zone} -> 区域名字
    if (!zoneName.empty())
    {
        size_t pos = 0;
        while ((pos = result.find("{zone}", pos)) != std::string::npos)
        {
            result.replace(pos, 6, zoneName);
            pos += zoneName.size();
        }
    }
    return result;
}

bool EmoteActionBase::ReceiveEmote(Player* source, uint32 emote, bool verbal)
{
    uint32 emoteId = 0;
    uint32 textEmote = 0;
    std::string chosen;
    bool isYell = false;

    // === 特殊命令：stay ===
    if (emote == 325)
    {
        if (botAI->GetMaster() == source)
        {
            botAI->ChangeStrategy("-follow,+stay", BOT_STATE_NON_COMBAT);
            botAI->TellMasterNoFacing("Fine.. I'll stay right here..");
        }
        return true;
    }
    // === 特殊命令：follow ===
    if (emote == TEXT_EMOTE_BECKON || emote == 324)
    {
        if (botAI->GetMaster() == source)
        {
            botAI->ChangeStrategy("+follow", BOT_STATE_NON_COMBAT);
            botAI->TellMasterNoFacing("Wherever you go, I'll follow..");
        }
        return true;
    }

    // === 上下文判断 ===
    Player* master = botAI->GetMaster();
    bool isMaster = source && master && source->GetGUID() == master->GetGUID();

    // 获取 source 的名字
    std::string srcName = source ? source->GetName() : "";

    // 区域名称
    uint32 zoneId = bot->GetZoneId();
    std::string zoneName = "";
    AreaTableEntry const* area = sAreaTableStore.LookupEntry(zoneId);
    if (area)
        zoneName = area->area_name[0];

    // 基本状态
    bool inCombat = bot->IsInCombat();
    bool isDead = bot->isDead();
    bool lowHP = bot->GetHealthPct() < 30 && !isDead;
    bool isCasting = bot->IsNonMeleeSpellCast(false);
    bool isMounted = bot->IsMounted();
    bool isMoving = bot->isMoving();

    // 队伍/公会判断
    Group* group = bot->GetGroup();
    bool isInRaid = group && group->isRaidGroup();
    bool isInParty = group && !group->isRaidGroup();
    bool sameGroup = source && group && group->IsMember(source->GetGUID());
    bool isGroupLeader = sameGroup && group->GetLeaderGUID() == bot->GetGUID();
    bool sameGuild = source && bot->GetGuildId() && source->GetGuildId() == bot->GetGuildId();

    // 阵营/种族/职业
    bool sameFaction = source && bot->GetTeamId() == source->GetTeamId();
    bool sameRace = source && bot->getRace() == source->getRace();
    bool sameClass = source && bot->getClass() == source->getClass();

    // 位置场景
    bool inBattleground = bot->InBattleground();
    bool inArena = bot->InArena();
    bool inDungeon = bot->GetMap() && bot->GetMap()->IsDungeon();
    bool inRaidInstance = bot->GetMap() && bot->GetMap()->IsRaid();

    // 城市判断
    bool inCity = bot->GetAreaId() && area && bot->GetAreaId() == area->zone && area->flags & AREA_FLAG_CAPITAL;

    bool inCave = false;
    bool isUnderwater = bot->IsUnderWater();
    // 洞穴：区域名字包含 'cave' 或 'mine'（不区分大小写）
    if (!zoneName.empty())
    {
        std::string lower = zoneName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        inCave = (lower.find("cave") != std::string::npos || lower.find("mine") != std::string::npos);
    }

    // 状态跟踪更新
    if (inCombat) m_lastInCombat = true;
    if (!inCombat && m_lastInCombat) { m_killedRecently = true; m_lastInCombat = false; }
    if (m_killedRecently && !isDead && !inCombat)
    {
        time_t now = time(nullptr);
        if (now - m_lastActionTime > 30) m_killedRecently = false;
    }
    m_lastActionTime = time(nullptr);

    // =================================================
    // 先判断 emote 类型 -> 对应分类池，再按优先级选回复
    // 优先级: A 关系 -> B 场景 -> C 状态 -> D 兜底
    // =================================================

    // 分类 1: 问候类
    if (emote == TEXT_EMOTE_WAVE || emote == TEXT_EMOTE_GREET || emote == TEXT_EMOTE_HAIL ||
        emote == TEXT_EMOTE_HELLO || emote == TEXT_EMOTE_WELCOME || emote == TEXT_EMOTE_INTRODUCE ||
        emote == TEXT_EMOTE_SALUTE)
    {
        emoteId = EMOTE_ONESHOT_WAVE;
        textEmote = TEXT_EMOTE_HELLO;
        // A 关系
        if (isMaster)
            chosen = PickRandom("主人来啦！有什么吩咐|欢迎回来，主人|一直在等你呢，主人~");
        else if (isGroupLeader && source && source->GetGUID() == group->GetLeaderGUID())
            chosen = PickRandom("队长好！随时待命|队长来视察啦，一切正常|嗨队长，有什么计划");
        else if (sameGroup)
            chosen = PickRandom("嗨，队友！一起加油|你好呀，并肩作战|又见面了，伙伴");
        else if (sameGuild)
            chosen = PickRandom("公会战友你好|为了公会！|嗨，自家人");
        else if (sameFaction)
            chosen = PickRandom("你好，{target}|幸会幸会|嘿，{target}，近来如何");
        else
            chosen = PickRandom("你好|嗯……你好|……");
    }
    // 分类 2: 积极/赞美类
    else if (emote == TEXT_EMOTE_APPLAUD || emote == TEXT_EMOTE_CLAP || emote == TEXT_EMOTE_CONGRATULATE ||
             emote == TEXT_EMOTE_HAPPY || emote == TEXT_EMOTE_CHEER || emote == TEXT_EMOTE_VICTORY ||
             emote == TEXT_EMOTE_TOAST || emote == TEXT_EMOTE_PRAISE || emote == TEXT_EMOTE_COMMEND)
    {
        emoteId = EMOTE_ONESHOT_CHEER;
        textEmote = TEXT_EMOTE_CHEER;
        if (isMaster)
            chosen = PickRandom("主人太厉害了！|都是主人领导有方|主人威武！");
        else if (sameGroup)
            chosen = PickRandom("干得漂亮，队友|我们是最强的队伍|牛啊牛啊");
        else
            chosen = PickRandom("厉害厉害|不错不错|精彩！");
    }
    // 分类 3: 消极/否定类
    else if (emote == TEXT_EMOTE_ANGRY || emote == TEXT_EMOTE_GLARE || emote == TEXT_EMOTE_BLAME ||
             emote == TEXT_EMOTE_INSULT || emote == TEXT_EMOTE_NO || emote == TEXT_EMOTE_VETO ||
             emote == TEXT_EMOTE_DISAGREE || emote == TEXT_EMOTE_DOUBT || emote == TEXT_EMOTE_VIOLIN ||
             emote == TEXT_EMOTE_GLOAT || emote == TEXT_EMOTE_MOCK || emote == TEXT_EMOTE_TEASE ||
             emote == TEXT_EMOTE_EMBARRASS)
    {
        emoteId = EMOTE_ONESHOT_QUESTION;
        textEmote = TEXT_EMOTE_SHRUG;
        if (isMaster)
            chosen = PickRandom("主人说得对……|好的主人，我错了|都听主人的");
        else
            chosen = PickRandom("你认真的？|呵呵|关我什么事");
    }
    // 分类 4: 亲昵类
    else if (emote == TEXT_EMOTE_FLIRT || emote == TEXT_EMOTE_KISS || emote == TEXT_EMOTE_HUG ||
             emote == TEXT_EMOTE_BLUSH || emote == TEXT_EMOTE_SMILE || emote == TEXT_EMOTE_LOVE ||
             emote == TEXT_EMOTE_CUDDLE || emote == TEXT_EMOTE_PURR || emote == TEXT_EMOTE_SHIMMY ||
             emote == TEXT_EMOTE_SMIRK || emote == TEXT_EMOTE_WINK || emote == TEXT_EMOTE_SEXY ||
             emote == TEXT_EMOTE_MOAN || emote == TEXT_EMOTE_MOON || emote == TEXT_EMOTE_SHAKE ||
             emote == TEXT_EMOTE_WHISTLE)
    {
        emoteId = EMOTE_ONESHOT_SHY;
        textEmote = TEXT_EMOTE_SHY;
        if (isMaster)
            chosen = PickRandom("哎呀，主人别这样~|讨厌啦|主人你真会逗人开心");
        else
            chosen = PickRandom("呃……|你干嘛|保持距离谢谢");
    }
    // 分类 5: 情绪类（哭/恐惧/恐慌/悲伤）
    else if (emote == TEXT_EMOTE_CRY || emote == TEXT_EMOTE_BONK || emote == TEXT_EMOTE_SLAP ||
             emote == TEXT_EMOTE_COMFORT || emote == TEXT_EMOTE_SOOTHE || emote == TEXT_EMOTE_PAT ||
             emote == TEXT_EMOTE_SCARED || emote == TEXT_EMOTE_COWER || emote == TEXT_EMOTE_CRINGE ||
             emote == TEXT_EMOTE_PANIC || emote == TEXT_EMOTE_BLEED || emote == TEXT_EMOTE_MOURN ||
             emote == TEXT_EMOTE_FLOP || emote == TEXT_EMOTE_BRANDISH)
    {
        emoteId = EMOTE_ONESHOT_CRY;
        textEmote = TEXT_EMOTE_CRY;
        if (isDead)
            chosen = PickRandom("（已阵亡）|……");
        else if (lowHP)
            chosen = PickRandom("救命啊！！|奶我一口！！|要死了要死了");
        else if (m_killedRecently)
            chosen = PickRandom("刚活过来，让我缓缓|别再来了|差点就凉了……");
        else if (isMaster)
            chosen = PickRandom("主人你要保护我呀|别这样主人|呜呜呜主人");
        else
            chosen = PickRandom("呜呜呜|好痛|别这样……");
    }
    // 分类 6: 困惑/好奇类
    else if (emote == TEXT_EMOTE_CONFUSED || emote == TEXT_EMOTE_CURIOUS || emote == TEXT_EMOTE_FIDGET ||
             emote == TEXT_EMOTE_FROWN || emote == TEXT_EMOTE_SHRUG || emote == TEXT_EMOTE_SIGH ||
             emote == TEXT_EMOTE_STARE || emote == TEXT_EMOTE_TAP || emote == TEXT_EMOTE_SURPRISED ||
             emote == TEXT_EMOTE_WHINE || emote == TEXT_EMOTE_BOGGLE || emote == TEXT_EMOTE_LOST ||
             emote == TEXT_EMOTE_PONDER || emote == TEXT_EMOTE_SNUB || emote == TEXT_EMOTE_SERIOUS ||
             emote == TEXT_EMOTE_EYEBROW || emote == TEXT_EMOTE_AMAZE || emote == TEXT_EMOTE_KNEEL ||
             emote == TEXT_EMOTE_EYE || emote == TEXT_EMOTE_PEER || emote == TEXT_EMOTE_SURRENDER ||
             emote == TEXT_EMOTE_READY)
    {
        emoteId = EMOTE_ONESHOT_QUESTION;
        textEmote = TEXT_EMOTE_SHRUG;
        if (isMaster)
            chosen = PickRandom("主人有什么吩咐？|需要我做什么吗|我在听，主人");
        else
            chosen = PickRandom("？？？|啥情况|不理解……");
    }
    // 分类 7: 调皮/玩笑类
    else if (emote == TEXT_EMOTE_JOKE || emote == TEXT_EMOTE_CHICKEN || emote == TEXT_EMOTE_FART ||
             emote == TEXT_EMOTE_BURP || emote == TEXT_EMOTE_GASP || emote == TEXT_EMOTE_NOSEPICK ||
             emote == TEXT_EMOTE_SNIFF || emote == TEXT_EMOTE_STINK || emote == TEXT_EMOTE_TICKLE ||
             emote == TEXT_EMOTE_JK)
    {
        emoteId = EMOTE_ONESHOT_LAUGH;
        textEmote = TEXT_EMOTE_LAUGH;
        if (isMaster)
            chosen = PickRandom("哈哈哈哈主人你太有意思了|主人真幽默|笑死了");
        else
            chosen = PickRandom("噗哈哈|你认真的？|无聊……");
    }
    // 分类 8: 礼貌/感谢类
    else if (emote == TEXT_EMOTE_THANK || emote == TEXT_EMOTE_BOW || emote == TEXT_EMOTE_CURTSEY ||
             emote == TEXT_EMOTE_APOLOGIZE || emote == TEXT_EMOTE_AGREE || emote == TEXT_EMOTE_NOD ||
             emote == TEXT_EMOTE_GOODLUCK)
    {
        emoteId = EMOTE_ONESHOT_BOW;
        textEmote = TEXT_EMOTE_BOW;
        if (isMaster)
            chosen = PickRandom("不客气，主人|乐意为您效劳|这是我应该做的");
        else if (sameGroup)
            chosen = PickRandom("客气了|应该的|互相帮忙嘛");
        else
            chosen = PickRandom("不用谢|没关系|嗯嗯");
    }
    // 分类 9: 身体动作/状态类
    else if (emote == TEXT_EMOTE_DANCE || emote == TEXT_EMOTE_FLEX || emote == TEXT_EMOTE_SIT ||
             emote == TEXT_EMOTE_LAYDOWN || emote == TEXT_EMOTE_STAND || emote == TEXT_EMOTE_BRB ||
             emote == TEXT_EMOTE_EAT || emote == TEXT_EMOTE_DRINK || emote == TEXT_EMOTE_HUNGRY ||
             emote == TEXT_EMOTE_TIRED || emote == TEXT_EMOTE_YAWN || emote == TEXT_EMOTE_COUGH ||
             emote == TEXT_EMOTE_DROOL || emote == TEXT_EMOTE_SPIT || emote == TEXT_EMOTE_LICK ||
             emote == TEXT_EMOTE_BREATH || emote == TEXT_EMOTE_BOUNCE || emote == TEXT_EMOTE_BARK ||
             emote == TEXT_EMOTE_BEG || emote == TEXT_EMOTE_GROVEL ||
             emote == TEXT_EMOTE_PLEAD || emote == TEXT_EMOTE_BITE || emote == TEXT_EMOTE_POKE ||
             emote == TEXT_EMOTE_SCRATCH || emote == TEXT_EMOTE_BORED || emote == TEXT_EMOTE_BLINK ||
             emote == TEXT_EMOTE_CRACK || emote == TEXT_EMOTE_POINT || emote == TEXT_EMOTE_RAISE ||
             emote == TEXT_EMOTE_SHOO || emote == TEXT_EMOTE_RASP || emote == TEXT_EMOTE_COLD ||
             emote == TEXT_EMOTE_SHIVER || emote == TEXT_EMOTE_THIRSTY)
    {
        emoteId = EMOTE_ONESHOT_POINT;
        textEmote = TEXT_EMOTE_POINT;
        if (isMaster)
            chosen = PickRandom("？？|主人你干嘛|怎么啦主人");
        else
            chosen = PickRandom("……|你干啥|？？？");
    }
    // 分类 10: 战斗/威胁类
    else if (emote == TEXT_EMOTE_ROAR || emote == TEXT_EMOTE_THREATEN || emote == TEXT_EMOTE_CALM ||
             emote == TEXT_EMOTE_DUCK || emote == TEXT_EMOTE_TAUNT || emote == TEXT_EMOTE_PITY ||
             emote == TEXT_EMOTE_GROWL || emote == TEXT_EMOTE_OPENFIRE || emote == TEXT_EMOTE_ENCOURAGE ||
             emote == TEXT_EMOTE_ENEMY || emote == TEXT_EMOTE_RUDE)
    {
        emoteId = EMOTE_ONESHOT_ROAR;
        textEmote = TEXT_EMOTE_ROAR;
        if (inBattleground)
            chosen = PickRandom("冲啊！！|为了荣誉！|碾碎他们");
        else if (inArena)
            chosen = PickRandom("准备好了吗|别让我失望|干掉他们");
        else if (inDungeon || inRaidInstance)
            chosen = PickRandom("拉好仇恨！|打断打断|集中火力");
        else if (inCombat)
            chosen = PickRandom("来啊！|一起上|为了部落/联盟！");
        else if (isMaster)
            chosen = PickRandom("主人说打谁就打谁|随时准备战斗|谁敢惹主人");
        else
        {
            chosen = PickRandom("放马过来|谁怕谁|哼");
            isYell = true;
        }
    }
    // 分类 11: TALKQ / TALK / TALKEX / LISTEN
    else if (emote == TEXT_EMOTE_TALKQ || emote == TEXT_EMOTE_LISTEN ||
             emote == TEXT_EMOTE_TALK || emote == TEXT_EMOTE_TALKEX)
    {
        emoteId = EMOTE_ONESHOT_TALK;
        textEmote = TEXT_EMOTE_TALKQ;
        if (isMaster)
            chosen = PickRandom("在呢主人，请说|我在听|主人请讲");
        else if (inCity)
            chosen = PickRandom("这地方挺热闹的|{zone}的风景真不错|今天天气不错");
        else if (inDungeon || inRaidInstance)
            chosen = PickRandom("小心巡逻怪|注意ADD|控好怪，一波波打");
        else if (inBattleground)
            chosen = PickRandom("守好旗|支援中路|拿下墓地");
        else if (isUnderwater)
            chosen = PickRandom("咕噜咕噜……憋不住了|快上岸|谁能给我个水下呼吸");
        else if (inCave)
            chosen = PickRandom("这洞有点深啊|小心脚下|矿在哪呢");
        else
            chosen = PickRandom("嗯|啥事|说呗");
    }
    // 默认：未匹配的 emote 不做反应
    else
    {
        return false;
    }

    // 统一替换占位符
    chosen = ReplacePlaceholders(chosen, srcName, zoneId, zoneName);

    // === 转身 + 输出 ===
    if (source && !bot->isMoving() && !bot->HasInArc(static_cast<float>(M_PI), source, sPlayerbotAIConfig.farDistance))
        ServerFacade::instance().SetFacingTo(bot, source);

    if (verbal && !chosen.empty())
    {
        if (isYell)
            bot->Yell(chosen, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
        else
            bot->Say(chosen, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));

        LOG_INFO("playerbots", "bot={} source={} emote={} type=text text=\"{}\"",
            EscapeFmt(bot->GetName()), EscapeFmt(srcName), emote, EscapeFmt(chosen));
    }

    if (textEmote)
    {
        WorldPacket data(SMSG_TEXT_EMOTE);
        data << textEmote;
        data << GetNumberOfEmoteVariants((TextEmotes)textEmote, bot->getRace(), bot->getGender());
        data << ((source && urand(0, 1)) ? source->GetGUID() : ObjectGuid::Empty);
        bot->GetSession()->HandleTextEmoteOpcode(data);
    }
    else
    {
        if (emoteId)
            bot->HandleEmoteCommand(emoteId);
    }

    return true;
}

bool EmoteAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    uint32 emote = 0;

    Player* pSource = nullptr;
    bool isReact = false;
    if (!p.empty() && p.GetOpcode() == SMSG_TEXT_EMOTE)
    {
        isReact = true;
        ObjectGuid source;
        uint32 text_emote;
        uint32 emote_num;
        uint32 namlen;
        std::string nam;
        p.rpos(0);
        p >> source >> text_emote >> emote_num >> namlen;
        if (namlen > 1)
            p >> nam;

        pSource = ObjectAccessor::FindPlayer(source);
        if (pSource && (pSource->GetGUID() != bot->GetGUID()) &&
            ((urand(0, 1) && bot->HasInArc(static_cast<float>(M_PI), pSource, 10.0f)) ||
             (namlen > 1 && strstri(bot->GetName().c_str(), nam.c_str()))))
        {
            /*LOG_INFO("playerbots", "Bot {} {}:{} <{}> received SMSG_TEXT_EMOTE {} from player {} <{}>",
                bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
                bot->GetName(), text_emote, pSource->GetGUID().ToString().c_str(), pSource->GetName());*/

            emote = text_emote;
        }
    }

    if (!p.empty() && p.GetOpcode() == SMSG_EMOTE)
    {
        isReact = true;
        ObjectGuid source;
        uint32 emoteId;
        p.rpos(0);
        p >> emoteId >> source;

        pSource = ObjectAccessor::FindPlayer(source);
        if (pSource && pSource != bot && ServerFacade::instance().GetDistance2d(bot, pSource) < sPlayerbotAIConfig.farDistance &&
            emoteId != EMOTE_ONESHOT_NONE)
        {
            if ((pSource->GetGUID() != bot->GetGUID()) &&
                (pSource->GetTarget() == bot->GetGUID() ||
                 (urand(0, 1) && bot->HasInArc(static_cast<float>(M_PI), pSource, 10.0f))))
            {
                /*LOG_INFO("playerbots", "Bot {} {}:{} <{}> received SMSG_EMOTE {} from player {} <{}>",
                    bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
                   bot->GetName(), emoteId, pSource->GetGUID().ToString().c_str(), pSource->GetName());*/

                std::vector<uint32> types;
                for (int32 i = sEmotesTextStore.GetNumRows(); i >= 0; --i)
                {
                    EmotesTextEntry const* em = sEmotesTextStore.LookupEntry(uint32(i));
                    if (!em)
                        continue;

                    if (em->textid == EMOTE_ONESHOT_TALK)
                        continue;

                    if (em->textid == EMOTE_ONESHOT_QUESTION)
                        continue;

                    if (em->textid == EMOTE_ONESHOT_EXCLAMATION)
                        continue;

                    if (em->textid == emoteId)
                    {
                        types.push_back(em->Id);
                    }
                }

                if (types.size())
                    emote = types[urand(0, types.size() - 1)];
            }
        }
    }

    if (isReact && !emote)
        return false;

    std::string param = event.getParam();
    if ((!isReact && param.empty()) || emote)
    {
        // time_t lastEmote = AI_VALUE2(time_t, "last emote", qualifier); //not used, line marked for removal.
        botAI->GetAiObjectContext()
            ->GetValue<time_t>("last emote", qualifier)
            ->Set(time(nullptr) + urand(1000, sPlayerbotAIConfig.repeatDelay) / 1000);
        param = qualifier;
    }

    if (emote)
        return ReceiveEmote(pSource, emote, bot->InBattleground() ? false : urand(0, 1));

    if (param.find("sound") == 0)
    {
        return botAI->PlaySound(atoi(param.substr(5).c_str()));
    }

    if (!param.empty() && textEmotes.find(param) != textEmotes.end())
    {
        WorldPacket data(SMSG_TEXT_EMOTE);
        data << textEmotes[param];
        data << GetNumberOfEmoteVariants((TextEmotes)textEmotes[param], bot->getRace(), bot->getGender());
        data << ((bot->GetTarget() && urand(0, 1)) ? bot->GetTarget() : ObjectGuid::Empty);
        bot->GetSession()->HandleTextEmoteOpcode(data);
        return true;
    }

    if (param.empty() || emotes.find(param) == emotes.end())
    {
        uint32 index = rand() % emotes.size();
        for (std::map<std::string, uint32>::iterator i = emotes.begin(); i != emotes.end() && index; ++i, --index)
            emote = i->second;
    }
    else
    {
        emote = emotes[param];
    }

    if (param.find("text") == 0)
    {
        emote = atoi(param.substr(4).c_str());
    }

    return Emote(GetTarget(), emote);
}

bool EmoteAction::isUseful()
{
    if (!botAI->AllowActivity())
        return false;

    time_t lastEmote = AI_VALUE2(time_t, "last emote", qualifier);
    return time(nullptr) >= lastEmote;
}

bool TalkAction::Execute(Event /*event*/)
{
    Unit* target = botAI->GetUnit(AI_VALUE(ObjectGuid, "talk target"));
    if (!target)
        target = GetTarget();

    if (!urand(0, 100))
    {
        target = nullptr;
        context->GetValue<ObjectGuid>("talk target")->Set(ObjectGuid::Empty);
        return true;
    }

    if (target)
    {
        if (Player* player = dynamic_cast<Player*>(target))
            if (PlayerbotAI* playerBotAI = GET_PLAYERBOT_AI(player))
                playerBotAI->GetAiObjectContext()->GetValue<ObjectGuid>("talk target")->Set(bot->GetGUID());

        context->GetValue<ObjectGuid>("talk target")->Set(target->GetGUID());
        return Emote(target, GetRandomEmote(target, true), true);
    }

    return false;
}

uint32 TalkAction::GetRandomEmote(Unit* unit, bool textEmote)
{
    std::vector<uint32> types;
    if (textEmote)
    {
        if (!urand(0, 20))
        {
            // expressions
            types.push_back(TEXT_EMOTE_BOW);
            types.push_back(TEXT_EMOTE_RUDE);
            types.push_back(TEXT_EMOTE_CRY);
            types.push_back(TEXT_EMOTE_LAUGH);
            types.push_back(TEXT_EMOTE_POINT);
            types.push_back(TEXT_EMOTE_CHEER);
            types.push_back(TEXT_EMOTE_SHY);
            types.push_back(TEXT_EMOTE_JOKE);
        }
        else
        {
            // talk
            types.push_back(TEXT_EMOTE_TALK);
            types.push_back(TEXT_EMOTE_TALKEX);
            types.push_back(TEXT_EMOTE_TALKQ);

            if (unit && (unit->HasNpcFlag(UNIT_NPC_FLAG_TRAINER) ||
                         unit->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER)))
            {
                types.push_back(TEXT_EMOTE_SALUTE);
            }
        }
        return types[urand(0, types.size() - 1)];
    }

    if (!urand(0, 20))
    {
        // expressions
        types.push_back(EMOTE_ONESHOT_BOW);
        types.push_back(EMOTE_ONESHOT_RUDE);
        types.push_back(EMOTE_ONESHOT_CRY);
        types.push_back(EMOTE_ONESHOT_LAUGH);
        types.push_back(EMOTE_ONESHOT_POINT);
        types.push_back(EMOTE_ONESHOT_CHEER);
        types.push_back(EMOTE_ONESHOT_SHY);
    }
    else
    {
        // talk
        types.push_back(EMOTE_ONESHOT_TALK);
        types.push_back(EMOTE_ONESHOT_EXCLAMATION);
        types.push_back(EMOTE_ONESHOT_QUESTION);

        if (unit && (unit->HasNpcFlag(UNIT_NPC_FLAG_TRAINER) ||
                     unit->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER)))
        {
            types.push_back(EMOTE_ONESHOT_SALUTE);
        }
    }

    return types[urand(0, types.size() - 1)];
}

uint32 EmoteActionBase::GetNumberOfEmoteVariants(TextEmotes emote, uint8 Race, uint8 Gender)
{
    if (emote == 304)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_UNDEAD_PLAYER:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 1;

                return 1;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 3;
            }
        }
    }
    else if (emote == 305)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_UNDEAD_PLAYER:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 3;
            }
            case RACE_GNOME:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == 306)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_HELLO)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 4;
            }
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == 323)
    {
        return 2;
    }
    else if (emote == 324)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 1;
            }
        }
    }
    else if (emote == 325)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
        }
    }
    else if (emote == 326)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_CHEER)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
        }
    }
    else if (emote == TEXT_EMOTE_OPENFIRE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            case RACE_GNOME:
            {
                return 2;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_BYE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 4;
            }
        }
    }
    else if (emote == TEXT_EMOTE_NOD)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 4;
            }
        }
    }
    else if (emote == TEXT_EMOTE_NO)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_THANK)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 4;
            }
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_WELCOME)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;
                return 3;
            }
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 2;
                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_CONGRATULATE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 4;
            }
            case RACE_GNOME:
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 4;
            }
        }
    }
    else if (emote == TEXT_EMOTE_FLIRT)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            {
                if (Gender == GENDER_MALE)
                    return 6;
                return 3;
            }
            case RACE_DWARF:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 6;

                return 5;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 4;
            }
            case RACE_GNOME:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 5;
            }
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 6;

                return 6;
            }
        }
    }
    else if (emote == TEXT_EMOTE_JOKE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 6;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 6;

                return 5;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 7;

                return 4;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 3;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 5;
            }
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 4;
            }
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 7;
            }
        }
    }

    return 1;
}
