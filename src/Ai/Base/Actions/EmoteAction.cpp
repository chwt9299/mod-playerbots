/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "EmoteAction.h"

#include <algorithm>
#include <functional>

#include "Event.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include <ctime>
#include <deque>
#include <unordered_map>

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
    // {target} -> 目标玩家名字（浅蓝 |cFF40C0FF）
    if (!targetName.empty())
    {
        size_t pos = 0;
        while ((pos = result.find("{target}", pos)) != std::string::npos)
        {
            std::string colored = "|cFF40C0FF" + targetName + "|r";
            result.replace(pos, 8, colored);
            pos += colored.size();
        }
    }
    // {zone} -> 区域名字（浅绿 |cFF20FF20）
    if (!zoneName.empty())
    {
        size_t pos = 0;
        while ((pos = result.find("{zone}", pos)) != std::string::npos)
        {
            std::string colored = "|cFF20FF20" + zoneName + "|r";
            result.replace(pos, 6, colored);
            pos += colored.size();
        }
    }
    return result;
}

// zoneType 映射：17 个 zoneId → 6 类大区
// 1=森林 2=平原 3=雪地 4=沙漠 5=沼泽 6=海岸
static int GetZoneType(uint32 zoneId)
{
    switch (zoneId)
    {
        case 12:
        case 331:
        case 10:
        case 130:
        case 361:
            return 1;
        case 14:
        case 17:
        case 40:
        case 44:
        case 215:
            return 2;
        case 1:
        case 38:
        case 618:
            return 3;
        case 440:
        case 400:
        case 1377:
            return 4;
        case 51:
        case 357:
        case 490:
            return 5;
        case 16:
        case 33:
        case 85:
        case 405:
            return 6;
        default:
            return 0;  // default outdoor pool
    }
}

static std::string GetTimeOfDayDesc()
{
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    int hour = lt->tm_hour;

    static const char* timePools[][13] = {
        /* 清晨 4-7 */  { "天刚蒙蒙亮，雾气还没散", "晨露打湿了我的靴子", "早起的人有收获，不是吗", "太阳刚刚露出地平线", "空气真新鲜，清晨最好了", "鸟都还没起床呢", "第一缕阳光总是最美的", "又是一个新开始", "清晨的风让人精神一振", "趁还没热，赶紧赶路", "湖面上的薄雾真好看", "晨光从树叶间漏下来", "远处鸡鸣三声，新的一天" },
        /* 上午 7-12 */ { "早上好旅行者，今天精力充沛吗", "太阳升起来，暖和多了", "上午是干活的最好时间", "阳光还不算刺眼", "精力充沛，今天争取多走一段", "早上的集市刚开张", "这个时间最适合赶路", "上午的光线真舒服", "今天任务排满了", "上午的猎物最多", "趁着凉快把活干了", "太阳越升越高，影子越来越短", "上午的风还带着昨晚的凉意" },
        /* 下午 12-18 */{ "太阳正当头，晒得人发晕", "走到下午腿都酸了", "中午吃太多了有点困", "这日头可真毒", "下午的时光最慵懒", "该找个阴凉地方打个盹", "下午的猎物不好找", "热得我盔甲都不想穿了", "下午的阳光让一切都慢下来", "走了半天了，该歇歇了", "真想来杯凉水", "下午蝉鸣得最凶", "树荫下打盹是最幸福的事" },
        /* 黄昏 18-21 */{ "天快黑了，得找个地方扎营", "黄昏的余光真美", "一天又要结束了", "该准备篝火了", "趁着还有光再走一段", "黄昏是狼群出没的时间，小心", "夕阳把天边染成了金色", "又是忙碌的一天", "晚风开始吹起来了", "满天霞光，明天该是个好天气", "我喜欢黄昏的宁静", "远处的钟声在黄昏里回荡", "天色暗得很快，抓紧吧" },
        /* 深夜 21-4 */ { "好黑……星星真多", "该守夜了，你先睡吧", "深夜的荒野不太平", "篝火烧得噼啪响", "月亮又圆又亮", "夜里什么都看不清", "听到远处的狼嚎了吗", "冷……靠近篝火一点", "夜里的森林像另一个世界", "星空下思考人生", "这个点还不睡会倒霉的", "猫头鹰在叫，夜越来越深", "月光把树枝的影子投在地上" },
    };

    int idx = 0;
    if (hour >= 4 && hour < 7)      idx = 0;  // 清晨
    else if (hour >= 7 && hour < 12) idx = 1;  // 上午
    else if (hour >= 12 && hour < 18) idx = 2; // 下午
    else if (hour >= 18 && hour < 21) idx = 3; // 黄昏
    else                              idx = 4; // 深夜

    int count = sizeof(timePools[idx]) / sizeof(timePools[idx][0]);
    return timePools[idx][urand(0, count - 1)];
}

static std::string GetWeatherDesc(Player* bot)
{
    if (!bot) return "";
    uint32 zoneType = GetZoneType(bot->GetZoneId());

    // 雪地/沙漠/沼泽气候固定，森林/平原/海岸按时间随机
    switch (zoneType)
    {
        case 3: return "雪花一片片飘下来，落在肩上";
        case 4: return "风沙扑面，看不清远处的路";
        case 5: return "雾气浓得伸手不见五指";
    }

    // 森林/平原/海岸：根据游戏内时间随机
    time_t rawtime = time(nullptr);
    struct tm* ti = localtime(&rawtime);
    int hour = ti->tm_hour;

    // 夜间或清晨更可能下雨/雾
    uint32 roll = urand(0, 99);
    if (hour >= 22 || hour < 6)
    {
        if (roll < 20) return "夜里下起了小雨，淅淅沥沥";
        if (roll < 35) return "夜雾笼罩了整片林子";
        return "夜空清澈，星光明亮";
    }
    if (roll < 10) return "天边飘来几朵乌云，怕是要下雨了";
    if (roll < 20) return "薄雾在山谷间游荡";
    return "阳光正好，微风轻拂";
}

static std::string GetProfessionFlavor(Player* bot)
{
    if (!bot) return "";

    static const uint32 profSkills[] = { 164, 171, 182, 186, 197, 202, 333, 356, 393, 755, 773 };
    static const char* profPools[11][5] = {
        { "锤子都快敲断了", "这把剑的淬火还差一步", "锻造需要耐心和力量", "矿石不够了，出去采点", "这把武器一定能卖出好价" },
        { "材料快用完了……", "这瓶药水能顶一阵子", "炼金术需要精准，不能分心", "配方上写的是什么来着", "草药配比还差一点点" },
        { "等等，那边是不是宁神花", "采药要趁早晨，药效最好", "这片地的草药品质不错", "药篓快装满了", "踩到什么了……哦，是梦叶草" },
        { "这股矿脉味……附近有富矿", "矿镐在手，天下我有", "听说这山里有秘银", "采矿让我手臂越来越壮了", "敲敲打打，矿石掉一地" },
        { "线不够了……", "这件袍子的针脚还不错", "裁缝不光是手艺，更是艺术", "丝线缠成一团了，烦", "这件披风应该能卖好价钱" },
        { "我做了个新玩意儿，虽然偶尔会炸", "扳手呢？扳手去哪了", "工程学改变世界，一次一炸", "螺丝松了，得紧一紧", "下一件发明一定能成功" },
        { "你的护腕看起来需要附个魔", "附魔材料又用完了", "魔力在指尖流淌的感觉真好", "这块符文刻得不错", "附魔能让装备脱胎换骨" },
        { "这水里有大鱼，我敢肯定", "钓鱼是最好的冥想", "浮漂动了——哦，是风", "今天能钓上什么好东西呢", "鱼竿是我的第三只手" },
        { "这地方的野兽皮质量还行", "剥皮刀钝了，回去磨磨", "皮子在背包装不下了", "这头野兽的皮能卖好价钱", "爪子收好，炼金能用" },
        { "这颗宝石的成色不错", "珠宝是地底的眼泪", "切割宝石需要稳定的手", "这枚戒指配红宝石最好", "路上捡的石头品质不行" },
        { "我的墨水要干了", "这张卷轴的字迹要工整", "铭文是把力量刻进文字", "羊皮纸不多了", "这符文组合威力不小" },
    };

    // Fisher-Yates shuffle to randomize skill order
    uint32 temp[11];
    memcpy(temp, profSkills, sizeof(profSkills));
    for (int i = 10; i > 0; --i)
    {
        int j = urand(0, i);
        std::swap(temp[i], temp[j]);
    }

    for (int i = 0; i < 11; ++i)
    {
        if (bot->HasSkill(temp[i]))
            return profPools[i][urand(0, 4)];
    }
    return "";
}

static std::string GetSourceFlavor(Player* source)
{
    if (!source) return "";

    uint8 sClass = source->getClass();
    uint8 sRace  = source->getRace();
    Gender gender = static_cast<Gender>(source->getGender());

    // source 职业池（10 职业 + DK）
    static const uint32 classIds[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 11 };
    static const char* classPools[10][12] = {
        { "看你这一身肌肉，能扛住三只怪不", "战士的怒吼真提气", "冲在最前面的永远是你", "冲锋的时候姿势帅呆了", "这股蛮力……你是战士吧", "盾牌举得稳，队伍才站得住", "你的武器磨得真亮", "这身板一看就是冲锋的料", "战场上的怒吼让我热血沸腾", "你手里的武器够重吧", "听说战士的剑下没有逃兵", "战斗姿态摆出来就气势十足" },
        { "法师大人，给我开个暴风城门呗", "这火球术真够炫的", "法师身边永远凉飕飕的", "变个羊给我看看？", "传送门什么时候开", "法术书那么厚，你都背完了吗", "这奥术光辉真耀眼", "暴风雪一放，敌人全趴下了", "法师的智慧比剑还锋利", "闪现用得真利索", "你的法杖上刻着什么符文", "听说肯瑞托又在招人了" },
        { "你那宠物吃的比我还好", "猎人永远不缺朋友（宠物）", "箭射得真准", "你的陷阱差点绊到我", "野兽之眼看到了什么", "这弓弦声听着就带劲", "驯服的野兽比你还能打", "追踪术从来不会骗人", "鹰眼术看到什么了给我说说", "你的箭囊总是满满的", "宠物的忠诚让人羡慕", "听说猎人在野外从不迷路" },
        { "你的手最好别靠近我的口袋", "潜行的时候能不能提醒一下", "刀还没擦干净，上面还有血", "绕到背后去，我吸引注意", "开锁的时候别让人看见", "又被你吓了一跳……从哪冒出来的", "这匕首涂了毒吗", "潜行的时候别踩到树枝", "你的动作真利落", "背刺那一刀看着就痛", "毒药瓶叮当作响", "开锁手艺哪里学的" },
        { "给我来个耐力！加完血你就是恩人", "暗影形态下你的声音都变深沉了", "有人在怀疑你是暗牧", "神圣的光芒，温暖又治愈", "鞭子放下，有话好好说", "你的祷文听着让人安心", "圣光在你手里格外温暖", "暗影和圣光你都掌握了吗", "恢复术挂在身上真舒服", "这口奶加得及时", "暗言术的痛……想想都怕", "听说牧师见过凡人看不到的东西" },
        { "变个熊让我骑会儿……开玩笑的", "树皮术太实用了", "你是丛林的一部分了", "旅行形态跑得真快", "德鲁伊真的是大自然的宠儿", "能飞能跑能游，真羡慕", "月火术挂上去了", "塞纳里奥的守护者吗", "你的羽毛比鸟还轻", "治疗之触真温暖", "平衡之力在你手中流转", "你身上有森林的气息" },
        { "插根图腾暖暖身子", "大地之灵在回应你吗", "你喊闪电箭的时候真酷", "嗜血！快开嗜血！", "元素在跟着你跑", "先祖之魂在护佑你吗", "你的图腾立起来像个小营地", "熔岩爆裂砸下去真解气", "这个治疗链弹得漂亮", "跟元素沟通是什么感觉", "风怒的加持让人羡慕", "你的鼓点驱散了疲惫" },
        { "圣光照死对面的不死族", "你身上怎么老在发光", "圣盾一开，所向无敌", "你的锤子砸下来真响", "圣疗留好，关键时刻有大用", "你的光环让队伍更有底气", "这把灰烬使者……只是传说吧", "祝福给我来一个", "王者祝福永远不嫌多", "惩戒之力不容小觑", "圣光道标照亮了前方", "为了圣光……也为了艾泽拉斯" },
        { "你的小鬼又在偷看我", "灵魂石这玩意真是好东西", "被死亡缠绕打到可不是闹着玩的", "恐惧术放得好，敌人全跑了", "暗影之力……让人后背发凉", "你的恶魔仆从真听话", "腐蚀术挂上就跑", "暗影箭的轨迹真好看", "召唤仪式需要帮忙吗", "你的双眼透露着邪能的光芒", "听说术士和恶魔做了交易", "地狱火落下来时天都黑了" },
        { "你身上这股寒气……离我远点", "死亡骑士……你走过的地方草都枯了", "警惕你的符文剑", "死亡之握不好受", "从天灾回到生命这边……不容易吧", "你的符文剑在低语什么", "凛风冲击冻住了一片", "亡灵大军一开，场面壮观", "凋零缠绕精准命中", "黑锋骑士团来得正好", "你的身世……是秘密吧", "冰霜之路能在水上走？" },
    };

    // source 种族池（8 种族）
    static const uint32 raceIds[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    static const char* racePools[8][12] = {
        { "暴风城来的？带点奶酪没", "你们人类的领地越来越大了", "人类总是成群结队", "你的剑……是暴风城铁匠铺打的吧", "冒险精神就是人类最大的优点", "暴风城的旗帜在哪里都能看到", "人类的坚韧永不服输", "洛丹伦的子民还在", "联盟的心脏在暴风城跳动", "国王的演说让人振奋", "你的血脉里流淌着勇气", "听说人类的情感最丰富" },
        { "Lok'tar！兄弟", "你的战吼能让敌人胆寒", "兽人的荣誉，从未褪色", "绿色皮肤是荣耀的象征", "来，比比谁的斧头更大", "霜狼氏族的战旗永远不倒", "部落需要你这样的勇士", "兽人的脊梁比钢铁还硬", "你的伤疤都是荣耀的证明", "战歌在血液里流淌", "来自纳格兰的风还吹着吗", "你的斧刃闪着我喜欢的光" },
        { "来杯麦酒？我请", "矮人朋友，地下城缺人吗", "你这胡子……怎么编的", "铁炉堡的山风吹到这儿了", "矮人的烈酒能点燃血液", "探险者协会又发现了什么", "你的火枪擦得真亮", "矮人的锻造术是祖传的", "山丘之王的后裔总不会错", "遇到矮人就是遇到了朋友", "你的战锤挥舞得虎虎生风", "铜须家族的声望响彻艾泽拉斯" },
        { "愿艾露恩指引你的道路", "暗夜精灵……你们的寿命真让人羡慕", "你走路怎么没声", "月神之泪在发光", "暗夜精灵在月光下最自在", "泰达希尔的树冠遮天蔽日", "哨兵部队的箭从不落空", "你的身上有千年的智慧", "月光指引着你，也指引着我", "影遁藏得真隐蔽", "远古守护者还在沉睡", "森林在你的脚步声里低语" },
        { "呃，你还有肉吗（指正常食物）", "亡灵真的不睡觉吗", "你的骨头……咔啦咔啦响", "亡灵身后总是跟着阴影", "从坟墓里回来的战士，敬你一杯", "被遗忘者从不被遗忘", "幽暗城的黑暗是你的庇护", "你走路真的没有声音", "你的意志比活人还坚定", "黑暗游侠的传说还在流传", "被遗忘者的忠诚从不廉价", "你的眼神里……藏着什么故事" },
        { "愿大地母亲护佑你", "牛头人走路，地都在抖", "雷霆崖的风吹到了这里", "你的图腾纹在哪", "牛头人的耐心像山一样", "你的猎角声回荡在平原上", "莫高雷的草原是你的故乡", "牛头人的智慧像大地一样厚重", "血蹄部族的荣耀没有褪色", "你的步伐沉稳得像山岳", "凯恩的教诲还在耳边", "你的身躯挡住了多少风雨" },
        { "哈哈小不点，今天发明了什么", "侏儒的头脑比奥术还有力量", "你的高度刚好能钻过通风口", "侏儒科技，改变艾泽拉斯", "别从背后靠近，我看不到你", "你的发明……不会炸吧", "诺莫瑞根迟早要收复的", "大工匠的传人就是你吗", "你的机械宠物还挺可爱", "小个子也能撬动世界", "你的扳手别在我的盔甲上划", "侏儒的齿轮从不停止转动" },
        { "嘿 mon！有什么好事", "巨魔的舞步没人比得了", "你的獠牙真酷", "古拉巴什的血脉还在跳动", "听说你吃人……只是听说", "暗矛部族的勇士总是精神抖擞", "你的药剂味道挺冲的", "巨魔的巫术深不可测", "回音群岛的海风带来你的消息", "你的长矛投得真远", "赞达拉的荣光在你肩上", "你的猎豹跑起来像风一样" },
    };

    // RACE_UNDEAD_PLAYER alias fix
    uint8 raceCheck = sRace;
    if (raceCheck == RACE_UNDEAD_PLAYER)
        raceCheck = 5;  // normalize to RACE_UNDEAD for pool index

    std::vector<std::string> candidates;

    for (int i = 0; i < 10; ++i)
    {
        if (sClass == classIds[i])
        {
            for (int j = 0; j < 12; ++j)
            {
                std::string entry = classPools[i][j];
                // 性别适配：战士第1条
                if (i == 0 && j == 0 && gender == GENDER_FEMALE)
                    entry = "冲在最前面的永远是你";
                candidates.push_back(entry);
            }
            break;
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        if (raceCheck == raceIds[i])
        {
            for (int j = 0; j < 12; ++j)
            {
                std::string entry = racePools[i][j];
                // 性别适配：兽人第1条
                if (i == 1 && j == 0 && gender == GENDER_FEMALE)
                    entry = "Lok'tar！战友";
                // 性别适配：矮人第3条
                if (i == 2 && j == 2 && gender == GENDER_FEMALE)
                    entry = "铁炉堡的山风吹到这儿了";
                candidates.push_back(entry);
            }
            break;
        }
    }

    if (candidates.empty()) return "";
    return candidates[urand(0, candidates.size() - 1)];
}

static std::string GetHpFlavor(Player* bot)
{
    if (!bot) return "";

    float pct = bot->GetHealthPct();

    static const char* hpPools[4][4] = {
        { "状态不错，随时待命", "精力充沛，能打一整天", "满血满状态，稳得很", "稳如磐石，百毒不侵" },
        { "受了点伤，不过无大碍", "小伤不影响战斗", "皮外伤，问题不大", "还能撑一会儿，别担心" },
        { "快给我加血……", "这血量有点危险", "再挨一下可能就倒了", "治疗！治疗在哪儿" },
        { "要死了要死了！！", "快！奶我一口！！", "血量见底……撑不住了", "再不奶我就要躺了" },
    };

    int idx = 0;
    if (pct > 80.0f)            idx = 0;
    else if (pct > 50.0f)      idx = 1;
    else if (pct > 20.0f)      idx = 2;
    else                       idx = 3;

    return hpPools[idx][urand(0, 3)];
}

static std::string GetRumorFlavor(uint32 zoneType)
{
    if (zoneType < 1 || zoneType > 6) return "";

    static const char* rumorPools[6][12] = {
        { "听说荆棘谷有个失踪的海盗宝藏", "暮色森林的乌鸦最近特别多", "银松森林的狼人又多了几个", "费伍德的萨特在密谋什么", "灰谷的精灵最近不太友善", "森林里有只白鹿，看到的人都会交好运", "有猎人说在林子里看到了龙", "古树在低语，谁听懂了", "蘑菇圈是妖精的舞池，别踩", "森林里住着一位隐士", "昨晚林子里有绿光在闪", "据说这片林子的树会走路" },
        { "听说贫瘠之地的钢鬃野猪人又在搞事了", "十字路口的补给车队被劫了", "莫高雷的灵魂之泉最近不太平静", "西部荒野的迪菲亚要卷土重来", "赤脊山那边有个黑龙的传说", "平原上的斑马群在迁徙", "剃刀岭出了个了不起的新兵", "有人看到了半人马在集结", "草原上的日落会让人流泪……传说而已", "试炼谷深处藏着古老的试炼武器", "野猪人山洞里有宝藏", "草原上有个游荡的科多兽商队" },
        { "传说冬泉谷深处有永冻的财宝", "铁炉堡的矿工挖通了新矿脉", "洛克莫丹的水坝下面有东西在动", "冬天会有雪人下山，真的", "丹莫罗的矮人酿了新酒", "冰雪女王的故事……都是吓小孩的", "奥达曼下面有沉睡的泰坦造物", "暴风雪之夜出现了幽灵旅队", "冻住的湖里冻着一只龙", "听说冬泉火酒就是用永冻之地的火焰酿的", "雪地里发现了古老的符文", "有人说山顶上有冰霜巨人的足迹" },
        { "塔纳利斯的废墟下埋着巨魔帝国", "沙子里挖出了虫卵，当心", "加基森的拍卖行有人卖假货", "传说流沙之战时这里死了上万人", "千针石林下面真的有千根石针", "希利苏斯的暮光教徒在找什么", "沙漠里有一个永远不干涸的泉眼", "铜龙说有一种沙漏能让人回到过去", "有人看见时光龙飞过沙漠", "废旧渡口那边有幽灵船出没", "沙暴中隐约看到了巨龙的骨架", "沙丘下面埋着一座完整的神庙" },
        { "悲伤沼泽里有一座消失的神庙", "诅咒之地是兽人第一次踏进的地方", "安戈洛的水晶能用来做顶级附魔", "沼泽里有一种花，闻了会做梦", "沼泽深处住着一个老巫婆", "黑暗之门附近总有恶魔在徘徊", "有人说安戈洛是泰坦的实验场", "沼泽里的雾气会让人迷路到死", "泥沼里埋着上古之战的遗物", "失落的神庙里藏着阿塔哈卡的血", "腐败之水中似乎有什么在沉睡", "沼泽里有一种会发光的蘑菇" },
        { "艾萨拉的海底有娜迦的宫殿", "荆棘谷的海盗藏宝图在黑市上卖", "菲拉斯的遗迹里有一扇打不开的门", "提瑞斯法海岸的渔夫捞上来过骷髅", "传说大海深处有沉没的城市", "蓝龙军团为什么守着艾萨拉", "海妖的歌声……能让人发疯", "双子皇帝的石碑刻着远古诅咒", "有人在海岸看到过美人鱼？胡扯", "海盗们说荆棘谷有座骷髅岛", "满月之夜，海岸边传来鳍人的战歌", "退潮时能看到海底的古精灵雕像" },
    };

    return rumorPools[zoneType - 1][urand(0, 11)];
}

// 同队共享冷却（基于 group GUID）
static std::unordered_map<ObjectGuid, time_t> emoteCooldowns;

static std::string BuildOutdoorFlavor(Player* bot, Player* source, uint32 zoneId, uint32 mapId)
{
    int zoneType = GetZoneType(zoneId);

    // zone 主题池
    static const char* zonePools[6][30] = {
        { "林子里有动静……小心点", "这些古树比联盟部落还老", "树冠密得看不到天", "闻到了吗？松针和泥土的味道", "这边的小径通向哪来着", "林间的光斑真好看", "嘘……听到猫头鹰叫了吗", "蜘蛛网太多了，烦死了", "枯叶踩着哗哗响", "这棵树得有上千年了吧", "树下有蘑菇，能吃吗", "林间空地……适合扎营", "树枝上挂着什么东西", "别走散了，林子深", "松涛声真舒服", "这林子让我有点发毛", "暗影在树间摇曳……", "腐化的气息弥漫在空气中", "萨特在暗处窥视着我们", "碧火在树梢间闪烁", "被诅咒的树林永不安宁", "这里曾是精灵的圣地", "银松的暗影永无止境", "狼人的嚎叫还回荡在耳边", "阳光穿过树叶，在地上画着斑驳的影子", "这片林子里的鸟叫真好听", "小径蜿蜒，不知通向何方", "树下的野花开得正好", "松鼠从这棵树跳到那棵树", "雨后林子里的蘑菇都冒出来了" },
        { "烈日当头，连风都是热的", "草原一望无际，让人心潮澎湃", "这片土地很适合远征", "野猪人又闹事了", "十字路口的商队到了吗", "贫瘠之地的日落是最美的", "真想在草原上跑一跑", "看着天际线，觉得自己很渺小", "麦田里的稻草人……有点诡异", "迪菲亚盗贼真是烦人", "湖畔镇的鱼还不错", "山那边的黑石兽人不太安分", "在草原上深呼吸……舒服", "斑马群跑过去了", "这道路修得还算平整", "金棘草的刺扎人真疼", "兽人的战鼓声在回荡", "雷霆崖在远处若隐若现", "莫高雷的风永远那么温柔", "这片平原，是战士的故乡", "半人马的蹄声从远方传来", "贫瘠之地的水源就是生命", "西部荒野的风车还在转吗", "赤脊山的石堡巍然耸立", "远处的地平线上有炊烟", "风吹草低，能看见远处的兽群", "这片平原上曾经发生过大战……看那些遗迹", "牧草长到膝盖了，走起来费劲", "平原上的雷声比城里响十倍", "雨后的大地冒着泥土的芬芳" },
        { "冻死我了……来杯麦酒暖暖身子", "雪地里踩下去咯吱咯吱的", "丹莫罗的雪从不融化", "铁炉堡的锻炉声还听得见吗", "雪下得太大，脚印都被盖了", "冬泉谷的枭兽又在嚎了", "这地方冷得连火都打不着", "雪崩了吗？刚才听到了轰隆声", "洛克莫丹的水坝真壮观", "穴居人又来捣乱了，烦", "雪地里看到熊的脚印", "再往前走就是冰川了", "冻土硬得像石头", "奇美拉的尖叫划破天际", "这辈子最讨厌的就是暴风雪", "来口矮人烈酒就好了", "雪反射的阳光刺眼得很", "这温度能冻掉耳朵", "快走吧，风越来越大了", "冰面上走路小心点", "永冻之地的传说代代相传", "铁炉堡的灯火在风雪中闪烁", "雪人毛皮能卖个好价钱", "丹莫罗的麦酒是全艾泽拉斯最好的", "呼出的气都变成了白雾", "远处的雪山在阳光下闪着金光", "冰柱挂在悬崖边上，晶莹剔透", "暴风雪过后，世界一片洁白", "水面上结了一层透明的冰", "寒风吹过山谷，像哨子一样响" },
        { "呸，满嘴沙子", "这鬼地方连水都找不到", "沙漠里的夜晚冷得要死", "影子的方向……那边是东吧", "加基森的地精商人就是一群骗子", "风蛇在空中盘旋，当心", "铜龙在时之穴守着什么", "千针石林的峡谷太壮观了", "徒步穿越千针真是要命", "沙漠里的水比金子还贵", "前面有绿洲吗？渴死了", "虫群的嗡鸣声越来越大", "其拉虫人在沙下蠢蠢欲动", "暮光教徒的营地就在不远处", "希利苏斯的风……带着塑像的低语", "甲虫之锣还没敲响", "沙暴又来了，快找掩体", "废墟里或许埋着什么好东西", "这沙漠永远走不到尽头", "废旧渡口那边有海盗出没", "加基森的拍卖行什么稀奇玩意儿都有", "热砂港的海风也吹不凉这沙漠", "时光之穴的龙族让人敬畏", "千针石林的升降梯真刺激", "正午的沙漠能把盔甲烤化", "远处出现了海市蜃楼……还是绿洲", "蝎子在沙子里钻来钻去", "这个季节的沙漠之夜繁星满天", "古老的废墟在沙丘间若隐若现", "沙子烫得没法坐下" },
        { "泥泞的路真难走", "沼泽在冒泡……离远点", "失落的神庙就在前面", "悲伤沼泽的雾从不散开", "空气中弥漫着腐臭", "恶魔的能量扭曲了这片大地", "黑暗之门就在不远处", "诅咒之地的天空总是血红色的", "这片泥沼吞噬了太多英雄", "恐龙！那边有恐龙！", "安戈洛的水晶……闪耀着奇异的光", "探险队的营地还远吗", "沼泽里有什么东西在动", "这片土地的魔力还未消散", "每一步都在陷进泥里……烦", "火山口喷出的热气烫得吓人", "失落的神庙里……不该进去的", "诅咒之地的每一寸土地都在低语", "安戈洛环山，奇迹与危险并存", "沼泽深处传来了低沉的吼声", "恐龙足迹还新鲜着……小心", "腐败的气息越来越浓了", "安戈洛的火羽山隔三差五就喷发", "诅咒之地的恶魔不眠不休", "水面上漂着绿色的浮萍", "沼泽里冒出的气泡……下面是什么", "蕨类植物长得比人还高", "青蛙的合唱从早到晚不停", "朽木横七竖八地倒在水里", "沼泽中的孤岛上有奇怪的石碑" },
        { "海风带着咸味，真舒服", "娜迦的废墟就在不远处", "蓝龙盘旋在艾萨拉上空", "坠星海滩的沙子是银色的", "荆棘谷的丛林密不透风", "海盗！有海盗船！", "听到了吗？古拉巴什的鼓声", "巨魔的营地就藏在林子里", "潮湿的海风吹得盔甲生锈", "这海岸线上不知埋了多少宝藏", "海鸥的叫声吵死了", "双子皇帝的石碑刻着什么", "菲拉斯的食人魔……真够笨的", "遗迹里藏着远古的秘密", "幽暗城外的蝙蝠在盘旋", "被遗忘者的灰色皮肤在月光下更加苍白", "提瑞斯法的树林永远阴森", "海岸的雾气让我看不清路", "海水拍打礁石的声音真催眠", "在岸边走，当心涨潮", "艾萨拉的枫叶红得像火", "荆棘谷的竞技场今天又有决斗了", "藏宝海湾的地精商会日进斗金", "菲拉斯的高塔在迷雾中若隐若现", "海浪在礁石上撞出白色的花", "沙滩被太阳晒得发烫", "海水清澈见底，能看到游鱼", "海平线上有船帆的影子", "退潮后沙滩上留下了漂亮的贝壳", "海风带来远处岛屿的气息" },
    };

    static const char* defaultPool[] = {
        "野外风景不错", "今天适合打猎", "这片土地充满了故事", "冒险者，你从哪来", "路上的旅人越来越少了", "这附近有野兽出没，当心", "天气变化无常啊", "有空一起喝一杯", "远征的路还很长", "走累了，歇会儿", "前面的镇子还有多远", "路边的小花挺好看的", "太阳晒得盔甲发烫", "风吹在身上真舒服", "今天状态不错", "这世界真大啊", "星星真多，今晚是个好夜晚", "篝火噼啪响着", "冒险者们的篝火映红了天空", "走了这么久，该歇歇了", "山那边的夕阳真美", "天边的云像是被火烧过", "星光从树间洒下", "月光照在路上，像铺了银子",
        "听说北边有个新开的矿洞", "前面那座桥据说有几百年了", "旅行者，你的披风该补补了", "山路上看到一只狐狸，真漂亮", "路边有野果，尝尝吗", "看那些飞鸟，它们要迁徙了", "这附近有旅店吗，腿快断了", "远处的塔楼是哪个领主的", "护甲的带子松了，得紧一紧", "今天走了多少里了", "前面的岔路口往哪边", "有冒险者的篝火——安全了", "山谷里传来的回声真好玩", "草地里有什么在沙沙作响", "背后的路已经走远了", "前方能看到炊烟，应该有人家"
    };

    // 单话题加权抽选系统
    struct Topic {
        std::string name;
        int weight;
        std::function<std::string(Player*, Player*, uint32, int)> getText;
        bool hasContent;
    };

    auto getZoneText = [&](Player* b, Player* s, uint32 zid, int) -> std::string {
        int zt = GetZoneType(zid);
        if (zt >= 1 && zt <= 6)
            return zonePools[zt - 1][urand(0, 29)];
        return defaultPool[urand(0, 39)];
    };
    auto getTimeText = [&](Player*, Player*, uint32, int) -> std::string { return GetTimeOfDayDesc(); };
    auto getWeatherText = [&](Player* b, Player*, uint32, int) -> std::string { return GetWeatherDesc(b); };
    auto getProfText = [&](Player* b, Player*, uint32, int) -> std::string { return GetProfessionFlavor(b); };
    auto getSourceText = [&](Player*, Player* s, uint32, int) -> std::string { return GetSourceFlavor(s); };
    auto getHpText = [&](Player* b, Player*, uint32, int) -> std::string { return GetHpFlavor(b); };
    auto getRumorText = [&](Player*, Player*, uint32 zid, int) -> std::string {
        int zt = GetZoneType(zid);
        if (zt >= 1 && zt <= 6)
            return GetRumorFlavor(zt);
        return "";
    };

    Topic topics[7] = {
        { "Zone",       20, getZoneText,    false },
        { "Time",       15, getTimeText,    false },
        { "Weather",    15, getWeatherText, false },
        { "Profession", 15, getProfText,    false },
        { "Source",     15, getSourceText,  false },
        { "HP",         10, getHpText,      false },
        { "Rumor",      10, getRumorText,   false },
    };

    // 收集有内容的话题并标记
    int topicCount = 0;
    int nonHpWeightSum = 0;
    for (int i = 0; i < 7; ++i)
    {
        std::string text = topics[i].getText(bot, source, zoneId, 0);
        if (!text.empty())
        {
            topics[i].hasContent = true;
            ++topicCount;
            if (i != 5) // 非 HP 话题
                nonHpWeightSum += topics[i].weight;
        }
    }

    // 如果当前没内容，返回默认池
    if (topicCount == 0)
        return defaultPool[urand(0, 39)];

    // HP 阶梯加成
    float pct = bot ? bot->GetHealthPct() : 100.0f;
    int hpWeight = 10; // 默认
    if (pct > 80.0f)
        hpWeight = 10; // 不变
    else if (pct > 50.0f)
        hpWeight = nonHpWeightSum * 22 / 78; // HP 目标占比 22%
    else if (pct > 20.0f)
        hpWeight = nonHpWeightSum * 40 / 60; // HP 目标占比 40%
    else
        hpWeight = nonHpWeightSum * 80 / 20; // HP 目标占比 80%
    if (hpWeight < 1) hpWeight = 1;
    topics[5].weight = hpWeight;

    // 计算总权重
    int totalWeight = nonHpWeightSum;
    if (topics[5].hasContent)
        totalWeight += hpWeight;

    // 加权随机抽选
    int roll = urand(1, totalWeight);
    int cumulative = 0;
    for (int i = 0; i < 7; ++i)
    {
        if (!topics[i].hasContent)
            continue;
        cumulative += topics[i].weight;
        if (roll <= cumulative)
        {
            std::string finalText = topics[i].getText(bot, source, zoneId, 0);
            if (!finalText.empty())
                return finalText;
            break;
        }
    }

    // 兜底
    if (zoneType >= 1 && zoneType <= 6)
        return zonePools[zoneType - 1][urand(0, 29)];
    return defaultPool[urand(0, 39)];
}

bool EmoteActionBase::ReceiveEmote(Player* source, uint32 emote, bool verbal)
{
    LOG_INFO("playerbots", "[EMOTE_CN_REPLY] ReceiveEmote called: source={} emote={} verbal={}",
        source ? source->GetName() : "null", emote, verbal);
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
        bool isSalute = (emote == TEXT_EMOTE_SALUTE);
        bool isWelcomeIntro = (emote == TEXT_EMOTE_WELCOME || emote == TEXT_EMOTE_INTRODUCE);
        // A 关系
        if (isMaster)
        {
            if (isSalute)
                chosen = PickRandom("报告{target}！随时听令|{target}好！敬礼|有任务？随时就绪|长官好！|收到{target}！敬礼");
            else if (isWelcomeIntro)
                chosen = PickRandom("欢迎{target}！等你呢|{target}来啦，太好了|等你半天了！|新朋友？欢迎欢迎！|{target}终于到了啊");
            else
                chosen = PickRandom("{target}来了！|嗨{target}|来了来了！|回来啦？|我在我在|嗨{target}，好久不见|{target}来啦|嗯？{target}！|哈喽呀|你来啦！|今天状态如何{target}|哟！{target}|等你半天了|你可算来了|正好找你呢");
        }
        else if (isGroupLeader && source && source->GetGUID() == group->GetLeaderGUID())
        {
            if (isSalute)
                chosen = PickRandom("队长好！|随时就绪|敬礼！队长|收到指示|队长请说");
            else if (isWelcomeIntro)
                chosen = PickRandom("队长来了！欢迎|队长归队！|队长也来了|欢迎领导|队长请进");
            else
                chosen = PickRandom("队长好！随时待命|队长来视察啦，一切正常|嗨队长，有什么计划|{target}，等你好久了|来了队长|队长今天状态不错|收到指示，队长|队长打算打什么|好嘞队长|你来啦队长");
        }
        else if (sameGroup)
        {
            if (isSalute)
                chosen = PickRandom("战友客气！|敬礼！一起战斗|礼尚往来|好，开工！|打起精神！");
            else if (isWelcomeIntro)
                chosen = PickRandom("欢迎加入队伍！|新队友来了！|一起干！欢迎|欢迎欢迎，自己人|来了就是战友");
            else
                chosen = PickRandom("嗨，队友！一起加油|你好呀，并肩作战|又见面了，伙伴|嗨{target}|冲呀|嘿咱们上|今天干劲十足|队友来了！|走起|又碰到啦|嘿，看到你了|一起冲！|来得正好|就等你了|状态不错嘛");
        }
        else if (sameGuild)
        {
            if (isSalute)
                chosen = PickRandom("公会同仁好！|为公会荣誉！|荣幸遇见|敬礼，战友|公会威武");
            else if (isWelcomeIntro)
                chosen = PickRandom("欢迎新朋友！|自己人别客气|公会欢迎你|久仰久仰|来了就好，一家人");
            else
                chosen = PickRandom("公会战友你好|为了公会！|自己人{target}|嘿又碰到啦|好啊{target}|公会加油|自己人好说|一家人|是你呀|好久不见|公会集合！|巧了又碰上");
        }
        else if (sameFaction)
        {
            if (isSalute)
                chosen = PickRandom("向你致敬！|幸会，勇士|为了荣耀！|敬礼！|很荣幸遇见你");
            else if (isWelcomeIntro)
                chosen = PickRandom("欢迎来到这片土地！|很高兴认识你|旅途快乐！|欢迎欢迎|愿你的旅程顺利");
            else
                chosen = PickRandom("你好，{target}|幸会幸会|嘿，{target}|{target}好啊|今天天气不错|你好啊旅行者|愿圣光与你同在|嘿！|{target}，真巧|你好你好|有缘相遇|一路平安");
        }
        else
        {
            if (isSalute)
                chosen = PickRandom("你好|嗯|敬礼|……|哦");
            else if (isWelcomeIntro)
                chosen = PickRandom("欢迎|嗯，你好|哦…欢迎|啊，好|欢迎啊");
            else
                chosen = PickRandom("你好|嗯……你好|……|嗨|哦|…谁？|嗯？|好|…|嗯");
        }
    }
    // 分类 2: 积极/赞美类
    else if (emote == TEXT_EMOTE_APPLAUD || emote == TEXT_EMOTE_CLAP || emote == TEXT_EMOTE_CONGRATULATE ||
             emote == TEXT_EMOTE_HAPPY || emote == TEXT_EMOTE_CHEER || emote == TEXT_EMOTE_VICTORY ||
             emote == TEXT_EMOTE_TOAST || emote == TEXT_EMOTE_PRAISE || emote == TEXT_EMOTE_COMMEND)
    {
        emoteId = EMOTE_ONESHOT_CHEER;
        textEmote = TEXT_EMOTE_CHEER;
        bool isApplause = (emote == TEXT_EMOTE_APPLAUD || emote == TEXT_EMOTE_CLAP || emote == TEXT_EMOTE_PRAISE || emote == TEXT_EMOTE_COMMEND);
        bool isCongratulate = (emote == TEXT_EMOTE_CONGRATULATE || emote == TEXT_EMOTE_VICTORY);
        if (isMaster)
        {
            if (isApplause)
                chosen = PickRandom("过奖了{target}|哪里哪里|不敢当不敢当|你太客气了|低调低调|谬赞了|受之有愧|你就别取笑我了");
            else if (isCongratulate)
                chosen = PickRandom("胜利属于我们！|一起庆祝！|太棒了！|这把爽了！|值得喝一杯！|我们就是最强的|这波值了|今天手感火热");
            else
                chosen = PickRandom("{target}厉害！|太强了{target}|名不虚传|我服了|高手就是高手|稳|不愧是你|牛|666|给力！|这操作我服了|你是真大佬|可以啊{target}|牛逼！|绝了！");
        }
        else if (sameGroup)
        {
            if (isApplause)
                chosen = PickRandom("都是队伍的功劳|过奖了过奖了|你也不差！|互夸互夸|不敢当啊|你也很强！");
            else if (isCongratulate)
                chosen = PickRandom("一起赢的！|庆祝庆祝！|为队伍干杯！|我们又赢了！|下一场继续！|好样的队友！|这把漂亮！|大获全胜！");
            else
                chosen = PickRandom("干得漂亮，队友|我们是最强的队伍|牛啊牛啊|帅炸了|这把稳了|完美配合|打得太好了|MVP就是你|给力给力|队友们冲|配合天衣无缝|强到没朋友|这波操作满分|队伍有你就稳|一起冲|好配合！|给队友点赞|这波秀啊|手速拉满|真不错啊");
        }
        else
        {
            if (isApplause)
                chosen = PickRandom("过奖了|哪里哪里|不敢当|客气了|低调|谬赞|受不起|你也不错");
            else if (isCongratulate)
                chosen = PickRandom("谢谢你|同喜同喜|一起开心！|太好了！|值得庆祝！|为你高兴！|恭喜恭喜！|好样的！");
            else
                chosen = PickRandom("厉害厉害|不错不错|精彩！|好活|可以可以|帅|我认可了|nice|有点东西|强|有两下子|可以的|值了|还行吧|有看头|打得还行|凑合|没白来|可以啊|不赖嘛|有两把刷子|厉害了|这波秀|高手啊|不错哦");
        }
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
            chosen = PickRandom("……好吧|你说的对|知道了|嗯…行吧|我没意见|听你的|明白了|好吧好吧|你说了算|收到");
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
            chosen = PickRandom("哎哟别这样~|干嘛啦|你挺会啊|噗……|有意思哈|你还来真的|好吧好吧|？|得咧|皮一下很开心？|你赢了|我投降还不行吗|别闹……|真有你的|好吧你赢了");
        else
            chosen = PickRandom("呃……|你干嘛|保持距离谢谢|？？？|请你自重|无语|你认真的吗|别闹|不合适吧|行，我走|不要|别这样|我不喜欢|走开|离我远点");
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
        bool isCryMourn = (emote == TEXT_EMOTE_CRY || emote == TEXT_EMOTE_MOURN);
        bool isScaredPanic = (emote == TEXT_EMOTE_SCARED || emote == TEXT_EMOTE_COWER || emote == TEXT_EMOTE_PANIC || emote == TEXT_EMOTE_CRINGE);
        bool isComfortPat = (emote == TEXT_EMOTE_COMFORT || emote == TEXT_EMOTE_SOOTHE || emote == TEXT_EMOTE_PAT);
        if (isDead)
        {
            if (isCryMourn)
                chosen = PickRandom("（已阵亡…好不甘心）|（呜呜…我还没准备好）|（倒下了…再也起不来了）|（灵魂在哭泣）|（战死沙场，也算荣耀了）");
            else if (isScaredPanic)
                chosen = PickRandom("（死得好惨…）|（吓死了…真的死了）|（到死都在害怕…）|（灵魂都在颤抖）|（死不瞑目啊…）");
            else if (isComfortPat)
                chosen = PickRandom("（已经动不了了…）|（谢谢关心…但我凉了）|（跑尸中，别管我了）|（下次一定小心…）|（谢谢…不过先拉我起来再说）");
            else
                chosen = PickRandom("（已阵亡）|……|凉了|我躺好了|跑尸中……|谁拉我一把|哎|倒了|下次一定|呜呜");
        }
        else if (lowHP)
        {
            if (isCryMourn)
                chosen = PickRandom("好痛…要不行了|呜呜…快撑不住了|没血了…我不想死|疼死了…救救我|好难熬…快没了");
            else if (isScaredPanic)
                chosen = PickRandom("救命啊！！别过来！！|好可怕要死了！|快跑啊打不过！|吓死了救救我！|完了完了完了！");
            else if (isComfortPat)
                chosen = PickRandom("痛…呜呜|谢谢你关心…但我快不行了|别管我了快跑|加血加血！不然真凉了|快奶我…我要倒了");
            else
                chosen = PickRandom("救命啊！！|奶我一口！！|要死了要死了|快救我|撑不住了|加血加血！|顶不住|快倒下了|救！|要没了要没了");
        }
        else if (m_killedRecently)
        {
            if (isCryMourn)
                chosen = PickRandom("呜呜…刚才好可怕|差点就永远躺那了|刚死过一次…心里好难受|还没从死亡阴影里走出来|灵魂还在发颤…");
            else if (isScaredPanic)
                chosen = PickRandom("吓死我了…刚捡回一条命|太恐怖了不要再来！|我魂都吓没了…|别让我再经历一次了|腿还在发抖…缓不过来");
            else if (isComfortPat)
                chosen = PickRandom("谢谢你…我好多了|有你在真好…|呜呜…谢谢你陪我|安慰一下…还心有余悸|没事了…有你在就好");
            else
                chosen = PickRandom("刚活过来，让我缓缓|别再来了|差点就凉了……|腿还在发抖|缓口气……|好险|幸好跑得快|虚惊一场|别再来一次了|我真服了…");
        }
        else if (isMaster)
        {
            if (isCryMourn)
                chosen = PickRandom("呜呜{target}我好难过|好伤心啊{target}|{target}你得哄哄我|心里好难受…|抱抱我好吗");
            else if (isScaredPanic)
                chosen = PickRandom("{target}好可怕吓死我了！|救我{target}！我好怕|那边有东西…好恐怖|{target}快保护我！|不敢过去了…");
            else if (isComfortPat)
                chosen = PickRandom("谢谢{target}关心|有{target}在真好~|还是{target}最好了|好温暖…谢谢你|你对我真好");
            else
                chosen = PickRandom("{target}你得罩着我啊|好痛|呜呜呜|别啊……|下手轻点|我真的会伤心的|你不要我了吗|好难过|别这样……|过分了");
        }
        else
        {
            if (isCryMourn)
                chosen = PickRandom("呜呜呜…好伤心|真的好难过|心里堵得慌…|别管我，让我哭会儿|为什么总是这样…");
            else if (isScaredPanic)
                chosen = PickRandom("别过来！！|啊好可怕！|吓死我了…|救命有鬼！|快跑啊！");
            else if (isComfortPat)
                chosen = PickRandom("谢谢你…|有被暖到…|呜呜你真好|好感动…|谢谢你的安慰");
            else
                chosen = PickRandom("呜呜呜|好痛|别这样……|难受|我真的受伤了|干嘛啊这是|过分别了|哎|心累|你赢了");
        }
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
            chosen = PickRandom("{target}有什么吩咐？|嗯？需要我做什么？|我在呢|说呗|啥事|怎么啦|有什么需要？|我在听|说吧说吧|来事了？");
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
            chosen = PickRandom("哈哈哈哈{target}你太有意思了|真幽默|笑死我了|哎哟你真是|可以可以|乐了|哈哈哈|你是懂搞笑的|有你的|哈哈哈笑不活了|太有才了|笑得肚子疼|真会逗人|你这活宝|绷不住了");
        else
            chosen = PickRandom("噗哈哈|你认真的？|无聊……|什么鬼|不好笑|你的幽默我不懂|呃|这也算笑话？|谁教你的|下一个|行吧行吧|你开心就好");
    }
    // 分类 8: 礼貌/感谢类
    else if (emote == TEXT_EMOTE_THANK || emote == TEXT_EMOTE_BOW || emote == TEXT_EMOTE_CURTSEY ||
             emote == TEXT_EMOTE_APOLOGIZE || emote == TEXT_EMOTE_AGREE || emote == TEXT_EMOTE_NOD ||
             emote == TEXT_EMOTE_GOODLUCK)
    {
        emoteId = EMOTE_ONESHOT_BOW;
        textEmote = TEXT_EMOTE_BOW;
        bool isThank = (emote == TEXT_EMOTE_THANK);
        bool isBowCurtsey = (emote == TEXT_EMOTE_BOW || emote == TEXT_EMOTE_CURTSEY);
        bool isApologize = (emote == TEXT_EMOTE_APOLOGIZE);
        bool isGoodluck = (emote == TEXT_EMOTE_GOODLUCK);
        bool isAgreeNod = (emote == TEXT_EMOTE_AGREE || emote == TEXT_EMOTE_NOD);
        if (isMaster)
        {
            if (isThank)
                chosen = PickRandom("不客气{target}|应该的|乐意效劳|小事一桩|别客气|举手之劳|跟我客气啥|别这么客气|为{target}服务是我的荣幸|一句话的事|你满意就好|客气就见外了");
            else if (isBowCurtsey)
                chosen = PickRandom("不必多礼{target}|您太客气了|礼尚往来|回礼了|别这么客气|您请起|受之有愧|您太讲究了");
            else if (isApologize)
                chosen = PickRandom("没关系{target}|没事没事|别往心里去|原谅你了|下不为例哈");
            else if (isGoodluck)
                chosen = PickRandom("谢谢{target}！|借你吉言！|一起加油！|好运加持！|有你在稳了");
            else if (isAgreeNod)
                chosen = PickRandom("说得对{target}|英雄所见略同|嗯嗯完全同意|没毛病{target}|你说啥就是啥|同意+1|正合我意|我也这么想的");
            else
                chosen = PickRandom("不客气|乐意效劳|应该的|小事一桩|别客气|举手之劳|随时都行|没问题|好说|交给我");
        }
        else if (sameGroup)
        {
            if (isThank)
                chosen = PickRandom("客气啥|应该的|互相帮忙|自己人不说谢|小意思|没事儿|别放心上|同队就应该互相帮衬|一句话的事|别见外|你帮我也不是一次两次了|咱俩谁跟谁");
            else if (isBowCurtsey)
                chosen = PickRandom("别客气|都是自己人|你太见外了|互相尊重|不必多礼|回礼了|你我之间不用这套|别这么见外");
            else if (isApologize)
                chosen = PickRandom("没事|没关系|谁都有失误|不要紧的|下次注意就行");
            else if (isGoodluck)
                chosen = PickRandom("一起加油！|好运！|加油加油！|冲！|胜利属于我们");
            else if (isAgreeNod)
                chosen = PickRandom("同意！|我也觉得|没错没错|你说得对|有道理|正有此意|就是就是|英雄所见略同");
            else
                chosen = PickRandom("客气了|应该的|互相帮忙嘛|都自己人|好说好说|没事|小意思|别放心上|谁跟谁啊|路上小心");
        }
        else
        {
            if (isThank)
                chosen = PickRandom("不客气|不用谢|没事|别客气|应该的|小事|举手之劳|别这么客气|您慢走|能帮到你就好|不费事|您太客气了");
            else if (isBowCurtsey)
                chosen = PickRandom("不用多礼|你太客气了|回礼了|不必如此|礼尚往来|您请起|使不得使不得|您太讲究了");
            else if (isApologize)
                chosen = PickRandom("没关系|没事|算了|不要紧|原谅你了");
            else if (isGoodluck)
                chosen = PickRandom("谢谢|加油！|好运！|你也是|一起努力");
            else if (isAgreeNod)
                chosen = PickRandom("嗯|有道理|说得对|没错|我也这么想|同意|确实|你说得对");
            else
                chosen = PickRandom("不用谢|没关系|嗯嗯|客气|没事的|好|没事儿|OK|行|没事");
        }
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
             emote == TEXT_EMOTE_SHIVER || emote == TEXT_EMOTE_BURP || emote == TEXT_EMOTE_FART || emote == TEXT_EMOTE_SNIFF || emote == TEXT_EMOTE_STINK || emote == TEXT_EMOTE_NOSEPICK || emote == TEXT_EMOTE_THIRSTY)
    {
        emoteId = EMOTE_ONESHOT_POINT;
        textEmote = TEXT_EMOTE_POINT;
        bool isDance = (emote == TEXT_EMOTE_DANCE);
        bool isEatDrink = (emote == TEXT_EMOTE_EAT || emote == TEXT_EMOTE_DRINK || emote == TEXT_EMOTE_HUNGRY || emote == TEXT_EMOTE_THIRSTY);
        bool isSitLay = (emote == TEXT_EMOTE_SIT || emote == TEXT_EMOTE_STAND || emote == TEXT_EMOTE_LAYDOWN);
        bool isTiredYawn = (emote == TEXT_EMOTE_TIRED || emote == TEXT_EMOTE_YAWN || emote == TEXT_EMOTE_BORED);
        bool isBodyDiscomfort = (emote == TEXT_EMOTE_COLD || emote == TEXT_EMOTE_SHIVER || emote == TEXT_EMOTE_COUGH ||
                                 emote == TEXT_EMOTE_SPIT || emote == TEXT_EMOTE_DROOL || emote == TEXT_EMOTE_RASP ||
                                 emote == TEXT_EMOTE_BURP || emote == TEXT_EMOTE_FART || emote == TEXT_EMOTE_SNIFF ||
                                 emote == TEXT_EMOTE_STINK || emote == TEXT_EMOTE_NOSEPICK);
        if (isMaster)
        {
            if (isDance)
                chosen = PickRandom("哈哈哈哈{target}你舞跳得不错嘛|来来来一起跳{target}|这舞步真带劲|你还会跳舞啊{target}|笑死了{target}你还会这招|咱俩比比舞技|你节奏感不错嘛|这扭腰绝了|哈哈哈有那味儿了|再来一遍！");
            else if (isEatDrink)
                chosen = PickRandom("饿了？我这里有干粮{target}|来，一起吃点|吃饱了才有力气干活|好香啊，给我留点|渴了？我这有水|你这吃相让我也饿了|慢慢吃别噎着|伙食不错啊分我点");
            else if (isSitLay)
                chosen = PickRandom("歇会儿也好{target}|坐会儿聊聊天|是该休息下了|躺好了叫一声|起来继续赶路");
            else if (isTiredYawn)
                chosen = PickRandom("你困了{target}？去睡会儿|累了就歇歇|我也差不多了…|你打哈欠会传染的|撑不住就去休息吧");
            else if (isBodyDiscomfort)
                chosen = PickRandom("你没事吧{target}？|冷的话加件衣服|不舒服就歇会儿|这鬼天气真是的|注意身体啊");
            else
                chosen = PickRandom("？？|你干嘛|怎么啦|啥情况|嗯？|哎？|有事？|怎么|什么|干嘛呀");
        }
        else
        {
            if (isDance)
                chosen = PickRandom("跳得不错|哈哈哈有模有样|这舞步谁教你的|厉害了，还会跳舞|来一段？|有那味儿了|一起扭起来|你跳得真欢乐|这舞姿绝了|教教我呗");
            else if (isEatDrink)
                chosen = PickRandom("饿了？我请你|一起吃？|好香，分我点|渴了吧|来来来碰一杯|有吃的怎么能少了我|你这也太香了|给我尝一口嘛");
            else if (isSitLay)
                chosen = PickRandom("坐会儿|歇歇脚|休息一下|躺平了？|起来干活了");
            else if (isTiredYawn)
                chosen = PickRandom("你也困了？|累了就睡吧|打个哈欠都被传染了|我也快了…|确实累了");
            else if (isBodyDiscomfort)
                chosen = PickRandom("你还好吗？|冷了就多穿点|这天气确实难受|保重身体啊|咳咳……离我远点");
            else
                chosen = PickRandom("……|你干啥|？？？");
        }
    }
    // 分类 10: 战斗/威胁类
    else if (emote == TEXT_EMOTE_ROAR || emote == TEXT_EMOTE_THREATEN || emote == TEXT_EMOTE_CALM ||
             emote == TEXT_EMOTE_DUCK || emote == TEXT_EMOTE_TAUNT || emote == TEXT_EMOTE_PITY ||
             emote == TEXT_EMOTE_GROWL || emote == TEXT_EMOTE_OPENFIRE || emote == TEXT_EMOTE_ENCOURAGE ||
             emote == TEXT_EMOTE_ENEMY || emote == TEXT_EMOTE_RUDE || emote == TEXT_EMOTE_CHARGE || emote == TEXT_EMOTE_FLEE || emote == TEXT_EMOTE_INCOMING || emote == TEXT_EMOTE_ATTACKMYTARGET)
    {
        emoteId = EMOTE_ONESHOT_ROAR;
        textEmote = TEXT_EMOTE_ROAR;
        bool isRoarThreaten = (emote == TEXT_EMOTE_ROAR || emote == TEXT_EMOTE_THREATEN || emote == TEXT_EMOTE_TAUNT ||
                               emote == TEXT_EMOTE_GROWL || emote == TEXT_EMOTE_ENEMY || emote == TEXT_EMOTE_RUDE);
        bool isChargeOpenfire = (emote == TEXT_EMOTE_OPENFIRE || emote == TEXT_EMOTE_CHARGE || emote == TEXT_EMOTE_ATTACKMYTARGET);
        bool isFleeDuck = (emote == TEXT_EMOTE_DUCK || emote == TEXT_EMOTE_FLEE);
        bool isEncourage = (emote == TEXT_EMOTE_ENCOURAGE || emote == TEXT_EMOTE_INCOMING);
        if (inBattleground)
        {
            if (isRoarThreaten)
                chosen = PickRandom("碾碎他们！|为了胜利！|一个不留！|杀光对面！|荣誉即吾命！|干掉他们！|别让他们跑了！|压上去！");
            else if (isChargeOpenfire)
                chosen = PickRandom("集火！冲啊！|开火！上！|先手打了！|冲他们阵地！|一波带走！|冲锋！|压过去！|一波推！");
            else if (isFleeDuck)
                chosen = PickRandom("卧倒！小心！|找掩体！|闪避！|躲开！|别暴露！|隐蔽！|散开！|后撤！");
            else if (isEncourage)
                chosen = PickRandom("加油！守住！|别放弃！撑住！|还有机会！|顶住！支援马上到|我们能赢！|守住了！|坚持住！|稳住阵脚！");
            else
                chosen = PickRandom("冲啊！！|为了荣誉！|碾碎他们");
        }
        else if (inArena)
        {
            if (isRoarThreaten)
                chosen = PickRandom("来吧别怂！|干翻他们！|两个一起上！|让你们见识一下！|等死吧你们！|来啊别躲！|正面刚！|你过来啊！");
            else if (isChargeOpenfire)
                chosen = PickRandom("开怪！集火！|上吧！开干了！|先手秒一个！|冲！别犹豫！|直接上！|开战！|动手！|压上去！");
            else if (isFleeDuck)
                chosen = PickRandom("躲一下！|走位走位！|避一避锋芒|闪开他们的技能|绕柱子！|躲好！|卡视角！|避一下！");
            else if (isEncourage)
                chosen = PickRandom("加油！我们能赢！|集中注意力！|打乱他们的节奏！|稳着打！别急！|好机会！一波！|不急！慢慢来！|调整一下！|有机会！");
            else
                chosen = PickRandom("准备好了吗|别让我失望|干掉他们|集中秒一个|别分心|走，开干|别犹豫|集火！|注意控制|上！");
        }
        else if (inDungeon || inRaidInstance)
        {
            if (isRoarThreaten)
                chosen = PickRandom("谁敢过来！|拉住了别跑！|别让它去找治疗！|看好仇恨！|嘲讽住了！|嘲讽拉满！|往我这看！|别让它跑了！");
            else if (isChargeOpenfire)
                chosen = PickRandom("开怪了！准备！|我先上了！|拉怪了跟上！|开了开了！|坦克上！|开啦开啦！|上了上了！|坦克拉住！");
            else if (isFleeDuck)
                chosen = PickRandom("躲技能！|快躲开！|跑位！|闪！|退后！|快躲！|让开！|退后撤！");
            else if (isEncourage)
                chosen = PickRandom("稳住！能过！|加油加油！快了！|还差一点！坚持住！|别慌！慢慢打！|好！保持节奏！|别急！稳着来！|可以的！慢慢打！|快了快了！继续！");
            else
                chosen = PickRandom("拉好仇恨！|打断打断|集中火力|别OT了|注意躲技能|AOE准备|治疗注意|一波波清|出小怪了|稳住");
        }
        else if (inCombat)
        {
            bool isHorde = (bot->GetTeamId() == TEAM_HORDE);
            if (isRoarThreaten)
            {
                if (isHorde)
                    chosen = PickRandom("为了部落！|来啊！看剑！|让你尝尝厉害！|部落的荣耀！|碾碎你！");
                else
                    chosen = PickRandom("为了联盟！|接受制裁！|正义必胜！|以圣光之名！|你的死期到了！");
            }
            else if (isChargeOpenfire)
                chosen = PickRandom("先手！上！|进攻！|冲锋！|干他！|上了！");
            else if (isFleeDuck)
                chosen = PickRandom("撤一下！|躲开！|闪避！|先避一避|退后！");
            else if (isEncourage)
                chosen = PickRandom("加油！撑住！|好样的！继续！|别松懈！|就这样打！|节奏不错！");
            else
            {
                if (isHorde)
                    chosen = PickRandom("来啊！|一起上|为了部落！|打！|谁怕谁|来|有点意思|放开了打|干|来吧");
                else
                    chosen = PickRandom("来啊！|一起上|为了联盟！|打！|谁怕谁|来|有点意思|放开了打|干|来吧");
            }
        }
        else if (isMaster)
        {
            if (isRoarThreaten)
                chosen = PickRandom("要打架？我给{target}你撑场面！|谁敢动{target}先过我这一关！|放心{target}，我罩你|{target}退后，让我来！|谁惹{target}了？我可不客气！");
            else if (isChargeOpenfire)
                chosen = PickRandom("好！冲了{target}|开路！跟上{target}|我先上{target}随后！|上吧{target}！|冲{target}我给你打头阵！");
            else if (isFleeDuck)
                chosen = PickRandom("先撤{target}！|跟我走{target}！|快躲开{target}！|这边{target}！安全|小心{target}！");
            else if (isEncourage)
                chosen = PickRandom("加油{target}！你是最棒的！|没问题{target}！有我呢！|稳住{target}！能行！|好兆头{target}！这把稳了！|放心{target}！一切顺利！");
            else
                chosen = PickRandom("{target}说打谁就打谁|随时准备战斗|谁敢惹你|我罩着|指哪打哪|走，带路|打谁？你说|随时待命|打架叫我|我拳头痒了");
        }
        else
        {
            if (isRoarThreaten)
                chosen = PickRandom("放马过来！|你谁啊？找打？|哼，不自量力|来试试看？|你认真的？行啊");
            else if (isChargeOpenfire)
                chosen = PickRandom("我先上了！|冲啊！|打！|来了|上！");
            else if (isFleeDuck)
                chosen = PickRandom("快跑！|躲开！|小心！|闪了|别打到我");
            else if (isEncourage)
                chosen = PickRandom("加油！|好样的！|冲！|努力！|你可以的！");
            else
            {
                chosen = PickRandom("放马过来|谁怕谁|哼|你来试试？|我可不客气了|找打？|脾气不小嘛|行啊，来|有种|想清楚了？");
                isYell = true;
            }
        }
    }
    // 分类 11: TALKQ / TALK / TALKEX / LISTEN
    else if (emote == TEXT_EMOTE_TALKQ || emote == TEXT_EMOTE_LISTEN ||
             emote == TEXT_EMOTE_TALK || emote == TEXT_EMOTE_TALKEX)
    {
        if (isMaster)
        {
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_TALKQ;
            chosen = PickRandom("在呢，请说|我在听|说吧|洗耳恭听|嗯？|有话说|来|听听|什么事|你讲");
        }
        else if (inDungeon || inRaidInstance)
        {
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_TALK;
            chosen = PickRandom("小心巡逻怪|注意ADD|控好怪，一波波打|别引多了|注意Boss技能|准备好就开|还有一波|别急|站位对了吗|看看Buff");
        }
        else if (inBattleground)
        {
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_TALK;
            chosen = PickRandom("守好旗|支援中路|拿下墓地|别乱跑|资源点守好|跟上大部队|注意防守|旗子！旗子！|对面在偷塔|稳住");
        }
        else if (isUnderwater)
        {
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_TALK;
            chosen = PickRandom("咕噜咕噜……憋不住了|快上岸|谁能给我个水下呼吸|缺氧了……|快不行了|泡太久了|上来透口气|要淹死了|别游太远|上去吧");
        }
        else if (inCave)
        {
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_TALK;
            chosen = PickRandom("这洞有点深啊|小心脚下|矿在哪呢|好黑|小心塌方|不太对劲这里|有东西动了一下|别走散了|能出去了吗|路还通吗");
        }
        else if (inCity)
        {
            if (emote == TEXT_EMOTE_LISTEN)
            {
                emoteId = EMOTE_ONESHOT_TALK;
                textEmote = 0;
                chosen = PickRandom("嗯？你说|我在听呢|你讲|说吧说吧|听着呢|耳朵在这儿|洗耳恭听|嗯哼|来来来说|继续");
            }
            else if (emote == TEXT_EMOTE_TALKQ)
            {
                emoteId = EMOTE_ONESHOT_QUESTION;
                textEmote = TEXT_EMOTE_TALKQ;
                chosen = PickRandom("{target}知道这座城的历史吗？|{target}你说这座城市存在了多少年……|……{target}信命吗？|{target}觉得城门口那雕像到底是谁？|{target}，你说这座城以前是什么样子的？|{target}有没有想过我们为什么在这里？|这个地方见证过多少人的来来去去啊，{target}|{target}，你觉得这世上真有正义吗？|你说{target}，那些年我们追的东西到头来有意义吗？|{target}听说过这座城的传说吗？|你觉得这座城和野外有多大不同，{target}？|{target}，你说活着的意义是什么？|这城不知道存在多久了……{target}你怎么看|{target}想过去远方吗？|{target}觉得命运是被写好的还是自己选的？|你相信这座城里还有没被发现的故事吗{target}|{target}，你说人和人之间的缘分是偶然还是必然？|走遍世界就一定能找到答案吗，{target}？|这座城的每块砖都藏着秘密吧……{target}你怎么想|{target}你说世界上是先有光还是先有暗|人生苦短啊{target}，你觉得什么是值得的？|有时候觉得城里的一切都像一场梦，{target}觉得呢？|{target}，你相信传说吗？那些古老的故事……|你觉得这座城建城多少年了{target}|{target}，你说旅行的尽头是什么？|这座城像不像一个巨大的迷宫，{target}|{target}问过自己活着的理由吗？|你觉得这座城的守护者知道自己的宿命吗{target}|{target}，是什么让你选择了这条路？|这座城比我们以为的老得多，{target}你知道吗");
            }
            else
            {
                emoteId = EMOTE_ONESHOT_TALK;
                textEmote = TEXT_EMOTE_TALK;
                chosen = PickRandom("{target}听过城东那家酒馆的传闻没？|刚才在市场看到个奇特的商人，{target}|{target}我跟你说，昨天我路过银行门口看到有人被偷了……|哈哈{target}，这地方永远有新鲜事|{target}知道吗，这城里有条密道通到城外|我跟你说个有意思的事{target}，这里的铁匠据说是个退役的老兵|{target}你看那边那个NPC，表情特别有意思|说起来{target}，你吃过这里旅店的炖肉吗？绝了|{target}有空去看看武器店的陈列，有些武器绝对不是凡品|哈哈{target}，这让我想起上次在这里的糗事|你觉得这座城市最有趣的是哪个区{target}|{target}我认识一个NPC在这城里住了四十年……|诶{target}，你在这城里迷路过吗？我第一次来绕了半小时|{target}这城的建筑风格你注意看过没有？跟别的城不一样|说起来{target}，这里的拍卖行总是有人在吵架，有意思|{target}你知道这城里最高的塔从哪上去吗？|我在这城见过各个种族的商人，{target}你猜谁最会砍价|{target}我跟你说，上次在城里听到了不得了的秘密……|这城里有家店卖的香料能让人做一整晚的美梦{target}|哈哈{target}你说城里的卫兵一天要站多久|{target}看到喷泉边那个吟游诗人了吗？他的故事三天三夜讲不完|{target}你有没有发现这座城最亮的那个路灯总在子时灭掉|我最喜欢这座城的黄昏，{target}你呢|诶{target}你知道这里的旅店老板娘年轻时是个海盗吗？|{target}你见过城墙上那些抓痕吗……我怀疑有龙来过|这边的面包房清晨五点开门，{target}有空去尝尝|{target}有人说这城地下有座被遗忘的神殿……|你在这城里碰到过最奇怪的事是什么{target}|{target}这城市就像一个巨大的故事口袋，每次来都能掏出新东西|今天城里有人在卖一种会发光的蘑菇{target}你见过吗");
            }
        }
        else
        {
            if (emote == TEXT_EMOTE_LISTEN)
            {
                emoteId = EMOTE_ONESHOT_TALK;
                textEmote = 0;
                chosen = PickRandom("嗯，你讲|洗耳恭听|说吧|我在听|来|你说|嗯哼|然后呢|让我听听|接着讲");
            }
            else if (emote == TEXT_EMOTE_TALKQ)
            {
                emoteId = EMOTE_ONESHOT_QUESTION;
                textEmote = TEXT_EMOTE_TALKQ;
                chosen = PickRandom("{target}，活着到底是为了什么？|{target}，你说命运这东西真的存在吗？|……{target}，你相信来世吗？|{target}觉得这片大陆到底有多大？远方的尽头是什么？|{target}，你有没有想过我们为什么在这里？|{target}，人生的意义到底是什么呢……|你说这片土地见证过多少故事，{target}？|{target}觉得世界上有真正的公平吗？|{target}，你说我们死后会去哪里？|看到这片天了吗{target}，你说宇宙之外还有什么？|{target}你说一个人活多久才算够|{target}，你相信这个世界上有神吗？|你觉得爱与恨哪个更持久{target}|{target}，如果我告诉你这一切都不是偶然你信吗|{target}想过回到过去改变一些事情吗？|你说这世界是先有黑暗还是先有光明{target}|{target}觉得力量重要还是智慧重要？|如果给你一次重来的机会{target}，你会怎么选|这片大地的每一寸土都藏着秘密吧{target}|{target}你看这星空，几千年前的人也这样抬头看过呢……|你说旅行的意义是终点还是过程{target}|{target}，你觉得真正的强大是什么样的？|这风里有故事，{target}你闻到了吗|{target}为什么有些人生来就注定要战斗呢|走在路上总想问一个问题……{target}你说什么是自由？|你还记得最初踏上旅途的原因吗{target}|这片荒野不知道埋葬了多少人的梦想……{target}你呢|{target}，你说我们的选择决定了命运，还是命运决定了选择？|你看远处的山{target}，你说山那边有人和我们想一样的事吗|如果说一切终将归于尘土，{target}你说我们战斗的意义何在");
            }
            else
            {
                emoteId = EMOTE_ONESHOT_TALK;
                textEmote = TEXT_EMOTE_TALK;
                chosen = BuildOutdoorFlavor(bot, source, zoneId, bot->GetMapId());
            }
        }
    }
    // 默认：未匹配的 emote 不做反应
    else
    {
        return false;
    }

    // 统一替换占位符
    chosen = ReplacePlaceholders(chosen, srcName, zoneId, zoneName);

    // === per-bot 记忆退避（仅 verbal 输出） ===
    if (verbal && !chosen.empty())
    {
        static std::unordered_map<ObjectGuid, std::deque<std::string>> recentOutputs;
        ObjectGuid botGuid = bot->GetGUID();
        auto& recent = recentOutputs[botGuid];
        if (std::find(recent.begin(), recent.end(), chosen) != recent.end())
        {
            if (urand(1, 100) <= 90)
                return true;  // 90% 概率跳过重复
        }
        recent.push_back(chosen);
        if (recent.size() > 3)
            recent.pop_front();
    }

    // === 同队冷却（仅 verbal 输出，世界 NPC 无队自然跳过） ===
    if (verbal && group && !chosen.empty())
    {
        uint32 cooldownWindow = urand(2, 5);
        ObjectGuid guid = group->GetGUID();
        time_t now = time(nullptr);
        auto it = emoteCooldowns.find(guid);
        if (it != emoteCooldowns.end() && (now - it->second) < cooldownWindow)
            return true;  // 冷却中，跳过本次输出
        emoteCooldowns[guid] = now;
    }

    // === 转身 + 输出 ===
    if (source && !bot->isMoving() && !bot->HasInArc(static_cast<float>(M_PI), source, sPlayerbotAIConfig.farDistance))
        ServerFacade::instance().SetFacingTo(bot, source);

    if (verbal && !chosen.empty())
    {
        if (isYell)
            bot->Yell(chosen, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
        else
            bot->Say(chosen, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
    }

    LOG_INFO("playerbots", "[EMOTE_CN_REPLY] bot={} source={} emote={} verbal={} chosen=\"{}\"",
        EscapeFmt(bot->GetName()), EscapeFmt(srcName), emote, verbal, EscapeFmt(chosen));

    // textEmote 抑制：verbal 已输出自然语言 → 不发黄色系统通知（无聊天输出时保持沉默好过刷通用黄字）
    if (verbal)
        textEmote = 0;

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
        return ReceiveEmote(pSource, emote, bot->InBattleground() ? false : (urand(0, 19) >= 1));

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
