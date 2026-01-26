#include "constants/pokemon.h"
#include "constants/species.h"
#include "global.h"
#include "malloc.h"
#include "apprentice.h"
#include "battle.h"
#include "battle_ai_switch_items.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_z_move.h"
#include "data.h"
#include "daycare.h"
#include "dexnav.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "evolution_scene.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "field_weather.h"
#include "fishing.h"
#include "follower_npc.h"
#include "graphics.h"
#include "item.h"
#include "caps.h"
#include "link.h"
#include "main.h"
#include "move_relearner.h"
#include "overworld.h"
#include "m4a.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokeblock.h"
#include "pokemon.h"
#include "pokemon_animation.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "recorded_battle.h"
#include "regions.h"
#include "rtc.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "test_runner.h"
#include "text.h"
#include "trainer_hill.h"
#include "util.h"
#include "constants/abilities.h"
#include "constants/battle_frontier.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_partner.h"
#include "constants/battle_string_ids.h"
#include "constants/cries.h"
#include "constants/event_objects.h"
#include "constants/form_change_types.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/moves.h"
#include "constants/regions.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/union_room.h"
#include "constants/weather.h"
#include "tx_randomizer_and_challenges.h"

// #define FRIENDSHIP_EVO_THRESHOLD ((P_FRIENDSHIP_EVO_THRESHOLD >= GEN_8) ? 160 : 220)
#define FRIENDSHIP_EVO_THRESHOLD 100

struct SpeciesItem
{
    u16 species;
    u16 item;
};

static u16 CalculateBoxMonChecksum(struct BoxPokemon *boxMon);
static u16 CalculateBoxMonChecksumDecrypt(struct BoxPokemon *boxMon);
static u16 CalculateBoxMonChecksumReencrypt(struct BoxPokemon *boxMon);
static union PokemonSubstruct *GetSubstruct(struct BoxPokemon *boxMon, u32 personality, enum SubstructType substructType);
static void EncryptBoxMon(struct BoxPokemon *boxMon);
static void DecryptBoxMon(struct BoxPokemon *boxMon);
static void Task_PlayMapChosenOrBattleBGM(u8 taskId);
void TrySpecialOverworldEvo();
static void SetUpSpeciesAllLookup();

EWRAM_DATA static u8 sLearningMoveTableID = 0;
EWRAM_DATA u8 gPlayerPartyCount = 0;
EWRAM_DATA u8 gEnemyPartyCount = 0;
EWRAM_DATA struct Pokemon gPlayerParty[PARTY_SIZE] = {0};
EWRAM_DATA struct Pokemon gPlayerPartyBackup[PARTY_SIZE] = {0}; // tx_randomizer
EWRAM_DATA struct Pokemon gEnemyParty[PARTY_SIZE] = {0};
EWRAM_DATA struct SpriteTemplate gMultiuseSpriteTemplate = {0};
EWRAM_DATA static struct MonSpritesGfxManager *sMonSpritesGfxManagers[MON_SPR_GFX_MANAGERS_COUNT] = {NULL};
EWRAM_DATA static u8 sTriedEvolving = 0;
EWRAM_DATA u16 gFollowerSteps = 0;

// tx_randomizer
// EWRAM_DATA static u16 sSpeciesList[1] = {0}; //[NUM_SPECIES] = {0};


#include "data/abilities.h"
#if P_TUTOR_MOVES_ARRAY
#include "data/tutor_moves.h"
#endif // P_TUTOR_MOVES_ARRAY

// Used in an unreferenced function in RS.
// Unreferenced here and in FRLG.
struct CombinedMove
{
    u16 move1;
    u16 move2;
    u16 newMove;
};

static const struct CombinedMove sCombinedMoves[2] =
{
    {MOVE_EMBER, MOVE_GUST, MOVE_HEAT_WAVE},
    {0xFFFF, 0xFFFF, 0xFFFF}
};

// NOTE: The order of the elements in the array below is irrelevant.
// To reorder the pokedex, see the values in include/constants/pokedex.h.

#define HOENN_TO_NATIONAL(name)     [HOENN_DEX_##name - 1] = NATIONAL_DEX_##name

// Assigns all Hoenn Dex Indexes to a National Dex Index
static const enum NationalDexOrder sHoennToNationalOrder[HOENN_DEX_COUNT - 1] =
{
    HOENN_TO_NATIONAL(TREECKO),
    HOENN_TO_NATIONAL(GROVYLE),
    HOENN_TO_NATIONAL(SCEPTILE),
    HOENN_TO_NATIONAL(TORCHIC),
    HOENN_TO_NATIONAL(COMBUSKEN),
    HOENN_TO_NATIONAL(BLAZIKEN),
    HOENN_TO_NATIONAL(MUDKIP),
    HOENN_TO_NATIONAL(MARSHTOMP),
    HOENN_TO_NATIONAL(SWAMPERT),
    HOENN_TO_NATIONAL(POOCHYENA),
    HOENN_TO_NATIONAL(MIGHTYENA),
    HOENN_TO_NATIONAL(ZIGZAGOON),
    HOENN_TO_NATIONAL(LINOONE),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GALAR_FORMS
    HOENN_TO_NATIONAL(OBSTAGOON),
#endif
    HOENN_TO_NATIONAL(WURMPLE),
    HOENN_TO_NATIONAL(SILCOON),
    HOENN_TO_NATIONAL(BEAUTIFLY),
    HOENN_TO_NATIONAL(CASCOON),
    HOENN_TO_NATIONAL(DUSTOX),
    HOENN_TO_NATIONAL(LOTAD),
    HOENN_TO_NATIONAL(LOMBRE),
    HOENN_TO_NATIONAL(LUDICOLO),
    HOENN_TO_NATIONAL(SEEDOT),
    HOENN_TO_NATIONAL(NUZLEAF),
    HOENN_TO_NATIONAL(SHIFTRY),
    HOENN_TO_NATIONAL(TAILLOW),
    HOENN_TO_NATIONAL(SWELLOW),
    HOENN_TO_NATIONAL(WINGULL),
    HOENN_TO_NATIONAL(PELIPPER),
    HOENN_TO_NATIONAL(RALTS),
    HOENN_TO_NATIONAL(KIRLIA),
    HOENN_TO_NATIONAL(GARDEVOIR),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(GALLADE),
#endif
    HOENN_TO_NATIONAL(SURSKIT),
    HOENN_TO_NATIONAL(MASQUERAIN),
    HOENN_TO_NATIONAL(SHROOMISH),
    HOENN_TO_NATIONAL(BRELOOM),
    HOENN_TO_NATIONAL(SLAKOTH),
    HOENN_TO_NATIONAL(VIGOROTH),
    HOENN_TO_NATIONAL(SLAKING),
    HOENN_TO_NATIONAL(ABRA),
    HOENN_TO_NATIONAL(KADABRA),
    HOENN_TO_NATIONAL(ALAKAZAM),
    HOENN_TO_NATIONAL(NINCADA),
    HOENN_TO_NATIONAL(NINJASK),
    HOENN_TO_NATIONAL(SHEDINJA),
    HOENN_TO_NATIONAL(WHISMUR),
    HOENN_TO_NATIONAL(LOUDRED),
    HOENN_TO_NATIONAL(EXPLOUD),
    HOENN_TO_NATIONAL(MAKUHITA),
    HOENN_TO_NATIONAL(HARIYAMA),
    HOENN_TO_NATIONAL(GOLDEEN),
    HOENN_TO_NATIONAL(SEAKING),
    HOENN_TO_NATIONAL(MAGIKARP),
    HOENN_TO_NATIONAL(GYARADOS),
    HOENN_TO_NATIONAL(AZURILL),
    HOENN_TO_NATIONAL(MARILL),
    HOENN_TO_NATIONAL(AZUMARILL),
    HOENN_TO_NATIONAL(GEODUDE),
    HOENN_TO_NATIONAL(GRAVELER),
    HOENN_TO_NATIONAL(GOLEM),
    HOENN_TO_NATIONAL(NOSEPASS),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(PROBOPASS),
#endif
    HOENN_TO_NATIONAL(SKITTY),
    HOENN_TO_NATIONAL(DELCATTY),
    HOENN_TO_NATIONAL(ZUBAT),
    HOENN_TO_NATIONAL(GOLBAT),
    HOENN_TO_NATIONAL(CROBAT),
    HOENN_TO_NATIONAL(TENTACOOL),
    HOENN_TO_NATIONAL(TENTACRUEL),
    HOENN_TO_NATIONAL(SABLEYE),
    HOENN_TO_NATIONAL(MAWILE),
    HOENN_TO_NATIONAL(ARON),
    HOENN_TO_NATIONAL(LAIRON),
    HOENN_TO_NATIONAL(AGGRON),
    HOENN_TO_NATIONAL(MACHOP),
    HOENN_TO_NATIONAL(MACHOKE),
    HOENN_TO_NATIONAL(MACHAMP),
    HOENN_TO_NATIONAL(MEDITITE),
    HOENN_TO_NATIONAL(MEDICHAM),
    HOENN_TO_NATIONAL(ELECTRIKE),
    HOENN_TO_NATIONAL(MANECTRIC),
    HOENN_TO_NATIONAL(PLUSLE),
    HOENN_TO_NATIONAL(MINUN),
    HOENN_TO_NATIONAL(MAGNEMITE),
    HOENN_TO_NATIONAL(MAGNETON),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(MAGNEZONE),
#endif
    HOENN_TO_NATIONAL(VOLTORB),
    HOENN_TO_NATIONAL(ELECTRODE),
    HOENN_TO_NATIONAL(VOLBEAT),
    HOENN_TO_NATIONAL(ILLUMISE),
    HOENN_TO_NATIONAL(ODDISH),
    HOENN_TO_NATIONAL(GLOOM),
    HOENN_TO_NATIONAL(VILEPLUME),
    HOENN_TO_NATIONAL(BELLOSSOM),
    HOENN_TO_NATIONAL(DODUO),
    HOENN_TO_NATIONAL(DODRIO),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(BUDEW),
    HOENN_TO_NATIONAL(ROSELIA),
    HOENN_TO_NATIONAL(ROSERADE),
#else
    HOENN_TO_NATIONAL(ROSELIA),
#endif
    HOENN_TO_NATIONAL(GULPIN),
    HOENN_TO_NATIONAL(SWALOT),
    HOENN_TO_NATIONAL(CARVANHA),
    HOENN_TO_NATIONAL(SHARPEDO),
    HOENN_TO_NATIONAL(WAILMER),
    HOENN_TO_NATIONAL(WAILORD),
    HOENN_TO_NATIONAL(NUMEL),
    HOENN_TO_NATIONAL(CAMERUPT),
    HOENN_TO_NATIONAL(SLUGMA),
    HOENN_TO_NATIONAL(MAGCARGO),
    HOENN_TO_NATIONAL(TORKOAL),
    HOENN_TO_NATIONAL(GRIMER),
    HOENN_TO_NATIONAL(MUK),
    HOENN_TO_NATIONAL(KOFFING),
    HOENN_TO_NATIONAL(WEEZING),
    HOENN_TO_NATIONAL(SPOINK),
    HOENN_TO_NATIONAL(GRUMPIG),
    HOENN_TO_NATIONAL(SANDSHREW),
    HOENN_TO_NATIONAL(SANDSLASH),
    HOENN_TO_NATIONAL(SPINDA),
    HOENN_TO_NATIONAL(SKARMORY),
    HOENN_TO_NATIONAL(TRAPINCH),
    HOENN_TO_NATIONAL(VIBRAVA),
    HOENN_TO_NATIONAL(FLYGON),
    HOENN_TO_NATIONAL(CACNEA),
    HOENN_TO_NATIONAL(CACTURNE),
    HOENN_TO_NATIONAL(SWABLU),
    HOENN_TO_NATIONAL(ALTARIA),
    HOENN_TO_NATIONAL(ZANGOOSE),
    HOENN_TO_NATIONAL(SEVIPER),
    HOENN_TO_NATIONAL(LUNATONE),
    HOENN_TO_NATIONAL(SOLROCK),
    HOENN_TO_NATIONAL(BARBOACH),
    HOENN_TO_NATIONAL(WHISCASH),
    HOENN_TO_NATIONAL(CORPHISH),
    HOENN_TO_NATIONAL(CRAWDAUNT),
    HOENN_TO_NATIONAL(BALTOY),
    HOENN_TO_NATIONAL(CLAYDOL),
    HOENN_TO_NATIONAL(LILEEP),
    HOENN_TO_NATIONAL(CRADILY),
    HOENN_TO_NATIONAL(ANORITH),
    HOENN_TO_NATIONAL(ARMALDO),
    HOENN_TO_NATIONAL(IGGLYBUFF),
    HOENN_TO_NATIONAL(JIGGLYPUFF),
    HOENN_TO_NATIONAL(WIGGLYTUFF),
    HOENN_TO_NATIONAL(FEEBAS),
    HOENN_TO_NATIONAL(MILOTIC),
    HOENN_TO_NATIONAL(CASTFORM),
    HOENN_TO_NATIONAL(STARYU),
    HOENN_TO_NATIONAL(STARMIE),
    HOENN_TO_NATIONAL(KECLEON),
    HOENN_TO_NATIONAL(SHUPPET),
    HOENN_TO_NATIONAL(BANETTE),
    HOENN_TO_NATIONAL(DUSKULL),
    HOENN_TO_NATIONAL(DUSCLOPS),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(DUSKNOIR),
    HOENN_TO_NATIONAL(TROPIUS),
    HOENN_TO_NATIONAL(CHINGLING),
#else
    HOENN_TO_NATIONAL(TROPIUS),
#endif
    HOENN_TO_NATIONAL(CHIMECHO),
    HOENN_TO_NATIONAL(ABSOL),
    HOENN_TO_NATIONAL(VULPIX),
    HOENN_TO_NATIONAL(NINETALES),
    HOENN_TO_NATIONAL(PICHU),
    HOENN_TO_NATIONAL(PIKACHU),
    HOENN_TO_NATIONAL(RAICHU),
    HOENN_TO_NATIONAL(PSYDUCK),
    HOENN_TO_NATIONAL(GOLDUCK),
    HOENN_TO_NATIONAL(WYNAUT),
    HOENN_TO_NATIONAL(WOBBUFFET),
    HOENN_TO_NATIONAL(NATU),
    HOENN_TO_NATIONAL(XATU),
    HOENN_TO_NATIONAL(GIRAFARIG),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_9_CROSS_EVOS
    HOENN_TO_NATIONAL(FARIGIRAF),
#endif
    HOENN_TO_NATIONAL(PHANPY),
    HOENN_TO_NATIONAL(DONPHAN),
    HOENN_TO_NATIONAL(PINSIR),
    HOENN_TO_NATIONAL(HERACROSS),
    HOENN_TO_NATIONAL(RHYHORN),
    HOENN_TO_NATIONAL(RHYDON),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(RHYPERIOR),
#endif
    HOENN_TO_NATIONAL(SNORUNT),
    HOENN_TO_NATIONAL(GLALIE),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GEN_4_CROSS_EVOS
    HOENN_TO_NATIONAL(FROSLASS),
#endif
    HOENN_TO_NATIONAL(SPHEAL),
    HOENN_TO_NATIONAL(SEALEO),
    HOENN_TO_NATIONAL(WALREIN),
    HOENN_TO_NATIONAL(CLAMPERL),
    HOENN_TO_NATIONAL(HUNTAIL),
    HOENN_TO_NATIONAL(GOREBYSS),
    HOENN_TO_NATIONAL(RELICANTH),
    HOENN_TO_NATIONAL(CORSOLA),
#if P_NEW_EVOS_IN_REGIONAL_DEX && P_GALAR_FORMS
    HOENN_TO_NATIONAL(CURSOLA),
#endif
    HOENN_TO_NATIONAL(CHINCHOU),
    HOENN_TO_NATIONAL(LANTURN),
    HOENN_TO_NATIONAL(LUVDISC),
    HOENN_TO_NATIONAL(HORSEA),
    HOENN_TO_NATIONAL(SEADRA),
    HOENN_TO_NATIONAL(KINGDRA),
    HOENN_TO_NATIONAL(BAGON),
    HOENN_TO_NATIONAL(SHELGON),
    HOENN_TO_NATIONAL(SALAMENCE),
    HOENN_TO_NATIONAL(BELDUM),
    HOENN_TO_NATIONAL(METANG),
    HOENN_TO_NATIONAL(METAGROSS),
    HOENN_TO_NATIONAL(REGIROCK),
    HOENN_TO_NATIONAL(REGICE),
    HOENN_TO_NATIONAL(REGISTEEL),
    HOENN_TO_NATIONAL(LATIAS),
    HOENN_TO_NATIONAL(LATIOS),
    HOENN_TO_NATIONAL(KYOGRE),
    HOENN_TO_NATIONAL(GROUDON),
    HOENN_TO_NATIONAL(RAYQUAZA),
    HOENN_TO_NATIONAL(JIRACHI),
    HOENN_TO_NATIONAL(DEOXYS),
};

const struct SpindaSpot gSpindaSpotGraphics[] =
{
    {.x = 16, .y =  7, .image = INCBIN_U16("graphics/pokemon/spinda/spots/spot_0.1bpp")},
    {.x = 40, .y =  8, .image = INCBIN_U16("graphics/pokemon/spinda/spots/spot_1.1bpp")},
    {.x = 22, .y = 25, .image = INCBIN_U16("graphics/pokemon/spinda/spots/spot_2.1bpp")},
    {.x = 34, .y = 26, .image = INCBIN_U16("graphics/pokemon/spinda/spots/spot_3.1bpp")}
};

// In Battle Palace, moves are chosen based on the pokemons nature rather than by the player
// Moves are grouped into "Attack", "Defense", or "Support" (see PALACE_MOVE_GROUP_*)
// Each nature has a certain percent chance of selecting a move from a particular group
// and a separate percent chance for each group when at or below 50% HP
// The table below doesn't list percentages for Support because you can subtract the other two
// Support percentages are listed in comments off to the side instead
#define PALACE_STYLE(atk, def, atkLow, defLow) {atk, atk + def, atkLow, atkLow + defLow}

const struct NatureInfo gNaturesInfo[NUM_NATURES] =
{
    [NATURE_HARDY] =
    {
        .name = COMPOUND_STRING("Hardy"),
        .statUp = STAT_ATK,
        .statDown = STAT_ATK,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_HARDY, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(61, 7, 61, 7), //32% support >= 50% HP, 32% support < 50% HP
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_LONELY] =
    {
        .name = COMPOUND_STRING("Lonely"),
        .statUp = STAT_ATK,
        .statDown = STAT_DEF,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_LONELY, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(20, 25, 84, 8), //55%,  8%
        .battlePalaceFlavorText = B_MSG_GLINT_IN_EYE,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_BRAVE] =
    {
        .name = COMPOUND_STRING("Brave"),
        .statUp = STAT_ATK,
        .statDown = STAT_SPEED,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_BRAVE, AFFINE_TURN_UP},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(70, 15, 32, 60), //15%, 8%
        .battlePalaceFlavorText = B_MSG_GETTING_IN_POS,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_ADAMANT] =
    {
        .name = COMPOUND_STRING("Adamant"),
        .statUp = STAT_ATK,
        .statDown = STAT_SPATK,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_ADAMANT, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(38, 31, 70, 15), //31%, 15%
        .battlePalaceFlavorText = B_MSG_GLINT_IN_EYE,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_NAUGHTY] =
    {
        .name = COMPOUND_STRING("Naughty"),
        .statUp = STAT_ATK,
        .statDown = STAT_SPDEF,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_NAUGHTY, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(20, 70, 70, 22), //10%, 8%
        .battlePalaceFlavorText = B_MSG_GLINT_IN_EYE,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_BOLD] =
    {
        .name = COMPOUND_STRING("Bold"),
        .statUp = STAT_DEF,
        .statDown = STAT_ATK,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_BOLD, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(30, 20, 32, 58), //50%, 10%
        .battlePalaceFlavorText = B_MSG_GETTING_IN_POS,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_DOCILE] =
    {
        .name = COMPOUND_STRING("Docile"),
        .statUp = STAT_DEF,
        .statDown = STAT_DEF,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_DOCILE, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(56, 22, 56, 22), //22%, 22%
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_RANDOM,
    },
    [NATURE_RELAXED] =
    {
        .name = COMPOUND_STRING("Relaxed"),
        .statUp = STAT_DEF,
        .statDown = STAT_SPEED,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_RELAXED, AFFINE_TURN_UP_AND_DOWN},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(25, 15, 75, 15), //60%, 10%
        .battlePalaceFlavorText = B_MSG_GLINT_IN_EYE,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_IMPISH] =
    {
        .name = COMPOUND_STRING("Impish"),
        .statUp = STAT_DEF,
        .statDown = STAT_SPATK,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_IMPISH, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(69, 6, 28, 55), //25%, 17%
        .battlePalaceFlavorText = B_MSG_GETTING_IN_POS,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_LAX] =
    {
        .name = COMPOUND_STRING("Lax"),
        .statUp = STAT_DEF,
        .statDown = STAT_SPDEF,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_LAX, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(35, 10, 29, 6), //55%, 65%
        .battlePalaceFlavorText = B_MSG_GROWL_DEEPLY,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_TIMID] =
    {
        .name = COMPOUND_STRING("Timid"),
        .statUp = STAT_SPEED,
        .statDown = STAT_ATK,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_TIMID, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(62, 10, 30, 20), //28%, 50%
        .battlePalaceFlavorText = B_MSG_GROWL_DEEPLY,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_HASTY] =
    {
        .name = COMPOUND_STRING("Hasty"),
        .statUp = STAT_SPEED,
        .statDown = STAT_DEF,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_HASTY, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(58, 37, 88, 6), //5%, 6%
        .battlePalaceFlavorText = B_MSG_GLINT_IN_EYE,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_SERIOUS] =
    {
        .name = COMPOUND_STRING("Serious"),
        .statUp = STAT_SPEED,
        .statDown = STAT_SPEED,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_SERIOUS, AFFINE_TURN_DOWN},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(34, 11, 29, 11), //55%, 60%
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_JOLLY] =
    {
        .name = COMPOUND_STRING("Jolly"),
        .statUp = STAT_SPEED,
        .statDown = STAT_SPATK,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_JOLLY, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(35, 5, 35, 60), //60%, 5%
        .battlePalaceFlavorText = B_MSG_GETTING_IN_POS,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_NAIVE] =
    {
        .name = COMPOUND_STRING("Naive"),
        .statUp = STAT_SPEED,
        .statDown = STAT_SPDEF,
        .backAnim = 0,
        .pokeBlockAnim = {ANIM_NAIVE, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(56, 22, 56, 22), //22%, 22%
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_RANDOM,
    },
    [NATURE_MODEST] =
    {
        .name = COMPOUND_STRING("Modest"),
        .statUp = STAT_SPATK,
        .statDown = STAT_ATK,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_MODEST, AFFINE_TURN_DOWN_SLOW},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(35, 45, 34, 60), //20%, 6%
        .battlePalaceFlavorText = B_MSG_GETTING_IN_POS,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_MILD] =
    {
        .name = COMPOUND_STRING("Mild"),
        .statUp = STAT_SPATK,
        .statDown = STAT_DEF,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_MILD, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(44, 50, 34, 6), //6%, 60%
        .battlePalaceFlavorText = B_MSG_GROWL_DEEPLY,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_QUIET] =
    {
        .name = COMPOUND_STRING("Quiet"),
        .statUp = STAT_SPATK,
        .statDown = STAT_SPEED,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_QUIET, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(56, 22, 56, 22), //22%, 22%
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_BASHFUL] =
    {
        .name = COMPOUND_STRING("Bashful"),
        .statUp = STAT_SPATK,
        .statDown = STAT_SPATK,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_BASHFUL, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(30, 58, 30, 58), //12%, 12%
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_RASH] =
    {
        .name = COMPOUND_STRING("Rash"),
        .statUp = STAT_SPATK,
        .statDown = STAT_SPDEF,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_RASH, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlSupportHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(30, 13, 27, 6), //57%, 67%
        .battlePalaceFlavorText = B_MSG_GROWL_DEEPLY,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_CALM] =
    {
        .name = COMPOUND_STRING("Calm"),
        .statUp = STAT_SPDEF,
        .statDown = STAT_ATK,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_CALM, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighDefenseLow,
        .battlePalacePercents = PALACE_STYLE(40, 50, 25, 62), //10%, 13%
        .battlePalaceFlavorText = B_MSG_GETTING_IN_POS,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_GENTLE] =
    {
        .name = COMPOUND_STRING("Gentle"),
        .statUp = STAT_SPDEF,
        .statDown = STAT_DEF,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_GENTLE, AFFINE_TURN_DOWN_SLIGHT},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(18, 70, 90, 5), //12%, 5%
        .battlePalaceFlavorText = B_MSG_GLINT_IN_EYE,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
    [NATURE_SASSY] =
    {
        .name = COMPOUND_STRING("Sassy"),
        .statUp = STAT_SPDEF,
        .statDown = STAT_SPEED,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_SASSY, AFFINE_TURN_UP_HIGH},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(88, 6, 22, 20), //6%, 58%
        .battlePalaceFlavorText = B_MSG_GROWL_DEEPLY,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_CAREFUL] =
    {
        .name = COMPOUND_STRING("Careful"),
        .statUp = STAT_SPDEF,
        .statDown = STAT_SPATK,
        .backAnim = 2,
        .pokeBlockAnim = {ANIM_CAREFUL, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlDefenseHighSupportLow,
        .battlePalacePercents = PALACE_STYLE(42, 50, 42, 5), //8%, 53%
        .battlePalaceFlavorText = B_MSG_GROWL_DEEPLY,
        .battlePalaceSmokescreen = PALACE_TARGET_WEAKER,
    },
    [NATURE_QUIRKY] =
    {
        .name = COMPOUND_STRING("Quirky"),
        .statUp = STAT_SPDEF,
        .statDown = STAT_SPDEF,
        .backAnim = 1,
        .pokeBlockAnim = {ANIM_QUIRKY, AFFINE_NONE},
        .natureGirlMessage = BattleFrontier_Lounge5_Text_NatureGirlAttackHighAttackLow,
        .battlePalacePercents = PALACE_STYLE(56, 22, 56, 22), //22%, 22%
        .battlePalaceFlavorText = B_MSG_EAGER_FOR_MORE,
        .battlePalaceSmokescreen = PALACE_TARGET_STRONGER,
    },
};

#include "data/graphics/pokemon.h"

#include "data/pokemon/trainer_class_lookups.h"
#include "data/pokemon/experience_tables.h"

#if P_LVL_UP_LEARNSETS >= GEN_9
#include "data/pokemon/level_up_learnsets/gen_9.h" // Scarlet/Violet
#elif P_LVL_UP_LEARNSETS >= GEN_8
#include "data/pokemon/level_up_learnsets/gen_8.h" // Sword/Shield
#elif P_LVL_UP_LEARNSETS >= GEN_7
#include "data/pokemon/level_up_learnsets/gen_7.h" // Ultra Sun/Ultra Moon
#elif P_LVL_UP_LEARNSETS >= GEN_6
#include "data/pokemon/level_up_learnsets/gen_6.h" // Omega Ruby/Alpha Sapphire
#elif P_LVL_UP_LEARNSETS >= GEN_5
#include "data/pokemon/level_up_learnsets/gen_5.h" // Black 2/White 2
#elif P_LVL_UP_LEARNSETS >= GEN_4
#include "data/pokemon/level_up_learnsets/gen_4.h" // HeartGold/SoulSilver
#elif P_LVL_UP_LEARNSETS >= GEN_3
#include "data/pokemon/level_up_learnsets/gen_3.h" // Ruby/Sapphire/Emerald
#elif P_LVL_UP_LEARNSETS >= GEN_2
#include "data/pokemon/level_up_learnsets/gen_2.h" // Crystal
#elif P_LVL_UP_LEARNSETS >= GEN_1
#include "data/pokemon/level_up_learnsets/gen_1.h" // Yellow
#endif

#include "data/pokemon/teachable_learnsets.h"
#include "data/pokemon/egg_moves.h"
#include "data/pokemon/form_species_tables.h"
#include "data/pokemon/form_change_tables.h"
#include "data/pokemon/form_change_table_pointers.h"
#include "data/object_events/object_event_pic_tables_followers.h"

#include "data/pokemon/species_info.h"

#define PP_UP_SHIFTS(val)           val,        (val) << 2,        (val) << 4,        (val) << 6
#define PP_UP_SHIFTS_INV(val) (u8)~(val), (u8)~((val) << 2), (u8)~((val) << 4), (u8)~((val) << 6)

// PP Up bonuses are stored for a Pokémon as a single byte.
// There are 2 bits (a value 0-3) for each move slot that
// represent how many PP Ups have been applied.
// The following arrays take a move slot id and return:
// gPPUpGetMask - A mask to get the number of PP Ups applied to that move slot
// gPPUpClearMask - A mask to clear the number of PP Ups applied to that move slot
// gPPUpAddValues - A value to add to the PP Bonuses byte to apply 1 PP Up to that move slot
const u8 gPPUpGetMask[MAX_MON_MOVES]   = {PP_UP_SHIFTS(3)};
const u8 gPPUpClearMask[MAX_MON_MOVES] = {PP_UP_SHIFTS_INV(3)};
const u8 gPPUpAddValues[MAX_MON_MOVES] = {PP_UP_SHIFTS(1)};

const u8 gStatStageRatios[MAX_STAT_STAGE + 1][2] =
{
    {10, 40}, // -6, MIN_STAT_STAGE
    {10, 35}, // -5
    {10, 30}, // -4
    {10, 25}, // -3
    {10, 20}, // -2
    {10, 15}, // -1
    {10, 10}, //  0, DEFAULT_STAT_STAGE
    {15, 10}, // +1
    {20, 10}, // +2
    {25, 10}, // +3
    {30, 10}, // +4
    {35, 10}, // +5
    {40, 10}, // +6, MAX_STAT_STAGE
};

// The classes used by other players in the Union Room.
// These should correspond with the overworld graphics in sUnionRoomObjGfxIds
const u16 gUnionRoomFacilityClasses[NUM_UNION_ROOM_CLASSES * GENDER_COUNT] =
{
    // Male classes
    FACILITY_CLASS_COOLTRAINER_M,
    FACILITY_CLASS_BLACK_BELT,
    FACILITY_CLASS_CAMPER,
    FACILITY_CLASS_YOUNGSTER,
    FACILITY_CLASS_PSYCHIC_M,
    FACILITY_CLASS_BUG_CATCHER,
    FACILITY_CLASS_PKMN_BREEDER_M,
    FACILITY_CLASS_GUITARIST,
    // Female classes
    FACILITY_CLASS_COOLTRAINER_F,
    FACILITY_CLASS_HEX_MANIAC,
    FACILITY_CLASS_PICNICKER,
    FACILITY_CLASS_LASS,
    FACILITY_CLASS_PSYCHIC_F,
    FACILITY_CLASS_BATTLE_GIRL,
    FACILITY_CLASS_PKMN_BREEDER_F,
    FACILITY_CLASS_BEAUTY
};

const struct SpriteTemplate gBattlerSpriteTemplates[MAX_BATTLERS_COUNT] =
{
    [B_POSITION_PLAYER_LEFT] = {
        .tileTag = TAG_NONE,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gBattlerPicTable_PlayerLeft,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [B_POSITION_OPPONENT_LEFT] = {
        .tileTag = TAG_NONE,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpriteOpponentSide,
        .anims = NULL,
        .images = gBattlerPicTable_OpponentLeft,
        .affineAnims = gAffineAnims_BattleSpriteOpponentSide,
        .callback = SpriteCB_WildMon,
    },
    [B_POSITION_PLAYER_RIGHT] = {
        .tileTag = TAG_NONE,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpritePlayerSide,
        .anims = NULL,
        .images = gBattlerPicTable_PlayerRight,
        .affineAnims = gAffineAnims_BattleSpritePlayerSide,
        .callback = SpriteCB_BattleSpriteStartSlideLeft,
    },
    [B_POSITION_OPPONENT_RIGHT] = {
        .tileTag = TAG_NONE,
        .paletteTag = 0,
        .oam = &gOamData_BattleSpriteOpponentSide,
        .anims = NULL,
        .images = gBattlerPicTable_OpponentRight,
        .affineAnims = gAffineAnims_BattleSpriteOpponentSide,
        .callback = SpriteCB_WildMon
    },
};

static const struct SpriteTemplate sTrainerBackSpriteTemplate =
{
    .tileTag = TAG_NONE,
    .paletteTag = 0,
    .oam = &gOamData_BattleSpritePlayerSide,
    .anims = NULL,
    .images = NULL,
    .affineAnims = gAffineAnims_BattleSpritePlayerSide,
    .callback = SpriteCB_BattleSpriteStartSlideLeft,
};

#define NUM_SECRET_BASE_CLASSES 5
static const u8 sSecretBaseFacilityClasses[GENDER_COUNT][NUM_SECRET_BASE_CLASSES] =
{
    [MALE] = {
        FACILITY_CLASS_YOUNGSTER,
        FACILITY_CLASS_BUG_CATCHER,
        FACILITY_CLASS_RICH_BOY,
        FACILITY_CLASS_CAMPER,
        FACILITY_CLASS_COOLTRAINER_M
    },
    [FEMALE] = {
        FACILITY_CLASS_LASS,
        FACILITY_CLASS_SCHOOL_KID_F,
        FACILITY_CLASS_LADY,
        FACILITY_CLASS_PICNICKER,
        FACILITY_CLASS_COOLTRAINER_F
    }
};

static const u8 sGetMonDataEVConstants[] =
{
    MON_DATA_HP_EV,
    MON_DATA_ATK_EV,
    MON_DATA_DEF_EV,
    MON_DATA_SPEED_EV,
    MON_DATA_SPDEF_EV,
    MON_DATA_SPATK_EV
};

// For stat-raising items
static const enum Stat sStatsToRaise[] =
{
    STAT_ATK, STAT_ATK, STAT_DEF, STAT_SPEED, STAT_SPATK, STAT_SPDEF, STAT_ACC
};

// 3 modifiers each for how much to change friendship for different ranges
// 0-99, 100-199, 200+
static const s8 sFriendshipEventModifiers[][3] =
{
    [FRIENDSHIP_EVENT_GROW_LEVEL]      = { 5,  3,  2},
    [FRIENDSHIP_EVENT_VITAMIN]         = { 5,  3,  2},
    [FRIENDSHIP_EVENT_BATTLE_ITEM]     = { 1,  1,  0},
    [FRIENDSHIP_EVENT_LEAGUE_BATTLE]   = { 3,  2,  1},
    [FRIENDSHIP_EVENT_LEARN_TMHM]      = { 1,  1,  0},
    [FRIENDSHIP_EVENT_WALKING]         = { 1,  1,  1},
    [FRIENDSHIP_EVENT_FAINT_SMALL]     = {-1, -1, -1},
    [FRIENDSHIP_EVENT_FAINT_FIELD_PSN] = {-5, -5, -10},
    [FRIENDSHIP_EVENT_FAINT_LARGE]     = {-5, -5, -10},
};

static const struct SpeciesItem sAlteringCaveWildMonHeldItems[] =
{
    {SPECIES_NONE,      ITEM_NONE},
    {SPECIES_MAREEP,    ITEM_GANLON_BERRY},
    {SPECIES_PINECO,    ITEM_APICOT_BERRY},
    {SPECIES_HOUNDOUR,  ITEM_BIG_MUSHROOM},
    {SPECIES_TEDDIURSA, ITEM_PETAYA_BERRY},
    {SPECIES_AIPOM,     ITEM_BERRY_JUICE},
    {SPECIES_SHUCKLE,   ITEM_BERRY_JUICE},
    {SPECIES_STANTLER,  ITEM_PETAYA_BERRY},
    {SPECIES_SMEARGLE,  ITEM_SALAC_BERRY},
};

static const struct OamData sOamData_64x64 =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct SpriteTemplate sSpriteTemplate_64x64 =
{
    .tileTag = TAG_NONE,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_64x64,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};


// tx_randomizer

//*********************** //tx_randomizer_and_challenges
#define EVO_TYPE_0 0
#define EVO_TYPE_1 1
#define EVO_TYPE_2 2
#define EVO_TYPE_SELF 3
#define EVO_TYPE_LEGENDARY 4

const u8 gRandomizationTypes[8][25] =
{
    [TX_RANDOM_T_WILD_POKEMON]    = _("TX RANDOM WILD PKMN"),
    [TX_RANDOM_T_TRAINER]         = _("TX RANDOM TRAINER  "),
    [TX_RANDOM_T_MOVES]           = _("TX RANDOM MOVES    "),
    [TX_RANDOM_T_ABILITY]         = _("TX RANDOM ABILITY  "),
    [TX_RANDOM_T_EVO]             = _("TX RANDOM EVO      "),
    [TX_RANDOM_T_EVO_METH]        = _("TX RANDOM EVO METH "),
    [TX_RANDOM_T_STATIC]          = _("TX RANDOM STATIC   "),
    [TX_RANDOM_T_STARTER_POKEMON] = _("TX RANDOM STARTER  "),
};
const u8 gEvoStages[5][20] = 
{
    [EVO_TYPE_0]            = _("EVO TYPE 0"),
    [EVO_TYPE_1]            = _("EVO TYPE 1"),
    [EVO_TYPE_2]            = _("EVO TYPE 2"),
    [EVO_TYPE_SELF]         = _("EVO TYPE SELF"),
    [EVO_TYPE_LEGENDARY]    = _("EVO TYPE LEGENDARY"),
};

// static const u8 gSpeciesMapping[NUM_SPECIES+1] =
// {
//     [SPECIES_NONE]              = EVO_TYPE_SELF,
//     [SPECIES_BULBASAUR]         = EVO_TYPE_0,
//     [SPECIES_IVYSAUR]           = EVO_TYPE_1,
//     [SPECIES_VENUSAUR]          = EVO_TYPE_2,
//     [SPECIES_CHARMANDER]        = EVO_TYPE_0,
//     [SPECIES_CHARMELEON]        = EVO_TYPE_1,
//     [SPECIES_CHARIZARD]         = EVO_TYPE_2,
//     [SPECIES_SQUIRTLE]          = EVO_TYPE_0,
//     [SPECIES_WARTORTLE]         = EVO_TYPE_1,
//     [SPECIES_BLASTOISE]         = EVO_TYPE_2,
//     [SPECIES_CATERPIE]          = EVO_TYPE_0,
//     [SPECIES_METAPOD]           = EVO_TYPE_1,
//     [SPECIES_BUTTERFREE]        = EVO_TYPE_2,
//     [SPECIES_WEEDLE]            = EVO_TYPE_0,
//     [SPECIES_KAKUNA]            = EVO_TYPE_1,
//     [SPECIES_BEEDRILL]          = EVO_TYPE_2,
//     [SPECIES_PIDGEY]            = EVO_TYPE_0,
//     [SPECIES_PIDGEOTTO]         = EVO_TYPE_1,
//     [SPECIES_PIDGEOT]           = EVO_TYPE_2,
//     [SPECIES_RATTATA]           = EVO_TYPE_0,
//     [SPECIES_RATICATE]          = EVO_TYPE_1,
//     [SPECIES_SPEAROW]           = EVO_TYPE_0,
//     [SPECIES_FEAROW]            = EVO_TYPE_1,
//     [SPECIES_EKANS]             = EVO_TYPE_0,
//     [SPECIES_ARBOK]             = EVO_TYPE_1,
//     [SPECIES_PIKACHU]           = EVO_TYPE_1,
//     [SPECIES_RAICHU]            = EVO_TYPE_2,
//     [SPECIES_SANDSHREW]         = EVO_TYPE_0,
//     [SPECIES_SANDSLASH]         = EVO_TYPE_1,
//     [SPECIES_NIDORAN_F]         = EVO_TYPE_0,
//     [SPECIES_NIDORINA]          = EVO_TYPE_1,
//     [SPECIES_NIDOQUEEN]         = EVO_TYPE_2,
//     [SPECIES_NIDORAN_M]         = EVO_TYPE_0,
//     [SPECIES_NIDORINO]          = EVO_TYPE_1,
//     [SPECIES_NIDOKING]          = EVO_TYPE_2,
//     [SPECIES_CLEFAIRY]          = EVO_TYPE_1,
//     [SPECIES_CLEFABLE]          = EVO_TYPE_2,
//     [SPECIES_VULPIX]            = EVO_TYPE_0,
//     [SPECIES_NINETALES]         = EVO_TYPE_1,
//     [SPECIES_JIGGLYPUFF]        = EVO_TYPE_1,
//     [SPECIES_WIGGLYTUFF]        = EVO_TYPE_2,
//     [SPECIES_ZUBAT]             = EVO_TYPE_0,
//     [SPECIES_GOLBAT]            = EVO_TYPE_1,
//     [SPECIES_ODDISH]            = EVO_TYPE_0,
//     [SPECIES_GLOOM]             = EVO_TYPE_1,
//     [SPECIES_VILEPLUME]         = EVO_TYPE_2,
//     [SPECIES_PARAS]             = EVO_TYPE_0,
//     [SPECIES_PARASECT]          = EVO_TYPE_1,
//     [SPECIES_VENONAT]           = EVO_TYPE_0,
//     [SPECIES_VENOMOTH]          = EVO_TYPE_1,
//     [SPECIES_DIGLETT]           = EVO_TYPE_0,
//     [SPECIES_DUGTRIO]           = EVO_TYPE_1,
//     [SPECIES_MEOWTH]            = EVO_TYPE_0,
//     [SPECIES_PERSIAN]           = EVO_TYPE_1,
//     [SPECIES_PSYDUCK]           = EVO_TYPE_0,
//     [SPECIES_GOLDUCK]           = EVO_TYPE_1,
//     [SPECIES_MANKEY]            = EVO_TYPE_0,
//     [SPECIES_PRIMEAPE]          = EVO_TYPE_1,
//     [SPECIES_GROWLITHE]         = EVO_TYPE_0,
//     [SPECIES_ARCANINE]          = EVO_TYPE_1,
//     [SPECIES_POLIWAG]           = EVO_TYPE_0,
//     [SPECIES_POLIWHIRL]         = EVO_TYPE_1,
//     [SPECIES_POLIWRATH]         = EVO_TYPE_2,
//     [SPECIES_ABRA]              = EVO_TYPE_0,
//     [SPECIES_KADABRA]           = EVO_TYPE_1,
//     [SPECIES_ALAKAZAM]          = EVO_TYPE_2,
//     [SPECIES_MACHOP]            = EVO_TYPE_0,
//     [SPECIES_MACHOKE]           = EVO_TYPE_1,
//     [SPECIES_MACHAMP]           = EVO_TYPE_2,
//     [SPECIES_BELLSPROUT]        = EVO_TYPE_0,
//     [SPECIES_WEEPINBELL]        = EVO_TYPE_1,
//     [SPECIES_VICTREEBEL]        = EVO_TYPE_2,
//     [SPECIES_TENTACOOL]         = EVO_TYPE_0,
//     [SPECIES_TENTACRUEL]        = EVO_TYPE_1,
//     [SPECIES_GEODUDE]           = EVO_TYPE_0,
//     [SPECIES_GRAVELER]          = EVO_TYPE_1,
//     [SPECIES_GOLEM]             = EVO_TYPE_2,
//     [SPECIES_PONYTA]            = EVO_TYPE_0,
//     [SPECIES_RAPIDASH]          = EVO_TYPE_1,
//     [SPECIES_SLOWPOKE]          = EVO_TYPE_0,
//     [SPECIES_SLOWBRO]           = EVO_TYPE_2,
//     [SPECIES_MAGNEMITE]         = EVO_TYPE_0,
//     [SPECIES_MAGNETON]          = EVO_TYPE_1,
//     [SPECIES_FARFETCHD]         = EVO_TYPE_0,
//     [SPECIES_DODUO]             = EVO_TYPE_0,
//     [SPECIES_DODRIO]            = EVO_TYPE_1,
//     [SPECIES_SEEL]              = EVO_TYPE_0,
//     [SPECIES_DEWGONG]           = EVO_TYPE_1,
//     [SPECIES_GRIMER]            = EVO_TYPE_0,
//     [SPECIES_MUK]               = EVO_TYPE_1,
//     [SPECIES_SHELLDER]          = EVO_TYPE_0,
//     [SPECIES_CLOYSTER]          = EVO_TYPE_1,
//     [SPECIES_GASTLY]            = EVO_TYPE_0,
//     [SPECIES_HAUNTER]           = EVO_TYPE_1,
//     [SPECIES_GENGAR]            = EVO_TYPE_2,
//     [SPECIES_ONIX]              = EVO_TYPE_0,
//     [SPECIES_DROWZEE]           = EVO_TYPE_0,
//     [SPECIES_HYPNO]             = EVO_TYPE_1,
//     [SPECIES_KRABBY]            = EVO_TYPE_0,
//     [SPECIES_KINGLER]           = EVO_TYPE_1,
//     [SPECIES_VOLTORB]           = EVO_TYPE_0,
//     [SPECIES_ELECTRODE]         = EVO_TYPE_1,
//     [SPECIES_EXEGGCUTE]         = EVO_TYPE_0,
//     [SPECIES_EXEGGUTOR]         = EVO_TYPE_1,
//     [SPECIES_CUBONE]            = EVO_TYPE_0,
//     [SPECIES_MAROWAK]           = EVO_TYPE_1,
//     [SPECIES_HITMONLEE]         = EVO_TYPE_1,
//     [SPECIES_HITMONCHAN]        = EVO_TYPE_1,
//     [SPECIES_LICKITUNG]         = EVO_TYPE_0,
//     [SPECIES_KOFFING]           = EVO_TYPE_0,
//     [SPECIES_WEEZING]           = EVO_TYPE_1,
//     [SPECIES_RHYHORN]           = EVO_TYPE_0,
//     [SPECIES_RHYDON]            = EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_CHANSEY]           = EVO_TYPE_1,
//     #else
//     [SPECIES_CHANSEY]           = EVO_TYPE_0,
//     #endif
//     [SPECIES_TANGELA]           = EVO_TYPE_0,
//     [SPECIES_KANGASKHAN]        = EVO_TYPE_0,
//     [SPECIES_HORSEA]            = EVO_TYPE_0,
//     [SPECIES_SEADRA]            = EVO_TYPE_1,
//     [SPECIES_GOLDEEN]           = EVO_TYPE_0,
//     [SPECIES_SEAKING]           = EVO_TYPE_1,
//     [SPECIES_STARYU]            = EVO_TYPE_0,
//     [SPECIES_STARMIE]           = EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_MR_MIME]           = EVO_TYPE_1,
//     #else
//     [SPECIES_MR_MIME]           = EVO_TYPE_0,
//     #endif
//     [SPECIES_SCYTHER]           = EVO_TYPE_0,
//     [SPECIES_JYNX]              = EVO_TYPE_1,
//     [SPECIES_ELECTABUZZ]        = EVO_TYPE_1,
//     [SPECIES_MAGMAR]            = EVO_TYPE_1,
//     [SPECIES_PINSIR]            = EVO_TYPE_0,
//     [SPECIES_TAUROS]            = EVO_TYPE_0,
//     [SPECIES_MAGIKARP]          = EVO_TYPE_0,
//     [SPECIES_GYARADOS]          = EVO_TYPE_2,
//     [SPECIES_LAPRAS]            = EVO_TYPE_0,
//     [SPECIES_DITTO]             = EVO_TYPE_0,
//     [SPECIES_EEVEE]             = EVO_TYPE_0,
//     [SPECIES_VAPOREON]          = EVO_TYPE_1,
//     [SPECIES_JOLTEON]           = EVO_TYPE_1,
//     [SPECIES_FLAREON]           = EVO_TYPE_1,
//     [SPECIES_PORYGON]           = EVO_TYPE_0,
//     [SPECIES_OMANYTE]           = EVO_TYPE_0,
//     [SPECIES_OMASTAR]           = EVO_TYPE_1,
//     [SPECIES_KABUTO]            = EVO_TYPE_0,
//     [SPECIES_KABUTOPS]          = EVO_TYPE_1,
//     [SPECIES_AERODACTYL]        = EVO_TYPE_0,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SNORLAX]           = EVO_TYPE_1,
//     #else
//     [SPECIES_SNORLAX]           = EVO_TYPE_0,
//     #endif
//     [SPECIES_ARTICUNO]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZAPDOS]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_MOLTRES]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_DRATINI]           = EVO_TYPE_0,
//     [SPECIES_DRAGONAIR]         = EVO_TYPE_1,
//     [SPECIES_DRAGONITE]         = EVO_TYPE_2,
//     [SPECIES_MEWTWO]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_MEW]               = EVO_TYPE_LEGENDARY,
//     [SPECIES_CHIKORITA]         = EVO_TYPE_0,
//     [SPECIES_BAYLEEF]           = EVO_TYPE_1,
//     [SPECIES_MEGANIUM]          = EVO_TYPE_2,
//     [SPECIES_CYNDAQUIL]         = EVO_TYPE_0,
//     [SPECIES_QUILAVA]           = EVO_TYPE_1,
//     [SPECIES_TYPHLOSION]        = EVO_TYPE_2,
//     [SPECIES_TOTODILE]          = EVO_TYPE_0,
//     [SPECIES_CROCONAW]          = EVO_TYPE_1,
//     [SPECIES_FERALIGATR]        = EVO_TYPE_2,
//     [SPECIES_SENTRET]           = EVO_TYPE_0,
//     [SPECIES_FURRET]            = EVO_TYPE_1,
//     [SPECIES_HOOTHOOT]          = EVO_TYPE_0,
//     [SPECIES_NOCTOWL]           = EVO_TYPE_1,
//     [SPECIES_LEDYBA]            = EVO_TYPE_0,
//     [SPECIES_LEDIAN]            = EVO_TYPE_1,
//     [SPECIES_SPINARAK]          = EVO_TYPE_0,
//     [SPECIES_ARIADOS]           = EVO_TYPE_1,
//     [SPECIES_CROBAT]            = EVO_TYPE_2,
//     [SPECIES_CHINCHOU]          = EVO_TYPE_0,
//     [SPECIES_LANTURN]           = EVO_TYPE_1,
//     [SPECIES_PICHU]             = EVO_TYPE_0,
//     [SPECIES_CLEFFA]            = EVO_TYPE_0,
//     [SPECIES_IGGLYBUFF]         = EVO_TYPE_0,
//     [SPECIES_TOGEPI]            = EVO_TYPE_0,
//     [SPECIES_TOGETIC]           = EVO_TYPE_1,
//     [SPECIES_NATU]              = EVO_TYPE_0,
//     [SPECIES_XATU]              = EVO_TYPE_1,
//     [SPECIES_MAREEP]            = EVO_TYPE_0,
//     [SPECIES_FLAAFFY]           = EVO_TYPE_1,
//     [SPECIES_AMPHAROS]          = EVO_TYPE_2,
//     [SPECIES_BELLOSSOM]         = EVO_TYPE_2,
//     [SPECIES_MARILL]            = EVO_TYPE_1,
//     [SPECIES_AZUMARILL]         = EVO_TYPE_2,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SUDOWOODO]         = EVO_TYPE_1,
//     #else
//     [SPECIES_SUDOWOODO]         = EVO_TYPE_0,
//     #endif
//     [SPECIES_POLITOED]          = EVO_TYPE_2,
//     [SPECIES_HOPPIP]            = EVO_TYPE_0,
//     [SPECIES_SKIPLOOM]          = EVO_TYPE_1,
//     [SPECIES_JUMPLUFF]          = EVO_TYPE_2,
//     [SPECIES_AIPOM]             = EVO_TYPE_0,
//     [SPECIES_SUNKERN]           = EVO_TYPE_0,
//     [SPECIES_SUNFLORA]          = EVO_TYPE_1,
//     [SPECIES_YANMA]             = EVO_TYPE_0,
//     [SPECIES_WOOPER]            = EVO_TYPE_0,
//     [SPECIES_QUAGSIRE]          = EVO_TYPE_1,
//     [SPECIES_ESPEON]            = EVO_TYPE_1,
//     [SPECIES_UMBREON]           = EVO_TYPE_1,
//     [SPECIES_MURKROW]           = EVO_TYPE_0,
//     [SPECIES_SLOWKING]          = EVO_TYPE_2,
//     [SPECIES_MISDREAVUS]        = EVO_TYPE_0,
//     [SPECIES_UNOWN]             = EVO_TYPE_0,
//     [SPECIES_WOBBUFFET]         = EVO_TYPE_1,
//     [SPECIES_GIRAFARIG]         = EVO_TYPE_0,
//     [SPECIES_PINECO]            = EVO_TYPE_0,
//     [SPECIES_FORRETRESS]        = EVO_TYPE_1,
//     [SPECIES_DUNSPARCE]         = EVO_TYPE_0,
//     [SPECIES_GLIGAR]            = EVO_TYPE_0,
//     [SPECIES_STEELIX]           = EVO_TYPE_1,
//     [SPECIES_SNUBBULL]          = EVO_TYPE_0,
//     [SPECIES_GRANBULL]          = EVO_TYPE_1,
//     [SPECIES_QWILFISH]          = EVO_TYPE_0,
//     [SPECIES_SCIZOR]            = EVO_TYPE_1,
//     [SPECIES_SHUCKLE]           = EVO_TYPE_0,
//     [SPECIES_HERACROSS]         = EVO_TYPE_0,
//     [SPECIES_SNEASEL]           = EVO_TYPE_0,
//     [SPECIES_TEDDIURSA]         = EVO_TYPE_0,
//     [SPECIES_URSARING]          = EVO_TYPE_1,
//     [SPECIES_SLUGMA]            = EVO_TYPE_0,
//     [SPECIES_MAGCARGO]          = EVO_TYPE_1,
//     [SPECIES_SWINUB]            = EVO_TYPE_0,
//     [SPECIES_PILOSWINE]         = EVO_TYPE_1,
//     [SPECIES_CORSOLA]           = EVO_TYPE_0,
//     [SPECIES_REMORAID]          = EVO_TYPE_0,
//     [SPECIES_OCTILLERY]         = EVO_TYPE_1,
//     [SPECIES_DELIBIRD]          = EVO_TYPE_0,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_MANTINE]           = EVO_TYPE_1,
//     #else
//     [SPECIES_MANTINE]           = EVO_TYPE_0,
//     #endif
//     [SPECIES_SKARMORY]          = EVO_TYPE_0,
//     [SPECIES_HOUNDOUR]          = EVO_TYPE_0,
//     [SPECIES_HOUNDOOM]          = EVO_TYPE_1,
//     [SPECIES_KINGDRA]           = EVO_TYPE_2,
//     [SPECIES_PHANPY]            = EVO_TYPE_0,
//     [SPECIES_DONPHAN]           = EVO_TYPE_1,
//     [SPECIES_PORYGON2]          = EVO_TYPE_1,
//     [SPECIES_STANTLER]          = EVO_TYPE_0,
//     [SPECIES_SMEARGLE]          = EVO_TYPE_0,
//     [SPECIES_TYROGUE]           = EVO_TYPE_0,
//     [SPECIES_HITMONTOP]         = EVO_TYPE_1,
//     [SPECIES_SMOOCHUM]          = EVO_TYPE_0,
//     [SPECIES_ELEKID]            = EVO_TYPE_0,
//     [SPECIES_MAGBY]             = EVO_TYPE_0,
//     [SPECIES_MILTANK]           = EVO_TYPE_0,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_BLISSEY]           = EVO_TYPE_2,
//     #else
//     [SPECIES_BLISSEY]           = EVO_TYPE_1,
//     #endif
//     [SPECIES_RAIKOU]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_ENTEI]             = EVO_TYPE_LEGENDARY,
//     [SPECIES_SUICUNE]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_LARVITAR]          = EVO_TYPE_0,
//     [SPECIES_PUPITAR]           = EVO_TYPE_1,
//     [SPECIES_TYRANITAR]         = EVO_TYPE_2,
//     [SPECIES_LUGIA]             = EVO_TYPE_LEGENDARY,
//     [SPECIES_HO_OH]             = EVO_TYPE_LEGENDARY,
//     [SPECIES_CELEBI]            = EVO_TYPE_LEGENDARY,
//     #ifndef POKEMON_EXPANSION
//     [SPECIES_OLD_UNOWN_B]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_C]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_D]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_E]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_F]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_G]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_H]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_I]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_J]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_K]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_L]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_M]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_N]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_O]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_P]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_Q]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_R]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_S]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_T]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_U]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_V]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_W]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_X]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_Y]       = EVO_TYPE_SELF,
//     [SPECIES_OLD_UNOWN_Z]       = EVO_TYPE_SELF,
//     #endif
//     [SPECIES_TREECKO]           = EVO_TYPE_0,
//     [SPECIES_GROVYLE]           = EVO_TYPE_1,
//     [SPECIES_SCEPTILE]          = EVO_TYPE_2,
//     [SPECIES_TORCHIC]           = EVO_TYPE_0,
//     [SPECIES_COMBUSKEN]         = EVO_TYPE_1,
//     [SPECIES_BLAZIKEN]          = EVO_TYPE_2,
//     [SPECIES_MUDKIP]            = EVO_TYPE_0,
//     [SPECIES_MARSHTOMP]         = EVO_TYPE_1,
//     [SPECIES_SWAMPERT]          = EVO_TYPE_2,
//     [SPECIES_POOCHYENA]         = EVO_TYPE_0,
//     [SPECIES_MIGHTYENA]         = EVO_TYPE_1,
//     [SPECIES_ZIGZAGOON]         = EVO_TYPE_0,
//     [SPECIES_LINOONE]           = EVO_TYPE_1,
//     [SPECIES_WURMPLE]           = EVO_TYPE_0,
//     [SPECIES_SILCOON]           = EVO_TYPE_1,
//     [SPECIES_BEAUTIFLY]         = EVO_TYPE_2,
//     [SPECIES_CASCOON]           = EVO_TYPE_1,
//     [SPECIES_DUSTOX]            = EVO_TYPE_2,
//     [SPECIES_LOTAD]             = EVO_TYPE_0,
//     [SPECIES_LOMBRE]            = EVO_TYPE_1,
//     [SPECIES_LUDICOLO]          = EVO_TYPE_2,
//     [SPECIES_SEEDOT]            = EVO_TYPE_0,
//     [SPECIES_NUZLEAF]           = EVO_TYPE_1,
//     [SPECIES_SHIFTRY]           = EVO_TYPE_2,
//     [SPECIES_NINCADA]           = EVO_TYPE_0,
//     [SPECIES_NINJASK]           = EVO_TYPE_1,
//     [SPECIES_SHEDINJA]          = EVO_TYPE_1,
//     [SPECIES_TAILLOW]           = EVO_TYPE_0,
//     [SPECIES_SWELLOW]           = EVO_TYPE_1,
//     [SPECIES_SHROOMISH]         = EVO_TYPE_0,
//     [SPECIES_BRELOOM]           = EVO_TYPE_1,
//     [SPECIES_SPINDA]            = EVO_TYPE_0,
//     [SPECIES_WINGULL]           = EVO_TYPE_0,
//     [SPECIES_PELIPPER]          = EVO_TYPE_1,
//     [SPECIES_SURSKIT]           = EVO_TYPE_0,
//     [SPECIES_MASQUERAIN]        = EVO_TYPE_1,
//     [SPECIES_WAILMER]           = EVO_TYPE_0,
//     [SPECIES_WAILORD]           = EVO_TYPE_1,
//     [SPECIES_SKITTY]            = EVO_TYPE_0,
//     [SPECIES_DELCATTY]          = EVO_TYPE_1,
//     [SPECIES_KECLEON]           = EVO_TYPE_0,
//     [SPECIES_BALTOY]            = EVO_TYPE_0,
//     [SPECIES_CLAYDOL]           = EVO_TYPE_1,
//     [SPECIES_NOSEPASS]          = EVO_TYPE_0,
//     [SPECIES_TORKOAL]           = EVO_TYPE_0,
//     [SPECIES_SABLEYE]           = EVO_TYPE_0,
//     [SPECIES_BARBOACH]          = EVO_TYPE_0,
//     [SPECIES_WHISCASH]          = EVO_TYPE_0,
//     [SPECIES_LUVDISC]           = EVO_TYPE_0,
//     [SPECIES_CORPHISH]          = EVO_TYPE_0,
//     [SPECIES_CRAWDAUNT]         = EVO_TYPE_1,
//     [SPECIES_FEEBAS]            = EVO_TYPE_0,
//     [SPECIES_MILOTIC]           = EVO_TYPE_1,
//     [SPECIES_CARVANHA]          = EVO_TYPE_0,
//     [SPECIES_SHARPEDO]          = EVO_TYPE_1,
//     [SPECIES_TRAPINCH]          = EVO_TYPE_0,
//     [SPECIES_VIBRAVA]           = EVO_TYPE_1,
//     [SPECIES_FLYGON]            = EVO_TYPE_2,
//     [SPECIES_MAKUHITA]          = EVO_TYPE_0,
//     [SPECIES_HARIYAMA]          = EVO_TYPE_1,
//     [SPECIES_ELECTRIKE]         = EVO_TYPE_0,
//     [SPECIES_MANECTRIC]         = EVO_TYPE_1,
//     [SPECIES_NUMEL]             = EVO_TYPE_0,
//     [SPECIES_CAMERUPT]          = EVO_TYPE_1,
//     [SPECIES_SPHEAL]            = EVO_TYPE_0,
//     [SPECIES_SEALEO]            = EVO_TYPE_1,
//     [SPECIES_WALREIN]           = EVO_TYPE_2,
//     [SPECIES_CACNEA]            = EVO_TYPE_0,
//     [SPECIES_CACTURNE]          = EVO_TYPE_1,
//     [SPECIES_SNORUNT]           = EVO_TYPE_0,
//     [SPECIES_GLALIE]            = EVO_TYPE_1,
//     [SPECIES_LUNATONE]          = EVO_TYPE_0,
//     [SPECIES_SOLROCK]           = EVO_TYPE_0,
//     [SPECIES_AZURILL]           = EVO_TYPE_0,
//     [SPECIES_SPOINK]            = EVO_TYPE_0,
//     [SPECIES_GRUMPIG]           = EVO_TYPE_1,
//     [SPECIES_PLUSLE]            = EVO_TYPE_0,
//     [SPECIES_MINUN]             = EVO_TYPE_0,
//     [SPECIES_MAWILE]            = EVO_TYPE_0,
//     [SPECIES_MEDITITE]          = EVO_TYPE_0,
//     [SPECIES_MEDICHAM]          = EVO_TYPE_1,
//     [SPECIES_SWABLU]            = EVO_TYPE_0,
//     [SPECIES_ALTARIA]           = EVO_TYPE_1,
//     [SPECIES_WYNAUT]            = EVO_TYPE_0,
//     [SPECIES_DUSKULL]           = EVO_TYPE_0,
//     [SPECIES_DUSCLOPS]          = EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_ROSELIA]           = EVO_TYPE_1,
//     #else
//     [SPECIES_ROSELIA]           = EVO_TYPE_0,
//     #endif
//     [SPECIES_SLAKOTH]           = EVO_TYPE_0,
//     [SPECIES_VIGOROTH]          = EVO_TYPE_1,
//     [SPECIES_SLAKING]           = EVO_TYPE_2,
//     [SPECIES_GULPIN]            = EVO_TYPE_0,
//     [SPECIES_SWALOT]            = EVO_TYPE_1,
//     [SPECIES_TROPIUS]           = EVO_TYPE_0,
//     [SPECIES_WHISMUR]           = EVO_TYPE_0,
//     [SPECIES_LOUDRED]           = EVO_TYPE_1,
//     [SPECIES_EXPLOUD]           = EVO_TYPE_2,
//     [SPECIES_CLAMPERL]          = EVO_TYPE_0,
//     [SPECIES_HUNTAIL]           = EVO_TYPE_1,
//     [SPECIES_GOREBYSS]          = EVO_TYPE_1,
//     [SPECIES_ABSOL]             = EVO_TYPE_0,
//     [SPECIES_SHUPPET]           = EVO_TYPE_0,
//     [SPECIES_BANETTE]           = EVO_TYPE_1,
//     [SPECIES_SEVIPER]           = EVO_TYPE_0,
//     [SPECIES_ZANGOOSE]          = EVO_TYPE_0,
//     [SPECIES_RELICANTH]         = EVO_TYPE_0,
//     [SPECIES_ARON]              = EVO_TYPE_0,
//     [SPECIES_LAIRON]            = EVO_TYPE_1,
//     [SPECIES_AGGRON]            = EVO_TYPE_2,
//     [SPECIES_CASTFORM]          = EVO_TYPE_SELF,
//     [SPECIES_VOLBEAT]           = EVO_TYPE_1,
//     [SPECIES_ILLUMISE]          = EVO_TYPE_1,
//     [SPECIES_LILEEP]            = EVO_TYPE_0,
//     [SPECIES_CRADILY]           = EVO_TYPE_1,
//     [SPECIES_ANORITH]           = EVO_TYPE_0,
//     [SPECIES_ARMALDO]           = EVO_TYPE_1,
//     [SPECIES_RALTS]             = EVO_TYPE_0,
//     [SPECIES_KIRLIA]            = EVO_TYPE_1,
//     [SPECIES_GARDEVOIR]         = EVO_TYPE_2,
//     [SPECIES_BAGON]             = EVO_TYPE_0,
//     [SPECIES_SHELGON]           = EVO_TYPE_1,
//     [SPECIES_SALAMENCE]         = EVO_TYPE_2,
//     [SPECIES_BELDUM]            = EVO_TYPE_0,
//     [SPECIES_METANG]            = EVO_TYPE_1,
//     [SPECIES_METAGROSS]         = EVO_TYPE_2,
//     [SPECIES_REGIROCK]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_REGICE]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_REGISTEEL]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_KYOGRE]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_GROUDON]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_RAYQUAZA]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_LATIAS]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_LATIOS]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_JIRACHI]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_DEOXYS]            = EVO_TYPE_LEGENDARY,
//     #ifndef POKEMON_EXPANSION
//     [SPECIES_CHIMECHO]          = EVO_TYPE_0,
//     #else
//     [SPECIES_CHIMECHO]          = EVO_TYPE_1,
//     [SPECIES_TURTWIG]           = EVO_TYPE_0,
//     [SPECIES_GROTLE]            = EVO_TYPE_1,
//     [SPECIES_TORTERRA]          = EVO_TYPE_2,
//     [SPECIES_CHIMCHAR]          = EVO_TYPE_0,
//     [SPECIES_MONFERNO]          = EVO_TYPE_1,
//     [SPECIES_INFERNAPE]         = EVO_TYPE_2,
//     [SPECIES_PIPLUP]            = EVO_TYPE_0,
//     [SPECIES_PRINPLUP]          = EVO_TYPE_1,
//     [SPECIES_EMPOLEON]          = EVO_TYPE_2,
//     [SPECIES_STARLY]            = EVO_TYPE_0,
//     [SPECIES_STARAVIA]          = EVO_TYPE_1,
//     [SPECIES_STARAPTOR]         = EVO_TYPE_2,
//     [SPECIES_BIDOOF]            = EVO_TYPE_0,
//     [SPECIES_BIBAREL]           = EVO_TYPE_1,
//     [SPECIES_KRICKETOT]         = EVO_TYPE_0,
//     [SPECIES_KRICKETUNE]        = EVO_TYPE_1,
//     [SPECIES_SHINX]             = EVO_TYPE_0,
//     [SPECIES_LUXIO]             = EVO_TYPE_1,
//     [SPECIES_LUXRAY]            = EVO_TYPE_2,
//     [SPECIES_BUDEW]             = EVO_TYPE_0,
//     [SPECIES_ROSERADE]          = EVO_TYPE_2,
//     [SPECIES_CRANIDOS]          = EVO_TYPE_0,
//     [SPECIES_RAMPARDOS]         = EVO_TYPE_1,
//     [SPECIES_SHIELDON]          = EVO_TYPE_0,
//     [SPECIES_BASTIODON]         = EVO_TYPE_1,
//     [SPECIES_BURMY]             = EVO_TYPE_0,
//     [SPECIES_WORMADAM]          = EVO_TYPE_1,
//     [SPECIES_MOTHIM]            = EVO_TYPE_1,
//     [SPECIES_COMBEE]            = EVO_TYPE_0,
//     [SPECIES_VESPIQUEN]         = EVO_TYPE_1,
//     [SPECIES_PACHIRISU]         = EVO_TYPE_0,
//     [SPECIES_BUIZEL]            = EVO_TYPE_0,
//     [SPECIES_FLOATZEL]          = EVO_TYPE_1,
//     [SPECIES_CHERUBI]           = EVO_TYPE_0,
//     [SPECIES_CHERRIM]           = EVO_TYPE_1,
//     [SPECIES_SHELLOS]           = EVO_TYPE_0,
//     [SPECIES_GASTRODON]         = EVO_TYPE_1,
//     [SPECIES_AMBIPOM]           = EVO_TYPE_1,
//     [SPECIES_DRIFLOON]          = EVO_TYPE_0,
//     [SPECIES_DRIFBLIM]          = EVO_TYPE_1,
//     [SPECIES_BUNEARY]           = EVO_TYPE_0,
//     [SPECIES_LOPUNNY]           = EVO_TYPE_1,
//     [SPECIES_MISMAGIUS]         = EVO_TYPE_1,
//     [SPECIES_HONCHKROW]         = EVO_TYPE_1,
//     [SPECIES_GLAMEOW]           = EVO_TYPE_0,
//     [SPECIES_PURUGLY]           = EVO_TYPE_1,
//     [SPECIES_CHINGLING]         = EVO_TYPE_0,
//     [SPECIES_STUNKY]            = EVO_TYPE_0,
//     [SPECIES_SKUNTANK]          = EVO_TYPE_1,
//     [SPECIES_BRONZOR]           = EVO_TYPE_0,
//     [SPECIES_BRONZONG]          = EVO_TYPE_1,
//     [SPECIES_BONSLY]            = EVO_TYPE_0,
//     [SPECIES_MIME_JR]           = EVO_TYPE_0,
//     [SPECIES_HAPPINY]           = EVO_TYPE_0,
//     [SPECIES_CHATOT]            = EVO_TYPE_0,
//     [SPECIES_SPIRITOMB]         = EVO_TYPE_0,
//     [SPECIES_GIBLE]             = EVO_TYPE_0,
//     [SPECIES_GABITE]            = EVO_TYPE_1,
//     [SPECIES_GARCHOMP]          = EVO_TYPE_2,
//     [SPECIES_MUNCHLAX]          = EVO_TYPE_0,
//     [SPECIES_RIOLU]             = EVO_TYPE_0,
//     [SPECIES_LUCARIO]           = EVO_TYPE_1,
//     [SPECIES_HIPPOPOTAS]        = EVO_TYPE_0,
//     [SPECIES_HIPPOWDON]         = EVO_TYPE_1,
//     [SPECIES_SKORUPI]           = EVO_TYPE_0,
//     [SPECIES_DRAPION]           = EVO_TYPE_1,
//     [SPECIES_CROAGUNK]          = EVO_TYPE_0,
//     [SPECIES_TOXICROAK]         = EVO_TYPE_1,
//     [SPECIES_CARNIVINE]         = EVO_TYPE_0,
//     [SPECIES_FINNEON]           = EVO_TYPE_0,
//     [SPECIES_LUMINEON]          = EVO_TYPE_1,
//     [SPECIES_MANTYKE]           = EVO_TYPE_0,
//     [SPECIES_SNOVER]            = EVO_TYPE_0,
//     [SPECIES_ABOMASNOW]         = EVO_TYPE_1,
//     [SPECIES_WEAVILE]           = EVO_TYPE_1,
//     [SPECIES_MAGNEZONE]         = EVO_TYPE_2,
//     [SPECIES_LICKILICKY]        = EVO_TYPE_1,
//     [SPECIES_RHYPERIOR]         = EVO_TYPE_2,
//     [SPECIES_TANGROWTH]         = EVO_TYPE_1,
//     [SPECIES_ELECTIVIRE]        = EVO_TYPE_2,
//     [SPECIES_MAGMORTAR]         = EVO_TYPE_2,
//     [SPECIES_TOGEKISS]          = EVO_TYPE_2,
//     [SPECIES_YANMEGA]           = EVO_TYPE_1,
//     [SPECIES_LEAFEON]           = EVO_TYPE_1,
//     [SPECIES_GLACEON]           = EVO_TYPE_1,
//     [SPECIES_GLISCOR]           = EVO_TYPE_1,
//     [SPECIES_MAMOSWINE]         = EVO_TYPE_2,
//     [SPECIES_PORYGON_Z]         = EVO_TYPE_2,
//     [SPECIES_GALLADE]           = EVO_TYPE_2,
//     [SPECIES_PROBOPASS]         = EVO_TYPE_1,
//     [SPECIES_DUSKNOIR]          = EVO_TYPE_2,
//     [SPECIES_FROSLASS]          = EVO_TYPE_1,
//     [SPECIES_ROTOM]             = EVO_TYPE_0,
//     [SPECIES_UXIE]              = EVO_TYPE_LEGENDARY,
//     [SPECIES_MESPRIT]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_AZELF]             = EVO_TYPE_LEGENDARY,
//     [SPECIES_DIALGA]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_PALKIA]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_HEATRAN]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_REGIGIGAS]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_GIRATINA]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_CRESSELIA]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_PHIONE]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_MANAPHY]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_DARKRAI]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_SHAYMIN]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_VICTINI]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_SNIVY]             = EVO_TYPE_0,
//     [SPECIES_SERVINE]           = EVO_TYPE_1,
//     [SPECIES_SERPERIOR]         = EVO_TYPE_2,
//     [SPECIES_TEPIG]             = EVO_TYPE_0,
//     [SPECIES_PIGNITE]           = EVO_TYPE_1,
//     [SPECIES_EMBOAR]            = EVO_TYPE_2,
//     [SPECIES_OSHAWOTT]          = EVO_TYPE_0,
//     [SPECIES_DEWOTT]            = EVO_TYPE_1,
//     [SPECIES_SAMUROTT]          = EVO_TYPE_2,
//     [SPECIES_PATRAT]            = EVO_TYPE_0,
//     [SPECIES_WATCHOG]           = EVO_TYPE_1,
//     [SPECIES_LILLIPUP]          = EVO_TYPE_0,
//     [SPECIES_HERDIER]           = EVO_TYPE_1,
//     [SPECIES_STOUTLAND]         = EVO_TYPE_2,
//     [SPECIES_PURRLOIN]          = EVO_TYPE_0,
//     [SPECIES_LIEPARD]           = EVO_TYPE_1,
//     [SPECIES_PANSAGE]           = EVO_TYPE_0,
//     [SPECIES_SIMISAGE]          = EVO_TYPE_1,
//     [SPECIES_PANSEAR]           = EVO_TYPE_0,
//     [SPECIES_SIMISEAR]          = EVO_TYPE_1,
//     [SPECIES_PANPOUR]           = EVO_TYPE_0,
//     [SPECIES_SIMIPOUR]          = EVO_TYPE_1,
//     [SPECIES_MUNNA]             = EVO_TYPE_0,
//     [SPECIES_MUSHARNA]          = EVO_TYPE_1,
//     [SPECIES_PIDOVE]            = EVO_TYPE_0,
//     [SPECIES_TRANQUILL]         = EVO_TYPE_1,
//     [SPECIES_UNFEZANT]          = EVO_TYPE_2,
//     [SPECIES_BLITZLE]           = EVO_TYPE_0,
//     [SPECIES_ZEBSTRIKA]         = EVO_TYPE_1,
//     [SPECIES_ROGGENROLA]        = EVO_TYPE_0,
//     [SPECIES_BOLDORE]           = EVO_TYPE_1,
//     [SPECIES_GIGALITH]          = EVO_TYPE_2,
//     [SPECIES_WOOBAT]            = EVO_TYPE_0,
//     [SPECIES_SWOOBAT]           = EVO_TYPE_1,
//     [SPECIES_DRILBUR]           = EVO_TYPE_0,
//     [SPECIES_EXCADRILL]         = EVO_TYPE_1,
//     [SPECIES_AUDINO]            = EVO_TYPE_0,
//     [SPECIES_TIMBURR]           = EVO_TYPE_0,
//     [SPECIES_GURDURR]           = EVO_TYPE_1,
//     [SPECIES_CONKELDURR]        = EVO_TYPE_2,
//     [SPECIES_TYMPOLE]           = EVO_TYPE_0,
//     [SPECIES_PALPITOAD]         = EVO_TYPE_1,
//     [SPECIES_SEISMITOAD]        = EVO_TYPE_2,
//     [SPECIES_THROH]             = EVO_TYPE_0,
//     [SPECIES_SAWK]              = EVO_TYPE_0,
//     [SPECIES_SEWADDLE]          = EVO_TYPE_0,
//     [SPECIES_SWADLOON]          = EVO_TYPE_1,
//     [SPECIES_LEAVANNY]          = EVO_TYPE_2,
//     [SPECIES_VENIPEDE]          = EVO_TYPE_0,
//     [SPECIES_WHIRLIPEDE]        = EVO_TYPE_1,
//     [SPECIES_SCOLIPEDE]         = EVO_TYPE_2,
//     [SPECIES_COTTONEE]          = EVO_TYPE_0,
//     [SPECIES_WHIMSICOTT]        = EVO_TYPE_1,
//     [SPECIES_PETILIL]           = EVO_TYPE_0,
//     [SPECIES_LILLIGANT]         = EVO_TYPE_1,
//     [SPECIES_BASCULIN]          = EVO_TYPE_0,
//     [SPECIES_SANDILE]           = EVO_TYPE_0,
//     [SPECIES_KROKOROK]          = EVO_TYPE_1,
//     [SPECIES_KROOKODILE]        = EVO_TYPE_2,
//     [SPECIES_DARUMAKA]          = EVO_TYPE_0,
//     [SPECIES_DARMANITAN]        = EVO_TYPE_1,
//     [SPECIES_MARACTUS]          = EVO_TYPE_0,
//     [SPECIES_DWEBBLE]           = EVO_TYPE_0,
//     [SPECIES_CRUSTLE]           = EVO_TYPE_1,
//     [SPECIES_SCRAGGY]           = EVO_TYPE_0,
//     [SPECIES_SCRAFTY]           = EVO_TYPE_1,
//     [SPECIES_SIGILYPH]          = EVO_TYPE_0,
//     [SPECIES_YAMASK]            = EVO_TYPE_0,
//     [SPECIES_COFAGRIGUS]        = EVO_TYPE_1,
//     [SPECIES_TIRTOUGA]          = EVO_TYPE_0,
//     [SPECIES_CARRACOSTA]        = EVO_TYPE_1,
//     [SPECIES_ARCHEN]            = EVO_TYPE_0,
//     [SPECIES_ARCHEOPS]          = EVO_TYPE_1,
//     [SPECIES_TRUBBISH]          = EVO_TYPE_0,
//     [SPECIES_GARBODOR]          = EVO_TYPE_1,
//     [SPECIES_ZORUA]             = EVO_TYPE_0,
//     [SPECIES_ZOROARK]           = EVO_TYPE_1,
//     [SPECIES_MINCCINO]          = EVO_TYPE_0,
//     [SPECIES_CINCCINO]          = EVO_TYPE_1,
//     [SPECIES_GOTHITA]           = EVO_TYPE_0,
//     [SPECIES_GOTHORITA]         = EVO_TYPE_1,
//     [SPECIES_GOTHITELLE]        = EVO_TYPE_2,
//     [SPECIES_SOLOSIS]           = EVO_TYPE_0,
//     [SPECIES_DUOSION]           = EVO_TYPE_1,
//     [SPECIES_REUNICLUS]         = EVO_TYPE_2,
//     [SPECIES_DUCKLETT]          = EVO_TYPE_0,
//     [SPECIES_SWANNA]            = EVO_TYPE_1,
//     [SPECIES_VANILLITE]         = EVO_TYPE_0,
//     [SPECIES_VANILLISH]         = EVO_TYPE_1,
//     [SPECIES_VANILLUXE]         = EVO_TYPE_2,
//     [SPECIES_DEERLING]          = EVO_TYPE_0,
//     [SPECIES_SAWSBUCK]          = EVO_TYPE_1,
//     [SPECIES_EMOLGA]            = EVO_TYPE_0,
//     [SPECIES_KARRABLAST]        = EVO_TYPE_0,
//     [SPECIES_ESCAVALIER]        = EVO_TYPE_1,
//     [SPECIES_FOONGUS]           = EVO_TYPE_0,
//     [SPECIES_AMOONGUSS]         = EVO_TYPE_1,
//     [SPECIES_FRILLISH]          = EVO_TYPE_0,
//     [SPECIES_JELLICENT]         = EVO_TYPE_1,
//     [SPECIES_ALOMOMOLA]         = EVO_TYPE_0,
//     [SPECIES_JOLTIK]            = EVO_TYPE_0,
//     [SPECIES_GALVANTULA]        = EVO_TYPE_1,
//     [SPECIES_FERROSEED]         = EVO_TYPE_0,
//     [SPECIES_FERROTHORN]        = EVO_TYPE_1,
//     [SPECIES_KLINK]             = EVO_TYPE_0,
//     [SPECIES_KLANG]             = EVO_TYPE_1,
//     [SPECIES_KLINKLANG]         = EVO_TYPE_2,
//     [SPECIES_TYNAMO]            = EVO_TYPE_0,
//     [SPECIES_EELEKTRIK]         = EVO_TYPE_1,
//     [SPECIES_EELEKTROSS]        = EVO_TYPE_2,
//     [SPECIES_ELGYEM]            = EVO_TYPE_0,
//     [SPECIES_BEHEEYEM]          = EVO_TYPE_1,
//     [SPECIES_LITWICK]           = EVO_TYPE_0,
//     [SPECIES_LAMPENT]           = EVO_TYPE_1,
//     [SPECIES_CHANDELURE]        = EVO_TYPE_2,
//     [SPECIES_AXEW]              = EVO_TYPE_0,
//     [SPECIES_FRAXURE]           = EVO_TYPE_1,
//     [SPECIES_HAXORUS]           = EVO_TYPE_2,
//     [SPECIES_CUBCHOO]           = EVO_TYPE_0,
//     [SPECIES_BEARTIC]           = EVO_TYPE_1,
//     [SPECIES_CRYOGONAL]         = EVO_TYPE_0,
//     [SPECIES_SHELMET]           = EVO_TYPE_0,
//     [SPECIES_ACCELGOR]          = EVO_TYPE_1,
//     [SPECIES_STUNFISK]          = EVO_TYPE_0,
//     [SPECIES_MIENFOO]           = EVO_TYPE_0,
//     [SPECIES_MIENSHAO]          = EVO_TYPE_1,
//     [SPECIES_DRUDDIGON]         = EVO_TYPE_0,
//     [SPECIES_GOLETT]            = EVO_TYPE_0,
//     [SPECIES_GOLURK]            = EVO_TYPE_1,
//     [SPECIES_PAWNIARD]          = EVO_TYPE_0,
//     [SPECIES_BISHARP]           = EVO_TYPE_1,
//     [SPECIES_BOUFFALANT]        = EVO_TYPE_0,
//     [SPECIES_RUFFLET]           = EVO_TYPE_0,
//     [SPECIES_BRAVIARY]          = EVO_TYPE_1,
//     [SPECIES_VULLABY]           = EVO_TYPE_0,
//     [SPECIES_MANDIBUZZ]         = EVO_TYPE_1,
//     [SPECIES_HEATMOR]           = EVO_TYPE_0,
//     [SPECIES_DURANT]            = EVO_TYPE_0,
//     [SPECIES_DEINO]             = EVO_TYPE_0,
//     [SPECIES_ZWEILOUS]          = EVO_TYPE_1,
//     [SPECIES_HYDREIGON]         = EVO_TYPE_2,
//     [SPECIES_LARVESTA]          = EVO_TYPE_0,
//     [SPECIES_VOLCARONA]         = EVO_TYPE_1,
//     [SPECIES_COBALION]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_TERRAKION]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_VIRIZION]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_TORNADUS]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_THUNDURUS]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_RESHIRAM]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZEKROM]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_LANDORUS]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_KYUREM]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_KELDEO]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_MELOETTA]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_GENESECT]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_CHESPIN]           = EVO_TYPE_0,
//     [SPECIES_QUILLADIN]         = EVO_TYPE_1,
//     [SPECIES_CHESNAUGHT]        = EVO_TYPE_2,
//     [SPECIES_FENNEKIN]          = EVO_TYPE_0,
//     [SPECIES_BRAIXEN]           = EVO_TYPE_1,
//     [SPECIES_DELPHOX]           = EVO_TYPE_2,
//     [SPECIES_FROAKIE]           = EVO_TYPE_0,
//     [SPECIES_FROGADIER]         = EVO_TYPE_1,
//     [SPECIES_GRENINJA]          = EVO_TYPE_2,
//     [SPECIES_BUNNELBY]          = EVO_TYPE_0,
//     [SPECIES_DIGGERSBY]         = EVO_TYPE_1,
//     [SPECIES_FLETCHLING]        = EVO_TYPE_0,
//     [SPECIES_FLETCHINDER]       = EVO_TYPE_1,
//     [SPECIES_TALONFLAME]        = EVO_TYPE_2,
//     [SPECIES_SCATTERBUG]        = EVO_TYPE_0,
//     [SPECIES_SPEWPA]            = EVO_TYPE_1,
//     [SPECIES_VIVILLON]          = EVO_TYPE_2,
//     [SPECIES_LITLEO]            = EVO_TYPE_0,
//     [SPECIES_PYROAR]            = EVO_TYPE_1,
//     [SPECIES_FLABEBE]           = EVO_TYPE_0,
//     [SPECIES_FLOETTE]           = EVO_TYPE_1,
//     [SPECIES_FLORGES]           = EVO_TYPE_2,
//     [SPECIES_SKIDDO]            = EVO_TYPE_0,
//     [SPECIES_GOGOAT]            = EVO_TYPE_1,
//     [SPECIES_PANCHAM]           = EVO_TYPE_0,
//     [SPECIES_PANGORO]           = EVO_TYPE_1,
//     [SPECIES_FURFROU]           = EVO_TYPE_0,
//     [SPECIES_ESPURR]            = EVO_TYPE_0,
//     [SPECIES_MEOWSTIC]          = EVO_TYPE_1,
//     [SPECIES_HONEDGE]           = EVO_TYPE_0,
//     [SPECIES_DOUBLADE]          = EVO_TYPE_1,
//     [SPECIES_AEGISLASH]         = EVO_TYPE_2,
//     [SPECIES_SPRITZEE]          = EVO_TYPE_0,
//     [SPECIES_AROMATISSE]        = EVO_TYPE_1,
//     [SPECIES_SWIRLIX]           = EVO_TYPE_0,
//     [SPECIES_SLURPUFF]          = EVO_TYPE_1,
//     [SPECIES_INKAY]             = EVO_TYPE_0,
//     [SPECIES_MALAMAR]           = EVO_TYPE_1,
//     [SPECIES_BINACLE]           = EVO_TYPE_0,
//     [SPECIES_BARBARACLE]        = EVO_TYPE_1,
//     [SPECIES_SKRELP]            = EVO_TYPE_0,
//     [SPECIES_DRAGALGE]          = EVO_TYPE_1,
//     [SPECIES_CLAUNCHER]         = EVO_TYPE_0,
//     [SPECIES_CLAWITZER]         = EVO_TYPE_1,
//     [SPECIES_HELIOPTILE]        = EVO_TYPE_0,
//     [SPECIES_HELIOLISK]         = EVO_TYPE_1,
//     [SPECIES_TYRUNT]            = EVO_TYPE_0,
//     [SPECIES_TYRANTRUM]         = EVO_TYPE_1,
//     [SPECIES_AMAURA]            = EVO_TYPE_0,
//     [SPECIES_AURORUS]           = EVO_TYPE_1,
//     [SPECIES_SYLVEON]           = EVO_TYPE_1,
//     [SPECIES_HAWLUCHA]          = EVO_TYPE_0,
//     [SPECIES_DEDENNE]           = EVO_TYPE_0,
//     [SPECIES_CARBINK]           = EVO_TYPE_0,
//     [SPECIES_GOOMY]             = EVO_TYPE_0,
//     [SPECIES_SLIGGOO]           = EVO_TYPE_1,
//     [SPECIES_GOODRA]            = EVO_TYPE_2,
//     [SPECIES_KLEFKI]            = EVO_TYPE_0,
//     [SPECIES_PHANTUMP]          = EVO_TYPE_0,
//     [SPECIES_TREVENANT]         = EVO_TYPE_1,
//     [SPECIES_PUMPKABOO]         = EVO_TYPE_0,
//     [SPECIES_GOURGEIST]         = EVO_TYPE_1,
//     [SPECIES_BERGMITE]          = EVO_TYPE_0,
//     [SPECIES_AVALUGG]           = EVO_TYPE_1,
//     [SPECIES_NOIBAT]            = EVO_TYPE_0,
//     [SPECIES_NOIVERN]           = EVO_TYPE_1,
//     [SPECIES_XERNEAS]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_YVELTAL]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZYGARDE]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_DIANCIE]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_HOOPA]             = EVO_TYPE_LEGENDARY,
//     [SPECIES_VOLCANION]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_ROWLET]            = EVO_TYPE_0,
//     [SPECIES_DARTRIX]           = EVO_TYPE_1,
//     [SPECIES_DECIDUEYE]         = EVO_TYPE_2,
//     [SPECIES_LITTEN]            = EVO_TYPE_0,
//     [SPECIES_TORRACAT]          = EVO_TYPE_1,
//     [SPECIES_INCINEROAR]        = EVO_TYPE_2,
//     [SPECIES_POPPLIO]           = EVO_TYPE_0,
//     [SPECIES_BRIONNE]           = EVO_TYPE_1,
//     [SPECIES_PRIMARINA]         = EVO_TYPE_2,
//     [SPECIES_PIKIPEK]           = EVO_TYPE_0,
//     [SPECIES_TRUMBEAK]          = EVO_TYPE_1,
//     [SPECIES_TOUCANNON]         = EVO_TYPE_2,
//     [SPECIES_YUNGOOS]           = EVO_TYPE_0,
//     [SPECIES_GUMSHOOS]          = EVO_TYPE_1,
//     [SPECIES_GRUBBIN]           = EVO_TYPE_0,
//     [SPECIES_CHARJABUG]         = EVO_TYPE_1,
//     [SPECIES_VIKAVOLT]          = EVO_TYPE_2,
//     [SPECIES_CRABRAWLER]        = EVO_TYPE_0,
//     [SPECIES_CRABOMINABLE]      = EVO_TYPE_1,
//     [SPECIES_ORICORIO]          = EVO_TYPE_0,
//     [SPECIES_CUTIEFLY]          = EVO_TYPE_0,
//     [SPECIES_RIBOMBEE]          = EVO_TYPE_1,
//     [SPECIES_ROCKRUFF]          = EVO_TYPE_0,
//     [SPECIES_LYCANROC]          = EVO_TYPE_1,
//     [SPECIES_WISHIWASHI]        = EVO_TYPE_0,
//     [SPECIES_MAREANIE]          = EVO_TYPE_0,
//     [SPECIES_TOXAPEX]           = EVO_TYPE_1,
//     [SPECIES_MUDBRAY]           = EVO_TYPE_0,
//     [SPECIES_MUDSDALE]          = EVO_TYPE_1,
//     [SPECIES_DEWPIDER]          = EVO_TYPE_0,
//     [SPECIES_ARAQUANID]         = EVO_TYPE_1,
//     [SPECIES_FOMANTIS]          = EVO_TYPE_0,
//     [SPECIES_LURANTIS]          = EVO_TYPE_1,
//     [SPECIES_MORELULL]          = EVO_TYPE_0,
//     [SPECIES_SHIINOTIC]         = EVO_TYPE_1,
//     [SPECIES_SALANDIT]          = EVO_TYPE_0,
//     [SPECIES_SALAZZLE]          = EVO_TYPE_1,
//     [SPECIES_STUFFUL]           = EVO_TYPE_0,
//     [SPECIES_BEWEAR]            = EVO_TYPE_1,
//     [SPECIES_BOUNSWEET]         = EVO_TYPE_0,
//     [SPECIES_STEENEE]           = EVO_TYPE_1,
//     [SPECIES_TSAREENA]          = EVO_TYPE_2,
//     [SPECIES_COMFEY]            = EVO_TYPE_0,
//     [SPECIES_ORANGURU]          = EVO_TYPE_0,
//     [SPECIES_PASSIMIAN]         = EVO_TYPE_0,
//     [SPECIES_WIMPOD]            = EVO_TYPE_0,
//     [SPECIES_GOLISOPOD]         = EVO_TYPE_1,
//     [SPECIES_SANDYGAST]         = EVO_TYPE_0,
//     [SPECIES_PALOSSAND]         = EVO_TYPE_1,
//     [SPECIES_PYUKUMUKU]         = EVO_TYPE_0,
//     [SPECIES_TYPE_NULL]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_MINIOR]            = EVO_TYPE_0,
//     [SPECIES_KOMALA]            = EVO_TYPE_0,
//     [SPECIES_TURTONATOR]        = EVO_TYPE_0,
//     [SPECIES_TOGEDEMARU]        = EVO_TYPE_0,
//     [SPECIES_MIMIKYU]           = EVO_TYPE_0,
//     [SPECIES_BRUXISH]           = EVO_TYPE_0,
//     [SPECIES_DRAMPA]            = EVO_TYPE_0,
//     [SPECIES_DHELMISE]          = EVO_TYPE_0,
//     [SPECIES_JANGMO_O]          = EVO_TYPE_0,
//     [SPECIES_HAKAMO_O]          = EVO_TYPE_1,
//     [SPECIES_KOMMO_O]           = EVO_TYPE_2,
//     [SPECIES_TAPU_KOKO]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_TAPU_LELE]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_TAPU_BULU]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_TAPU_FINI]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_COSMOG]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_COSMOEM]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_SOLGALEO]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_LUNALA]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_NIHILEGO]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_BUZZWOLE]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_PHEROMOSA]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_XURKITREE]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_CELESTEELA]        = EVO_TYPE_LEGENDARY,
//     [SPECIES_KARTANA]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_GUZZLORD]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_NECROZMA]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_MAGEARNA]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_MARSHADOW]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_POIPOLE]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_NAGANADEL]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_STAKATAKA]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_BLACEPHALON]       = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZERAORA]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_MELTAN]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_MELMETAL]          = EVO_TYPE_LEGENDARY,
//     [SPECIES_GROOKEY]           = EVO_TYPE_0,
//     [SPECIES_THWACKEY]          = EVO_TYPE_1,
//     [SPECIES_RILLABOOM]         = EVO_TYPE_2,
//     [SPECIES_SCORBUNNY]         = EVO_TYPE_0,
//     [SPECIES_RABOOT]            = EVO_TYPE_1,
//     [SPECIES_CINDERACE]         = EVO_TYPE_2,
//     [SPECIES_SOBBLE]            = EVO_TYPE_0,
//     [SPECIES_DRIZZILE]          = EVO_TYPE_1,
//     [SPECIES_INTELEON]          = EVO_TYPE_2,
//     [SPECIES_SKWOVET]           = EVO_TYPE_0,
//     [SPECIES_GREEDENT]          = EVO_TYPE_1,
//     [SPECIES_ROOKIDEE]          = EVO_TYPE_0,
//     [SPECIES_CORVISQUIRE]       = EVO_TYPE_1,
//     [SPECIES_CORVIKNIGHT]       = EVO_TYPE_2,
//     [SPECIES_BLIPBUG]           = EVO_TYPE_0,
//     [SPECIES_DOTTLER]           = EVO_TYPE_1,
//     [SPECIES_ORBEETLE]          = EVO_TYPE_2,
//     [SPECIES_NICKIT]            = EVO_TYPE_0,
//     [SPECIES_THIEVUL]           = EVO_TYPE_1,
//     [SPECIES_GOSSIFLEUR]        = EVO_TYPE_0,
//     [SPECIES_ELDEGOSS]          = EVO_TYPE_1,
//     [SPECIES_WOOLOO]            = EVO_TYPE_0,
//     [SPECIES_DUBWOOL]           = EVO_TYPE_1,
//     [SPECIES_CHEWTLE]           = EVO_TYPE_0,
//     [SPECIES_DREDNAW]           = EVO_TYPE_1,
//     [SPECIES_YAMPER]            = EVO_TYPE_0,
//     [SPECIES_BOLTUND]           = EVO_TYPE_1,
//     [SPECIES_ROLYCOLY]          = EVO_TYPE_0,
//     [SPECIES_CARKOL]            = EVO_TYPE_1,
//     [SPECIES_COALOSSAL]         = EVO_TYPE_2,
//     [SPECIES_APPLIN]            = EVO_TYPE_0,
//     [SPECIES_FLAPPLE]           = EVO_TYPE_1,
//     [SPECIES_APPLETUN]          = EVO_TYPE_1,
//     [SPECIES_SILICOBRA]         = EVO_TYPE_0,
//     [SPECIES_SANDACONDA]        = EVO_TYPE_1,
//     [SPECIES_CRAMORANT]         = EVO_TYPE_0,
//     [SPECIES_ARROKUDA]          = EVO_TYPE_0,
//     [SPECIES_BARRASKEWDA]       = EVO_TYPE_1,
//     [SPECIES_TOXEL]             = EVO_TYPE_0,
//     [SPECIES_TOXTRICITY]        = EVO_TYPE_1,
//     [SPECIES_SIZZLIPEDE]        = EVO_TYPE_0,
//     [SPECIES_CENTISKORCH]       = EVO_TYPE_1,
//     [SPECIES_CLOBBOPUS]         = EVO_TYPE_0,
//     [SPECIES_GRAPPLOCT]         = EVO_TYPE_1,
//     [SPECIES_SINISTEA]          = EVO_TYPE_0,
//     [SPECIES_POLTEAGEIST]       = EVO_TYPE_1,
//     [SPECIES_HATENNA]           = EVO_TYPE_0,
//     [SPECIES_HATTREM]           = EVO_TYPE_1,
//     [SPECIES_HATTERENE]         = EVO_TYPE_2,
//     [SPECIES_IMPIDIMP]          = EVO_TYPE_0,
//     [SPECIES_MORGREM]           = EVO_TYPE_1,
//     [SPECIES_GRIMMSNARL]        = EVO_TYPE_2,
//     [SPECIES_OBSTAGOON]         = EVO_TYPE_2,
//     [SPECIES_PERRSERKER]        = EVO_TYPE_1,
//     [SPECIES_CURSOLA]           = EVO_TYPE_1,
//     [SPECIES_SIRFETCHD]         = EVO_TYPE_1,
//     [SPECIES_MR_RIME]           = EVO_TYPE_2,
//     [SPECIES_RUNERIGUS]         = EVO_TYPE_1,
//     [SPECIES_MILCERY]           = EVO_TYPE_0,
//     [SPECIES_ALCREMIE]          = EVO_TYPE_1,
//     [SPECIES_FALINKS]           = EVO_TYPE_0,
//     [SPECIES_PINCURCHIN]        = EVO_TYPE_0,
//     [SPECIES_SNOM]              = EVO_TYPE_0,
//     [SPECIES_FROSMOTH]          = EVO_TYPE_1,
//     [SPECIES_STONJOURNER]       = EVO_TYPE_0,
//     [SPECIES_EISCUE]            = EVO_TYPE_0,
//     [SPECIES_INDEEDEE]          = EVO_TYPE_0,
//     [SPECIES_MORPEKO]           = EVO_TYPE_0,
//     [SPECIES_CUFANT]            = EVO_TYPE_0,
//     [SPECIES_COPPERAJAH]        = EVO_TYPE_1,
//     [SPECIES_DRACOZOLT]         = EVO_TYPE_0,
//     [SPECIES_ARCTOZOLT]         = EVO_TYPE_0,
//     [SPECIES_DRACOVISH]         = EVO_TYPE_0,
//     [SPECIES_ARCTOVISH]         = EVO_TYPE_0,
//     [SPECIES_DURALUDON]         = EVO_TYPE_0,
//     [SPECIES_DREEPY]            = EVO_TYPE_0,
//     [SPECIES_DRAKLOAK]          = EVO_TYPE_1,
//     [SPECIES_DRAGAPULT]         = EVO_TYPE_2,
//     [SPECIES_ZACIAN]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZAMAZENTA]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_ETERNATUS]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_KUBFU]             = EVO_TYPE_LEGENDARY,
//     [SPECIES_URSHIFU]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZARUDE]            = EVO_TYPE_LEGENDARY,
//     [SPECIES_REGIELEKI]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_REGIDRAGO]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_GLASTRIER]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_SPECTRIER]         = EVO_TYPE_LEGENDARY,
//     [SPECIES_CALYREX]           = EVO_TYPE_LEGENDARY,
//     [SPECIES_VENUSAUR_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_CHARIZARD_MEGA_X]  = EVO_TYPE_SELF,
//     [SPECIES_CHARIZARD_MEGA_Y]  = EVO_TYPE_SELF,
//     [SPECIES_BLASTOISE_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_BEEDRILL_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_PIDGEOT_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_ALAKAZAM_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_SLOWBRO_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_GENGAR_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_KANGASKHAN_MEGA]   = EVO_TYPE_SELF,
//     [SPECIES_PINSIR_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_GYARADOS_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_AERODACTYL_MEGA]   = EVO_TYPE_SELF,
//     [SPECIES_MEWTWO_MEGA_X]     = EVO_TYPE_SELF,
//     [SPECIES_MEWTWO_MEGA_Y]     = EVO_TYPE_SELF,
//     [SPECIES_AMPHAROS_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_STEELIX_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_SCIZOR_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_HERACROSS_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_HOUNDOOM_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_TYRANITAR_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_SCEPTILE_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_BLAZIKEN_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_SWAMPERT_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_GARDEVOIR_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_SABLEYE_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_MAWILE_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_AGGRON_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_MEDICHAM_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_MANECTRIC_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_SHARPEDO_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_CAMERUPT_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_ALTARIA_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_BANETTE_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_ABSOL_MEGA]        = EVO_TYPE_SELF,
//     [SPECIES_GLALIE_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_SALAMENCE_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_METAGROSS_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_LATIAS_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_LATIOS_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_LOPUNNY_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_GARCHOMP_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_LUCARIO_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_ABOMASNOW_MEGA]    = EVO_TYPE_SELF,
//     [SPECIES_GALLADE_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_AUDINO_MEGA]       = EVO_TYPE_SELF,
//     [SPECIES_DIANCIE_MEGA]      = EVO_TYPE_SELF,
//     [SPECIES_RAYQUAZA_MEGA]     = EVO_TYPE_SELF,
//     [SPECIES_KYOGRE_PRIMAL]     = EVO_TYPE_SELF,
//     [SPECIES_GROUDON_PRIMAL]    = EVO_TYPE_SELF,
//     [SPECIES_RATTATA_ALOLA]    = EVO_TYPE_0,
//     [SPECIES_RATICATE_ALOLA]   = EVO_TYPE_1,
//     [SPECIES_RAICHU_ALOLA]     = EVO_TYPE_2,
//     [SPECIES_SANDSHREW_ALOLA]  = EVO_TYPE_0,
//     [SPECIES_SANDSLASH_ALOLA]  = EVO_TYPE_1,
//     [SPECIES_VULPIX_ALOLA]     = EVO_TYPE_0,
//     [SPECIES_NINETALES_ALOLA]  = EVO_TYPE_1,
//     [SPECIES_DIGLETT_ALOLA]    = EVO_TYPE_0,
//     [SPECIES_DUGTRIO_ALOLA]    = EVO_TYPE_1,
//     [SPECIES_MEOWTH_ALOLA]     = EVO_TYPE_0,
//     [SPECIES_PERSIAN_ALOLA]    = EVO_TYPE_1,
//     [SPECIES_GEODUDE_ALOLA]    = EVO_TYPE_0,
//     [SPECIES_GRAVELER_ALOLA]   = EVO_TYPE_1,
//     [SPECIES_GOLEM_ALOLA]      = EVO_TYPE_2,
//     [SPECIES_GRIMER_ALOLA]     = EVO_TYPE_0,
//     [SPECIES_MUK_ALOLA]        = EVO_TYPE_1,
//     [SPECIES_EXEGGUTOR_ALOLA]  = EVO_TYPE_1,
//     [SPECIES_MAROWAK_ALOLA]    = EVO_TYPE_1,
//     [SPECIES_MEOWTH_GALAR]   = EVO_TYPE_0,
//     [SPECIES_PONYTA_GALAR]   = EVO_TYPE_0,
//     [SPECIES_RAPIDASH_GALAR] = EVO_TYPE_1,
//     [SPECIES_SLOWPOKE_GALAR] = EVO_TYPE_0,
//     [SPECIES_SLOWBRO_GALAR]  = EVO_TYPE_1,
//     [SPECIES_FARFETCHD_GALAR] = EVO_TYPE_0,
//     [SPECIES_WEEZING_GALAR]  = EVO_TYPE_1,
//     [SPECIES_MR_MIME_GALAR]  = EVO_TYPE_1,
//     [SPECIES_ARTICUNO_GALAR] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZAPDOS_GALAR]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_MOLTRES_GALAR]  = EVO_TYPE_LEGENDARY,
//     [SPECIES_SLOWKING_GALAR] = EVO_TYPE_1,
//     [SPECIES_CORSOLA_GALAR]  = EVO_TYPE_1,
//     [SPECIES_ZIGZAGOON_GALAR] = EVO_TYPE_0,
//     [SPECIES_LINOONE_GALAR]  = EVO_TYPE_1,
//     [SPECIES_DARUMAKA_GALAR] = EVO_TYPE_0,
//     [SPECIES_DARMANITAN_GALAR] = EVO_TYPE_1,
//     [SPECIES_YAMASK_GALAR]   = EVO_TYPE_0,
//     [SPECIES_STUNFISK_GALAR] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_COSPLAY]   = EVO_TYPE_0,
//     [SPECIES_PIKACHU_ROCK_STAR] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_BELLE]     = EVO_TYPE_0,
//     [SPECIES_PIKACHU_POP_STAR]  = EVO_TYPE_0,
//     [SPECIES_PIKACHU_PHD]      = EVO_TYPE_0,
//     [SPECIES_PIKACHU_LIBRE]     = EVO_TYPE_0,
//     [SPECIES_PIKACHU_ORIGINAL] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_HOENN] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_SINNOH] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_UNOVA] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_KALOS] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_ALOLA] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_PARTNER] = EVO_TYPE_0,
//     [SPECIES_PIKACHU_WORLD] = EVO_TYPE_0,
//     [SPECIES_PICHU_SPIKY_EARED] = EVO_TYPE_0,
//     [SPECIES_UNOWN_B]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_C]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_D]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_E]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_F]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_G]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_H]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_I]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_J]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_K]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_L]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_M]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_N]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_O]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_P]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_Q]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_R]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_S]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_T]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_U]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_V]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_W]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_X]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_Y]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_Z]           = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_EXCLAMATION]       = EVO_TYPE_SELF,
//     [SPECIES_UNOWN_QUESTION]       = EVO_TYPE_SELF,
//     [SPECIES_CASTFORM_SUNNY]    = EVO_TYPE_SELF,
//     [SPECIES_CASTFORM_RAINY]    = EVO_TYPE_SELF,
//     [SPECIES_CASTFORM_SNOWY]    = EVO_TYPE_SELF,
//     [SPECIES_DEOXYS_ATTACK]     = EVO_TYPE_SELF,
//     [SPECIES_DEOXYS_DEFENSE]    = EVO_TYPE_SELF,
//     [SPECIES_DEOXYS_SPEED]      = EVO_TYPE_SELF,
//     [SPECIES_BURMY_SANDY] = EVO_TYPE_0,
//     [SPECIES_BURMY_TRASH] = EVO_TYPE_0,
//     [SPECIES_WORMADAM_SANDY] = EVO_TYPE_1,
//     [SPECIES_WORMADAM_TRASH] = EVO_TYPE_1,
//     [SPECIES_CHERRIM_SUNSHINE]  = EVO_TYPE_1,
//     [SPECIES_SHELLOS_EAST]  = EVO_TYPE_0,
//     [SPECIES_GASTRODON_EAST] = EVO_TYPE_1,
//     [SPECIES_ROTOM_HEAT]        = EVO_TYPE_0,
//     [SPECIES_ROTOM_WASH]        = EVO_TYPE_0,
//     [SPECIES_ROTOM_FROST]       = EVO_TYPE_0,
//     [SPECIES_ROTOM_FAN]         = EVO_TYPE_0,
//     [SPECIES_ROTOM_MOW]         = EVO_TYPE_0,
//     [SPECIES_GIRATINA_ORIGIN]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_SHAYMIN_SKY]       = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_FIGHTING]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_FLYING]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_POISON]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_GROUND]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_ROCK]       = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_BUG]        = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_GHOST]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_STEEL]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_FIRE]       = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_WATER]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_GRASS]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_ELECTRIC]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_PSYCHIC]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_ICE]        = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_DRAGON]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_DARK]       = EVO_TYPE_LEGENDARY,
//     [SPECIES_ARCEUS_FAIRY]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_BASCULIN_BLUE_STRIPED] = EVO_TYPE_0,
//     [SPECIES_DARMANITAN_ZEN] = EVO_TYPE_1,
//     [SPECIES_DARMANITAN_GALAR_ZEN] = EVO_TYPE_1,
//     [SPECIES_DEERLING_SUMMER]   = EVO_TYPE_0,
//     [SPECIES_DEERLING_AUTUMN]   = EVO_TYPE_0,
//     [SPECIES_DEERLING_WINTER]   = EVO_TYPE_0,
//     [SPECIES_SAWSBUCK_SUMMER]   = EVO_TYPE_1,
//     [SPECIES_SAWSBUCK_AUTUMN]   = EVO_TYPE_1,
//     [SPECIES_SAWSBUCK_WINTER]   = EVO_TYPE_1,
//     [SPECIES_TORNADUS_THERIAN]  = EVO_TYPE_LEGENDARY,
//     [SPECIES_THUNDURUS_THERIAN] = EVO_TYPE_LEGENDARY,
//     [SPECIES_LANDORUS_THERIAN]  = EVO_TYPE_LEGENDARY,
//     [SPECIES_KYUREM_WHITE]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_KYUREM_BLACK]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_KELDEO_RESOLUTE]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_MELOETTA_PIROUETTE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_GENESECT_DOUSE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_GENESECT_SHOCK] = EVO_TYPE_LEGENDARY,
//     [SPECIES_GENESECT_BURN] = EVO_TYPE_LEGENDARY,
//     [SPECIES_GENESECT_CHILL] = EVO_TYPE_LEGENDARY,
//     [SPECIES_GRENINJA_BATTLE_BOND] = EVO_TYPE_2,
//     [SPECIES_GRENINJA_ASH]      = EVO_TYPE_2,
//     [SPECIES_VIVILLON_POLAR]    = EVO_TYPE_2,
//     [SPECIES_VIVILLON_TUNDRA]   = EVO_TYPE_2,
//     [SPECIES_VIVILLON_CONTINENTAL] = EVO_TYPE_2,
//     [SPECIES_VIVILLON_GARDEN]   = EVO_TYPE_2,
//     [SPECIES_VIVILLON_ELEGANT]  = EVO_TYPE_2,
//     [SPECIES_VIVILLON_MEADOW]   = EVO_TYPE_2,
//     [SPECIES_VIVILLON_MODERN]   = EVO_TYPE_2,
//     [SPECIES_VIVILLON_MARINE]   = EVO_TYPE_2,
//     [SPECIES_VIVILLON_ARCHIPELAGO] = EVO_TYPE_2,
//     [SPECIES_VIVILLON_HIGH_PLAINS] = EVO_TYPE_2,
//     [SPECIES_VIVILLON_SANDSTORM] = EVO_TYPE_2,
//     [SPECIES_VIVILLON_RIVER]    = EVO_TYPE_2,
//     [SPECIES_VIVILLON_MONSOON]  = EVO_TYPE_2,
//     [SPECIES_VIVILLON_SAVANNA]  = EVO_TYPE_2,
//     [SPECIES_VIVILLON_SUN]      = EVO_TYPE_2,
//     [SPECIES_VIVILLON_OCEAN]    = EVO_TYPE_2,
//     [SPECIES_VIVILLON_JUNGLE]   = EVO_TYPE_2,
//     [SPECIES_VIVILLON_FANCY]    = EVO_TYPE_2,
//     [SPECIES_VIVILLON_POKEBALL] = EVO_TYPE_2,
//     [SPECIES_FLABEBE_YELLOW] = EVO_TYPE_0,
//     [SPECIES_FLABEBE_ORANGE] = EVO_TYPE_0,
//     [SPECIES_FLABEBE_BLUE] = EVO_TYPE_0,
//     [SPECIES_FLABEBE_WHITE] = EVO_TYPE_0,
//     [SPECIES_FLOETTE_YELLOW] = EVO_TYPE_1,
//     [SPECIES_FLOETTE_ORANGE] = EVO_TYPE_1,
//     [SPECIES_FLOETTE_BLUE] = EVO_TYPE_1,
//     [SPECIES_FLOETTE_WHITE] = EVO_TYPE_1,
//     [SPECIES_FLOETTE_ETERNAL] = EVO_TYPE_0,
//     [SPECIES_FLORGES_YELLOW] = EVO_TYPE_2,
//     [SPECIES_FLORGES_ORANGE] = EVO_TYPE_2,
//     [SPECIES_FLORGES_BLUE] = EVO_TYPE_2,
//     [SPECIES_FLORGES_WHITE] = EVO_TYPE_2,
//     [SPECIES_FURFROU_HEART] = EVO_TYPE_0,
//     [SPECIES_FURFROU_STAR] = EVO_TYPE_0,
//     [SPECIES_FURFROU_DIAMOND] = EVO_TYPE_0,
//     [SPECIES_FURFROU_DEBUTANTE] = EVO_TYPE_0,
//     [SPECIES_FURFROU_MATRON] = EVO_TYPE_0,
//     [SPECIES_FURFROU_DANDY] = EVO_TYPE_0,
//     [SPECIES_FURFROU_LA_REINE] = EVO_TYPE_0,
//     [SPECIES_FURFROU_KABUKI] = EVO_TYPE_0,
//     [SPECIES_FURFROU_PHARAOH] = EVO_TYPE_0,
//     [SPECIES_MEOWSTIC_F]   = EVO_TYPE_1,
//     [SPECIES_AEGISLASH_BLADE]   = EVO_TYPE_2,
//     [SPECIES_PUMPKABOO_SMALL]   = EVO_TYPE_0,
//     [SPECIES_PUMPKABOO_LARGE]   = EVO_TYPE_0,
//     [SPECIES_PUMPKABOO_SUPER]   = EVO_TYPE_0,
//     [SPECIES_GOURGEIST_SMALL]   = EVO_TYPE_1,
//     [SPECIES_GOURGEIST_LARGE]   = EVO_TYPE_1,
//     [SPECIES_GOURGEIST_SUPER]   = EVO_TYPE_1,
//     [SPECIES_XERNEAS_ACTIVE]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZYGARDE_10]        = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZYGARDE_10_POWER_CONSTRUCT] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZYGARDE_50_POWER_CONSTRUCT] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZYGARDE_COMPLETE]  = EVO_TYPE_LEGENDARY,
//     [SPECIES_HOOPA_UNBOUND]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_ORICORIO_POM_POM]  = EVO_TYPE_0,
//     [SPECIES_ORICORIO_PAU]      = EVO_TYPE_0,
//     [SPECIES_ORICORIO_SENSU]    = EVO_TYPE_0,
//     [SPECIES_ROCKRUFF_OWN_TEMPO] = EVO_TYPE_0,
//     [SPECIES_LYCANROC_MIDNIGHT] = EVO_TYPE_1,
//     [SPECIES_LYCANROC_DUSK]     = EVO_TYPE_1,
//     [SPECIES_WISHIWASHI_SCHOOL] = EVO_TYPE_0,
//     [SPECIES_SILVALLY_FIGHTING] = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_FLYING]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_POISON]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_GROUND]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_ROCK]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_BUG]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_GHOST]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_STEEL]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_FIRE]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_WATER]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_GRASS]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_ELECTRIC] = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_PSYCHIC]  = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_ICE]      = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_DRAGON]   = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_DARK]     = EVO_TYPE_LEGENDARY,
//     [SPECIES_SILVALLY_FAIRY]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_MINIOR_METEOR_ORANGE] = EVO_TYPE_0,
//     [SPECIES_MINIOR_METEOR_YELLOW] = EVO_TYPE_0,
//     [SPECIES_MINIOR_METEOR_GREEN] = EVO_TYPE_0,
//     [SPECIES_MINIOR_METEOR_BLUE] = EVO_TYPE_0,
//     [SPECIES_MINIOR_METEOR_INDIGO] = EVO_TYPE_0,
//     [SPECIES_MINIOR_METEOR_VIOLET] = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_RED]   = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_ORANGE] = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_YELLOW] = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_GREEN] = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_BLUE]  = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_INDIGO] = EVO_TYPE_0,
//     [SPECIES_MINIOR_CORE_VIOLET] = EVO_TYPE_0,
//     [SPECIES_MIMIKYU_BUSTED]    = EVO_TYPE_0,
//     [SPECIES_NECROZMA_DUSK_MANE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_NECROZMA_DAWN_WINGS] = EVO_TYPE_LEGENDARY,
//     [SPECIES_NECROZMA_ULTRA]    = EVO_TYPE_LEGENDARY,
//     [SPECIES_MAGEARNA_ORIGINAL] = EVO_TYPE_LEGENDARY,
//     [SPECIES_CRAMORANT_GULPING] = EVO_TYPE_0,
//     [SPECIES_CRAMORANT_GORGING] = EVO_TYPE_0,
//     [SPECIES_TOXTRICITY_LOW_KEY] = EVO_TYPE_1,
//     [SPECIES_SINISTEA_ANTIQUE]  = EVO_TYPE_0,
//     [SPECIES_POLTEAGEIST_ANTIQUE] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_RUBY_CREAM] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_MATCHA_CREAM] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_MINT_CREAM] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_LEMON_CREAM] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_SALTED_CREAM] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_RUBY_SWIRL] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_CARAMEL_SWIRL] = EVO_TYPE_1,
//     [SPECIES_ALCREMIE_RAINBOW_SWIRL] = EVO_TYPE_1,
//     [SPECIES_EISCUE_NOICE] = EVO_TYPE_0,
//     [SPECIES_INDEEDEE_F]   = EVO_TYPE_1,
//     [SPECIES_MORPEKO_HANGRY]    = EVO_TYPE_0,
//     [SPECIES_ZACIAN_CROWNED] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZAMAZENTA_CROWNED] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ETERNATUS_ETERNAMAX] = EVO_TYPE_LEGENDARY,
//     [SPECIES_URSHIFU_RAPID_STRIKE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ZARUDE_DADA]       = EVO_TYPE_LEGENDARY,
//     [SPECIES_CALYREX_ICE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_CALYREX_SHADOW] = EVO_TYPE_LEGENDARY,
//     // Legends Arceus
//     [SPECIES_WYRDEER] = EVO_TYPE_1,
//     [SPECIES_KLEAVOR] = EVO_TYPE_1,
//     [SPECIES_URSALUNA] = EVO_TYPE_2,
//     [SPECIES_BASCULEGION_M] = EVO_TYPE_1,
//     [SPECIES_BASCULEGION_F] = EVO_TYPE_1,
//     [SPECIES_SNEASLER] = EVO_TYPE_1,
//     [SPECIES_OVERQWIL] = EVO_TYPE_1,
//     [SPECIES_ENAMORUS_INCARNATE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ENAMORUS_THERIAN] = EVO_TYPE_LEGENDARY,
//     //Hisuian Forms
//     [SPECIES_GROWLITHE_HISUI] = EVO_TYPE_0,
//     [SPECIES_ARCANINE_HISUI] = EVO_TYPE_1,
//     [SPECIES_VOLTORB_HISUI] = EVO_TYPE_0,
//     [SPECIES_ELECTRODE_HISUI] = EVO_TYPE_1,
//     [SPECIES_TYPHLOSION_HISUI] = EVO_TYPE_2,
//     [SPECIES_QWILFISH_HISUI] = EVO_TYPE_0,
//     [SPECIES_SNEASEL_HISUI] = EVO_TYPE_0,
//     [SPECIES_SAMUROTT_HISUI] = EVO_TYPE_2,
//     [SPECIES_LILLIGANT_HISUI] = EVO_TYPE_1,
//     [SPECIES_ZORUA_HISUI] = EVO_TYPE_0,
//     [SPECIES_ZOROARK_HISUI] = EVO_TYPE_1,
//     [SPECIES_BRAVIARY_HISUI] = EVO_TYPE_1,
//     [SPECIES_SLIGGOO_HISUI] = EVO_TYPE_1,
//     [SPECIES_GOODRA_HISUI] = EVO_TYPE_2,
//     [SPECIES_AVALUGG_HISUI] = EVO_TYPE_1,
//     [SPECIES_DECIDUEYE_HISUI] = EVO_TYPE_2,
//     // Gen 8
//     [SPECIES_SPRIGATITO] = EVO_TYPE_0,
//     [SPECIES_FLORAGATO] = EVO_TYPE_1,
//     [SPECIES_MEOWSCARADA] = EVO_TYPE_2,
//     [SPECIES_FUECOCO] = EVO_TYPE_0,
//     [SPECIES_CROCALOR] = EVO_TYPE_1,
//     [SPECIES_SKELEDIRGE] = EVO_TYPE_2,
//     [SPECIES_QUAXLY] = EVO_TYPE_0,
//     [SPECIES_QUAXWELL] = EVO_TYPE_1,
//     [SPECIES_QUAQUAVAL] = EVO_TYPE_2,
//     [SPECIES_LECHONK] = EVO_TYPE_0,
//     [SPECIES_OINKOLOGNE_M] = EVO_TYPE_1,
//     [SPECIES_OINKOLOGNE_F] = EVO_TYPE_1,
//     [SPECIES_TAROUNTULA] = EVO_TYPE_0,
//     [SPECIES_SPIDOPS] = EVO_TYPE_1,
//     [SPECIES_NYMBLE] = EVO_TYPE_0,
//     [SPECIES_LOKIX] = EVO_TYPE_1,
//     [SPECIES_PAWMI] = EVO_TYPE_0,
//     [SPECIES_PAWMO] = EVO_TYPE_1,
//     [SPECIES_PAWMOT] = EVO_TYPE_2,
//     [SPECIES_TANDEMAUS] = EVO_TYPE_0,
//     [SPECIES_MAUSHOLD_THREE] = EVO_TYPE_1,
//     [SPECIES_MAUSHOLD_FOUR] = EVO_TYPE_1,
//     [SPECIES_FIDOUGH] = EVO_TYPE_0,
//     [SPECIES_DACHSBUN] = EVO_TYPE_1,
//     [SPECIES_SMOLIV] = EVO_TYPE_0,
//     [SPECIES_DOLLIV] = EVO_TYPE_1,
//     [SPECIES_ARBOLIVA] = EVO_TYPE_2,
//     [SPECIES_SQUAWKABILLY_GREEN] = EVO_TYPE_0,
//     [SPECIES_SQUAWKABILLY_BLUE] = EVO_TYPE_0,
//     [SPECIES_SQUAWKABILLY_YELLOW] = EVO_TYPE_0,
//     [SPECIES_SQUAWKABILLY_WHITE] = EVO_TYPE_0,
//     [SPECIES_NACLI] = EVO_TYPE_0,
//     [SPECIES_NACLSTACK] = EVO_TYPE_1,
//     [SPECIES_GARGANACL] = EVO_TYPE_2,
//     [SPECIES_CHARCADET] = EVO_TYPE_0,
//     [SPECIES_ARMAROUGE] = EVO_TYPE_1,
//     [SPECIES_CERULEDGE] = EVO_TYPE_1,
//     [SPECIES_TADBULB] = EVO_TYPE_0,
//     [SPECIES_BELLIBOLT] = EVO_TYPE_1,
//     [SPECIES_WATTREL] = EVO_TYPE_0,
//     [SPECIES_KILOWATTREL] = EVO_TYPE_1,
//     [SPECIES_MASCHIFF] = EVO_TYPE_0,
//     [SPECIES_MABOSSTIFF] = EVO_TYPE_1,
//     [SPECIES_SHROODLE] = EVO_TYPE_0,
//     [SPECIES_GRAFAIAI] = EVO_TYPE_1,
//     [SPECIES_BRAMBLIN] = EVO_TYPE_0,
//     [SPECIES_BRAMBLEGHAST] = EVO_TYPE_1,
//     [SPECIES_TOEDSCOOL] = EVO_TYPE_0,
//     [SPECIES_TOEDSCRUEL] = EVO_TYPE_1,
//     [SPECIES_KLAWF] = EVO_TYPE_0,
//     [SPECIES_CAPSAKID] = EVO_TYPE_0,
//     [SPECIES_SCOVILLAIN] = EVO_TYPE_1,
//     [SPECIES_RELLOR] = EVO_TYPE_0,
//     [SPECIES_RABSCA] = EVO_TYPE_1,
//     [SPECIES_FLITTLE] = EVO_TYPE_0,
//     [SPECIES_ESPATHRA] = EVO_TYPE_1,
//     [SPECIES_TINKATINK] = EVO_TYPE_0,
//     [SPECIES_TINKATUFF] = EVO_TYPE_1,
//     [SPECIES_TINKATON] = EVO_TYPE_2,
//     [SPECIES_WIGLETT] = EVO_TYPE_0,
//     [SPECIES_WUGTRIO] = EVO_TYPE_1,
//     [SPECIES_BOMBIRDIER] = EVO_TYPE_0,
//     [SPECIES_FINIZEN] = EVO_TYPE_0,
//     [SPECIES_PALAFIN_ZERO] = EVO_TYPE_0,
//     [SPECIES_PALAFIN_HERO] = EVO_TYPE_1,
//     [SPECIES_VAROOM] = EVO_TYPE_0,
//     [SPECIES_REVAVROOM] = EVO_TYPE_1,
//     [SPECIES_CYCLIZAR] = EVO_TYPE_0,
//     [SPECIES_ORTHWORM] = EVO_TYPE_0,
//     [SPECIES_GLIMMET] = EVO_TYPE_0,
//     [SPECIES_GLIMMORA] = EVO_TYPE_1,
//     [SPECIES_GREAVARD] = EVO_TYPE_0,
//     [SPECIES_HOUNDSTONE] = EVO_TYPE_1,
//     [SPECIES_FLAMIGO] = EVO_TYPE_0,
//     [SPECIES_CETODDLE] = EVO_TYPE_0,
//     [SPECIES_CETITAN] = EVO_TYPE_1,
//     [SPECIES_VELUZA] = EVO_TYPE_0,
//     [SPECIES_DONDOZO] = EVO_TYPE_0,
//     [SPECIES_TATSUGIRI_CURLY] = EVO_TYPE_0,
//     [SPECIES_TATSUGIRI_DROOPY] = EVO_TYPE_0,
//     [SPECIES_TATSUGIRI_STRETCHY] = EVO_TYPE_0,
//     [SPECIES_ANNIHILAPE] = EVO_TYPE_2,
//     [SPECIES_CLODSIRE] = EVO_TYPE_1,
//     [SPECIES_FARIGIRAF] = EVO_TYPE_1,
//     [SPECIES_DUDUNSPARCE_TWO_SEGMENT] = EVO_TYPE_1,
//     [SPECIES_DUDUNSPARCE_THREE_SEGMENT] = EVO_TYPE_1,
//     [SPECIES_KINGAMBIT] = EVO_TYPE_2,
//     [SPECIES_GREAT_TUSK] = EVO_TYPE_2,
//     [SPECIES_SCREAM_TAIL] = EVO_TYPE_2,
//     [SPECIES_BRUTE_BONNET] = EVO_TYPE_2,
//     [SPECIES_FLUTTER_MANE] = EVO_TYPE_2,
//     [SPECIES_SLITHER_WING] = EVO_TYPE_2,
//     [SPECIES_SANDY_SHOCKS] = EVO_TYPE_2,
//     [SPECIES_IRON_TREADS] = EVO_TYPE_2,
//     [SPECIES_IRON_BUNDLE] = EVO_TYPE_2,
//     [SPECIES_IRON_HANDS] = EVO_TYPE_2,
//     [SPECIES_IRON_JUGULIS] = EVO_TYPE_2,
//     [SPECIES_IRON_MOTH] = EVO_TYPE_2,
//     [SPECIES_IRON_THORNS] = EVO_TYPE_2,
//     [SPECIES_FRIGIBAX] = EVO_TYPE_0,
//     [SPECIES_ARCTIBAX] = EVO_TYPE_1,
//     [SPECIES_BAXCALIBUR] = EVO_TYPE_2,
//     [SPECIES_GIMMIGHOUL_CHEST] = EVO_TYPE_0,
//     [SPECIES_GIMMIGHOUL_ROAMING] = EVO_TYPE_0,
//     [SPECIES_GHOLDENGO] = EVO_TYPE_1,
//     [SPECIES_WO_CHIEN] = EVO_TYPE_LEGENDARY,
//     [SPECIES_CHIEN_PAO] = EVO_TYPE_LEGENDARY,
//     [SPECIES_TING_LU] = EVO_TYPE_LEGENDARY,
//     [SPECIES_CHI_YU] = EVO_TYPE_LEGENDARY,
//     [SPECIES_ROARING_MOON] = EVO_TYPE_2,
//     [SPECIES_IRON_VALIANT] = EVO_TYPE_2,
//     [SPECIES_KORAIDON] = EVO_TYPE_LEGENDARY,
//     [SPECIES_MIRAIDON] = EVO_TYPE_LEGENDARY,
//     // Paldean Forms
//     [SPECIES_TAUROS_PALDEA_COMBAT] = EVO_TYPE_0,
//     [SPECIES_TAUROS_PALDEA_BLAZE] = EVO_TYPE_0,
//     [SPECIES_TAUROS_PALDEA_AQUA] = EVO_TYPE_0,
//     [SPECIES_WOOPER_PALDEA] = EVO_TYPE_0,
//     // Scarlet and Violet 0.2.0
//     [SPECIES_WALKING_WAKE] = EVO_TYPE_2,
//     [SPECIES_IRON_LEAVES] = EVO_TYPE_2,
//     // Teal Mask
//     [SPECIES_DIPPLIN] = EVO_TYPE_1,
//     [SPECIES_POLTCHAGEIST_COUNTERFEIT] = EVO_TYPE_0,
//     [SPECIES_POLTCHAGEIST_ARTISAN] = EVO_TYPE_0,
//     [SPECIES_SINISTCHA_UNREMARKABLE] = EVO_TYPE_1,
//     [SPECIES_SINISTCHA_MASTERPIECE] = EVO_TYPE_1,
//     [SPECIES_OKIDOGI] = EVO_TYPE_LEGENDARY,
//     [SPECIES_MUNKIDORI] = EVO_TYPE_LEGENDARY,
//     [SPECIES_FEZANDIPITI] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_TEAL] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_WELLSPRING] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_HEARTHFLAME] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_CORNERSTONE] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_TEAL_TERA] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_WELLSPRING_TERA] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_HEARTHFLAME_TERA] = EVO_TYPE_LEGENDARY,
//     [SPECIES_OGERPON_CORNERSTONE_TERA] = EVO_TYPE_LEGENDARY,
//     [SPECIES_URSALUNA_BLOODMOON] = EVO_TYPE_2,
//     // Indigo Disk
//     [SPECIES_ARCHALUDON] = EVO_TYPE_1,
//     [SPECIES_HYDRAPPLE] = EVO_TYPE_2,
//     [SPECIES_GOUGING_FIRE] = EVO_TYPE_2,
//     [SPECIES_RAGING_BOLT] = EVO_TYPE_2,
//     [SPECIES_IRON_BOULDER] = EVO_TYPE_2,
//     [SPECIES_IRON_CROWN] = EVO_TYPE_2,
//     [SPECIES_TERAPAGOS_NORMAL] = EVO_TYPE_LEGENDARY,
//     [SPECIES_TERAPAGOS_TERASTAL] = EVO_TYPE_LEGENDARY,
//     [SPECIES_TERAPAGOS_STELLAR] = EVO_TYPE_LEGENDARY,
//     [SPECIES_PECHARUNT] = EVO_TYPE_LEGENDARY,
//     [SPECIES_LUGIA_SHADOW] = EVO_TYPE_LEGENDARY,
//     [SPECIES_MOTHIM_SANDY] = EVO_TYPE_1,
//     [SPECIES_MOTHIM_TRASH] = EVO_TYPE_1,
//     [SPECIES_SCATTERBUG_POLAR] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_TUNDRA] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_CONTINENTAL] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_GARDEN] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_ELEGANT] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_MEADOW] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_MODERN] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_MARINE] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_ARCHIPELAGO] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_HIGH_PLAINS] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_SANDSTORM] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_RIVER] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_MONSOON] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_SAVANNA] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_SUN] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_OCEAN] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_JUNGLE] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_FANCY] = EVO_TYPE_SELF,
//     [SPECIES_SCATTERBUG_POKEBALL] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_POLAR] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_TUNDRA] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_CONTINENTAL] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_GARDEN] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_ELEGANT] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_MEADOW] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_MODERN] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_MARINE] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_ARCHIPELAGO] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_HIGH_PLAINS] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_SANDSTORM] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_RIVER] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_MONSOON] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_SAVANNA] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_SUN] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_OCEAN] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_JUNGLE] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_FANCY] = EVO_TYPE_SELF,
//     [SPECIES_SPEWPA_POKEBALL] = EVO_TYPE_SELF,
//     [SPECIES_RATICATE_ALOLA_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_GUMSHOOS_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_VIKAVOLT_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_LURANTIS_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_SALAZZLE_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_MIMIKYU_TOTEM_DISGUISED] = EVO_TYPE_SELF,
//     [SPECIES_KOMMO_O_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_MAROWAK_ALOLA_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_RIBOMBEE_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_ARAQUANID_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_TOGEDEMARU_TOTEM] = EVO_TYPE_SELF,
//     [SPECIES_PIKACHU_STARTER] = EVO_TYPE_SELF,
//     [SPECIES_EEVEE_STARTER] = EVO_TYPE_SELF,
//     [SPECIES_VENUSAUR_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_BLASTOISE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_CHARIZARD_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_BUTTERFREE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_PIKACHU_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_MEOWTH_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_MACHAMP_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_GENGAR_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_KINGLER_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_LAPRAS_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_EEVEE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_SNORLAX_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_GARBODOR_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_MELMETAL_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_RILLABOOM_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_CINDERACE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_INTELEON_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_CORVIKNIGHT_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_ORBEETLE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_DREDNAW_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_COALOSSAL_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_FLAPPLE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_APPLETUN_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_SANDACONDA_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_TOXTRICITY_AMPED_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_TOXTRICITY_LOW_KEY_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_CENTISKORCH_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_HATTERENE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_GRIMMSNARL_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_ALCREMIE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_COPPERAJAH_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_DURALUDON_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_URSHIFU_SINGLE_STRIKE_GMAX] = EVO_TYPE_SELF,
//     // [SPECIES_URSHIFU_GMAX] = EVO_TYPE_SELF,
//     // [SPECIES_URSHIFU_SINGLE_STRIKE_STYLE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_URSHIFU_RAPID_STRIKE_GMAX] = EVO_TYPE_SELF,
//     // [SPECIES_URSHIFU_RAPID_STRIKE_STYLE_GMAX] = EVO_TYPE_SELF,
//     [SPECIES_MIMIKYU_BUSTED_TOTEM] = EVO_TYPE_SELF,
//     // [SPECIES_MIMIKYU_TOTEM_BUSTED] = EVO_TYPE_SELF,
//     // Legends Z-A
//     [SPECIES_CLEFABLE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_VICTREEBEL_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_STARMIE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_DRAGONITE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_MEGANIUM_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_FERALIGATR_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_SKARMORY_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_FROSLASS_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_EMBOAR_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_EXCADRILL_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_SCOLIPEDE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_SCRAFTY_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_EELEKTROSS_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_CHANDELURE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_CHESNAUGHT_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_DELPHOX_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_GRENINJA_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_PYROAR_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_MALAMAR_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_DRAGALGE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_HAWLUCHA_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_FLOETTE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_BARBARACLE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_ZYGARDE_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_DRAMPA_MEGA] = EVO_TYPE_SELF,
//     [SPECIES_FALINKS_MEGA] = EVO_TYPE_SELF,
//     #endif
//     [SPECIES_EGG]             = EVO_TYPE_SELF,
// };
#define RANDOM_SPECIES_COUNT ARRAY_COUNT(sRandomSpecies)
static const u16 sRandomSpecies[] =
{
    //SPECIES_NONE                    ,
    SPECIES_BULBASAUR               ,
    SPECIES_IVYSAUR                 ,
    SPECIES_VENUSAUR                ,
    SPECIES_CHARMANDER              ,
    SPECIES_CHARMELEON              ,
    SPECIES_CHARIZARD               ,
    SPECIES_SQUIRTLE                ,
    SPECIES_WARTORTLE               ,
    SPECIES_BLASTOISE               ,
    SPECIES_CATERPIE                ,
    SPECIES_METAPOD                 ,
    SPECIES_BUTTERFREE              ,
    SPECIES_WEEDLE                  ,
    SPECIES_KAKUNA                  ,
    SPECIES_BEEDRILL                ,
    SPECIES_PIDGEY                  ,
    SPECIES_PIDGEOTTO               ,
    SPECIES_PIDGEOT                 ,
    SPECIES_RATTATA                 ,
    SPECIES_RATICATE                ,
    SPECIES_SPEAROW                 ,
    SPECIES_FEAROW                  ,
    SPECIES_EKANS                   ,
    SPECIES_ARBOK                   ,
    SPECIES_PIKACHU                 ,
    SPECIES_RAICHU                  ,
    SPECIES_SANDSHREW               ,
    SPECIES_SANDSLASH               ,
    SPECIES_NIDORAN_F               ,
    SPECIES_NIDORINA                ,
    SPECIES_NIDOQUEEN               ,
    SPECIES_NIDORAN_M               ,
    SPECIES_NIDORINO                ,
    SPECIES_NIDOKING                ,
    SPECIES_CLEFAIRY                ,
    SPECIES_CLEFABLE                ,
    SPECIES_VULPIX                  ,
    SPECIES_NINETALES               ,
    SPECIES_JIGGLYPUFF              ,
    SPECIES_WIGGLYTUFF              ,
    SPECIES_ZUBAT                   ,
    SPECIES_GOLBAT                  ,
    SPECIES_ODDISH                  ,
    SPECIES_GLOOM                   ,
    SPECIES_VILEPLUME               ,
    SPECIES_PARAS                   ,
    SPECIES_PARASECT                ,
    SPECIES_VENONAT                 ,
    SPECIES_VENOMOTH                ,
    SPECIES_DIGLETT                 ,
    SPECIES_DUGTRIO                 ,
    SPECIES_MEOWTH                  ,
    SPECIES_PERSIAN                 ,
    SPECIES_PSYDUCK                 ,
    SPECIES_GOLDUCK                 ,
    SPECIES_MANKEY                  ,
    SPECIES_PRIMEAPE                ,
    SPECIES_GROWLITHE               ,
    SPECIES_ARCANINE                ,
    SPECIES_POLIWAG                 ,
    SPECIES_POLIWHIRL               ,
    SPECIES_POLIWRATH               ,
    SPECIES_ABRA                    ,
    SPECIES_KADABRA                 ,
    SPECIES_ALAKAZAM                ,
    SPECIES_MACHOP                  ,
    SPECIES_MACHOKE                 ,
    SPECIES_MACHAMP                 ,
    SPECIES_BELLSPROUT              ,
    SPECIES_WEEPINBELL              ,
    SPECIES_VICTREEBEL              ,
    SPECIES_TENTACOOL               ,
    SPECIES_TENTACRUEL              ,
    SPECIES_GEODUDE                 ,
    SPECIES_GRAVELER                ,
    SPECIES_GOLEM                   ,
    SPECIES_PONYTA                  ,
    SPECIES_RAPIDASH                ,
    SPECIES_SLOWPOKE                ,
    SPECIES_SLOWBRO                 ,
    SPECIES_MAGNEMITE               ,
    SPECIES_MAGNETON                ,
    SPECIES_FARFETCHD               ,
    SPECIES_DODUO                   ,
    SPECIES_DODRIO                  ,
    SPECIES_SEEL                    ,
    SPECIES_DEWGONG                 ,
    SPECIES_GRIMER                  ,
    SPECIES_MUK                     ,
    SPECIES_SHELLDER                ,
    SPECIES_CLOYSTER                ,
    SPECIES_GASTLY                  ,
    SPECIES_HAUNTER                 ,
    SPECIES_GENGAR                  ,
    SPECIES_ONIX                    ,
    SPECIES_DROWZEE                 ,
    SPECIES_HYPNO                   ,
    SPECIES_KRABBY                  ,
    SPECIES_KINGLER                 ,
    SPECIES_VOLTORB                 ,
    SPECIES_ELECTRODE               ,
    SPECIES_EXEGGCUTE               ,
    SPECIES_EXEGGUTOR               ,
    SPECIES_CUBONE                  ,
    SPECIES_MAROWAK                 ,
    SPECIES_HITMONLEE               ,
    SPECIES_HITMONCHAN              ,
    SPECIES_LICKITUNG               ,
    SPECIES_KOFFING                 ,
    SPECIES_WEEZING                 ,
    SPECIES_RHYHORN                 ,
    SPECIES_RHYDON                  ,
    SPECIES_CHANSEY                 ,
    SPECIES_TANGELA                 ,
    SPECIES_KANGASKHAN              ,
    SPECIES_HORSEA                  ,
    SPECIES_SEADRA                  ,
    SPECIES_GOLDEEN                 ,
    SPECIES_SEAKING                 ,
    SPECIES_STARYU                  ,
    SPECIES_STARMIE                 ,
    SPECIES_MR_MIME                 ,
    SPECIES_SCYTHER                 ,
    SPECIES_JYNX                    ,
    SPECIES_ELECTABUZZ              ,
    SPECIES_MAGMAR                  ,
    SPECIES_PINSIR                  ,
    SPECIES_TAUROS                  ,
    SPECIES_MAGIKARP                ,
    SPECIES_GYARADOS                ,
    SPECIES_LAPRAS                  ,
    SPECIES_DITTO                   ,
    SPECIES_EEVEE                   ,
    SPECIES_VAPOREON                ,
    SPECIES_JOLTEON                 ,
    SPECIES_FLAREON                 ,
    SPECIES_PORYGON                 ,
    SPECIES_OMANYTE                 ,
    SPECIES_OMASTAR                 ,
    SPECIES_KABUTO                  ,
    SPECIES_KABUTOPS                ,
    SPECIES_AERODACTYL              ,
    SPECIES_SNORLAX                 ,
    // SPECIES_ARTICUNO  ,
    // SPECIES_ZAPDOS    ,
    // SPECIES_MOLTRES   ,
    SPECIES_DRATINI                 ,
    SPECIES_DRAGONAIR               ,
    SPECIES_DRAGONITE               ,
    // SPECIES_MEWTWO    ,
    // SPECIES_MEW       ,
    SPECIES_CHIKORITA                  ,
    SPECIES_BAYLEEF                    ,
    SPECIES_MEGANIUM                   ,
    SPECIES_CYNDAQUIL                  ,
    SPECIES_QUILAVA                    ,
    SPECIES_TYPHLOSION                 ,
    SPECIES_TOTODILE                   ,
    SPECIES_CROCONAW                   ,
    SPECIES_FERALIGATR                 ,
    SPECIES_SENTRET                    ,
    SPECIES_FURRET                     ,
    SPECIES_HOOTHOOT                   ,
    SPECIES_NOCTOWL                    ,
    SPECIES_LEDYBA                     ,
    SPECIES_LEDIAN                     ,
    SPECIES_SPINARAK                   ,
    SPECIES_ARIADOS                    ,
    SPECIES_CROBAT                     ,
    SPECIES_CHINCHOU                   ,
    SPECIES_LANTURN                    ,
    SPECIES_PICHU                      ,
    SPECIES_CLEFFA                     ,
    SPECIES_IGGLYBUFF                  ,
    SPECIES_TOGEPI                     ,
    SPECIES_TOGETIC                    ,
    SPECIES_NATU                       ,
    SPECIES_XATU                       ,
    SPECIES_MAREEP                     ,
    SPECIES_FLAAFFY                    ,
    SPECIES_AMPHAROS                   ,
    SPECIES_BELLOSSOM                  ,
    SPECIES_MARILL                     ,
    SPECIES_AZUMARILL                  ,
    SPECIES_SUDOWOODO                  ,
    SPECIES_POLITOED                   ,
    SPECIES_HOPPIP                     ,
    SPECIES_SKIPLOOM                   ,
    SPECIES_JUMPLUFF                   ,
    SPECIES_AIPOM                      ,
    SPECIES_SUNKERN                    ,
    SPECIES_SUNFLORA                   ,
    SPECIES_YANMA                      ,
    SPECIES_WOOPER                     ,
    SPECIES_QUAGSIRE                   ,
    SPECIES_ESPEON                     ,
    SPECIES_UMBREON                    ,
    SPECIES_MURKROW                    ,
    SPECIES_SLOWKING                   ,
    SPECIES_MISDREAVUS                 ,
    SPECIES_UNOWN                      ,
    SPECIES_WOBBUFFET                  ,
    SPECIES_GIRAFARIG                  ,
    SPECIES_PINECO                     ,
    SPECIES_FORRETRESS                 ,
    SPECIES_DUNSPARCE                  ,
    SPECIES_GLIGAR                     ,
    SPECIES_STEELIX                    ,
    SPECIES_SNUBBULL                   ,
    SPECIES_GRANBULL                   ,
    SPECIES_QWILFISH                   ,
    SPECIES_SCIZOR                     ,
    SPECIES_SHUCKLE                    ,
    SPECIES_HERACROSS                  ,
    SPECIES_SNEASEL                    ,
    SPECIES_TEDDIURSA                  ,
    SPECIES_URSARING                   ,
    SPECIES_SLUGMA                     ,
    SPECIES_MAGCARGO                   ,
    SPECIES_SWINUB                     ,
    SPECIES_PILOSWINE                  ,
    SPECIES_CORSOLA                    ,
    SPECIES_REMORAID                   ,
    SPECIES_OCTILLERY                  ,
    SPECIES_DELIBIRD                   ,
    SPECIES_MANTINE                    ,
    SPECIES_SKARMORY                   ,
    SPECIES_HOUNDOUR                   ,
    SPECIES_HOUNDOOM                   ,
    SPECIES_KINGDRA                    ,
    SPECIES_PHANPY                     ,
    SPECIES_DONPHAN                    ,
    SPECIES_PORYGON2                   ,
    SPECIES_STANTLER                   ,
    SPECIES_SMEARGLE                   ,
    SPECIES_TYROGUE                    ,
    SPECIES_HITMONTOP                  ,
    SPECIES_SMOOCHUM                   ,
    SPECIES_ELEKID                     ,
    SPECIES_MAGBY                      ,
    SPECIES_MILTANK                    ,
    SPECIES_BLISSEY                    ,
    //SPECIES_RAIKOU                     ,
    //SPECIES_ENTEI                      ,
    //SPECIES_SUICUNE                    ,
    SPECIES_LARVITAR                   ,
    SPECIES_PUPITAR                    ,
    SPECIES_TYRANITAR                  ,
    // SPECIES_LUGIA     ,
    // SPECIES_HO_OH     ,
    // SPECIES_CELEBI    ,
    // SPECIES_OLD_UNOWN_B,
    // SPECIES_OLD_UNOWN_C,
    // SPECIES_OLD_UNOWN_D,
    // SPECIES_OLD_UNOWN_E,
    // SPECIES_OLD_UNOWN_F,
    // SPECIES_OLD_UNOWN_G,
    // SPECIES_OLD_UNOWN_H,
    // SPECIES_OLD_UNOWN_I,
    // SPECIES_OLD_UNOWN_J,
    // SPECIES_OLD_UNOWN_K,
    // SPECIES_OLD_UNOWN_L,
    // SPECIES_OLD_UNOWN_M,
    // SPECIES_OLD_UNOWN_N,
    // SPECIES_OLD_UNOWN_O,
    // SPECIES_OLD_UNOWN_P,
    // SPECIES_OLD_UNOWN_Q,
    // SPECIES_OLD_UNOWN_R,
    // SPECIES_OLD_UNOWN_S,
    // SPECIES_OLD_UNOWN_T,
    // SPECIES_OLD_UNOWN_U,
    // SPECIES_OLD_UNOWN_V,
    // SPECIES_OLD_UNOWN_W,
    // SPECIES_OLD_UNOWN_X,
    // SPECIES_OLD_UNOWN_Y,
    // SPECIES_OLD_UNOWN_Z,
    SPECIES_TREECKO           ,
    SPECIES_GROVYLE           ,
    SPECIES_SCEPTILE          ,
    SPECIES_TORCHIC           ,
    SPECIES_COMBUSKEN         ,
    SPECIES_BLAZIKEN          ,
    SPECIES_MUDKIP            ,
    SPECIES_MARSHTOMP         ,
    SPECIES_SWAMPERT          ,
    SPECIES_POOCHYENA         ,
    SPECIES_MIGHTYENA         ,
    SPECIES_ZIGZAGOON         ,
    SPECIES_LINOONE           ,
    SPECIES_WURMPLE           ,
    SPECIES_SILCOON           ,
    SPECIES_BEAUTIFLY         ,
    SPECIES_CASCOON           ,
    SPECIES_DUSTOX            ,
    SPECIES_LOTAD             ,
    SPECIES_LOMBRE            ,
    SPECIES_LUDICOLO          ,
    SPECIES_SEEDOT            ,
    SPECIES_NUZLEAF           ,
    SPECIES_SHIFTRY           ,
    SPECIES_NINCADA           ,
    SPECIES_NINJASK           ,
    // SPECIES_SHEDINJA          ,
    SPECIES_TAILLOW           ,
    SPECIES_SWELLOW           ,
    SPECIES_SHROOMISH         ,
    SPECIES_BRELOOM           ,
    SPECIES_SPINDA            ,
    SPECIES_WINGULL           ,
    SPECIES_PELIPPER          ,
    SPECIES_SURSKIT           ,
    SPECIES_MASQUERAIN        ,
    SPECIES_WAILMER           ,
    SPECIES_WAILORD           ,
    SPECIES_SKITTY            ,
    SPECIES_DELCATTY          ,
    SPECIES_KECLEON           ,
    SPECIES_BALTOY            ,
    SPECIES_CLAYDOL           ,
    SPECIES_NOSEPASS          ,
    SPECIES_TORKOAL           ,
    SPECIES_SABLEYE           ,
    SPECIES_BARBOACH          ,
    SPECIES_WHISCASH          ,
    SPECIES_LUVDISC           ,
    SPECIES_CORPHISH          ,
    SPECIES_CRAWDAUNT         ,
    SPECIES_FEEBAS            ,
    SPECIES_MILOTIC           ,
    SPECIES_CARVANHA          ,
    SPECIES_SHARPEDO          ,
    SPECIES_TRAPINCH          ,
    SPECIES_VIBRAVA           ,
    SPECIES_FLYGON            ,
    SPECIES_MAKUHITA          ,
    SPECIES_HARIYAMA          ,
    SPECIES_ELECTRIKE         ,
    SPECIES_MANECTRIC         ,
    SPECIES_NUMEL             ,
    SPECIES_CAMERUPT          ,
    SPECIES_SPHEAL            ,
    SPECIES_SEALEO            ,
    SPECIES_WALREIN           ,
    SPECIES_CACNEA            ,
    SPECIES_CACTURNE          ,
    SPECIES_SNORUNT           ,
    SPECIES_GLALIE            ,
    SPECIES_LUNATONE          ,
    SPECIES_SOLROCK           ,
    SPECIES_AZURILL           ,
    SPECIES_SPOINK            ,
    SPECIES_GRUMPIG           ,
    SPECIES_PLUSLE            ,
    SPECIES_MINUN             ,
    SPECIES_MAWILE            ,
    SPECIES_MEDITITE          ,
    SPECIES_MEDICHAM          ,
    SPECIES_SWABLU            ,
    SPECIES_ALTARIA           ,
    SPECIES_WYNAUT            ,
    SPECIES_DUSKULL           ,
    SPECIES_DUSCLOPS          ,
    SPECIES_ROSELIA           ,
    SPECIES_SLAKOTH           ,
    SPECIES_VIGOROTH          ,
    SPECIES_SLAKING           ,
    SPECIES_GULPIN            ,
    SPECIES_SWALOT            ,
    SPECIES_TROPIUS           ,
    SPECIES_WHISMUR           ,
    SPECIES_LOUDRED           ,
    SPECIES_EXPLOUD           ,
    SPECIES_CLAMPERL          ,
    SPECIES_HUNTAIL           ,
    SPECIES_GOREBYSS          ,
    SPECIES_ABSOL             ,
    SPECIES_SHUPPET           ,
    SPECIES_BANETTE           ,
    SPECIES_SEVIPER           ,
    SPECIES_ZANGOOSE          ,
    SPECIES_RELICANTH         ,
    SPECIES_ARON              ,
    SPECIES_LAIRON            ,
    SPECIES_AGGRON            ,
    // SPECIES_CASTFORM          ,
    SPECIES_VOLBEAT           ,
    SPECIES_ILLUMISE          ,
    SPECIES_LILEEP            ,
    SPECIES_CRADILY           ,
    SPECIES_ANORITH           ,
    SPECIES_ARMALDO           ,
    SPECIES_RALTS             ,
    SPECIES_KIRLIA            ,
    SPECIES_GARDEVOIR         ,
    SPECIES_BAGON             ,
    SPECIES_SHELGON           ,
    SPECIES_SALAMENCE         ,
    SPECIES_BELDUM            ,
    SPECIES_METANG            ,
    SPECIES_METAGROSS         ,
    // SPECIES_REGIROCK  ,
    // SPECIES_REGICE    ,
    // SPECIES_REGISTEEL ,
    // SPECIES_KYOGRE    ,
    // SPECIES_GROUDON   ,
    // SPECIES_RAYQUAZA  ,
    // SPECIES_LATIAS    ,
    // SPECIES_LATIOS    ,
    // SPECIES_JIRACHI   ,
    // SPECIES_DEOXYS    ,
    SPECIES_CHIMECHO          ,
    #ifdef POKEMON_EXPANSION
    SPECIES_TURTWIG           ,
    SPECIES_GROTLE            ,
    SPECIES_TORTERRA          ,
    SPECIES_CHIMCHAR          ,
    SPECIES_MONFERNO          ,
    SPECIES_INFERNAPE         ,
    SPECIES_PIPLUP            ,
    SPECIES_PRINPLUP          ,
    SPECIES_EMPOLEON          ,
    SPECIES_STARLY            ,
    SPECIES_STARAVIA          ,
    SPECIES_STARAPTOR         ,
    SPECIES_BIDOOF            ,
    SPECIES_BIBAREL           ,
    SPECIES_KRICKETOT         ,
    SPECIES_KRICKETUNE        ,
    SPECIES_SHINX             ,
    SPECIES_LUXIO             ,
    SPECIES_LUXRAY            ,
    SPECIES_BUDEW             ,
    SPECIES_ROSERADE          ,
    SPECIES_CRANIDOS          ,
    SPECIES_RAMPARDOS         ,
    SPECIES_SHIELDON          ,
    SPECIES_BASTIODON         ,
    SPECIES_BURMY             ,
    SPECIES_WORMADAM          ,
    SPECIES_MOTHIM            ,
    SPECIES_COMBEE            ,
    SPECIES_VESPIQUEN         ,
    SPECIES_PACHIRISU         ,
    SPECIES_BUIZEL            ,
    SPECIES_FLOATZEL          ,
    SPECIES_CHERUBI           ,
    SPECIES_CHERRIM           ,
    SPECIES_SHELLOS           ,
    SPECIES_GASTRODON         ,
    SPECIES_AMBIPOM           ,
    SPECIES_DRIFLOON          ,
    SPECIES_DRIFBLIM          ,
    SPECIES_BUNEARY           ,
    SPECIES_LOPUNNY           ,
    SPECIES_MISMAGIUS         ,
    SPECIES_HONCHKROW         ,
    SPECIES_GLAMEOW           ,
    SPECIES_PURUGLY           ,
    SPECIES_CHINGLING         ,
    SPECIES_STUNKY            ,
    SPECIES_SKUNTANK          ,
    SPECIES_BRONZOR           ,
    SPECIES_BRONZONG          ,
    SPECIES_BONSLY            ,
    SPECIES_MIME_JR           ,
    SPECIES_HAPPINY           ,
    SPECIES_CHATOT            ,
    SPECIES_SPIRITOMB         ,
    SPECIES_GIBLE             ,
    SPECIES_GABITE            ,
    SPECIES_GARCHOMP          ,
    SPECIES_MUNCHLAX          ,
    SPECIES_RIOLU             ,
    SPECIES_LUCARIO           ,
    SPECIES_HIPPOPOTAS        ,
    SPECIES_HIPPOWDON         ,
    SPECIES_SKORUPI           ,
    SPECIES_DRAPION           ,
    SPECIES_CROAGUNK          ,
    SPECIES_TOXICROAK         ,
    SPECIES_CARNIVINE         ,
    SPECIES_FINNEON           ,
    SPECIES_LUMINEON          ,
    SPECIES_MANTYKE           ,
    SPECIES_SNOVER            ,
    SPECIES_ABOMASNOW         ,
    SPECIES_WEAVILE           ,
    SPECIES_MAGNEZONE         ,
    SPECIES_LICKILICKY        ,
    SPECIES_RHYPERIOR         ,
    SPECIES_TANGROWTH         ,
    SPECIES_ELECTIVIRE        ,
    SPECIES_MAGMORTAR         ,
    SPECIES_TOGEKISS          ,
    SPECIES_YANMEGA           ,
    SPECIES_LEAFEON           ,
    SPECIES_GLACEON           ,
    SPECIES_GLISCOR           ,
    SPECIES_MAMOSWINE         ,
    SPECIES_PORYGON_Z         ,
    SPECIES_GALLADE           ,
    SPECIES_PROBOPASS         ,
    SPECIES_DUSKNOIR          ,
    SPECIES_FROSLASS          ,
    SPECIES_ROTOM             ,
    //SPECIES_UXIE              ,
    //SPECIES_MESPRIT           ,
    //SPECIES_AZELF             ,
    //SPECIES_DIALGA            ,
    //SPECIES_PALKIA            ,
    //SPECIES_HEATRAN           ,
    //SPECIES_REGIGIGAS         ,
    //SPECIES_GIRATINA          ,
    //SPECIES_CRESSELIA         ,
    //SPECIES_PHIONE            ,
    //SPECIES_MANAPHY           ,
    //SPECIES_DARKRAI           ,
    //SPECIES_SHAYMIN           ,
    //SPECIES_ARCEUS            ,
    //SPECIES_VICTINI           ,
    SPECIES_SNIVY             ,
    SPECIES_SERVINE           ,
    SPECIES_SERPERIOR         ,
    SPECIES_TEPIG             ,
    SPECIES_PIGNITE           ,
    SPECIES_EMBOAR            ,
    SPECIES_OSHAWOTT          ,
    SPECIES_DEWOTT            ,
    SPECIES_SAMUROTT          ,
    SPECIES_PATRAT            ,
    SPECIES_WATCHOG           ,
    SPECIES_LILLIPUP          ,
    SPECIES_HERDIER           ,
    SPECIES_STOUTLAND         ,
    SPECIES_PURRLOIN          ,
    SPECIES_LIEPARD           ,
    SPECIES_PANSAGE           ,
    SPECIES_SIMISAGE          ,
    SPECIES_PANSEAR           ,
    SPECIES_SIMISEAR          ,
    SPECIES_PANPOUR           ,
    SPECIES_SIMIPOUR          ,
    SPECIES_MUNNA             ,
    SPECIES_MUSHARNA          ,
    SPECIES_PIDOVE            ,
    SPECIES_TRANQUILL         ,
    SPECIES_UNFEZANT          ,
    SPECIES_BLITZLE           ,
    SPECIES_ZEBSTRIKA         ,
    SPECIES_ROGGENROLA        ,
    SPECIES_BOLDORE           ,
    SPECIES_GIGALITH          ,
    SPECIES_WOOBAT            ,
    SPECIES_SWOOBAT           ,
    SPECIES_DRILBUR           ,
    SPECIES_EXCADRILL         ,
    SPECIES_AUDINO            ,
    SPECIES_TIMBURR           ,
    SPECIES_GURDURR           ,
    SPECIES_CONKELDURR        ,
    SPECIES_TYMPOLE           ,
    SPECIES_PALPITOAD         ,
    SPECIES_SEISMITOAD        ,
    SPECIES_THROH             ,
    SPECIES_SAWK              ,
    SPECIES_SEWADDLE          ,
    SPECIES_SWADLOON          ,
    SPECIES_LEAVANNY          ,
    SPECIES_VENIPEDE          ,
    SPECIES_WHIRLIPEDE        ,
    SPECIES_SCOLIPEDE         ,
    SPECIES_COTTONEE          ,
    SPECIES_WHIMSICOTT        ,
    SPECIES_PETILIL           ,
    SPECIES_LILLIGANT         ,
    SPECIES_BASCULIN          ,
    SPECIES_SANDILE           ,
    SPECIES_KROKOROK          ,
    SPECIES_KROOKODILE        ,
    SPECIES_DARUMAKA          ,
    SPECIES_DARMANITAN        ,
    SPECIES_MARACTUS          ,
    SPECIES_DWEBBLE           ,
    SPECIES_CRUSTLE           ,
    SPECIES_SCRAGGY           ,
    SPECIES_SCRAFTY           ,
    SPECIES_SIGILYPH          ,
    SPECIES_YAMASK            ,
    SPECIES_COFAGRIGUS        ,
    SPECIES_TIRTOUGA          ,
    SPECIES_CARRACOSTA        ,
    SPECIES_ARCHEN            ,
    SPECIES_ARCHEOPS          ,
    SPECIES_TRUBBISH          ,
    SPECIES_GARBODOR          ,
    SPECIES_ZORUA             ,
    SPECIES_ZOROARK           ,
    SPECIES_MINCCINO          ,
    SPECIES_CINCCINO          ,
    SPECIES_GOTHITA           ,
    SPECIES_GOTHORITA         ,
    SPECIES_GOTHITELLE        ,
    SPECIES_SOLOSIS           ,
    SPECIES_DUOSION           ,
    SPECIES_REUNICLUS         ,
    SPECIES_DUCKLETT          ,
    SPECIES_SWANNA            ,
    SPECIES_VANILLITE         ,
    SPECIES_VANILLISH         ,
    SPECIES_VANILLUXE         ,
    SPECIES_DEERLING          ,
    SPECIES_SAWSBUCK          ,
    SPECIES_EMOLGA            ,
    SPECIES_KARRABLAST        ,
    SPECIES_ESCAVALIER        ,
    SPECIES_FOONGUS           ,
    SPECIES_AMOONGUSS         ,
    SPECIES_FRILLISH          ,
    SPECIES_JELLICENT         ,
    SPECIES_ALOMOMOLA         ,
    SPECIES_JOLTIK            ,
    SPECIES_GALVANTULA        ,
    SPECIES_FERROSEED         ,
    SPECIES_FERROTHORN        ,
    SPECIES_KLINK             ,
    SPECIES_KLANG             ,
    SPECIES_KLINKLANG         ,
    SPECIES_TYNAMO            ,
    SPECIES_EELEKTRIK         ,
    SPECIES_EELEKTROSS        ,
    SPECIES_ELGYEM            ,
    SPECIES_BEHEEYEM          ,
    SPECIES_LITWICK           ,
    SPECIES_LAMPENT           ,
    SPECIES_CHANDELURE        ,
    SPECIES_AXEW              ,
    SPECIES_FRAXURE           ,
    SPECIES_HAXORUS           ,
    SPECIES_CUBCHOO           ,
    SPECIES_BEARTIC           ,
    SPECIES_CRYOGONAL         ,
    SPECIES_SHELMET           ,
    SPECIES_ACCELGOR          ,
    SPECIES_STUNFISK          ,
    SPECIES_MIENFOO           ,
    SPECIES_MIENSHAO          ,
    SPECIES_DRUDDIGON         ,
    SPECIES_GOLETT            ,
    SPECIES_GOLURK            ,
    SPECIES_PAWNIARD          ,
    SPECIES_BISHARP           ,
    SPECIES_BOUFFALANT        ,
    SPECIES_RUFFLET           ,
    SPECIES_BRAVIARY          ,
    SPECIES_VULLABY           ,
    SPECIES_MANDIBUZZ         ,
    SPECIES_HEATMOR           ,
    SPECIES_DURANT            ,
    SPECIES_DEINO             ,
    SPECIES_ZWEILOUS          ,
    SPECIES_HYDREIGON         ,
    SPECIES_LARVESTA          ,
    SPECIES_VOLCARONA         ,
    //SPECIES_COBALION          ,
    //SPECIES_TERRAKION         ,
    //SPECIES_VIRIZION          ,
    //SPECIES_TORNADUS          ,
    //SPECIES_THUNDURUS         ,
    //SPECIES_RESHIRAM          ,
    //SPECIES_ZEKROM            ,
    //SPECIES_LANDORUS          ,
    //SPECIES_KYUREM            ,
    //SPECIES_KELDEO            ,
    //SPECIES_MELOETTA          ,
    //SPECIES_GENESECT          ,
    SPECIES_CHESPIN           ,
    SPECIES_QUILLADIN         ,
    SPECIES_CHESNAUGHT        ,
    SPECIES_FENNEKIN          ,
    SPECIES_BRAIXEN           ,
    SPECIES_DELPHOX           ,
    SPECIES_FROAKIE           ,
    SPECIES_FROGADIER         ,
    SPECIES_GRENINJA          ,
    SPECIES_BUNNELBY          ,
    SPECIES_DIGGERSBY         ,
    SPECIES_FLETCHLING        ,
    SPECIES_FLETCHINDER       ,
    SPECIES_TALONFLAME        ,
    SPECIES_SCATTERBUG        ,
    SPECIES_SPEWPA            ,
    SPECIES_VIVILLON          ,
    SPECIES_LITLEO            ,
    SPECIES_PYROAR            ,
    SPECIES_FLABEBE           ,
    SPECIES_FLOETTE           ,
    SPECIES_FLORGES           ,
    SPECIES_SKIDDO            ,
    SPECIES_GOGOAT            ,
    SPECIES_PANCHAM           ,
    SPECIES_PANGORO           ,
    SPECIES_FURFROU           ,
    SPECIES_ESPURR            ,
    SPECIES_MEOWSTIC          ,
    SPECIES_HONEDGE           ,
    SPECIES_DOUBLADE          ,
    SPECIES_AEGISLASH         ,
    SPECIES_SPRITZEE          ,
    SPECIES_AROMATISSE        ,
    SPECIES_SWIRLIX           ,
    SPECIES_SLURPUFF          ,
    SPECIES_INKAY             ,
    SPECIES_MALAMAR           ,
    SPECIES_BINACLE           ,
    SPECIES_BARBARACLE        ,
    SPECIES_SKRELP            ,
    SPECIES_DRAGALGE          ,
    SPECIES_CLAUNCHER         ,
    SPECIES_CLAWITZER         ,
    SPECIES_HELIOPTILE        ,
    SPECIES_HELIOLISK         ,
    SPECIES_TYRUNT            ,
    SPECIES_TYRANTRUM         ,
    SPECIES_AMAURA            ,
    SPECIES_AURORUS           ,
    SPECIES_SYLVEON           ,
    SPECIES_HAWLUCHA          ,
    SPECIES_DEDENNE           ,
    SPECIES_CARBINK           ,
    SPECIES_GOOMY             ,
    SPECIES_SLIGGOO           ,
    SPECIES_GOODRA            ,
    SPECIES_KLEFKI            ,
    SPECIES_PHANTUMP          ,
    SPECIES_TREVENANT         ,
    SPECIES_PUMPKABOO         ,
    SPECIES_GOURGEIST         ,
    SPECIES_BERGMITE          ,
    SPECIES_AVALUGG           ,
    SPECIES_NOIBAT            ,
    SPECIES_NOIVERN           ,
    //SPECIES_XERNEAS           ,
    //SPECIES_YVELTAL           ,
    //SPECIES_ZYGARDE           ,
    //SPECIES_DIANCIE           ,
    //SPECIES_HOOPA             ,
    //SPECIES_VOLCANION         ,
    SPECIES_ROWLET            ,
    SPECIES_DARTRIX           ,
    SPECIES_DECIDUEYE         ,
    SPECIES_LITTEN            ,
    SPECIES_TORRACAT          ,
    SPECIES_INCINEROAR        ,
    SPECIES_POPPLIO           ,
    SPECIES_BRIONNE           ,
    SPECIES_PRIMARINA         ,
    SPECIES_PIKIPEK           ,
    SPECIES_TRUMBEAK          ,
    SPECIES_TOUCANNON         ,
    SPECIES_YUNGOOS           ,
    SPECIES_GUMSHOOS          ,
    SPECIES_GRUBBIN           ,
    SPECIES_CHARJABUG         ,
    SPECIES_VIKAVOLT          ,
    SPECIES_CRABRAWLER        ,
    SPECIES_CRABOMINABLE      ,
    SPECIES_ORICORIO          ,
    SPECIES_CUTIEFLY          ,
    SPECIES_RIBOMBEE          ,
    SPECIES_ROCKRUFF          ,
    SPECIES_LYCANROC          ,
    SPECIES_WISHIWASHI        ,
    SPECIES_MAREANIE          ,
    SPECIES_TOXAPEX           ,
    SPECIES_MUDBRAY           ,
    SPECIES_MUDSDALE          ,
    SPECIES_DEWPIDER          ,
    SPECIES_ARAQUANID         ,
    SPECIES_FOMANTIS          ,
    SPECIES_LURANTIS          ,
    SPECIES_MORELULL          ,
    SPECIES_SHIINOTIC         ,
    SPECIES_SALANDIT          ,
    SPECIES_SALAZZLE          ,
    SPECIES_STUFFUL           ,
    SPECIES_BEWEAR            ,
    SPECIES_BOUNSWEET         ,
    SPECIES_STEENEE           ,
    SPECIES_TSAREENA          ,
    SPECIES_COMFEY            ,
    SPECIES_ORANGURU          ,
    SPECIES_PASSIMIAN         ,
    SPECIES_WIMPOD            ,
    SPECIES_GOLISOPOD         ,
    SPECIES_SANDYGAST         ,
    SPECIES_PALOSSAND         ,
    SPECIES_PYUKUMUKU         ,
    //SPECIES_TYPE_NULL         ,
    //SPECIES_SILVALLY          ,
    SPECIES_MINIOR            ,
    SPECIES_KOMALA            ,
    SPECIES_TURTONATOR        ,
    SPECIES_TOGEDEMARU        ,
    SPECIES_MIMIKYU           ,
    SPECIES_BRUXISH           ,
    SPECIES_DRAMPA            ,
    SPECIES_DHELMISE          ,
    SPECIES_JANGMO_O          ,
    SPECIES_HAKAMO_O          ,
    SPECIES_KOMMO_O           ,
    //SPECIES_TAPU_KOKO         ,
    //SPECIES_TAPU_LELE         ,
    //SPECIES_TAPU_BULU         ,
    //SPECIES_TAPU_FINI         ,
    //SPECIES_COSMOG            ,
    //SPECIES_COSMOEM           ,
    //SPECIES_SOLGALEO          ,
    //SPECIES_LUNALA            ,
    //SPECIES_NIHILEGO          ,
    //SPECIES_BUZZWOLE          ,
    //SPECIES_PHEROMOSA         ,
    //SPECIES_XURKITREE         ,
    //SPECIES_CELESTEELA        ,
    //SPECIES_KARTANA           ,
    //SPECIES_GUZZLORD          ,
    //SPECIES_NECROZMA          ,
    //SPECIES_MAGEARNA          ,
    //SPECIES_MARSHADOW         ,
    //SPECIES_POIPOLE           ,
    //SPECIES_NAGANADEL         ,
    //SPECIES_STAKATAKA         ,
    //SPECIES_BLACEPHALON       ,
    //SPECIES_ZERAORA           ,
    //SPECIES_MELTAN            ,
    //SPECIES_MELMETAL          ,
    SPECIES_GROOKEY           ,
    SPECIES_THWACKEY          ,
    SPECIES_RILLABOOM         ,
    SPECIES_SCORBUNNY         ,
    SPECIES_RABOOT            ,
    SPECIES_CINDERACE         ,
    SPECIES_SOBBLE            ,
    SPECIES_DRIZZILE          ,
    SPECIES_INTELEON          ,
    SPECIES_SKWOVET           ,
    SPECIES_GREEDENT          ,
    SPECIES_ROOKIDEE          ,
    SPECIES_CORVISQUIRE       ,
    SPECIES_CORVIKNIGHT       ,
    SPECIES_BLIPBUG           ,
    SPECIES_DOTTLER           ,
    SPECIES_ORBEETLE          ,
    SPECIES_NICKIT            ,
    SPECIES_THIEVUL           ,
    SPECIES_GOSSIFLEUR        ,
    SPECIES_ELDEGOSS          ,
    SPECIES_WOOLOO            ,
    SPECIES_DUBWOOL           ,
    SPECIES_CHEWTLE           ,
    SPECIES_DREDNAW           ,
    SPECIES_YAMPER            ,
    SPECIES_BOLTUND           ,
    SPECIES_ROLYCOLY          ,
    SPECIES_CARKOL            ,
    SPECIES_COALOSSAL         ,
    SPECIES_APPLIN            ,
    SPECIES_FLAPPLE           ,
    SPECIES_APPLETUN          ,
    SPECIES_SILICOBRA         ,
    SPECIES_SANDACONDA        ,
    SPECIES_CRAMORANT         ,
    SPECIES_ARROKUDA          ,
    SPECIES_BARRASKEWDA       ,
    SPECIES_TOXEL             ,
    SPECIES_TOXTRICITY        ,
    SPECIES_SIZZLIPEDE        ,
    SPECIES_CENTISKORCH       ,
    SPECIES_CLOBBOPUS         ,
    SPECIES_GRAPPLOCT         ,
    SPECIES_SINISTEA          ,
    SPECIES_POLTEAGEIST       ,
    SPECIES_HATENNA           ,
    SPECIES_HATTREM           ,
    SPECIES_HATTERENE         ,
    SPECIES_IMPIDIMP          ,
    SPECIES_MORGREM           ,
    SPECIES_GRIMMSNARL        ,
    SPECIES_OBSTAGOON         ,
    SPECIES_PERRSERKER        ,
    SPECIES_CURSOLA           ,
    SPECIES_SIRFETCHD         ,
    SPECIES_MR_RIME           ,
    SPECIES_RUNERIGUS         ,
    SPECIES_MILCERY           ,
    SPECIES_ALCREMIE          ,
    SPECIES_FALINKS           ,
    SPECIES_PINCURCHIN        ,
    SPECIES_SNOM              ,
    SPECIES_FROSMOTH          ,
    SPECIES_STONJOURNER       ,
    SPECIES_EISCUE            ,
    SPECIES_INDEEDEE          ,
    SPECIES_MORPEKO           ,
    SPECIES_CUFANT            ,
    SPECIES_COPPERAJAH        ,
    SPECIES_DRACOZOLT         ,
    SPECIES_ARCTOZOLT         ,
    SPECIES_DRACOVISH         ,
    SPECIES_ARCTOVISH         ,
    SPECIES_DURALUDON         ,
    SPECIES_DREEPY            ,
    SPECIES_DRAKLOAK          ,
    SPECIES_DRAGAPULT         ,
    //SPECIES_ZACIAN            ,
    //SPECIES_ZAMAZENTA         ,
    //SPECIES_ETERNATUS         ,
    //SPECIES_KUBFU             ,
    //SPECIES_URSHIFU           ,
    //SPECIES_ZARUDE            ,
    //SPECIES_REGIELEKI         ,
    //SPECIES_REGIDRAGO         ,
    //SPECIES_GLASTRIER         ,
    //SPECIES_SPECTRIER         ,
    //SPECIES_CALYREX           ,
    //SPECIES_VENUSAUR_MEGA     ,
    //SPECIES_CHARIZARD_MEGA_X  ,
    //SPECIES_CHARIZARD_MEGA_Y  ,
    //SPECIES_BLASTOISE_MEGA    ,
    //SPECIES_BEEDRILL_MEGA     ,
    //SPECIES_PIDGEOT_MEGA      ,
    //SPECIES_ALAKAZAM_MEGA     ,
    //SPECIES_SLOWBRO_MEGA      ,
    //SPECIES_GENGAR_MEGA       ,
    //SPECIES_KANGASKHAN_MEGA   ,
    //SPECIES_PINSIR_MEGA       ,
    //SPECIES_GYARADOS_MEGA     ,
    //SPECIES_AERODACTYL_MEGA   ,
    //SPECIES_MEWTWO_MEGA_X     ,
    //SPECIES_MEWTWO_MEGA_Y     ,
    //SPECIES_AMPHAROS_MEGA     ,
    //SPECIES_STEELIX_MEGA      ,
    //SPECIES_SCIZOR_MEGA       ,
    //SPECIES_HERACROSS_MEGA    ,
    //SPECIES_HOUNDOOM_MEGA     ,
    //SPECIES_TYRANITAR_MEGA    ,
    //SPECIES_SCEPTILE_MEGA     ,
    //SPECIES_BLAZIKEN_MEGA     ,
    //SPECIES_SWAMPERT_MEGA     ,
    //SPECIES_GARDEVOIR_MEGA    ,
    //SPECIES_SABLEYE_MEGA      ,
    //SPECIES_MAWILE_MEGA       ,
    //SPECIES_AGGRON_MEGA       ,
    //SPECIES_MEDICHAM_MEGA     ,
    //SPECIES_MANECTRIC_MEGA    ,
    //SPECIES_SHARPEDO_MEGA     ,
    //SPECIES_CAMERUPT_MEGA     ,
    //SPECIES_ALTARIA_MEGA      ,
    //SPECIES_BANETTE_MEGA      ,
    //SPECIES_ABSOL_MEGA        ,
    //SPECIES_GLALIE_MEGA       ,
    //SPECIES_SALAMENCE_MEGA    ,
    //SPECIES_METAGROSS_MEGA    ,
    //SPECIES_LATIAS_MEGA       ,
    //SPECIES_LATIOS_MEGA       ,
    //SPECIES_LOPUNNY_MEGA      ,
    //SPECIES_GARCHOMP_MEGA     ,
    //SPECIES_LUCARIO_MEGA      ,
    //SPECIES_ABOMASNOW_MEGA    ,
    //SPECIES_GALLADE_MEGA      ,
    //SPECIES_AUDINO_MEGA       ,
    //SPECIES_DIANCIE_MEGA      ,
    //SPECIES_RAYQUAZA_MEGA     ,
    //SPECIES_KYOGRE_PRIMAL     ,
    //SPECIES_GROUDON_PRIMAL    ,
    SPECIES_RATTATA_ALOLA    ,
    SPECIES_RATICATE_ALOLA   ,
    SPECIES_RAICHU_ALOLA     ,
    SPECIES_SANDSHREW_ALOLA  ,
    SPECIES_SANDSLASH_ALOLA  ,
    SPECIES_VULPIX_ALOLA     ,
    SPECIES_NINETALES_ALOLA  ,
    SPECIES_DIGLETT_ALOLA    ,
    SPECIES_DUGTRIO_ALOLA    ,
    SPECIES_MEOWTH_ALOLA     ,
    SPECIES_PERSIAN_ALOLA    ,
    SPECIES_GEODUDE_ALOLA    ,
    SPECIES_GRAVELER_ALOLA   ,
    SPECIES_GOLEM_ALOLA      ,
    SPECIES_GRIMER_ALOLA     ,
    SPECIES_MUK_ALOLA        ,
    SPECIES_EXEGGUTOR_ALOLA  ,
    SPECIES_MAROWAK_ALOLA    ,
    SPECIES_MEOWTH_GALAR   ,
    SPECIES_PONYTA_GALAR   ,
    SPECIES_RAPIDASH_GALAR ,
    SPECIES_SLOWPOKE_GALAR ,
    SPECIES_SLOWBRO_GALAR  ,
    SPECIES_FARFETCHD_GALAR ,
    SPECIES_WEEZING_GALAR  ,
    SPECIES_MR_MIME_GALAR  ,
    //SPECIES_ARTICUNO_GALAR ,
    //SPECIES_ZAPDOS_GALAR   ,
    //SPECIES_MOLTRES_GALAR  ,
    SPECIES_SLOWKING_GALAR ,
    SPECIES_CORSOLA_GALAR  ,
    SPECIES_ZIGZAGOON_GALAR ,
    SPECIES_LINOONE_GALAR  ,
    SPECIES_DARUMAKA_GALAR ,
    SPECIES_DARMANITAN_GALAR ,
    SPECIES_YAMASK_GALAR   ,
    SPECIES_STUNFISK_GALAR ,
    SPECIES_PIKACHU_COSPLAY   ,
    SPECIES_PIKACHU_ROCK_STAR ,
    SPECIES_PIKACHU_BELLE     ,
    SPECIES_PIKACHU_POP_STAR  ,
    SPECIES_PIKACHU_PHD      ,
    SPECIES_PIKACHU_LIBRE     ,
    SPECIES_PIKACHU_ORIGINAL ,
    SPECIES_PIKACHU_HOENN ,
    SPECIES_PIKACHU_SINNOH ,
    SPECIES_PIKACHU_UNOVA ,
    SPECIES_PIKACHU_KALOS ,
    SPECIES_PIKACHU_ALOLA ,
    SPECIES_PIKACHU_PARTNER ,
    SPECIES_PIKACHU_WORLD ,
    SPECIES_PICHU_SPIKY_EARED ,
    //SPECIES_UNOWN_B           ,
    //SPECIES_UNOWN_C           ,
    //SPECIES_UNOWN_D           ,
    //SPECIES_UNOWN_E           ,
    //SPECIES_UNOWN_F           ,
    //SPECIES_UNOWN_G           ,
    //SPECIES_UNOWN_H           ,
    //SPECIES_UNOWN_I           ,
    //SPECIES_UNOWN_J           ,
    //SPECIES_UNOWN_K           ,
    //SPECIES_UNOWN_L           ,
    //SPECIES_UNOWN_M           ,
    //SPECIES_UNOWN_N           ,
    //SPECIES_UNOWN_O           ,
    //SPECIES_UNOWN_P           ,
    //SPECIES_UNOWN_Q           ,
    //SPECIES_UNOWN_R           ,
    //SPECIES_UNOWN_S           ,
    //SPECIES_UNOWN_T           ,
    //SPECIES_UNOWN_U           ,
    //SPECIES_UNOWN_V           ,
    //SPECIES_UNOWN_W           ,
    //SPECIES_UNOWN_X           ,
    //SPECIES_UNOWN_Y           ,
    //SPECIES_UNOWN_Z           ,
    //SPECIES_UNOWN_EMARK       ,
    //SPECIES_UNOWN_QMARK       ,
    //SPECIES_CASTFORM_SUNNY    ,
    //SPECIES_CASTFORM_RAINY    ,
    //SPECIES_CASTFORM_SNOWY    ,
    //SPECIES_DEOXYS_ATTACK     ,
    //SPECIES_DEOXYS_DEFENSE    ,
    //SPECIES_DEOXYS_SPEED      ,
    SPECIES_BURMY_SANDY ,
    SPECIES_BURMY_TRASH ,
    SPECIES_WORMADAM_SANDY ,
    SPECIES_WORMADAM_TRASH ,
    SPECIES_CHERRIM_SUNSHINE  ,
    SPECIES_SHELLOS_EAST  ,
    SPECIES_GASTRODON_EAST ,
    SPECIES_ROTOM_HEAT        ,
    SPECIES_ROTOM_WASH        ,
    SPECIES_ROTOM_FROST       ,
    SPECIES_ROTOM_FAN         ,
    SPECIES_ROTOM_MOW         ,
    //SPECIES_GIRATINA_ORIGIN   ,
    //SPECIES_SHAYMIN_SKY       ,
    //SPECIES_ARCEUS_FIGHTING   ,
    //SPECIES_ARCEUS_FLYING     ,
    //SPECIES_ARCEUS_POISON     ,
    //SPECIES_ARCEUS_GROUND     ,
    //SPECIES_ARCEUS_ROCK       ,
    //SPECIES_ARCEUS_BUG        ,
    //SPECIES_ARCEUS_GHOST      ,
    //SPECIES_ARCEUS_STEEL      ,
    //SPECIES_ARCEUS_FIRE       ,
    //SPECIES_ARCEUS_WATER      ,
    //SPECIES_ARCEUS_GRASS      ,
    //SPECIES_ARCEUS_ELECTRIC   ,
    //SPECIES_ARCEUS_PSYCHIC    ,
    //SPECIES_ARCEUS_ICE        ,
    //SPECIES_ARCEUS_DRAGON     ,
    //SPECIES_ARCEUS_DARK       ,
    //SPECIES_ARCEUS_FAIRY      ,
    SPECIES_BASCULIN_BLUE_STRIPED ,
    SPECIES_DARMANITAN_ZEN ,
    SPECIES_DARMANITAN_GALAR_ZEN ,
    SPECIES_DEERLING_SUMMER   ,
    SPECIES_DEERLING_AUTUMN   ,
    SPECIES_DEERLING_WINTER   ,
    SPECIES_SAWSBUCK_SUMMER   ,
    SPECIES_SAWSBUCK_AUTUMN   ,
    SPECIES_SAWSBUCK_WINTER   ,
    //SPECIES_TORNADUS_THERIAN  ,
    //SPECIES_THUNDURUS_THERIAN ,
    //SPECIES_LANDORUS_THERIAN  ,
    //SPECIES_KYUREM_WHITE      ,
    //SPECIES_KYUREM_BLACK      ,
    //SPECIES_KELDEO_RESOLUTE   ,
    //SPECIES_MELOETTA_PIROUETTE ,
    //SPECIES_GENESECT_DOUSE ,
    //SPECIES_GENESECT_SHOCK ,
    //SPECIES_GENESECT_BURN ,
    //SPECIES_GENESECT_CHILL ,
    SPECIES_GRENINJA_BATTLE_BOND ,
    SPECIES_GRENINJA_ASH      ,
    SPECIES_VIVILLON_POLAR    ,
    SPECIES_VIVILLON_TUNDRA   ,
    SPECIES_VIVILLON_CONTINENTAL ,
    SPECIES_VIVILLON_GARDEN   ,
    SPECIES_VIVILLON_ELEGANT  ,
    SPECIES_VIVILLON_MEADOW   ,
    SPECIES_VIVILLON_MODERN   ,
    SPECIES_VIVILLON_MARINE   ,
    SPECIES_VIVILLON_ARCHIPELAGO ,
    SPECIES_VIVILLON_HIGH_PLAINS ,
    SPECIES_VIVILLON_SANDSTORM ,
    SPECIES_VIVILLON_RIVER    ,
    SPECIES_VIVILLON_MONSOON  ,
    SPECIES_VIVILLON_SAVANNA  ,
    SPECIES_VIVILLON_SUN      ,
    SPECIES_VIVILLON_OCEAN    ,
    SPECIES_VIVILLON_JUNGLE   ,
    SPECIES_VIVILLON_FANCY    ,
    SPECIES_VIVILLON_POKEBALL ,
    SPECIES_FLABEBE_YELLOW ,
    SPECIES_FLABEBE_ORANGE ,
    SPECIES_FLABEBE_BLUE ,
    SPECIES_FLABEBE_WHITE ,
    SPECIES_FLOETTE_YELLOW ,
    SPECIES_FLOETTE_ORANGE ,
    SPECIES_FLOETTE_BLUE ,
    SPECIES_FLOETTE_WHITE ,
    SPECIES_FLOETTE_ETERNAL ,
    SPECIES_FLORGES_YELLOW ,
    SPECIES_FLORGES_ORANGE ,
    SPECIES_FLORGES_BLUE ,
    SPECIES_FLORGES_WHITE ,
    SPECIES_FURFROU_HEART ,
    SPECIES_FURFROU_STAR ,
    SPECIES_FURFROU_DIAMOND ,
    SPECIES_FURFROU_DEBUTANTE ,
    SPECIES_FURFROU_MATRON ,
    SPECIES_FURFROU_DANDY ,
    SPECIES_FURFROU_LA_REINE ,
    SPECIES_FURFROU_KABUKI ,
    SPECIES_FURFROU_PHARAOH ,
    SPECIES_MEOWSTIC_F   ,
    SPECIES_AEGISLASH_BLADE   ,
    SPECIES_PUMPKABOO_SMALL   ,
    SPECIES_PUMPKABOO_LARGE   ,
    SPECIES_PUMPKABOO_SUPER   ,
    SPECIES_GOURGEIST_SMALL   ,
    SPECIES_GOURGEIST_LARGE   ,
    SPECIES_GOURGEIST_SUPER   ,
    //SPECIES_XERNEAS_ACTIVE    ,
    //SPECIES_ZYGARDE_10        ,
    //SPECIES_ZYGARDE_10_POWER_CONSTRUCT ,
    //SPECIES_ZYGARDE_50_POWER_CONSTRUCT ,
    //SPECIES_ZYGARDE_COMPLETE  ,
    //SPECIES_HOOPA_UNBOUND     ,
    SPECIES_ORICORIO_POM_POM  ,
    SPECIES_ORICORIO_PAU      ,
    SPECIES_ORICORIO_SENSU    ,
    SPECIES_ROCKRUFF_OWN_TEMPO ,
    SPECIES_LYCANROC_MIDNIGHT ,
    SPECIES_LYCANROC_DUSK     ,
    SPECIES_WISHIWASHI_SCHOOL ,
    //SPECIES_SILVALLY_FIGHTING ,
    //SPECIES_SILVALLY_FLYING   ,
    //SPECIES_SILVALLY_POISON   ,
    //SPECIES_SILVALLY_GROUND   ,
    //SPECIES_SILVALLY_ROCK     ,
    //SPECIES_SILVALLY_BUG      ,
    //SPECIES_SILVALLY_GHOST    ,
    //SPECIES_SILVALLY_STEEL    ,
    //SPECIES_SILVALLY_FIRE     ,
    //SPECIES_SILVALLY_WATER    ,
    //SPECIES_SILVALLY_GRASS    ,
    //SPECIES_SILVALLY_ELECTRIC ,
    //SPECIES_SILVALLY_PSYCHIC  ,
    //SPECIES_SILVALLY_ICE      ,
    //SPECIES_SILVALLY_DRAGON   ,
    //SPECIES_SILVALLY_DARK     ,
    //SPECIES_SILVALLY_FAIRY    ,
    SPECIES_MINIOR_METEOR_ORANGE ,
    SPECIES_MINIOR_METEOR_YELLOW ,
    SPECIES_MINIOR_METEOR_GREEN ,
    SPECIES_MINIOR_METEOR_BLUE ,
    SPECIES_MINIOR_METEOR_INDIGO ,
    SPECIES_MINIOR_METEOR_VIOLET ,
    SPECIES_MINIOR_CORE_RED   ,
    SPECIES_MINIOR_CORE_ORANGE ,
    SPECIES_MINIOR_CORE_YELLOW ,
    SPECIES_MINIOR_CORE_GREEN ,
    SPECIES_MINIOR_CORE_BLUE  ,
    SPECIES_MINIOR_CORE_INDIGO ,
    SPECIES_MINIOR_CORE_VIOLET ,
    SPECIES_MIMIKYU_BUSTED    ,
    //SPECIES_NECROZMA_DUSK_MANE ,
    //SPECIES_NECROZMA_DAWN_WINGS ,
    //SPECIES_NECROZMA_ULTRA    ,
    //SPECIES_MAGEARNA_ORIGINAL ,
    SPECIES_CRAMORANT_GULPING ,
    SPECIES_CRAMORANT_GORGING ,
    SPECIES_TOXTRICITY_LOW_KEY ,
    SPECIES_SINISTEA_ANTIQUE  ,
    SPECIES_POLTEAGEIST_ANTIQUE ,
    SPECIES_ALCREMIE_RUBY_CREAM ,
    SPECIES_ALCREMIE_MATCHA_CREAM ,
    SPECIES_ALCREMIE_MINT_CREAM ,
    SPECIES_ALCREMIE_LEMON_CREAM ,
    SPECIES_ALCREMIE_SALTED_CREAM ,
    SPECIES_ALCREMIE_RUBY_SWIRL ,
    SPECIES_ALCREMIE_CARAMEL_SWIRL ,
    SPECIES_ALCREMIE_RAINBOW_SWIRL ,
    SPECIES_EISCUE_NOICE ,
    SPECIES_INDEEDEE_F   ,
    SPECIES_MORPEKO_HANGRY    ,
    //SPECIES_ZACIAN_CROWNED ,
    //SPECIES_ZAMAZENTA_CROWNED ,
    //SPECIES_ETERNATUS_ETERNAMAX ,
    //SPECIES_URSHIFU_RAPID_STRIKE ,
    //SPECIES_ZARUDE_DADA       ,
    //SPECIES_CALYREX_ICE ,
    //SPECIES_CALYREX_SHADOW ,
    #endif
    // SPECIES_EGG       ,
};
#define RANDOM_SPECIES_COUNT_LEGENDARY ARRAY_COUNT(sRandomSpeciesLegendary)
static const u16 sRandomSpeciesLegendary[] =
{
    SPECIES_TERAPAGOS_NORMAL,
    SPECIES_CHI_YU,
    SPECIES_TING_LU,
    SPECIES_CHIEN_PAO,
    SPECIES_WO_CHIEN,
    SPECIES_TAPU_FINI,
    SPECIES_TAPU_BULU,
    SPECIES_TAPU_LELE,
    SPECIES_TAPU_KOKO,
    SPECIES_SILVALLY,
    SPECIES_ENAMORUS_THERIAN,
    SPECIES_ENAMORUS_INCARNATE,
    SPECIES_SPECTRIER,
    SPECIES_GLASTRIER,
    SPECIES_REGIDRAGO,
    SPECIES_REGIELEKI,
    SPECIES_KELDEO,
    SPECIES_THUNDURUS_THERIAN,
    SPECIES_THUNDURUS_INCARNATE,
    SPECIES_TORNADUS_THERIAN,
    SPECIES_TORNADUS_INCARNATE,
    SPECIES_VIRIZION,
    SPECIES_TERRAKION,
    SPECIES_COBALION,
    SPECIES_CRESSELIA,
    SPECIES_AZELF,
    SPECIES_MESPRIT,
    SPECIES_UXIE,
    SPECIES_REGISTEEL,
    SPECIES_REGICE,
    SPECIES_REGIROCK,
    SPECIES_SUICUNE,
    SPECIES_ENTEI,
    SPECIES_RAIKOU,
    SPECIES_MOLTRES_GALAR,
    SPECIES_MOLTRES,
    SPECIES_ZAPDOS_GALAR,
    SPECIES_ZAPDOS,
    SPECIES_ARTICUNO_GALAR,
    SPECIES_ARTICUNO,
    SPECIES_PECHARUNT,
    SPECIES_ZARUDE,
    SPECIES_MELMETAL,
    SPECIES_ZERAORA,
    SPECIES_MARSHADOW,
    SPECIES_MAGEARNA,
    SPECIES_NECROZMA,
    SPECIES_VOLCANION,
    SPECIES_HOOPA_CONFINED,
    SPECIES_DIANCIE,
    SPECIES_ZYGARDE_50_POWER_CONSTRUCT,
    SPECIES_GENESECT,
    SPECIES_MELOETTA_PIROUETTE,
    SPECIES_MELOETTA_ARIA,
    SPECIES_LANDORUS_THERIAN,
    SPECIES_LANDORUS_INCARNATE,
    SPECIES_VICTINI,
    SPECIES_SHAYMIN_SKY,
    SPECIES_SHAYMIN_LAND,
    SPECIES_DARKRAI,
    SPECIES_MANAPHY,
    SPECIES_HEATRAN,
    SPECIES_DEOXYS_SPEED,
    SPECIES_DEOXYS_NORMAL,
    SPECIES_DEOXYS_DEFENSE,
    SPECIES_DEOXYS_ATTACK,
    SPECIES_JIRACHI,
    SPECIES_LATIOS,
    SPECIES_LATIAS,
    SPECIES_CELEBI,
    SPECIES_MEW,
    SPECIES_ZAMAZENTA_HERO,
    SPECIES_ZACIAN_HERO,
    SPECIES_KYUREM,
    SPECIES_MIRAIDON,
    SPECIES_KORAIDON,
    SPECIES_REGIGIGAS,
    SPECIES_GROUDON,
    SPECIES_KYOGRE,
    SPECIES_CALYREX_SHADOW,
    SPECIES_CALYREX_ICE,
    SPECIES_NECROZMA_DUSK_MANE,
    SPECIES_NECROZMA_DAWN_WINGS,
    SPECIES_LUNALA,
    SPECIES_SOLGALEO,
    SPECIES_YVELTAL,
    SPECIES_XERNEAS,
    SPECIES_ZEKROM,
    SPECIES_RESHIRAM,
    SPECIES_GIRATINA_ORIGIN,
    SPECIES_GIRATINA_ALTERED,
    SPECIES_PALKIA_ORIGIN,
    SPECIES_PALKIA,
    SPECIES_DIALGA_ORIGIN,
    SPECIES_DIALGA,
    SPECIES_RAYQUAZA,
    SPECIES_HO_OH,
    SPECIES_LUGIA,
    SPECIES_MEWTWO,
    SPECIES_ETERNATUS,
    SPECIES_KYUREM_WHITE,
    SPECIES_KYUREM_BLACK,
    SPECIES_ZYGARDE_COMPLETE,
    SPECIES_ARCEUS,
    SPECIES_NECROZMA_ULTRA,
    ////SPECIES_NONE                    ,
    //SPECIES_BULBASAUR               ,
    //SPECIES_IVYSAUR                 ,
    //SPECIES_VENUSAUR                ,
    //SPECIES_CHARMANDER              ,
    //SPECIES_CHARMELEON              ,
    //SPECIES_CHARIZARD               ,
    //SPECIES_SQUIRTLE                ,
    //SPECIES_WARTORTLE               ,
    //SPECIES_BLASTOISE               ,
    //SPECIES_CATERPIE                ,
    //SPECIES_METAPOD                 ,
    //SPECIES_BUTTERFREE              ,
    //SPECIES_WEEDLE                  ,
    //SPECIES_KAKUNA                  ,
    //SPECIES_BEEDRILL                ,
    //SPECIES_PIDGEY                  ,
    //SPECIES_PIDGEOTTO               ,
    //SPECIES_PIDGEOT                 ,
    //SPECIES_RATTATA                 ,
    //SPECIES_RATICATE                ,
    //SPECIES_SPEAROW                 ,
    //SPECIES_FEAROW                  ,
    //SPECIES_EKANS                   ,
    //SPECIES_ARBOK                   ,
    //SPECIES_PIKACHU                 ,
    //SPECIES_RAICHU                  ,
    //SPECIES_SANDSHREW               ,
    //SPECIES_SANDSLASH               ,
    //SPECIES_NIDORAN_F               ,
    //SPECIES_NIDORINA                ,
    //SPECIES_NIDOQUEEN               ,
    //SPECIES_NIDORAN_M               ,
    //SPECIES_NIDORINO                ,
    //SPECIES_NIDOKING                ,
    //SPECIES_CLEFAIRY                ,
    //SPECIES_CLEFABLE                ,
    //SPECIES_VULPIX                  ,
    //SPECIES_NINETALES               ,
    //SPECIES_JIGGLYPUFF              ,
    //SPECIES_WIGGLYTUFF              ,
    //SPECIES_ZUBAT                   ,
    //SPECIES_GOLBAT                  ,
    //SPECIES_ODDISH                  ,
    //SPECIES_GLOOM                   ,
    //SPECIES_VILEPLUME               ,
    //SPECIES_PARAS                   ,
    //SPECIES_PARASECT                ,
    //SPECIES_VENONAT                 ,
    //SPECIES_VENOMOTH                ,
    //SPECIES_DIGLETT                 ,
    //SPECIES_DUGTRIO                 ,
    //SPECIES_MEOWTH                  ,
    //SPECIES_PERSIAN                 ,
    //SPECIES_PSYDUCK                 ,
    //SPECIES_GOLDUCK                 ,
    //SPECIES_MANKEY                  ,
    //SPECIES_PRIMEAPE                ,
    //SPECIES_GROWLITHE               ,
    //SPECIES_ARCANINE                ,
    //SPECIES_POLIWAG                 ,
    //SPECIES_POLIWHIRL               ,
    //SPECIES_POLIWRATH               ,
    //SPECIES_ABRA                    ,
    //SPECIES_KADABRA                 ,
    //SPECIES_ALAKAZAM                ,
    //SPECIES_MACHOP                  ,
    //SPECIES_MACHOKE                 ,
    //SPECIES_MACHAMP                 ,
    //SPECIES_BELLSPROUT              ,
    //SPECIES_WEEPINBELL              ,
    //SPECIES_VICTREEBEL              ,
    //SPECIES_TENTACOOL               ,
    //SPECIES_TENTACRUEL              ,
    //SPECIES_GEODUDE                 ,
    //SPECIES_GRAVELER                ,
    //SPECIES_GOLEM                   ,
    //SPECIES_PONYTA                  ,
    //SPECIES_RAPIDASH                ,
    //SPECIES_SLOWPOKE                ,
    //SPECIES_SLOWBRO                 ,
    //SPECIES_MAGNEMITE               ,
    //SPECIES_MAGNETON                ,
    //SPECIES_FARFETCHD               ,
    //SPECIES_DODUO                   ,
    //SPECIES_DODRIO                  ,
    //SPECIES_SEEL                    ,
    //SPECIES_DEWGONG                 ,
    //SPECIES_GRIMER                  ,
    //SPECIES_MUK                     ,
    //SPECIES_SHELLDER                ,
    //SPECIES_CLOYSTER                ,
    //SPECIES_GASTLY                  ,
    //SPECIES_HAUNTER                 ,
    //SPECIES_GENGAR                  ,
    //SPECIES_ONIX                    ,
    //SPECIES_DROWZEE                 ,
    //SPECIES_HYPNO                   ,
    //SPECIES_KRABBY                  ,
    //SPECIES_KINGLER                 ,
    //SPECIES_VOLTORB                 ,
    //SPECIES_ELECTRODE               ,
    //SPECIES_EXEGGCUTE               ,
    //SPECIES_EXEGGUTOR               ,
    //SPECIES_CUBONE                  ,
    //SPECIES_MAROWAK                 ,
    //SPECIES_HITMONLEE               ,
    //SPECIES_HITMONCHAN              ,
    //SPECIES_LICKITUNG               ,
    //SPECIES_KOFFING                 ,
    //SPECIES_WEEZING                 ,
    //SPECIES_RHYHORN                 ,
    //SPECIES_RHYDON                  ,
    //SPECIES_CHANSEY                 ,
    //SPECIES_TANGELA                 ,
    //SPECIES_KANGASKHAN              ,
    //SPECIES_HORSEA                  ,
    //SPECIES_SEADRA                  ,
    //SPECIES_GOLDEEN                 ,
    //SPECIES_SEAKING                 ,
    //SPECIES_STARYU                  ,
    //SPECIES_STARMIE                 ,
    //SPECIES_MR_MIME                 ,
    //SPECIES_SCYTHER                 ,
    //SPECIES_JYNX                    ,
    //SPECIES_ELECTABUZZ              ,
    //SPECIES_MAGMAR                  ,
    //SPECIES_PINSIR                  ,
    //SPECIES_TAUROS                  ,
    //SPECIES_MAGIKARP                ,
    //SPECIES_GYARADOS                ,
    //SPECIES_LAPRAS                  ,
    //SPECIES_DITTO                   ,
    //SPECIES_EEVEE                   ,
    //SPECIES_VAPOREON                ,
    //SPECIES_JOLTEON                 ,
    //SPECIES_FLAREON                 ,
    //SPECIES_PORYGON                 ,
    //SPECIES_OMANYTE                 ,
    //SPECIES_OMASTAR                 ,
    //SPECIES_KABUTO                  ,
    //SPECIES_KABUTOPS                ,
    //SPECIES_AERODACTYL              ,
    //SPECIES_SNORLAX                 ,
    //SPECIES_ARTICUNO                ,
    //SPECIES_ZAPDOS                  ,
    //SPECIES_MOLTRES                 ,
    //SPECIES_DRATINI                 ,
    //SPECIES_DRAGONAIR               ,
    //SPECIES_DRAGONITE               ,
    //SPECIES_MEWTWO                  ,
    //SPECIES_MEW                     ,
    //SPECIES_CHIKORITA                  ,
    //SPECIES_BAYLEEF                    ,
    //SPECIES_MEGANIUM                   ,
    //SPECIES_CYNDAQUIL                  ,
    //SPECIES_QUILAVA                    ,
    //SPECIES_TYPHLOSION                 ,
    //SPECIES_TOTODILE                   ,
    //SPECIES_CROCONAW                   ,
    //SPECIES_FERALIGATR                 ,
    //SPECIES_SENTRET                    ,
    //SPECIES_FURRET                     ,
    //SPECIES_HOOTHOOT                   ,
    //SPECIES_NOCTOWL                    ,
    //SPECIES_LEDYBA                     ,
    //SPECIES_LEDIAN                     ,
    //SPECIES_SPINARAK                   ,
    //SPECIES_ARIADOS                    ,
    //SPECIES_CROBAT                     ,
    //SPECIES_CHINCHOU                   ,
    //SPECIES_LANTURN                    ,
    //SPECIES_PICHU                      ,
    //SPECIES_CLEFFA                     ,
    //SPECIES_IGGLYBUFF                  ,
    //SPECIES_TOGEPI                     ,
    //SPECIES_TOGETIC                    ,
    //SPECIES_NATU                       ,
    //SPECIES_XATU                       ,
    //SPECIES_MAREEP                     ,
    //SPECIES_FLAAFFY                    ,
    //SPECIES_AMPHAROS                   ,
    //SPECIES_BELLOSSOM                  ,
    //SPECIES_MARILL                     ,
    //SPECIES_AZUMARILL                  ,
    //SPECIES_SUDOWOODO                  ,
    //SPECIES_POLITOED                   ,
    //SPECIES_HOPPIP                     ,
    //SPECIES_SKIPLOOM                   ,
    //SPECIES_JUMPLUFF                   ,
    //SPECIES_AIPOM                      ,
    //SPECIES_SUNKERN                    ,
    //SPECIES_SUNFLORA                   ,
    //SPECIES_YANMA                      ,
    //SPECIES_WOOPER                     ,
    //SPECIES_QUAGSIRE                   ,
    //SPECIES_ESPEON                     ,
    //SPECIES_UMBREON                    ,
    //SPECIES_MURKROW                    ,
    //SPECIES_SLOWKING                   ,
    //SPECIES_MISDREAVUS                 ,
    //SPECIES_UNOWN                      ,
    //SPECIES_WOBBUFFET                  ,
    //SPECIES_GIRAFARIG                  ,
    //SPECIES_PINECO                     ,
    //SPECIES_FORRETRESS                 ,
    //SPECIES_DUNSPARCE                  ,
    //SPECIES_GLIGAR                     ,
    //SPECIES_STEELIX                    ,
    //SPECIES_SNUBBULL                   ,
    //SPECIES_GRANBULL                   ,
    //SPECIES_QWILFISH                   ,
    //SPECIES_SCIZOR                     ,
    //SPECIES_SHUCKLE                    ,
    //SPECIES_HERACROSS                  ,
    //SPECIES_SNEASEL                    ,
    //SPECIES_TEDDIURSA                  ,
    //SPECIES_URSARING                   ,
    //SPECIES_SLUGMA                     ,
    //SPECIES_MAGCARGO                   ,
    //SPECIES_SWINUB                     ,
    //SPECIES_PILOSWINE                  ,
    //SPECIES_CORSOLA                    ,
    //SPECIES_REMORAID                   ,
    //SPECIES_OCTILLERY                  ,
    //SPECIES_DELIBIRD                   ,
    //SPECIES_MANTINE                    ,
    //SPECIES_SKARMORY                   ,
    //SPECIES_HOUNDOUR                   ,
    //SPECIES_HOUNDOOM                   ,
    //SPECIES_KINGDRA                    ,
    //SPECIES_PHANPY                     ,
    //SPECIES_DONPHAN                    ,
    //SPECIES_PORYGON2                   ,
    //SPECIES_STANTLER                   ,
    //SPECIES_SMEARGLE                   ,
    //SPECIES_TYROGUE                    ,
    //SPECIES_HITMONTOP                  ,
    //SPECIES_SMOOCHUM                   ,
    //SPECIES_ELEKID                     ,
    //SPECIES_MAGBY                      ,
    //SPECIES_MILTANK                    ,
    //SPECIES_BLISSEY                    ,
    //SPECIES_RAIKOU                     ,
    //SPECIES_ENTEI                      ,
    //SPECIES_SUICUNE                    ,
    //SPECIES_LARVITAR                   ,
    //SPECIES_PUPITAR                    ,
    //SPECIES_TYRANITAR                  ,
    //SPECIES_LUGIA                      ,
    //SPECIES_HO_OH                      ,
    //SPECIES_CELEBI                     ,
    //// SPECIES_OLD_UNOWN_B,
    //// SPECIES_OLD_UNOWN_C,
    //// SPECIES_OLD_UNOWN_D,
    //// SPECIES_OLD_UNOWN_E,
    //// SPECIES_OLD_UNOWN_F,
    //// SPECIES_OLD_UNOWN_G,
    //// SPECIES_OLD_UNOWN_H,
    //// SPECIES_OLD_UNOWN_I,
    //// SPECIES_OLD_UNOWN_J,
    //// SPECIES_OLD_UNOWN_K,
    //// SPECIES_OLD_UNOWN_L,
    //// SPECIES_OLD_UNOWN_M,
    //// SPECIES_OLD_UNOWN_N,
    //// SPECIES_OLD_UNOWN_O,
    //// SPECIES_OLD_UNOWN_P,
    //// SPECIES_OLD_UNOWN_Q,
    //// SPECIES_OLD_UNOWN_R,
    //// SPECIES_OLD_UNOWN_S,
    //// SPECIES_OLD_UNOWN_T,
    //// SPECIES_OLD_UNOWN_U,
    //// SPECIES_OLD_UNOWN_V,
    //// SPECIES_OLD_UNOWN_W,
    //// SPECIES_OLD_UNOWN_X,
    //// SPECIES_OLD_UNOWN_Y,
    //// SPECIES_OLD_UNOWN_Z,
    //SPECIES_TREECKO           ,
    //SPECIES_GROVYLE           ,
    //SPECIES_SCEPTILE          ,
    //SPECIES_TORCHIC           ,
    //SPECIES_COMBUSKEN         ,
    //SPECIES_BLAZIKEN          ,
    //SPECIES_MUDKIP            ,
    //SPECIES_MARSHTOMP         ,
    //SPECIES_SWAMPERT          ,
    //SPECIES_POOCHYENA         ,
    //SPECIES_MIGHTYENA         ,
    //SPECIES_ZIGZAGOON         ,
    //SPECIES_LINOONE           ,
    //SPECIES_WURMPLE           ,
    //SPECIES_SILCOON           ,
    //SPECIES_BEAUTIFLY         ,
    //SPECIES_CASCOON           ,
    //SPECIES_DUSTOX            ,
    //SPECIES_LOTAD             ,
    //SPECIES_LOMBRE            ,
    //SPECIES_LUDICOLO          ,
    //SPECIES_SEEDOT            ,
    //SPECIES_NUZLEAF           ,
    //SPECIES_SHIFTRY           ,
    //SPECIES_NINCADA           ,
    //SPECIES_NINJASK           ,
    //// SPECIES_SHEDINJA          ,
    //SPECIES_TAILLOW           ,
    //SPECIES_SWELLOW           ,
    //SPECIES_SHROOMISH         ,
    //SPECIES_BRELOOM           ,
    //SPECIES_SPINDA            ,
    //SPECIES_WINGULL           ,
    //SPECIES_PELIPPER          ,
    //SPECIES_SURSKIT           ,
    //SPECIES_MASQUERAIN        ,
    //SPECIES_WAILMER           ,
    //SPECIES_WAILORD           ,
    //SPECIES_SKITTY            ,
    //SPECIES_DELCATTY          ,
    //SPECIES_KECLEON           ,
    //SPECIES_BALTOY            ,
    //SPECIES_CLAYDOL           ,
    //SPECIES_NOSEPASS          ,
    //SPECIES_TORKOAL           ,
    //SPECIES_SABLEYE           ,
    //SPECIES_BARBOACH          ,
    //SPECIES_WHISCASH          ,
    //SPECIES_LUVDISC           ,
    //SPECIES_CORPHISH          ,
    //SPECIES_CRAWDAUNT         ,
    //SPECIES_FEEBAS            ,
    //SPECIES_MILOTIC           ,
    //SPECIES_CARVANHA          ,
    //SPECIES_SHARPEDO          ,
    //SPECIES_TRAPINCH          ,
    //SPECIES_VIBRAVA           ,
    //SPECIES_FLYGON            ,
    //SPECIES_MAKUHITA          ,
    //SPECIES_HARIYAMA          ,
    //SPECIES_ELECTRIKE         ,
    //SPECIES_MANECTRIC         ,
    //SPECIES_NUMEL             ,
    //SPECIES_CAMERUPT          ,
    //SPECIES_SPHEAL            ,
    //SPECIES_SEALEO            ,
    //SPECIES_WALREIN           ,
    //SPECIES_CACNEA            ,
    //SPECIES_CACTURNE          ,
    //SPECIES_SNORUNT           ,
    //SPECIES_GLALIE            ,
    //SPECIES_LUNATONE          ,
    //SPECIES_SOLROCK           ,
    //SPECIES_AZURILL           ,
    //SPECIES_SPOINK            ,
    //SPECIES_GRUMPIG           ,
    //SPECIES_PLUSLE            ,
    //SPECIES_MINUN             ,
    //SPECIES_MAWILE            ,
    //SPECIES_MEDITITE          ,
    //SPECIES_MEDICHAM          ,
    //SPECIES_SWABLU            ,
    //SPECIES_ALTARIA           ,
    //SPECIES_WYNAUT            ,
    //SPECIES_DUSKULL           ,
    //SPECIES_DUSCLOPS          ,
    //SPECIES_ROSELIA           ,
    //SPECIES_SLAKOTH           ,
    //SPECIES_VIGOROTH          ,
    //SPECIES_SLAKING           ,
    //SPECIES_GULPIN            ,
    //SPECIES_SWALOT            ,
    //SPECIES_TROPIUS           ,
    //SPECIES_WHISMUR           ,
    //SPECIES_LOUDRED           ,
    //SPECIES_EXPLOUD           ,
    //SPECIES_CLAMPERL          ,
    //SPECIES_HUNTAIL           ,
    //SPECIES_GOREBYSS          ,
    //SPECIES_ABSOL             ,
    //SPECIES_SHUPPET           ,
    //SPECIES_BANETTE           ,
    //SPECIES_SEVIPER           ,
    //SPECIES_ZANGOOSE          ,
    //SPECIES_RELICANTH         ,
    //SPECIES_ARON              ,
    //SPECIES_LAIRON            ,
    //SPECIES_AGGRON            ,
    //// SPECIES_CASTFORM          ,
    //SPECIES_VOLBEAT           ,
    //SPECIES_ILLUMISE          ,
    //SPECIES_LILEEP            ,
    //SPECIES_CRADILY           ,
    //SPECIES_ANORITH           ,
    //SPECIES_ARMALDO           ,
    //SPECIES_RALTS             ,
    //SPECIES_KIRLIA            ,
    //SPECIES_GARDEVOIR         ,
    //SPECIES_BAGON             ,
    //SPECIES_SHELGON           ,
    //SPECIES_SALAMENCE         ,
    //SPECIES_BELDUM            ,
    //SPECIES_METANG            ,
    //SPECIES_METAGROSS         ,
    //SPECIES_REGIROCK          ,
    //SPECIES_REGICE            ,
    //SPECIES_REGISTEEL         ,
    //SPECIES_KYOGRE            ,
    //SPECIES_GROUDON           ,
    //SPECIES_RAYQUAZA          ,
    //SPECIES_LATIAS            ,
    //SPECIES_LATIOS            ,
    //SPECIES_JIRACHI           ,
    //SPECIES_DEOXYS            ,
    //SPECIES_CHIMECHO          ,
    //#ifdef POKEMON_EXPANSION
    //SPECIES_TURTWIG           ,
    //SPECIES_GROTLE            ,
    //SPECIES_TORTERRA          ,
    //SPECIES_CHIMCHAR          ,
    //SPECIES_MONFERNO          ,
    //SPECIES_INFERNAPE         ,
    //SPECIES_PIPLUP            ,
    //SPECIES_PRINPLUP          ,
    //SPECIES_EMPOLEON          ,
    //SPECIES_STARLY            ,
    //SPECIES_STARAVIA          ,
    //SPECIES_STARAPTOR         ,
    //SPECIES_BIDOOF            ,
    //SPECIES_BIBAREL           ,
    //SPECIES_KRICKETOT         ,
    //SPECIES_KRICKETUNE        ,
    //SPECIES_SHINX             ,
    //SPECIES_LUXIO             ,
    //SPECIES_LUXRAY            ,
    //SPECIES_BUDEW             ,
    //SPECIES_ROSERADE          ,
    //SPECIES_CRANIDOS          ,
    //SPECIES_RAMPARDOS         ,
    //SPECIES_SHIELDON          ,
    //SPECIES_BASTIODON         ,
    //SPECIES_BURMY             ,
    //SPECIES_WORMADAM          ,
    //SPECIES_MOTHIM            ,
    //SPECIES_COMBEE            ,
    //SPECIES_VESPIQUEN         ,
    //SPECIES_PACHIRISU         ,
    //SPECIES_BUIZEL            ,
    //SPECIES_FLOATZEL          ,
    //SPECIES_CHERUBI           ,
    //SPECIES_CHERRIM           ,
    //SPECIES_SHELLOS           ,
    //SPECIES_GASTRODON         ,
    //SPECIES_AMBIPOM           ,
    //SPECIES_DRIFLOON          ,
    //SPECIES_DRIFBLIM          ,
    //SPECIES_BUNEARY           ,
    //SPECIES_LOPUNNY           ,
    //SPECIES_MISMAGIUS         ,
    //SPECIES_HONCHKROW         ,
    //SPECIES_GLAMEOW           ,
    //SPECIES_PURUGLY           ,
    //SPECIES_CHINGLING         ,
    //SPECIES_STUNKY            ,
    //SPECIES_SKUNTANK          ,
    //SPECIES_BRONZOR           ,
    //SPECIES_BRONZONG          ,
    //SPECIES_BONSLY            ,
    //SPECIES_MIME_JR           ,
    //SPECIES_HAPPINY           ,
    //SPECIES_CHATOT            ,
    //SPECIES_SPIRITOMB         ,
    //SPECIES_GIBLE             ,
    //SPECIES_GABITE            ,
    //SPECIES_GARCHOMP          ,
    //SPECIES_MUNCHLAX          ,
    //SPECIES_RIOLU             ,
    //SPECIES_LUCARIO           ,
    //SPECIES_HIPPOPOTAS        ,
    //SPECIES_HIPPOWDON         ,
    //SPECIES_SKORUPI           ,
    //SPECIES_DRAPION           ,
    //SPECIES_CROAGUNK          ,
    //SPECIES_TOXICROAK         ,
    //SPECIES_CARNIVINE         ,
    //SPECIES_FINNEON           ,
    //SPECIES_LUMINEON          ,
    //SPECIES_MANTYKE           ,
    //SPECIES_SNOVER            ,
    //SPECIES_ABOMASNOW         ,
    //SPECIES_WEAVILE           ,
    //SPECIES_MAGNEZONE         ,
    //SPECIES_LICKILICKY        ,
    //SPECIES_RHYPERIOR         ,
    //SPECIES_TANGROWTH         ,
    //SPECIES_ELECTIVIRE        ,
    //SPECIES_MAGMORTAR         ,
    //SPECIES_TOGEKISS          ,
    //SPECIES_YANMEGA           ,
    //SPECIES_LEAFEON           ,
    //SPECIES_GLACEON           ,
    //SPECIES_GLISCOR           ,
    //SPECIES_MAMOSWINE         ,
    //SPECIES_PORYGON_Z         ,
    //SPECIES_GALLADE           ,
    //SPECIES_PROBOPASS         ,
    //SPECIES_DUSKNOIR          ,
    //SPECIES_FROSLASS          ,
    //SPECIES_ROTOM             ,
    //SPECIES_UXIE              ,
    //SPECIES_MESPRIT           ,
    //SPECIES_AZELF             ,
    //SPECIES_DIALGA            ,
    //SPECIES_PALKIA            ,
    //SPECIES_HEATRAN           ,
    //SPECIES_REGIGIGAS         ,
    //SPECIES_GIRATINA          ,
    //SPECIES_CRESSELIA         ,
    //SPECIES_PHIONE            ,
    //SPECIES_MANAPHY           ,
    //SPECIES_DARKRAI           ,
    //SPECIES_SHAYMIN           ,
    //SPECIES_ARCEUS            ,
    //SPECIES_VICTINI           ,
    //SPECIES_SNIVY             ,
    //SPECIES_SERVINE           ,
    //SPECIES_SERPERIOR         ,
    //SPECIES_TEPIG             ,
    //SPECIES_PIGNITE           ,
    //SPECIES_EMBOAR            ,
    //SPECIES_OSHAWOTT          ,
    //SPECIES_DEWOTT            ,
    //SPECIES_SAMUROTT          ,
    //SPECIES_PATRAT            ,
    //SPECIES_WATCHOG           ,
    //SPECIES_LILLIPUP          ,
    //SPECIES_HERDIER           ,
    //SPECIES_STOUTLAND         ,
    //SPECIES_PURRLOIN          ,
    //SPECIES_LIEPARD           ,
    //SPECIES_PANSAGE           ,
    //SPECIES_SIMISAGE          ,
    //SPECIES_PANSEAR           ,
    //SPECIES_SIMISEAR          ,
    //SPECIES_PANPOUR           ,
    //SPECIES_SIMIPOUR          ,
    //SPECIES_MUNNA             ,
    //SPECIES_MUSHARNA          ,
    //SPECIES_PIDOVE            ,
    //SPECIES_TRANQUILL         ,
    //SPECIES_UNFEZANT          ,
    //SPECIES_BLITZLE           ,
    //SPECIES_ZEBSTRIKA         ,
    //SPECIES_ROGGENROLA        ,
    //SPECIES_BOLDORE           ,
    //SPECIES_GIGALITH          ,
    //SPECIES_WOOBAT            ,
    //SPECIES_SWOOBAT           ,
    //SPECIES_DRILBUR           ,
    //SPECIES_EXCADRILL         ,
    //SPECIES_AUDINO            ,
    //SPECIES_TIMBURR           ,
    //SPECIES_GURDURR           ,
    //SPECIES_CONKELDURR        ,
    //SPECIES_TYMPOLE           ,
    //SPECIES_PALPITOAD         ,
    //SPECIES_SEISMITOAD        ,
    //SPECIES_THROH             ,
    //SPECIES_SAWK              ,
    //SPECIES_SEWADDLE          ,
    //SPECIES_SWADLOON          ,
    //SPECIES_LEAVANNY          ,
    //SPECIES_VENIPEDE          ,
    //SPECIES_WHIRLIPEDE        ,
    //SPECIES_SCOLIPEDE         ,
    //SPECIES_COTTONEE          ,
    //SPECIES_WHIMSICOTT        ,
    //SPECIES_PETILIL           ,
    //SPECIES_LILLIGANT         ,
    //SPECIES_BASCULIN          ,
    //SPECIES_SANDILE           ,
    //SPECIES_KROKOROK          ,
    //SPECIES_KROOKODILE        ,
    //SPECIES_DARUMAKA          ,
    //SPECIES_DARMANITAN        ,
    //SPECIES_MARACTUS          ,
    //SPECIES_DWEBBLE           ,
    //SPECIES_CRUSTLE           ,
    //SPECIES_SCRAGGY           ,
    //SPECIES_SCRAFTY           ,
    //SPECIES_SIGILYPH          ,
    //SPECIES_YAMASK            ,
    //SPECIES_COFAGRIGUS        ,
    //SPECIES_TIRTOUGA          ,
    //SPECIES_CARRACOSTA        ,
    //SPECIES_ARCHEN            ,
    //SPECIES_ARCHEOPS          ,
    //SPECIES_TRUBBISH          ,
    //SPECIES_GARBODOR          ,
    //SPECIES_ZORUA             ,
    //SPECIES_ZOROARK           ,
    //SPECIES_MINCCINO          ,
    //SPECIES_CINCCINO          ,
    //SPECIES_GOTHITA           ,
    //SPECIES_GOTHORITA         ,
    //SPECIES_GOTHITELLE        ,
    //SPECIES_SOLOSIS           ,
    //SPECIES_DUOSION           ,
    //SPECIES_REUNICLUS         ,
    //SPECIES_DUCKLETT          ,
    //SPECIES_SWANNA            ,
    //SPECIES_VANILLITE         ,
    //SPECIES_VANILLISH         ,
    //SPECIES_VANILLUXE         ,
    //SPECIES_DEERLING          ,
    //SPECIES_SAWSBUCK          ,
    //SPECIES_EMOLGA            ,
    //SPECIES_KARRABLAST        ,
    //SPECIES_ESCAVALIER        ,
    //SPECIES_FOONGUS           ,
    //SPECIES_AMOONGUSS         ,
    //SPECIES_FRILLISH          ,
    //SPECIES_JELLICENT         ,
    //SPECIES_ALOMOMOLA         ,
    //SPECIES_JOLTIK            ,
    //SPECIES_GALVANTULA        ,
    //SPECIES_FERROSEED         ,
    //SPECIES_FERROTHORN        ,
    //SPECIES_KLINK             ,
    //SPECIES_KLANG             ,
    //SPECIES_KLINKLANG         ,
    //SPECIES_TYNAMO            ,
    //SPECIES_EELEKTRIK         ,
    //SPECIES_EELEKTROSS        ,
    //SPECIES_ELGYEM            ,
    //SPECIES_BEHEEYEM          ,
    //SPECIES_LITWICK           ,
    //SPECIES_LAMPENT           ,
    //SPECIES_CHANDELURE        ,
    //SPECIES_AXEW              ,
    //SPECIES_FRAXURE           ,
    //SPECIES_HAXORUS           ,
    //SPECIES_CUBCHOO           ,
    //SPECIES_BEARTIC           ,
    //SPECIES_CRYOGONAL         ,
    //SPECIES_SHELMET           ,
    //SPECIES_ACCELGOR          ,
    //SPECIES_STUNFISK          ,
    //SPECIES_MIENFOO           ,
    //SPECIES_MIENSHAO          ,
    //SPECIES_DRUDDIGON         ,
    //SPECIES_GOLETT            ,
    //SPECIES_GOLURK            ,
    //SPECIES_PAWNIARD          ,
    //SPECIES_BISHARP           ,
    //SPECIES_BOUFFALANT        ,
    //SPECIES_RUFFLET           ,
    //SPECIES_BRAVIARY          ,
    //SPECIES_VULLABY           ,
    //SPECIES_MANDIBUZZ         ,
    //SPECIES_HEATMOR           ,
    //SPECIES_DURANT            ,
    //SPECIES_DEINO             ,
    //SPECIES_ZWEILOUS          ,
    //SPECIES_HYDREIGON         ,
    //SPECIES_LARVESTA          ,
    //SPECIES_VOLCARONA         ,
    //SPECIES_COBALION          ,
    //SPECIES_TERRAKION         ,
    //SPECIES_VIRIZION          ,
    //SPECIES_TORNADUS          ,
    //SPECIES_THUNDURUS         ,
    //SPECIES_RESHIRAM          ,
    //SPECIES_ZEKROM            ,
    //SPECIES_LANDORUS          ,
    //SPECIES_KYUREM            ,
    //SPECIES_KELDEO            ,
    //SPECIES_MELOETTA          ,
    //SPECIES_GENESECT          ,
    //SPECIES_CHESPIN           ,
    //SPECIES_QUILLADIN         ,
    //SPECIES_CHESNAUGHT        ,
    //SPECIES_FENNEKIN          ,
    //SPECIES_BRAIXEN           ,
    //SPECIES_DELPHOX           ,
    //SPECIES_FROAKIE           ,
    //SPECIES_FROGADIER         ,
    //SPECIES_GRENINJA          ,
    //SPECIES_BUNNELBY          ,
    //SPECIES_DIGGERSBY         ,
    //SPECIES_FLETCHLING        ,
    //SPECIES_FLETCHINDER       ,
    //SPECIES_TALONFLAME        ,
    //SPECIES_SCATTERBUG        ,
    //SPECIES_SPEWPA            ,
    //SPECIES_VIVILLON          ,
    //SPECIES_LITLEO            ,
    //SPECIES_PYROAR            ,
    //SPECIES_FLABEBE           ,
    //SPECIES_FLOETTE           ,
    //SPECIES_FLORGES           ,
    //SPECIES_SKIDDO            ,
    //SPECIES_GOGOAT            ,
    //SPECIES_PANCHAM           ,
    //SPECIES_PANGORO           ,
    //SPECIES_FURFROU           ,
    //SPECIES_ESPURR            ,
    //SPECIES_MEOWSTIC          ,
    //SPECIES_HONEDGE           ,
    //SPECIES_DOUBLADE          ,
    //SPECIES_AEGISLASH         ,
    //SPECIES_SPRITZEE          ,
    //SPECIES_AROMATISSE        ,
    //SPECIES_SWIRLIX           ,
    //SPECIES_SLURPUFF          ,
    //SPECIES_INKAY             ,
    //SPECIES_MALAMAR           ,
    //SPECIES_BINACLE           ,
    //SPECIES_BARBARACLE        ,
    //SPECIES_SKRELP            ,
    //SPECIES_DRAGALGE          ,
    //SPECIES_CLAUNCHER         ,
    //SPECIES_CLAWITZER         ,
    //SPECIES_HELIOPTILE        ,
    //SPECIES_HELIOLISK         ,
    //SPECIES_TYRUNT            ,
    //SPECIES_TYRANTRUM         ,
    //SPECIES_AMAURA            ,
    //SPECIES_AURORUS           ,
    //SPECIES_SYLVEON           ,
    //SPECIES_HAWLUCHA          ,
    //SPECIES_DEDENNE           ,
    //SPECIES_CARBINK           ,
    //SPECIES_GOOMY             ,
    //SPECIES_SLIGGOO           ,
    //SPECIES_GOODRA            ,
    //SPECIES_KLEFKI            ,
    //SPECIES_PHANTUMP          ,
    //SPECIES_TREVENANT         ,
    //SPECIES_PUMPKABOO         ,
    //SPECIES_GOURGEIST         ,
    //SPECIES_BERGMITE          ,
    //SPECIES_AVALUGG           ,
    //SPECIES_NOIBAT            ,
    //SPECIES_NOIVERN           ,
    ////SPECIES_XERNEAS           ,
    ////SPECIES_YVELTAL           ,
    ////SPECIES_ZYGARDE           ,
    ////SPECIES_DIANCIE           ,
    ////SPECIES_HOOPA             ,
    ////SPECIES_VOLCANION         ,
    //SPECIES_ROWLET            ,
    //SPECIES_DARTRIX           ,
    //SPECIES_DECIDUEYE         ,
    //SPECIES_LITTEN            ,
    //SPECIES_TORRACAT          ,
    //SPECIES_INCINEROAR        ,
    //SPECIES_POPPLIO           ,
    //SPECIES_BRIONNE           ,
    //SPECIES_PRIMARINA         ,
    //SPECIES_PIKIPEK           ,
    //SPECIES_TRUMBEAK          ,
    //SPECIES_TOUCANNON         ,
    //SPECIES_YUNGOOS           ,
    //SPECIES_GUMSHOOS          ,
    //SPECIES_GRUBBIN           ,
    //SPECIES_CHARJABUG         ,
    //SPECIES_VIKAVOLT          ,
    //SPECIES_CRABRAWLER        ,
    //SPECIES_CRABOMINABLE      ,
    //SPECIES_ORICORIO          ,
    //SPECIES_CUTIEFLY          ,
    //SPECIES_RIBOMBEE          ,
    //SPECIES_ROCKRUFF          ,
    //SPECIES_LYCANROC          ,
    //SPECIES_WISHIWASHI        ,
    //SPECIES_MAREANIE          ,
    //SPECIES_TOXAPEX           ,
    //SPECIES_MUDBRAY           ,
    //SPECIES_MUDSDALE          ,
    //SPECIES_DEWPIDER          ,
    //SPECIES_ARAQUANID         ,
    //SPECIES_FOMANTIS          ,
    //SPECIES_LURANTIS          ,
    //SPECIES_MORELULL          ,
    //SPECIES_SHIINOTIC         ,
    //SPECIES_SALANDIT          ,
    //SPECIES_SALAZZLE          ,
    //SPECIES_STUFFUL           ,
    //SPECIES_BEWEAR            ,
    //SPECIES_BOUNSWEET         ,
    //SPECIES_STEENEE           ,
    //SPECIES_TSAREENA          ,
    //SPECIES_COMFEY            ,
    //SPECIES_ORANGURU          ,
    //SPECIES_PASSIMIAN         ,
    //SPECIES_WIMPOD            ,
    //SPECIES_GOLISOPOD         ,
    //SPECIES_SANDYGAST         ,
    //SPECIES_PALOSSAND         ,
    //SPECIES_PYUKUMUKU         ,
    //SPECIES_TYPE_NULL         ,
    //SPECIES_SILVALLY          ,
    //SPECIES_MINIOR            ,
    //SPECIES_KOMALA            ,
    //SPECIES_TURTONATOR        ,
    //SPECIES_TOGEDEMARU        ,
    //SPECIES_MIMIKYU           ,
    //SPECIES_BRUXISH           ,
    //SPECIES_DRAMPA            ,
    //SPECIES_DHELMISE          ,
    //SPECIES_JANGMO_O          ,
    //SPECIES_HAKAMO_O          ,
    //SPECIES_KOMMO_O           ,
    //SPECIES_TAPU_KOKO         ,
    //SPECIES_TAPU_LELE         ,
    //SPECIES_TAPU_BULU         ,
    //SPECIES_TAPU_FINI         ,
    //SPECIES_COSMOG            ,
    //SPECIES_COSMOEM           ,
    //SPECIES_SOLGALEO          ,
    //SPECIES_LUNALA            ,
    //SPECIES_NIHILEGO          ,
    //SPECIES_BUZZWOLE          ,
    //SPECIES_PHEROMOSA         ,
    //SPECIES_XURKITREE         ,
    //SPECIES_CELESTEELA        ,
    //SPECIES_KARTANA           ,
    //SPECIES_GUZZLORD          ,
    //SPECIES_NECROZMA          ,
    //SPECIES_MAGEARNA          ,
    //SPECIES_MARSHADOW         ,
    //SPECIES_POIPOLE           ,
    //SPECIES_NAGANADEL         ,
    //SPECIES_STAKATAKA         ,
    //SPECIES_BLACEPHALON       ,
    //SPECIES_ZERAORA           ,
    //SPECIES_MELTAN            ,
    //SPECIES_MELMETAL          ,
    //SPECIES_GROOKEY           ,
    //SPECIES_THWACKEY          ,
    //SPECIES_RILLABOOM         ,
    //SPECIES_SCORBUNNY         ,
    //SPECIES_RABOOT            ,
    //SPECIES_CINDERACE         ,
    //SPECIES_SOBBLE            ,
    //SPECIES_DRIZZILE          ,
    //SPECIES_INTELEON          ,
    //SPECIES_SKWOVET           ,
    //SPECIES_GREEDENT          ,
    //SPECIES_ROOKIDEE          ,
    //SPECIES_CORVISQUIRE       ,
    //SPECIES_CORVIKNIGHT       ,
    //SPECIES_BLIPBUG           ,
    //SPECIES_DOTTLER           ,
    //SPECIES_ORBEETLE          ,
    //SPECIES_NICKIT            ,
    //SPECIES_THIEVUL           ,
    //SPECIES_GOSSIFLEUR        ,
    //SPECIES_ELDEGOSS          ,
    //SPECIES_WOOLOO            ,
    //SPECIES_DUBWOOL           ,
    //SPECIES_CHEWTLE           ,
    //SPECIES_DREDNAW           ,
    //SPECIES_YAMPER            ,
    //SPECIES_BOLTUND           ,
    //SPECIES_ROLYCOLY          ,
    //SPECIES_CARKOL            ,
    //SPECIES_COALOSSAL         ,
    //SPECIES_APPLIN            ,
    //SPECIES_FLAPPLE           ,
    //SPECIES_APPLETUN          ,
    //SPECIES_SILICOBRA         ,
    //SPECIES_SANDACONDA        ,
    //SPECIES_CRAMORANT         ,
    //SPECIES_ARROKUDA          ,
    //SPECIES_BARRASKEWDA       ,
    //SPECIES_TOXEL             ,
    //SPECIES_TOXTRICITY        ,
    //SPECIES_SIZZLIPEDE        ,
    //SPECIES_CENTISKORCH       ,
    //SPECIES_CLOBBOPUS         ,
    //SPECIES_GRAPPLOCT         ,
    //SPECIES_SINISTEA          ,
    //SPECIES_POLTEAGEIST       ,
    //SPECIES_HATENNA           ,
    //SPECIES_HATTREM           ,
    //SPECIES_HATTERENE         ,
    //SPECIES_IMPIDIMP          ,
    //SPECIES_MORGREM           ,
    //SPECIES_GRIMMSNARL        ,
    //SPECIES_OBSTAGOON         ,
    //SPECIES_PERRSERKER        ,
    //SPECIES_CURSOLA           ,
    //SPECIES_SIRFETCHD         ,
    //SPECIES_MR_RIME           ,
    //SPECIES_RUNERIGUS         ,
    //SPECIES_MILCERY           ,
    //SPECIES_ALCREMIE          ,
    //SPECIES_FALINKS           ,
    //SPECIES_PINCURCHIN        ,
    //SPECIES_SNOM              ,
    //SPECIES_FROSMOTH          ,
    //SPECIES_STONJOURNER       ,
    //SPECIES_EISCUE            ,
    //SPECIES_INDEEDEE          ,
    //SPECIES_MORPEKO           ,
    //SPECIES_CUFANT            ,
    //SPECIES_COPPERAJAH        ,
    //SPECIES_DRACOZOLT         ,
    //SPECIES_ARCTOZOLT         ,
    //SPECIES_DRACOVISH         ,
    //SPECIES_ARCTOVISH         ,
    //SPECIES_DURALUDON         ,
    //SPECIES_DREEPY            ,
    //SPECIES_DRAKLOAK          ,
    //SPECIES_DRAGAPULT         ,
    //SPECIES_ZACIAN            ,
    //SPECIES_ZAMAZENTA         ,
    //SPECIES_ETERNATUS         ,
    //SPECIES_KUBFU             ,
    //SPECIES_URSHIFU           ,
    //SPECIES_ZARUDE            ,
    //SPECIES_REGIELEKI         ,
    //SPECIES_REGIDRAGO         ,
    //SPECIES_GLASTRIER         ,
    //SPECIES_SPECTRIER         ,
    //SPECIES_CALYREX           ,
    ////SPECIES_VENUSAUR_MEGA     ,
    ////SPECIES_CHARIZARD_MEGA_X  ,
    ////SPECIES_CHARIZARD_MEGA_Y  ,
    ////SPECIES_BLASTOISE_MEGA    ,
    ////SPECIES_BEEDRILL_MEGA     ,
    ////SPECIES_PIDGEOT_MEGA      ,
    ////SPECIES_ALAKAZAM_MEGA     ,
    ////SPECIES_SLOWBRO_MEGA      ,
    ////SPECIES_GENGAR_MEGA       ,
    ////SPECIES_KANGASKHAN_MEGA   ,
    ////SPECIES_PINSIR_MEGA       ,
    ////SPECIES_GYARADOS_MEGA     ,
    ////SPECIES_AERODACTYL_MEGA   ,
    ////SPECIES_MEWTWO_MEGA_X     ,
    ////SPECIES_MEWTWO_MEGA_Y     ,
    ////SPECIES_AMPHAROS_MEGA     ,
    ////SPECIES_STEELIX_MEGA      ,
    ////SPECIES_SCIZOR_MEGA       ,
    ////SPECIES_HERACROSS_MEGA    ,
    ////SPECIES_HOUNDOOM_MEGA     ,
    ////SPECIES_TYRANITAR_MEGA    ,
    ////SPECIES_SCEPTILE_MEGA     ,
    ////SPECIES_BLAZIKEN_MEGA     ,
    ////SPECIES_SWAMPERT_MEGA     ,
    ////SPECIES_GARDEVOIR_MEGA    ,
    ////SPECIES_SABLEYE_MEGA      ,
    ////SPECIES_MAWILE_MEGA       ,
    ////SPECIES_AGGRON_MEGA       ,
    ////SPECIES_MEDICHAM_MEGA     ,
    ////SPECIES_MANECTRIC_MEGA    ,
    ////SPECIES_SHARPEDO_MEGA     ,
    ////SPECIES_CAMERUPT_MEGA     ,
    ////SPECIES_ALTARIA_MEGA      ,
    ////SPECIES_BANETTE_MEGA      ,
    ////SPECIES_ABSOL_MEGA        ,
    ////SPECIES_GLALIE_MEGA       ,
    ////SPECIES_SALAMENCE_MEGA    ,
    ////SPECIES_METAGROSS_MEGA    ,
    ////SPECIES_LATIAS_MEGA       ,
    ////SPECIES_LATIOS_MEGA       ,
    ////SPECIES_LOPUNNY_MEGA      ,
    ////SPECIES_GARCHOMP_MEGA     ,
    ////SPECIES_LUCARIO_MEGA      ,
    ////SPECIES_ABOMASNOW_MEGA    ,
    ////SPECIES_GALLADE_MEGA      ,
    ////SPECIES_AUDINO_MEGA       ,
    ////SPECIES_DIANCIE_MEGA      ,
    ////SPECIES_RAYQUAZA_MEGA     ,
    ////SPECIES_KYOGRE_PRIMAL     ,
    ////SPECIES_GROUDON_PRIMAL    ,
    //SPECIES_RATTATA_ALOLA    ,
    //SPECIES_RATICATE_ALOLA   ,
    //SPECIES_RAICHU_ALOLA     ,
    //SPECIES_SANDSHREW_ALOLA  ,
    //SPECIES_SANDSLASH_ALOLA  ,
    //SPECIES_VULPIX_ALOLA     ,
    //SPECIES_NINETALES_ALOLA  ,
    //SPECIES_DIGLETT_ALOLA    ,
    //SPECIES_DUGTRIO_ALOLA    ,
    //SPECIES_MEOWTH_ALOLA     ,
    //SPECIES_PERSIAN_ALOLA    ,
    //SPECIES_GEODUDE_ALOLA    ,
    //SPECIES_GRAVELER_ALOLA   ,
    //SPECIES_GOLEM_ALOLA      ,
    //SPECIES_GRIMER_ALOLA     ,
    //SPECIES_MUK_ALOLA        ,
    //SPECIES_EXEGGUTOR_ALOLA  ,
    //SPECIES_MAROWAK_ALOLA    ,
    //SPECIES_MEOWTH_GALAR   ,
    //SPECIES_PONYTA_GALAR   ,
    //SPECIES_RAPIDASH_GALAR ,
    //SPECIES_SLOWPOKE_GALAR ,
    //SPECIES_SLOWBRO_GALAR  ,
    //SPECIES_FARFETCHD_GALAR ,
    //SPECIES_WEEZING_GALAR  ,
    //SPECIES_MR_MIME_GALAR  ,
    //SPECIES_ARTICUNO_GALAR ,
    //SPECIES_ZAPDOS_GALAR   ,
    //SPECIES_MOLTRES_GALAR  ,
    //SPECIES_SLOWKING_GALAR ,
    //SPECIES_CORSOLA_GALAR  ,
    //SPECIES_ZIGZAGOON_GALAR ,
    //SPECIES_LINOONE_GALAR  ,
    //SPECIES_DARUMAKA_GALAR ,
    //SPECIES_DARMANITAN_GALAR ,
    //SPECIES_YAMASK_GALAR   ,
    //SPECIES_STUNFISK_GALAR ,
    //SPECIES_PIKACHU_COSPLAY   ,
    //SPECIES_PIKACHU_ROCK_STAR ,
    //SPECIES_PIKACHU_BELLE     ,
    //SPECIES_PIKACHU_POP_STAR  ,
    //SPECIES_PIKACHU_PHD      ,
    //SPECIES_PIKACHU_LIBRE     ,
    //SPECIES_PIKACHU_ORIGINAL ,
    //SPECIES_PIKACHU_HOENN ,
    //SPECIES_PIKACHU_SINNOH ,
    //SPECIES_PIKACHU_UNOVA ,
    //SPECIES_PIKACHU_KALOS ,
    //SPECIES_PIKACHU_ALOLA ,
    //SPECIES_PIKACHU_PARTNER ,
    //SPECIES_PIKACHU_WORLD ,
    //SPECIES_PICHU_SPIKY_EARED ,
    ////SPECIES_UNOWN_B           ,
    ////SPECIES_UNOWN_C           ,
    ////SPECIES_UNOWN_D           ,
    ////SPECIES_UNOWN_E           ,
    ////SPECIES_UNOWN_F           ,
    ////SPECIES_UNOWN_G           ,
    ////SPECIES_UNOWN_H           ,
    ////SPECIES_UNOWN_I           ,
    ////SPECIES_UNOWN_J           ,
    ////SPECIES_UNOWN_K           ,
    ////SPECIES_UNOWN_L           ,
    ////SPECIES_UNOWN_M           ,
    ////SPECIES_UNOWN_N           ,
    ////SPECIES_UNOWN_O           ,
    ////SPECIES_UNOWN_P           ,
    ////SPECIES_UNOWN_Q           ,
    ////SPECIES_UNOWN_R           ,
    ////SPECIES_UNOWN_S           ,
    ////SPECIES_UNOWN_T           ,
    ////SPECIES_UNOWN_U           ,
    ////SPECIES_UNOWN_V           ,
    ////SPECIES_UNOWN_W           ,
    ////SPECIES_UNOWN_X           ,
    ////SPECIES_UNOWN_Y           ,
    ////SPECIES_UNOWN_Z           ,
    ////SPECIES_UNOWN_EMARK       ,
    ////SPECIES_UNOWN_QMARK       ,
    ////SPECIES_CASTFORM_SUNNY    ,
    ////SPECIES_CASTFORM_RAINY    ,
    ////SPECIES_CASTFORM_SNOWY    ,
    //SPECIES_DEOXYS_ATTACK     ,
    //SPECIES_DEOXYS_DEFENSE    ,
    //SPECIES_DEOXYS_SPEED      ,
    //SPECIES_BURMY_SANDY ,
    //SPECIES_BURMY_TRASH ,
    //SPECIES_WORMADAM_SANDY ,
    //SPECIES_WORMADAM_TRASH ,
    //SPECIES_CHERRIM_SUNSHINE  ,
    //SPECIES_SHELLOS_EAST  ,
    //SPECIES_GASTRODON_EAST ,
    //SPECIES_ROTOM_HEAT        ,
    //SPECIES_ROTOM_WASH        ,
    //SPECIES_ROTOM_FROST       ,
    //SPECIES_ROTOM_FAN         ,
    //SPECIES_ROTOM_MOW         ,
    //SPECIES_GIRATINA_ORIGIN   ,
    //SPECIES_SHAYMIN_SKY       ,
    //SPECIES_ARCEUS_FIGHTING   ,
    //SPECIES_ARCEUS_FLYING     ,
    //SPECIES_ARCEUS_POISON     ,
    //SPECIES_ARCEUS_GROUND     ,
    //SPECIES_ARCEUS_ROCK       ,
    //SPECIES_ARCEUS_BUG        ,
    //SPECIES_ARCEUS_GHOST      ,
    //SPECIES_ARCEUS_STEEL      ,
    //SPECIES_ARCEUS_FIRE       ,
    //SPECIES_ARCEUS_WATER      ,
    //SPECIES_ARCEUS_GRASS      ,
    //SPECIES_ARCEUS_ELECTRIC   ,
    //SPECIES_ARCEUS_PSYCHIC    ,
    //SPECIES_ARCEUS_ICE        ,
    //SPECIES_ARCEUS_DRAGON     ,
    //SPECIES_ARCEUS_DARK       ,
    //SPECIES_ARCEUS_FAIRY      ,
    //SPECIES_BASCULIN_BLUE_STRIPED ,
    //SPECIES_DARMANITAN_ZEN ,
    //SPECIES_DARMANITAN_GALAR_ZEN ,
    //SPECIES_DEERLING_SUMMER   ,
    //SPECIES_DEERLING_AUTUMN   ,
    //SPECIES_DEERLING_WINTER   ,
    //SPECIES_SAWSBUCK_SUMMER   ,
    //SPECIES_SAWSBUCK_AUTUMN   ,
    //SPECIES_SAWSBUCK_WINTER   ,
    //SPECIES_TORNADUS_THERIAN  ,
    //SPECIES_THUNDURUS_THERIAN ,
    //SPECIES_LANDORUS_THERIAN  ,
    //SPECIES_KYUREM_WHITE      ,
    //SPECIES_KYUREM_BLACK      ,
    //SPECIES_KELDEO_RESOLUTE   ,
    //SPECIES_MELOETTA_PIROUETTE ,
    //SPECIES_GENESECT_DOUSE ,
    //SPECIES_GENESECT_SHOCK ,
    //SPECIES_GENESECT_BURN ,
    //SPECIES_GENESECT_CHILL ,
    //SPECIES_GRENINJA_BATTLE_BOND ,
    //SPECIES_GRENINJA_ASH      ,
    //SPECIES_VIVILLON_POLAR    ,
    //SPECIES_VIVILLON_TUNDRA   ,
    //SPECIES_VIVILLON_CONTINENTAL ,
    //SPECIES_VIVILLON_GARDEN   ,
    //SPECIES_VIVILLON_ELEGANT  ,
    //SPECIES_VIVILLON_MEADOW   ,
    //SPECIES_VIVILLON_MODERN   ,
    //SPECIES_VIVILLON_MARINE   ,
    //SPECIES_VIVILLON_ARCHIPELAGO ,
    //SPECIES_VIVILLON_HIGH_PLAINS ,
    //SPECIES_VIVILLON_SANDSTORM ,
    //SPECIES_VIVILLON_RIVER    ,
    //SPECIES_VIVILLON_MONSOON  ,
    //SPECIES_VIVILLON_SAVANNA  ,
    //SPECIES_VIVILLON_SUN      ,
    //SPECIES_VIVILLON_OCEAN    ,
    //SPECIES_VIVILLON_JUNGLE   ,
    //SPECIES_VIVILLON_FANCY    ,
    //SPECIES_VIVILLON_POKEBALL ,
    //SPECIES_FLABEBE_YELLOW ,
    //SPECIES_FLABEBE_ORANGE ,
    //SPECIES_FLABEBE_BLUE ,
    //SPECIES_FLABEBE_WHITE ,
    //SPECIES_FLOETTE_YELLOW ,
    //SPECIES_FLOETTE_ORANGE ,
    //SPECIES_FLOETTE_BLUE ,
    //SPECIES_FLOETTE_WHITE ,
    //SPECIES_FLOETTE_ETERNAL ,
    //SPECIES_FLORGES_YELLOW ,
    //SPECIES_FLORGES_ORANGE ,
    //SPECIES_FLORGES_BLUE ,
    //SPECIES_FLORGES_WHITE ,
    //SPECIES_FURFROU_HEART ,
    //SPECIES_FURFROU_STAR ,
    //SPECIES_FURFROU_DIAMOND ,
    //SPECIES_FURFROU_DEBUTANTE ,
    //SPECIES_FURFROU_MATRON ,
    //SPECIES_FURFROU_DANDY ,
    //SPECIES_FURFROU_LA_REINE ,
    //SPECIES_FURFROU_KABUKI ,
    //SPECIES_FURFROU_PHARAOH ,
    //SPECIES_MEOWSTIC_F   ,
    //SPECIES_AEGISLASH_BLADE   ,
    //SPECIES_PUMPKABOO_SMALL   ,
    //SPECIES_PUMPKABOO_LARGE   ,
    //SPECIES_PUMPKABOO_SUPER   ,
    //SPECIES_GOURGEIST_SMALL   ,
    //SPECIES_GOURGEIST_LARGE   ,
    //SPECIES_GOURGEIST_SUPER   ,
    //SPECIES_XERNEAS_ACTIVE    ,
    //SPECIES_ZYGARDE_10        ,
    //SPECIES_ZYGARDE_10_POWER_CONSTRUCT ,
    //SPECIES_ZYGARDE_50_POWER_CONSTRUCT ,
    //SPECIES_ZYGARDE_COMPLETE  ,
    //SPECIES_HOOPA_UNBOUND     ,
    //SPECIES_ORICORIO_POM_POM  ,
    //SPECIES_ORICORIO_PAU      ,
    //SPECIES_ORICORIO_SENSU    ,
    //SPECIES_ROCKRUFF_OWN_TEMPO ,
    //SPECIES_LYCANROC_MIDNIGHT ,
    //SPECIES_LYCANROC_DUSK     ,
    //SPECIES_WISHIWASHI_SCHOOL ,
    //SPECIES_SILVALLY_FIGHTING ,
    //SPECIES_SILVALLY_FLYING   ,
    //SPECIES_SILVALLY_POISON   ,
    //SPECIES_SILVALLY_GROUND   ,
    //SPECIES_SILVALLY_ROCK     ,
    //SPECIES_SILVALLY_BUG      ,
    //SPECIES_SILVALLY_GHOST    ,
    //SPECIES_SILVALLY_STEEL    ,
    //SPECIES_SILVALLY_FIRE     ,
    //SPECIES_SILVALLY_WATER    ,
    //SPECIES_SILVALLY_GRASS    ,
    //SPECIES_SILVALLY_ELECTRIC ,
    //SPECIES_SILVALLY_PSYCHIC  ,
    //SPECIES_SILVALLY_ICE      ,
    //SPECIES_SILVALLY_DRAGON   ,
    //SPECIES_SILVALLY_DARK     ,
    //SPECIES_SILVALLY_FAIRY    ,
    //SPECIES_MINIOR_METEOR_ORANGE ,
    //SPECIES_MINIOR_METEOR_YELLOW ,
    //SPECIES_MINIOR_METEOR_GREEN ,
    //SPECIES_MINIOR_METEOR_BLUE ,
    //SPECIES_MINIOR_METEOR_INDIGO ,
    //SPECIES_MINIOR_METEOR_VIOLET ,
    //SPECIES_MINIOR_CORE_RED   ,
    //SPECIES_MINIOR_CORE_ORANGE ,
    //SPECIES_MINIOR_CORE_YELLOW ,
    //SPECIES_MINIOR_CORE_GREEN ,
    //SPECIES_MINIOR_CORE_BLUE  ,
    //SPECIES_MINIOR_CORE_INDIGO ,
    //SPECIES_MINIOR_CORE_VIOLET ,
    //SPECIES_MIMIKYU_BUSTED    ,
    //SPECIES_NECROZMA_DUSK_MANE ,
    //SPECIES_NECROZMA_DAWN_WINGS ,
    //SPECIES_NECROZMA_ULTRA    ,
    //SPECIES_MAGEARNA_ORIGINAL ,
    //SPECIES_CRAMORANT_GULPING ,
    //SPECIES_CRAMORANT_GORGING ,
    //SPECIES_TOXTRICITY_LOW_KEY ,
    //SPECIES_SINISTEA_ANTIQUE  ,
    //SPECIES_POLTEAGEIST_ANTIQUE ,
    //SPECIES_ALCREMIE_RUBY_CREAM ,
    //SPECIES_ALCREMIE_MATCHA_CREAM ,
    //SPECIES_ALCREMIE_MINT_CREAM ,
    //SPECIES_ALCREMIE_LEMON_CREAM ,
    //SPECIES_ALCREMIE_SALTED_CREAM ,
    //SPECIES_ALCREMIE_RUBY_SWIRL ,
    //SPECIES_ALCREMIE_CARAMEL_SWIRL ,
    //SPECIES_ALCREMIE_RAINBOW_SWIRL ,
    //SPECIES_EISCUE_NOICE ,
    //SPECIES_INDEEDEE_F   ,
    //SPECIES_MORPEKO_HANGRY    ,
    //SPECIES_ZACIAN_CROWNED ,
    //SPECIES_ZAMAZENTA_CROWNED ,
    //SPECIES_ETERNATUS_ETERNAMAX ,
    //SPECIES_URSHIFU_RAPID_STRIKE ,
    //SPECIES_ZARUDE_DADA       ,
    //SPECIES_CALYREX_ICE ,
    //SPECIES_CALYREX_SHADOW ,
    //#endif
    //// SPECIES_EGG       ,
};

#define RANDOM_SPECIES_STARTER_COUNT ARRAY_COUNT(sRandomSpeciesStarter)
static const u16 sRandomSpeciesStarter[] =
{
    SPECIES_BLIPBUG,
    SPECIES_AZURILL,
    SPECIES_WURMPLE,
    SPECIES_WEEDLE,
    SPECIES_CATERPIE,
    SPECIES_RALTS,
    SPECIES_SCATTERBUG,
    SPECIES_PICHU,
    SPECIES_BOUNSWEET,
    SPECIES_IGGLYBUFF,
    SPECIES_CLEFFA,
    SPECIES_HAPPINY,
    SPECIES_SEEDOT,
    SPECIES_LOTAD,
    SPECIES_PAWMI,
    SPECIES_ROLYCOLY,
    SPECIES_WHISMUR,
    SPECIES_ZIGZAGOON_GALAR,
    SPECIES_ROOKIDEE,
    SPECIES_STARLY,
    SPECIES_TOGEPI,
    SPECIES_ZUBAT,
    SPECIES_SWINUB,
    SPECIES_HOPPIP,
    SPECIES_PIDGEY,
    SPECIES_SMOLIV,
    SPECIES_VENIPEDE,
    SPECIES_SHINX,
    SPECIES_PIDOVE,
    SPECIES_IMPIDIMP,
    SPECIES_HATENNA,
    SPECIES_PIKIPEK,
    SPECIES_DREEPY,
    SPECIES_NIDORAN_M,
    SPECIES_LITWICK,
    SPECIES_TYNAMO,
    SPECIES_LILLIPUP,
    SPECIES_NIDORAN_F,
    SPECIES_FLETCHLING,
    SPECIES_NACLI,
    SPECIES_ROGGENROLA,
    SPECIES_BUDEW,
    SPECIES_SLAKOTH,
    SPECIES_MAREEP,
    SPECIES_SOLOSIS,
    SPECIES_GOTHITA,
    SPECIES_SPHEAL,
    SPECIES_TRAPINCH,
    SPECIES_SANDILE,
    SPECIES_TYMPOLE,
    SPECIES_DUSKULL,
    SPECIES_HORSEA,
    SPECIES_TINKATINK,
    SPECIES_JANGMO_O,
    SPECIES_GRUBBIN,
    SPECIES_GOOMY,
    SPECIES_DEINO,
    SPECIES_KLINK,
    SPECIES_GIBLE,
    SPECIES_BELDUM,
    SPECIES_BAGON,
    SPECIES_LARVITAR,
    SPECIES_DRATINI,
    SPECIES_GEODUDE_ALOLA,
    SPECIES_GEODUDE,
    SPECIES_BELLSPROUT,
    SPECIES_POLIWAG,
    SPECIES_VANILLITE,
    SPECIES_TIMBURR,
    SPECIES_MACHOP,
    SPECIES_MANKEY,
    SPECIES_FENNEKIN,
    SPECIES_OSHAWOTT,
    SPECIES_TEPIG,
    SPECIES_SNIVY,
    SPECIES_CHIMCHAR,
    SPECIES_CYNDAQUIL,
    SPECIES_CHARMANDER,
    SPECIES_QUAXLY,
    SPECIES_FUECOCO,
    SPECIES_SPRIGATITO,
    SPECIES_SOBBLE,
    SPECIES_SCORBUNNY,
    SPECIES_GROOKEY,
    SPECIES_SEWADDLE,
    SPECIES_MUDKIP,
    SPECIES_TORCHIC,
    SPECIES_TREECKO,
    SPECIES_GASTLY,
    SPECIES_ABRA,
    SPECIES_CHESPIN,
    SPECIES_FROAKIE,
    SPECIES_PIPLUP,
    SPECIES_TOTODILE,
    SPECIES_SQUIRTLE,
    SPECIES_TURTWIG,
    SPECIES_CHIKORITA,
    SPECIES_BULBASAUR,
    SPECIES_FRIGIBAX,
    SPECIES_POPPLIO,
    SPECIES_LITTEN,
    SPECIES_ROWLET,
    SPECIES_AXEW,
    SPECIES_ODDISH,
    SPECIES_HONEDGE,
    SPECIES_EEVEE,
    SPECIES_MAGNEMITE,
    SPECIES_ARON,
    SPECIES_TEDDIURSA,
    SPECIES_PAWNIARD,
    SPECIES_RHYHORN,
    SPECIES_ELEKID,
    SPECIES_MAGBY,
    SPECIES_PORYGON,
};

#define RANDOM_GYM_TYPES_COUNT ARRAY_COUNT(sRandomGymTypes)
static const u8 sRandomGymTypes[] =
{
    TYPE_NORMAL,
    TYPE_FIGHTING,
    TYPE_FLYING,
    TYPE_POISON,
    TYPE_GROUND,
    TYPE_ROCK,
    TYPE_BUG,
    TYPE_GHOST,
    TYPE_STEEL,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_GRASS,
    TYPE_ELECTRIC,
    TYPE_PSYCHIC,
    TYPE_ICE,
    TYPE_DRAGON,
    TYPE_DARK,
    TYPE_FAIRY,
};
EWRAM_DATA static u8 sRandomGymTypesShuffled[RANDOM_GYM_TYPES_COUNT] = {0};
EWRAM_DATA static bool8 bRandomGymTypesIsSetup = FALSE;
static void SetUpRandomGymTypes() {
    if (bRandomGymTypesIsSetup != TRUE) {
        for (u8 i = 0; i < RANDOM_GYM_TYPES_COUNT; ++i)
        {
            sRandomGymTypesShuffled[i] = sRandomGymTypes[i];
        }
        u32 otId = gSaveBlock2Ptr->playerTrainerId[0]
              | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
              | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
              | (gSaveBlock2Ptr->playerTrainerId[3] << 24);

        for (u16 i = (RANDOM_GYM_TYPES_COUNT - 1); i > 0; i--)
        {
            u16 j = RandomSeeded(otId + i, TRUE) % (i + 1);
            u8 val = sRandomGymTypesShuffled[j];
            sRandomGymTypesShuffled[j] = sRandomGymTypesShuffled[i];
            sRandomGymTypesShuffled[i] = val;
        }
        bRandomGymTypesIsSetup = TRUE;
    }
}

#define RANDOM_SPECIES_ALL_COUNT ARRAY_COUNT(sRandomSpeciesAllBst)
static const u16 sRandomSpeciesAllBst[] =
{
    SPECIES_WISHIWASHI_SOLO,
    SPECIES_BLIPBUG,
    SPECIES_SUNKERN,
    SPECIES_SNOM,
    SPECIES_AZURILL,
    SPECIES_KRICKETOT,
    SPECIES_WURMPLE,
    SPECIES_WEEDLE,
    SPECIES_CATERPIE,
    SPECIES_RALTS,
    SPECIES_COSMOG,
    SPECIES_SCATTERBUG,
    SPECIES_FEEBAS,
    SPECIES_MAGIKARP,
    SPECIES_CASCOON,
    SPECIES_SILCOON,
    SPECIES_PICHU,
    SPECIES_KAKUNA,
    SPECIES_METAPOD,
    SPECIES_NYMBLE,
    SPECIES_TAROUNTULA,
    SPECIES_BOUNSWEET,
    SPECIES_TYROGUE,
    SPECIES_WOOPER_PALDEA,
    SPECIES_WOOPER,
    SPECIES_IGGLYBUFF,
    SPECIES_SPEWPA,
    SPECIES_SENTRET,
    SPECIES_CLEFFA,
    SPECIES_HAPPINY,
    SPECIES_SEEDOT,
    SPECIES_LOTAD,
    SPECIES_POOCHYENA,
    SPECIES_BURMY,
    SPECIES_WIMPOD,
    SPECIES_SHEDINJA,
    SPECIES_BUNNELBY,
    SPECIES_MAKUHITA,
    SPECIES_PAWMI,
    SPECIES_ROLYCOLY,
    SPECIES_WHISMUR,
    SPECIES_ZIGZAGOON_GALAR,
    SPECIES_ZIGZAGOON,
    SPECIES_TOXEL,
    SPECIES_COMBEE,
    SPECIES_WIGLETT,
    SPECIES_NICKIT,
    SPECIES_ROOKIDEE,
    SPECIES_NOIBAT,
    SPECIES_STARLY,
    SPECIES_TOGEPI,
    SPECIES_ZUBAT,
    SPECIES_GOSSIFLEUR,
    SPECIES_FOMANTIS,
    SPECIES_BIDOOF,
    SPECIES_SMEARGLE,
    SPECIES_SWINUB,
    SPECIES_SLUGMA,
    SPECIES_HOPPIP,
    SPECIES_MARILL,
    SPECIES_SPINARAK,
    SPECIES_PIDGEY,
    SPECIES_YUNGOOS,
    SPECIES_RATTATA_ALOLA,
    SPECIES_RATTATA,
    SPECIES_LECHONK,
    SPECIES_FLITTLE,
    SPECIES_CHARCADET,
    SPECIES_PATRAT,
    SPECIES_SMOLIV,
    SPECIES_APPLIN,
    SPECIES_VENIPEDE,
    SPECIES_WYNAUT,
    SPECIES_SKITTY,
    SPECIES_HOOTHOOT,
    SPECIES_SPEAROW,
    SPECIES_SHINX,
    SPECIES_PIDOVE,
    SPECIES_IMPIDIMP,
    SPECIES_HATENNA,
    SPECIES_PIKIPEK,
    SPECIES_LEDYBA,
    SPECIES_DIGLETT_ALOLA,
    SPECIES_DIGLETT,
    SPECIES_NINCADA,
    SPECIES_DEWPIDER,
    SPECIES_SURSKIT,
    SPECIES_RELLOR,
    SPECIES_DREEPY,
    SPECIES_MILCERY,
    SPECIES_YAMPER,
    SPECIES_WOOLOO,
    SPECIES_WINGULL,
    SPECIES_TAILLOW,
    SPECIES_JIGGLYPUFF,
    SPECIES_TADBULB,
    SPECIES_NIDORAN_M,
    SPECIES_BRAMBLIN,
    SPECIES_SKWOVET,
    SPECIES_LITWICK,
    SPECIES_TYNAMO,
    SPECIES_LILLIPUP,
    SPECIES_CHERUBI,
    SPECIES_NIDORAN_F,
    SPECIES_FLETCHLING,
    SPECIES_KIRLIA,
    SPECIES_WATTREL,
    SPECIES_NACLI,
    SPECIES_ARROKUDA,
    SPECIES_ROCKRUFF,
    SPECIES_PETILIL,
    SPECIES_COTTONEE,
    SPECIES_ROGGENROLA,
    SPECIES_BUDEW,
    SPECIES_MEDITITE,
    SPECIES_SLAKOTH,
    SPECIES_MAREEP,
    SPECIES_PURRLOIN,
    SPECIES_CHEWTLE,
    SPECIES_MORELULL,
    SPECIES_RIOLU,
    SPECIES_CHINGLING,
    SPECIES_PARAS,
    SPECIES_INKAY,
    SPECIES_BARBOACH,
    SPECIES_DITTO,
    SPECIES_EKANS,
    SPECIES_HELIOPTILE,
    SPECIES_GREAVARD,
    SPECIES_SHROODLE,
    SPECIES_STEENEE,
    SPECIES_SOLOSIS,
    SPECIES_GOTHITA,
    SPECIES_BONSLY,
    SPECIES_SPHEAL,
    SPECIES_TRAPINCH,
    SPECIES_PINECO,
    SPECIES_MEOWTH_GALAR,
    SPECIES_MEOWTH_ALOLA,
    SPECIES_MEOWTH,
    SPECIES_SANDILE,
    SPECIES_MUNNA,
    SPECIES_FOONGUS,
    SPECIES_TYMPOLE,
    SPECIES_BLITZLE,
    SPECIES_DUSKULL,
    SPECIES_SHUPPET,
    SPECIES_ELECTRIKE,
    SPECIES_SHROOMISH,
    SPECIES_HORSEA,
    SPECIES_TINKATINK,
    SPECIES_VULPIX_ALOLA,
    SPECIES_VULPIX,
    SPECIES_GIMMIGHOUL_ROAMING,
    SPECIES_GIMMIGHOUL_CHEST,
    SPECIES_VAROOM,
    SPECIES_MELTAN,
    SPECIES_JANGMO_O,
    SPECIES_GRUBBIN,
    SPECIES_GOOMY,
    SPECIES_DEINO,
    SPECIES_KLINK,
    SPECIES_MINCCINO,
    SPECIES_CROAGUNK,
    SPECIES_GIBLE,
    SPECIES_BRONZOR,
    SPECIES_BELDUM,
    SPECIES_BAGON,
    SPECIES_SNORUNT,
    SPECIES_BALTOY,
    SPECIES_LARVITAR,
    SPECIES_REMORAID,
    SPECIES_SNUBBULL,
    SPECIES_DRATINI,
    SPECIES_GEODUDE_ALOLA,
    SPECIES_GEODUDE,
    SPECIES_BELLSPROUT,
    SPECIES_POLIWAG,
    SPECIES_SANDSHREW_ALOLA,
    SPECIES_SANDSHREW,
    SPECIES_GULPIN,
    SPECIES_FLABEBE,
    SPECIES_GOLETT,
    SPECIES_YAMASK_GALAR,
    SPECIES_YAMASK,
    SPECIES_CAPSAKID,
    SPECIES_CUTIEFLY,
    SPECIES_BERGMITE,
    SPECIES_TANDEMAUS,
    SPECIES_SIZZLIPEDE,
    SPECIES_MAREANIE,
    SPECIES_SHELMET,
    SPECIES_CUBCHOO,
    SPECIES_FERROSEED,
    SPECIES_VANILLITE,
    SPECIES_DUCKLETT,
    SPECIES_TIMBURR,
    SPECIES_NUMEL,
    SPECIES_CARVANHA,
    SPECIES_SMOOCHUM,
    SPECIES_SHELLDER,
    SPECIES_MACHOP,
    SPECIES_MANKEY,
    SPECIES_VENONAT,
    SPECIES_BINACLE,
    SPECIES_FENNEKIN,
    SPECIES_POLTCHAGEIST,
    SPECIES_SINISTEA,
    SPECIES_OSHAWOTT,
    SPECIES_TEPIG,
    SPECIES_SNIVY,
    SPECIES_CORPHISH,
    SPECIES_PHANTUMP,
    SPECIES_CHIMCHAR,
    SPECIES_CYNDAQUIL,
    SPECIES_CHARMANDER,
    SPECIES_QUAXLY,
    SPECIES_FUECOCO,
    SPECIES_SPRIGATITO,
    SPECIES_CLOBBOPUS,
    SPECIES_SOBBLE,
    SPECIES_SCORBUNNY,
    SPECIES_GROOKEY,
    SPECIES_SEWADDLE,
    SPECIES_MIME_JR,
    SPECIES_GLAMEOW,
    SPECIES_SWABLU,
    SPECIES_MUDKIP,
    SPECIES_TORCHIC,
    SPECIES_TREECKO,
    SPECIES_GASTLY,
    SPECIES_DODUO,
    SPECIES_ABRA,
    SPECIES_FIDOUGH,
    SPECIES_CHESPIN,
    SPECIES_FROAKIE,
    SPECIES_PIPLUP,
    SPECIES_TOTODILE,
    SPECIES_SQUIRTLE,
    SPECIES_FINIZEN,
    SPECIES_SILICOBRA,
    SPECIES_KARRABLAST,
    SPECIES_DARUMAKA_GALAR,
    SPECIES_DARUMAKA,
    SPECIES_SLOWPOKE_GALAR,
    SPECIES_SLOWPOKE,
    SPECIES_PANPOUR,
    SPECIES_PANSEAR,
    SPECIES_PANSAGE,
    SPECIES_TURTWIG,
    SPECIES_CHIKORITA,
    SPECIES_BULBASAUR,
    SPECIES_JOLTIK,
    SPECIES_FRIGIBAX,
    SPECIES_SANDYGAST,
    SPECIES_SALANDIT,
    SPECIES_POPPLIO,
    SPECIES_LITTEN,
    SPECIES_ROWLET,
    SPECIES_SKRELP,
    SPECIES_AXEW,
    SPECIES_NATU,
    SPECIES_GOLDEEN,
    SPECIES_CUBONE,
    SPECIES_PSYDUCK,
    SPECIES_ODDISH,
    SPECIES_PIKACHU,
    SPECIES_WOOBAT,
    SPECIES_CLEFAIRY,
    SPECIES_HONEDGE,
    SPECIES_DWEBBLE,
    SPECIES_SHELLOS,
    SPECIES_EEVEE,
    SPECIES_EXEGGCUTE,
    SPECIES_KRABBY,
    SPECIES_GRIMER_ALOLA,
    SPECIES_GRIMER,
    SPECIES_SEEL,
    SPECIES_MAGNEMITE,
    SPECIES_DRILBUR,
    SPECIES_DROWZEE,
    SPECIES_TRUBBISH,
    SPECIES_STUNKY,
    SPECIES_CUFANT,
    SPECIES_CLAUNCHER,
    SPECIES_ZORUA_HISUI,
    SPECIES_ZORUA,
    SPECIES_FINNEON,
    SPECIES_SKORUPI,
    SPECIES_HIPPOPOTAS,
    SPECIES_BUIZEL,
    SPECIES_LUVDISC,
    SPECIES_SPOINK,
    SPECIES_ARON,
    SPECIES_PHANPY,
    SPECIES_HOUNDOUR,
    SPECIES_DELIBIRD,
    SPECIES_TEDDIURSA,
    SPECIES_CHINCHOU,
    SPECIES_VOLTORB_HISUI,
    SPECIES_VOLTORB,
    SPECIES_CETODDLE,
    SPECIES_SNOVER,
    SPECIES_TOEDSCOOL,
    SPECIES_DOTTLER,
    SPECIES_PUMPKABOO_SUPER,
    SPECIES_PUMPKABOO_SMALL,
    SPECIES_PUMPKABOO_LARGE,
    SPECIES_PUMPKABOO_AVERAGE,
    SPECIES_ELGYEM,
    SPECIES_FRILLISH,
    SPECIES_DEERLING,
    SPECIES_CACNEA,
    SPECIES_TENTACOOL,
    SPECIES_UNOWN,
    SPECIES_CRABRAWLER,
    SPECIES_MASCHIFF,
    SPECIES_STUFFUL,
    SPECIES_PAWNIARD,
    SPECIES_STARAVIA,
    SPECIES_VIBRAVA,
    SPECIES_NUZLEAF,
    SPECIES_LOMBRE,
    SPECIES_SKIPLOOM,
    SPECIES_STARYU,
    SPECIES_KOFFING,
    SPECIES_SWIRLIX,
    SPECIES_SPRITZEE,
    SPECIES_MANTYKE,
    SPECIES_CLAMPERL,
    SPECIES_RHYHORN,
    SPECIES_PANCHAM,
    SPECIES_SCRAGGY,
    SPECIES_DRIFLOON,
    SPECIES_PIDGEOTTO,
    SPECIES_GLIMMET,
    SPECIES_PAWMO,
    SPECIES_SKIDDO,
    SPECIES_RUFFLET,
    SPECIES_MIENFOO,
    SPECIES_BUNEARY,
    SPECIES_SHIELDON,
    SPECIES_CRANIDOS,
    SPECIES_GROWLITHE_HISUI,
    SPECIES_GROWLITHE,
    SPECIES_KROKOROK,
    SPECIES_DOLLIV,
    SPECIES_NACLSTACK,
    SPECIES_TRUMBEAK,
    SPECIES_ESPURR,
    SPECIES_TIRTOUGA,
    SPECIES_ANORITH,
    SPECIES_LILEEP,
    SPECIES_KABUTO,
    SPECIES_OMANYTE,
    SPECIES_TRANQUILL,
    SPECIES_LARVESTA,
    SPECIES_WHIRLIPEDE,
    SPECIES_SPINDA,
    SPECIES_LOUDRED,
    SPECIES_ELEKID,
    SPECIES_AIPOM,
    SPECIES_AMAURA,
    SPECIES_TYRUNT,
    SPECIES_LUXIO,
    SPECIES_CORVISQUIRE,
    SPECIES_MAGBY,
    SPECIES_FLAAFFY,
    SPECIES_NIDORINO,
    SPECIES_NIDORINA,
    SPECIES_LITLEO,
    SPECIES_MORGREM,
    SPECIES_HATTREM,
    SPECIES_VULLABY,
    SPECIES_LAMPENT,
    SPECIES_DUOSION,
    SPECIES_HERDIER,
    SPECIES_FLOETTE,
    SPECIES_NOSEPASS,
    SPECIES_FARFETCHD_GALAR,
    SPECIES_FARFETCHD,
    SPECIES_TINKATUFF,
    SPECIES_SWADLOON,
    SPECIES_MAWILE,
    SPECIES_SABLEYE,
    SPECIES_FLETCHINDER,
    SPECIES_PALPITOAD,
    SPECIES_KRICKETUNE,
    SPECIES_KUBFU,
    SPECIES_MUDBRAY,
    SPECIES_DUSTOX,
    SPECIES_LICKITUNG,
    SPECIES_ONIX,
    SPECIES_POLIWHIRL,
    SPECIES_GOTHORITA,
    SPECIES_BOLDORE,
    SPECIES_MUNCHLAX,
    SPECIES_YANMA,
    SPECIES_LEDIAN,
    SPECIES_GRAVELER_ALOLA,
    SPECIES_GRAVELER,
    SPECIES_WEEPINBELL,
    SPECIES_VANILLISH,
    SPECIES_BEAUTIFLY,
    SPECIES_PORYGON,
    SPECIES_GLOOM,
    SPECIES_BEEDRILL,
    SPECIES_BUTTERFREE,
    SPECIES_COSMOEM,
    SPECIES_CHARJABUG,
    SPECIES_WAILMER,
    SPECIES_ROSELIA,
    SPECIES_DELCATTY,
    SPECIES_ARIADOS,
    SPECIES_KADABRA,
    SPECIES_ARCHEN,
    SPECIES_SPIDOPS,
    SPECIES_SHIINOTIC,
    SPECIES_FROGADIER,
    SPECIES_QUILLADIN,
    SPECIES_EELEKTRIK,
    SPECIES_GURDURR,
    SPECIES_PACHIRISU,
    SPECIES_PRINPLUP,
    SPECIES_MONFERNO,
    SPECIES_GROTLE,
    SPECIES_MINUN,
    SPECIES_PLUSLE,
    SPECIES_MARSHTOMP,
    SPECIES_COMBUSKEN,
    SPECIES_GROVYLE,
    SPECIES_WOBBUFFET,
    SPECIES_MURKROW,
    SPECIES_TOGETIC,
    SPECIES_CROCONAW,
    SPECIES_QUILAVA,
    SPECIES_BAYLEEF,
    SPECIES_HAUNTER,
    SPECIES_MACHOKE,
    SPECIES_PARASECT,
    SPECIES_WARTORTLE,
    SPECIES_CHARMELEON,
    SPECIES_IVYSAUR,
    SPECIES_BRAIXEN,
    SPECIES_QUAXWELL,
    SPECIES_FLORAGATO,
    SPECIES_DRAKLOAK,
    SPECIES_CARKOL,
    SPECIES_PYUKUMUKU,
    SPECIES_FRAXURE,
    SPECIES_GABITE,
    SPECIES_BIBAREL,
    SPECIES_SEALEO,
    SPECIES_MEDICHAM,
    SPECIES_PUPITAR,
    SPECIES_CORSOLA_GALAR,
    SPECIES_CORSOLA,
    SPECIES_SUDOWOODO,
    SPECIES_PONYTA_GALAR,
    SPECIES_PONYTA,
    SPECIES_CROCALOR,
    SPECIES_VIVILLON,
    SPECIES_CHATOT,
    SPECIES_DEWOTT,
    SPECIES_SERVINE,
    SPECIES_RATICATE_ALOLA,
    SPECIES_RATICATE,
    SPECIES_DUNSPARCE,
    SPECIES_FURRET,
    SPECIES_SQUAWKABILLY,
    SPECIES_GUMSHOOS,
    SPECIES_PIGNITE,
    SPECIES_DRIZZILE,
    SPECIES_RABOOT,
    SPECIES_THWACKEY,
    SPECIES_POIPOLE,
    SPECIES_HAKAMO_O,
    SPECIES_BRIONNE,
    SPECIES_TORRACAT,
    SPECIES_DARTRIX,
    SPECIES_ZWEILOUS,
    SPECIES_WATCHOG,
    SPECIES_METANG,
    SPECIES_SHELGON,
    SPECIES_CASTFORM,
    SPECIES_LINOONE_GALAR,
    SPECIES_LINOONE,
    SPECIES_MIGHTYENA,
    SPECIES_AZUMARILL,
    SPECIES_DRAGONAIR,
    SPECIES_ARCTIBAX,
    SPECIES_DIGGERSBY,
    SPECIES_MOTHIM,
    SPECIES_WORMADAM_TRASH,
    SPECIES_WORMADAM_SANDY,
    SPECIES_WORMADAM_PLANT,
    SPECIES_WUGTRIO,
    SPECIES_SWOOBAT,
    SPECIES_SUNFLORA,
    SPECIES_MAROWAK_ALOLA,
    SPECIES_MAROWAK,
    SPECIES_DUGTRIO_ALOLA,
    SPECIES_DUGTRIO,
    SPECIES_EMOLGA,
    SPECIES_CLODSIRE,
    SPECIES_ILLUMISE,
    SPECIES_VOLBEAT,
    SPECIES_LAIRON,
    SPECIES_MAGCARGO,
    SPECIES_SNEASEL_HISUI,
    SPECIES_SNEASEL,
    SPECIES_GLIGAR,
    SPECIES_QUAGSIRE,
    SPECIES_PIKACHU_STARTER,
    SPECIES_DEDENNE,
    SPECIES_PINCURCHIN,
    SPECIES_TOGEDEMARU,
    SPECIES_MISDREAVUS,
    SPECIES_EEVEE_STARTER,
    SPECIES_TANGELA,
    SPECIES_WIGGLYTUFF,
    SPECIES_MORPEKO,
    SPECIES_PERRSERKER,
    SPECIES_MINIOR_METEOR,
    SPECIES_KLANG,
    SPECIES_ROTOM,
    SPECIES_KECLEON,
    SPECIES_VIGOROTH,
    SPECIES_PELIPPER,
    SPECIES_QWILFISH_HISUI,
    SPECIES_QWILFISH,
    SPECIES_SEADRA,
    SPECIES_PERSIAN_ALOLA,
    SPECIES_PERSIAN,
    SPECIES_FEAROW,
    SPECIES_AUDINO,
    SPECIES_LIEPARD,
    SPECIES_DOUBLADE,
    SPECIES_ARBOK,
    SPECIES_TERAPAGOS_NORMAL,
    SPECIES_KLAWF,
    SPECIES_LOKIX,
    SPECIES_CHERRIM,
    SPECIES_PILOSWINE,
    SPECIES_GRANBULL,
    SPECIES_SEAKING,
    SPECIES_CHANSEY,
    SPECIES_VENOMOTH,
    SPECIES_SANDSLASH_ALOLA,
    SPECIES_SANDSLASH,
    SPECIES_SLIGGOO_HISUI,
    SPECIES_SLIGGOO,
    SPECIES_PURUGLY,
    SPECIES_NOCTOWL,
    SPECIES_ARAQUANID,
    SPECIES_CARNIVINE,
    SPECIES_MASQUERAIN,
    SPECIES_THIEVUL,
    SPECIES_CHIMECHO,
    SPECIES_DUSCLOPS,
    SPECIES_BANETTE,
    SPECIES_SWELLOW,
    SPECIES_HITMONTOP,
    SPECIES_GIRAFARIG,
    SPECIES_JYNX,
    SPECIES_HITMONCHAN,
    SPECIES_HITMONLEE,
    SPECIES_PRIMEAPE,
    SPECIES_GOLBAT,
    SPECIES_NINJASK,
    SPECIES_PALAFIN_ZERO,
    SPECIES_SEVIPER,
    SPECIES_ZANGOOSE,
    SPECIES_ELDEGOSS,
    SPECIES_GREEDENT,
    SPECIES_BASCULIN,
    SPECIES_LUMINEON,
    SPECIES_TROPIUS,
    SPECIES_SOLROCK,
    SPECIES_LUNATONE,
    SPECIES_CAMERUPT,
    SPECIES_SHARPEDO,
    SPECIES_BRELOOM,
    SPECIES_JUMPLUFF,
    SPECIES_LANTURN,
    SPECIES_MR_MIME_GALAR,
    SPECIES_MR_MIME,
    SPECIES_MARACTUS,
    SPECIES_AROMATISSE,
    SPECIES_RIBOMBEE,
    SPECIES_AMOONGUSS,
    SPECIES_SAWK,
    SPECIES_THROH,
    SPECIES_ABSOL,
    SPECIES_STANTLER,
    SPECIES_SKARMORY,
    SPECIES_FORRETRESS,
    SPECIES_MAGNETON,
    SPECIES_MEOWSTIC,
    SPECIES_SWALOT,
    SPECIES_CRAWDAUNT,
    SPECIES_WHISCASH,
    SPECIES_RABSCA,
    SPECIES_MAUSHOLD,
    SPECIES_EISCUE_NOICE,
    SPECIES_EISCUE_ICE,
    SPECIES_STONJOURNER,
    SPECIES_FALINKS,
    SPECIES_KLEFKI,
    SPECIES_ALOMOMOLA,
    SPECIES_CINCCINO,
    SPECIES_GRUMPIG,
    SPECIES_TORKOAL,
    SPECIES_XATU,
    SPECIES_DODRIO,
    SPECIES_STUNFISK_GALAR,
    SPECIES_STUNFISK,
    SPECIES_FURFROU,
    SPECIES_GALVANTULA,
    SPECIES_SWANNA,
    SPECIES_TREVENANT,
    SPECIES_GARBODOR,
    SPECIES_VESPIQUEN,
    SPECIES_HARIYAMA,
    SPECIES_TATSUGIRI,
    SPECIES_INDEEDEE_M,
    SPECIES_INDEEDEE_F,
    SPECIES_FROSMOTH,
    SPECIES_CRAMORANT,
    SPECIES_BRUXISH,
    SPECIES_SAWSBUCK,
    SPECIES_GASTRODON,
    SPECIES_CACTURNE,
    SPECIES_MANECTRIC,
    SPECIES_KINGLER,
    SPECIES_DEWGONG,
    SPECIES_MIMIKYU,
    SPECIES_ORICORIO,
    SPECIES_DACHSBUN,
    SPECIES_VELUZA,
    SPECIES_CRABOMINABLE,
    SPECIES_SKUNTANK,
    SPECIES_PIDGEOT,
    SPECIES_ORTHWORM,
    SPECIES_BRAMBLEGHAST,
    SPECIES_GRAPPLOCT,
    SPECIES_KOMALA,
    SPECIES_PALOSSAND,
    SPECIES_SALAZZLE,
    SPECIES_LURANTIS,
    SPECIES_SLURPUFF,
    SPECIES_JELLICENT,
    SPECIES_DARMANITAN_STANDARD,
    SPECIES_DARMANITAN_GALAR_STANDARD,
    SPECIES_LILLIGANT_HISUI,
    SPECIES_LILLIGANT,
    SPECIES_WHIMSICOTT,
    SPECIES_PHIONE,
    SPECIES_FROSLASS,
    SPECIES_LOPUNNY,
    SPECIES_GLALIE,
    SPECIES_MAWILE_MEGA,
    SPECIES_SABLEYE_MEGA,
    SPECIES_SHIFTRY,
    SPECIES_LUDICOLO,
    SPECIES_OCTILLERY,
    SPECIES_ESPATHRA,
    SPECIES_HELIOLISK,
    SPECIES_MALAMAR,
    SPECIES_AMBIPOM,
    SPECIES_RUNERIGUS,
    SPECIES_GOLURK,
    SPECIES_COFAGRIGUS,
    SPECIES_HYPNO,
    SPECIES_CLEFABLE,
    SPECIES_DURANT,
    SPECIES_HEATMOR,
    SPECIES_DIPPLIN,
    SPECIES_BOMBIRDIER,
    SPECIES_GRAFAIAI,
    SPECIES_APPLETUN,
    SPECIES_FLAPPLE,
    SPECIES_DREDNAW,
    SPECIES_DRAMPA,
    SPECIES_TURTONATOR,
    SPECIES_COMFEY,
    SPECIES_TOUCANNON,
    SPECIES_DRUDDIGON,
    SPECIES_BEHEEYEM,
    SPECIES_CRUSTLE,
    SPECIES_SCOLIPEDE,
    SPECIES_SPIRITOMB,
    SPECIES_STARAPTOR,
    SPECIES_RELICANTH,
    SPECIES_GOREBYSS,
    SPECIES_HUNTAIL,
    SPECIES_MANTINE,
    SPECIES_RHYDON,
    SPECIES_RAICHU_ALOLA,
    SPECIES_RAICHU,
    SPECIES_SCOVILLAIN,
    SPECIES_ZYGARDE_10_POWER_CONSTRUCT,
    SPECIES_LYCANROC_MIDNIGHT,
    SPECIES_LYCANROC_MIDDAY,
    SPECIES_LYCANROC_DUSK,
    SPECIES_MUSHARNA,
    SPECIES_HOUNDSTONE,
    SPECIES_SCRAFTY,
    SPECIES_UNFEZANT,
    SPECIES_OINKOLOGNE_M,
    SPECIES_OINKOLOGNE_F,
    SPECIES_FERROTHORN,
    SPECIES_KILOWATTREL,
    SPECIES_PAWMOT,
    SPECIES_BARRASKEWDA,
    SPECIES_BOLTUND,
    SPECIES_DUBWOOL,
    SPECIES_PASSIMIAN,
    SPECIES_ORANGURU,
    SPECIES_BOUFFALANT,
    SPECIES_BISHARP,
    SPECIES_REUNICLUS,
    SPECIES_GOTHITELLE,
    SPECIES_SIGILYPH,
    SPECIES_TOXICROAK,
    SPECIES_ALTARIA,
    SPECIES_EXPLOUD,
    SPECIES_MILTANK,
    SPECIES_SLOWKING_GALAR,
    SPECIES_SLOWKING,
    SPECIES_BELLOSSOM,
    SPECIES_TAUROS_PALDEA_COMBAT,
    SPECIES_TAUROS_PALDEA_BLAZE,
    SPECIES_TAUROS_PALDEA_AQUA,
    SPECIES_TAUROS,
    SPECIES_ELECTABUZZ,
    SPECIES_KANGASKHAN,
    SPECIES_WEEZING_GALAR,
    SPECIES_WEEZING,
    SPECIES_ELECTRODE_HISUI,
    SPECIES_ELECTRODE,
    SPECIES_SLOWBRO_GALAR,
    SPECIES_SLOWBRO,
    SPECIES_VICTREEBEL,
    SPECIES_VILEPLUME,
    SPECIES_GOURGEIST_SUPER,
    SPECIES_GOURGEIST_SMALL,
    SPECIES_GOURGEIST_LARGE,
    SPECIES_GOURGEIST_AVERAGE,
    SPECIES_DRAGALGE,
    SPECIES_ABOMASNOW,
    SPECIES_BELLIBOLT,
    SPECIES_ALCREMIE,
    SPECIES_CORVIKNIGHT,
    SPECIES_TOXAPEX,
    SPECIES_PANGORO,
    SPECIES_ACCELGOR,
    SPECIES_ESCAVALIER,
    SPECIES_CARRACOSTA,
    SPECIES_MISMAGIUS,
    SPECIES_FLOATZEL,
    SPECIES_BASTIODON,
    SPECIES_RAMPARDOS,
    SPECIES_ARMALDO,
    SPECIES_CRADILY,
    SPECIES_KABUTOPS,
    SPECIES_OMASTAR,
    SPECIES_MAGMAR,
    SPECIES_GOLEM_ALOLA,
    SPECIES_GOLEM,
    SPECIES_BEEDRILL_MEGA,
    SPECIES_ZEBSTRIKA,
    SPECIES_SIMIPOUR,
    SPECIES_SIMISEAR,
    SPECIES_SIMISAGE,
    SPECIES_DRIFBLIM,
    SPECIES_TALONFLAME,
    SPECIES_FLAMIGO,
    SPECIES_REVAVROOM,
    SPECIES_GARGANACL,
    SPECIES_KLEAVOR,
    SPECIES_CALYREX,
    SPECIES_COPPERAJAH,
    SPECIES_MINIOR_CORE,
    SPECIES_BEWEAR,
    SPECIES_MUDSDALE,
    SPECIES_VIKAVOLT,
    SPECIES_CARBINK,
    SPECIES_HAWLUCHA,
    SPECIES_CLAWITZER,
    SPECIES_BARBARACLE,
    SPECIES_AEGISLASH_SHIELD,
    SPECIES_AEGISLASH_BLADE,
    SPECIES_LEAVANNY,
    SPECIES_STOUTLAND,
    SPECIES_DRAPION,
    SPECIES_BRONZONG,
    SPECIES_CLAYDOL,
    SPECIES_WAILORD,
    SPECIES_DONPHAN,
    SPECIES_HOUNDOOM,
    SPECIES_URSARING,
    SPECIES_HERACROSS,
    SPECIES_SCIZOR,
    SPECIES_POLITOED,
    SPECIES_PINSIR,
    SPECIES_SCYTHER,
    SPECIES_GENGAR,
    SPECIES_MUK_ALOLA,
    SPECIES_MUK,
    SPECIES_RAPIDASH_GALAR,
    SPECIES_RAPIDASH,
    SPECIES_ALAKAZAM,
    SPECIES_GOLDUCK,
    SPECIES_CYCLIZAR,
    SPECIES_TOXTRICITY,
    SPECIES_MABOSSTIFF,
    SPECIES_ARCTOVISH,
    SPECIES_DRACOVISH,
    SPECIES_ARCTOZOLT,
    SPECIES_DRACOZOLT,
    SPECIES_ORBEETLE,
    SPECIES_BEARTIC,
    SPECIES_CONKELDURR,
    SPECIES_HONCHKROW,
    SPECIES_SHUCKLE,
    SPECIES_MACHAMP,
    SPECIES_NINETALES_ALOLA,
    SPECIES_NINETALES,
    SPECIES_NIDOKING,
    SPECIES_NIDOQUEEN,
    SPECIES_TINKATON,
    SPECIES_SIRFETCHD,
    SPECIES_PYROAR,
    SPECIES_SINISTCHA,
    SPECIES_POLTEAGEIST,
    SPECIES_EXCADRILL,
    SPECIES_SEISMITOAD,
    SPECIES_ARBOLIVA,
    SPECIES_OVERQWIL,
    SPECIES_SNEASLER,
    SPECIES_CURSOLA,
    SPECIES_GRIMMSNARL,
    SPECIES_HATTERENE,
    SPECIES_SANDACONDA,
    SPECIES_COALOSSAL,
    SPECIES_TSAREENA,
    SPECIES_MANDIBUZZ,
    SPECIES_BRAVIARY_HISUI,
    SPECIES_BRAVIARY,
    SPECIES_MIENSHAO,
    SPECIES_ZOROARK_HISUI,
    SPECIES_ZOROARK,
    SPECIES_GLISCOR,
    SPECIES_WEAVILE,
    SPECIES_MEDICHAM_MEGA,
    SPECIES_STEELIX,
    SPECIES_AMPHAROS,
    SPECIES_POLIWRATH,
    SPECIES_AVALUGG_HISUI,
    SPECIES_AVALUGG,
    SPECIES_TOEDSCRUEL,
    SPECIES_CRYOGONAL,
    SPECIES_EELEKTROSS,
    SPECIES_GIGALITH,
    SPECIES_YANMEGA,
    SPECIES_LICKILICKY,
    SPECIES_ROSERADE,
    SPECIES_PORYGON2,
    SPECIES_AERODACTYL,
    SPECIES_TENTACRUEL,
    SPECIES_DHELMISE,
    SPECIES_GALLADE,
    SPECIES_GARDEVOIR,
    SPECIES_KROOKODILE,
    SPECIES_DUDUNSPARCE,
    SPECIES_FARIGIRAF,
    SPECIES_MR_RIME,
    SPECIES_OBSTAGOON,
    SPECIES_CHANDELURE,
    SPECIES_KLINKLANG,
    SPECIES_ROTOM_WASH,
    SPECIES_ROTOM_MOW,
    SPECIES_ROTOM_HEAT,
    SPECIES_ROTOM_FROST,
    SPECIES_ROTOM_FAN,
    SPECIES_FLYGON,
    SPECIES_STARMIE,
    SPECIES_CETITAN,
    SPECIES_AURORUS,
    SPECIES_TYRANTRUM,
    SPECIES_LUXRAY,
    SPECIES_GLIMMORA,
    SPECIES_CERULEDGE,
    SPECIES_ARMAROUGE,
    SPECIES_WYRDEER,
    SPECIES_CENTISKORCH,
    SPECIES_SYLVEON,
    SPECIES_DUSKNOIR,
    SPECIES_PROBOPASS,
    SPECIES_GLACEON,
    SPECIES_LEAFEON,
    SPECIES_HIPPOWDON,
    SPECIES_LUCARIO,
    SPECIES_TORTERRA,
    SPECIES_UMBREON,
    SPECIES_ESPEON,
    SPECIES_MEGANIUM,
    SPECIES_FLAREON,
    SPECIES_JOLTEON,
    SPECIES_VAPOREON,
    SPECIES_CLOYSTER,
    SPECIES_VENUSAUR,
    SPECIES_SAMUROTT_HISUI,
    SPECIES_SAMUROTT,
    SPECIES_EMBOAR,
    SPECIES_SERPERIOR,
    SPECIES_DONDOZO,
    SPECIES_QUAQUAVAL,
    SPECIES_SKELEDIRGE,
    SPECIES_MEOWSCARADA,
    SPECIES_BASCULEGION_M,
    SPECIES_BASCULEGION_F,
    SPECIES_INTELEON,
    SPECIES_CINDERACE,
    SPECIES_RILLABOOM,
    SPECIES_GOLISOPOD,
    SPECIES_PRIMARINA,
    SPECIES_INCINEROAR,
    SPECIES_DECIDUEYE_HISUI,
    SPECIES_DECIDUEYE,
    SPECIES_GRENINJA,
    SPECIES_CHESNAUGHT,
    SPECIES_MAMOSWINE,
    SPECIES_EMPOLEON,
    SPECIES_WALREIN,
    SPECIES_AGGRON,
    SPECIES_BLAZIKEN,
    SPECIES_SCEPTILE,
    SPECIES_FERALIGATR,
    SPECIES_EXEGGUTOR_ALOLA,
    SPECIES_EXEGGUTOR,
    SPECIES_BLASTOISE,
    SPECIES_GOGOAT,
    SPECIES_TYPE_NULL,
    SPECIES_DELPHOX,
    SPECIES_INFERNAPE,
    SPECIES_TYPHLOSION_HISUI,
    SPECIES_TYPHLOSION,
    SPECIES_CHARIZARD,
    SPECIES_ANNIHILAPE,
    SPECIES_DURALUDON,
    SPECIES_NOIVERN,
    SPECIES_VANILLUXE,
    SPECIES_PORYGON_Z,
    SPECIES_TANGROWTH,
    SPECIES_RHYPERIOR,
    SPECIES_MAGNEZONE,
    SPECIES_SWAMPERT,
    SPECIES_CROBAT,
    SPECIES_LAPRAS,
    SPECIES_HYDRAPPLE,
    SPECIES_NAGANADEL,
    SPECIES_HAXORUS,
    SPECIES_DARMANITAN_ZEN,
    SPECIES_DARMANITAN_GALAR_ZEN,
    SPECIES_MAGMORTAR,
    SPECIES_ELECTIVIRE,
    SPECIES_MILOTIC,
    SPECIES_BLISSEY,
    SPECIES_KINGDRA,
    SPECIES_SNORLAX,
    SPECIES_GYARADOS,
    SPECIES_AUDINO_MEGA,
    SPECIES_TOGEKISS,
    SPECIES_OGERPON,
    SPECIES_GHOLDENGO,
    SPECIES_KINGAMBIT,
    SPECIES_URSALUNA,
    SPECIES_URSHIFU,
    SPECIES_VOLCARONA,
    SPECIES_FLOETTE_ETERNAL,
    SPECIES_FLORGES,
    SPECIES_FEZANDIPITI,
    SPECIES_MUNKIDORI,
    SPECIES_OKIDOGI,
    SPECIES_URSALUNA_BLOODMOON,
    SPECIES_BANETTE_MEGA,
    SPECIES_ARCANINE_HISUI,
    SPECIES_ARCANINE,
    SPECIES_CAMERUPT_MEGA,
    SPECIES_SHARPEDO_MEGA,
    SPECIES_ABSOL_MEGA,
    SPECIES_SKARMORY_MEGA,
    SPECIES_ARCHEOPS,
    SPECIES_CHI_YU,
    SPECIES_TING_LU,
    SPECIES_CHIEN_PAO,
    SPECIES_WO_CHIEN,
    SPECIES_IRON_THORNS,
    SPECIES_IRON_MOTH,
    SPECIES_IRON_JUGULIS,
    SPECIES_IRON_HANDS,
    SPECIES_IRON_BUNDLE,
    SPECIES_IRON_TREADS,
    SPECIES_SANDY_SHOCKS,
    SPECIES_SLITHER_WING,
    SPECIES_FLUTTER_MANE,
    SPECIES_BRUTE_BONNET,
    SPECIES_SCREAM_TAIL,
    SPECIES_GREAT_TUSK,
    SPECIES_FALINKS_MEGA,
    SPECIES_BLACEPHALON,
    SPECIES_STAKATAKA,
    SPECIES_GUZZLORD,
    SPECIES_KARTANA,
    SPECIES_CELESTEELA,
    SPECIES_XURKITREE,
    SPECIES_PHEROMOSA,
    SPECIES_BUZZWOLE,
    SPECIES_NIHILEGO,
    SPECIES_TAPU_FINI,
    SPECIES_TAPU_BULU,
    SPECIES_TAPU_LELE,
    SPECIES_TAPU_KOKO,
    SPECIES_SILVALLY,
    SPECIES_MANECTRIC_MEGA,
    SPECIES_PIDGEOT_MEGA,
    SPECIES_ENAMORUS_THERIAN,
    SPECIES_ENAMORUS_INCARNATE,
    SPECIES_SPECTRIER,
    SPECIES_GLASTRIER,
    SPECIES_REGIDRAGO,
    SPECIES_REGIELEKI,
    SPECIES_KELDEO,
    SPECIES_THUNDURUS_THERIAN,
    SPECIES_THUNDURUS_INCARNATE,
    SPECIES_TORNADUS_THERIAN,
    SPECIES_TORNADUS_INCARNATE,
    SPECIES_VIRIZION,
    SPECIES_TERRAKION,
    SPECIES_COBALION,
    SPECIES_CRESSELIA,
    SPECIES_AZELF,
    SPECIES_MESPRIT,
    SPECIES_UXIE,
    SPECIES_FROSLASS_MEGA,
    SPECIES_LOPUNNY_MEGA,
    SPECIES_REGISTEEL,
    SPECIES_REGICE,
    SPECIES_REGIROCK,
    SPECIES_GLALIE_MEGA,
    SPECIES_SUICUNE,
    SPECIES_ENTEI,
    SPECIES_RAIKOU,
    SPECIES_MOLTRES_GALAR,
    SPECIES_MOLTRES,
    SPECIES_ZAPDOS_GALAR,
    SPECIES_ZAPDOS,
    SPECIES_ARTICUNO_GALAR,
    SPECIES_ARTICUNO,
    SPECIES_MALAMAR_MEGA,
    SPECIES_CLEFABLE_MEGA,
    SPECIES_DRAMPA_MEGA,
    SPECIES_SCOLIPEDE_MEGA,
    SPECIES_SCRAFTY_MEGA,
    SPECIES_IRON_CROWN,
    SPECIES_IRON_BOULDER,
    SPECIES_RAGING_BOLT,
    SPECIES_GOUGING_FIRE,
    SPECIES_IRON_LEAVES,
    SPECIES_WALKING_WAKE,
    SPECIES_IRON_VALIANT,
    SPECIES_ROARING_MOON,
    SPECIES_ALTARIA_MEGA,
    SPECIES_KANGASKHAN_MEGA,
    SPECIES_SLOWBRO_MEGA,
    SPECIES_VICTREEBEL_MEGA,
    SPECIES_DRAGALGE_MEGA,
    SPECIES_ABOMASNOW_MEGA,
    SPECIES_PECHARUNT,
    SPECIES_TERAPAGOS_TERASTAL,
    SPECIES_ARCHALUDON,
    SPECIES_BAXCALIBUR,
    SPECIES_ZARUDE,
    SPECIES_DRAGAPULT,
    SPECIES_MELMETAL,
    SPECIES_ZERAORA,
    SPECIES_MARSHADOW,
    SPECIES_MAGEARNA,
    SPECIES_NECROZMA,
    SPECIES_KOMMO_O,
    SPECIES_VOLCANION,
    SPECIES_HOOPA_CONFINED,
    SPECIES_DIANCIE,
    SPECIES_ZYGARDE_50_POWER_CONSTRUCT,
    SPECIES_GOODRA_HISUI,
    SPECIES_GOODRA,
    SPECIES_HAWLUCHA_MEGA,
    SPECIES_BARBARACLE_MEGA,
    SPECIES_GENESECT,
    SPECIES_MELOETTA_PIROUETTE,
    SPECIES_MELOETTA_ARIA,
    SPECIES_LANDORUS_THERIAN,
    SPECIES_LANDORUS_INCARNATE,
    SPECIES_HYDREIGON,
    SPECIES_VICTINI,
    SPECIES_SHAYMIN_SKY,
    SPECIES_SHAYMIN_LAND,
    SPECIES_DARKRAI,
    SPECIES_MANAPHY,
    SPECIES_HEATRAN,
    SPECIES_GARCHOMP,
    SPECIES_DEOXYS_SPEED,
    SPECIES_DEOXYS_NORMAL,
    SPECIES_DEOXYS_DEFENSE,
    SPECIES_DEOXYS_ATTACK,
    SPECIES_JIRACHI,
    SPECIES_LATIOS,
    SPECIES_LATIAS,
    SPECIES_METAGROSS,
    SPECIES_SALAMENCE,
    SPECIES_CELEBI,
    SPECIES_TYRANITAR,
    SPECIES_HOUNDOOM_MEGA,
    SPECIES_HERACROSS_MEGA,
    SPECIES_SCIZOR_MEGA,
    SPECIES_MEW,
    SPECIES_DRAGONITE,
    SPECIES_PINSIR_MEGA,
    SPECIES_GENGAR_MEGA,
    SPECIES_ALAKAZAM_MEGA,
    SPECIES_PYROAR_MEGA,
    SPECIES_EXCADRILL_MEGA,
    SPECIES_STEELIX_MEGA,
    SPECIES_AMPHAROS_MEGA,
    SPECIES_EELEKTROSS_MEGA,
    SPECIES_AERODACTYL_MEGA,
    SPECIES_GALLADE_MEGA,
    SPECIES_GARDEVOIR_MEGA,
    SPECIES_WISHIWASHI_SCHOOL,
    SPECIES_CHANDELURE_MEGA,
    SPECIES_LUCARIO_MEGA,
    SPECIES_MEGANIUM_MEGA,
    SPECIES_VENUSAUR_MEGA,
    SPECIES_EMBOAR_MEGA,
    SPECIES_GRENINJA_MEGA,
    SPECIES_CHESNAUGHT_MEGA,
    SPECIES_AGGRON_MEGA,
    SPECIES_BLAZIKEN_MEGA,
    SPECIES_SCEPTILE_MEGA,
    SPECIES_FERALIGATR_MEGA,
    SPECIES_BLASTOISE_MEGA,
    SPECIES_DELPHOX_MEGA,
    SPECIES_CHARIZARD_MEGA_Y,
    SPECIES_CHARIZARD_MEGA_X,
    SPECIES_SWAMPERT_MEGA,
    SPECIES_GRENINJA_ASH,
    SPECIES_GYARADOS_MEGA,
    SPECIES_PALAFIN_HERO,
    SPECIES_FLOETTE_MEGA,
    SPECIES_ZAMAZENTA_HERO,
    SPECIES_ZACIAN_HERO,
    SPECIES_KYUREM,
    SPECIES_STARMIE_MEGA,
    SPECIES_MIRAIDON,
    SPECIES_KORAIDON,
    SPECIES_REGIGIGAS,
    SPECIES_GROUDON,
    SPECIES_KYOGRE,
    SPECIES_SLAKING,
    SPECIES_CALYREX_SHADOW,
    SPECIES_CALYREX_ICE,
    SPECIES_NECROZMA_DUSK_MANE,
    SPECIES_NECROZMA_DAWN_WINGS,
    SPECIES_LUNALA,
    SPECIES_SOLGALEO,
    SPECIES_HOOPA_UNBOUND,
    SPECIES_YVELTAL,
    SPECIES_XERNEAS,
    SPECIES_ZEKROM,
    SPECIES_RESHIRAM,
    SPECIES_GIRATINA_ORIGIN,
    SPECIES_GIRATINA_ALTERED,
    SPECIES_PALKIA_ORIGIN,
    SPECIES_PALKIA,
    SPECIES_DIALGA_ORIGIN,
    SPECIES_DIALGA,
    SPECIES_RAYQUAZA,
    SPECIES_HO_OH,
    SPECIES_LUGIA,
    SPECIES_MEWTWO,
    SPECIES_ETERNATUS,
    SPECIES_TERAPAGOS_STELLAR,
    SPECIES_ZAMAZENTA_CROWNED,
    SPECIES_ZACIAN_CROWNED,
    SPECIES_DIANCIE_MEGA,
    SPECIES_KYUREM_WHITE,
    SPECIES_KYUREM_BLACK,
    SPECIES_GARCHOMP_MEGA,
    SPECIES_LATIOS_MEGA,
    SPECIES_LATIAS_MEGA,
    SPECIES_METAGROSS_MEGA,
    SPECIES_SALAMENCE_MEGA,
    SPECIES_TYRANITAR_MEGA,
    SPECIES_DRAGONITE_MEGA,
    SPECIES_ZYGARDE_COMPLETE,
    SPECIES_ARCEUS,
    SPECIES_NECROZMA_ULTRA,
    SPECIES_GROUDON_PRIMAL,
    SPECIES_KYOGRE_PRIMAL,
    SPECIES_ZYGARDE_MEGA,
    SPECIES_RAYQUAZA_MEGA,
    SPECIES_MEWTWO_MEGA_Y,
    SPECIES_MEWTWO_MEGA_X,
    // SPECIES_ETERNATUS_ETERNAMAX,
};

EWRAM_DATA static u16 sRandomSpeciesAllMap[RANDOM_SPECIES_ALL_COUNT] = {0};
EWRAM_DATA static bool8 bRandomSpeciesAllMapIsSetup = FALSE;
static void SetUpSpeciesAllLookup() {
    if (bRandomSpeciesAllMapIsSetup != TRUE) {
        for (u16 i = 0; i < RANDOM_SPECIES_ALL_COUNT; ++i)
        {
            sRandomSpeciesAllMap[sRandomSpeciesAllBst[i]] = i;
        }
        bRandomSpeciesAllMapIsSetup = TRUE;
    }
}

static const u16 sSpeciesAllBst[NUM_SPECIES] =
{
    // [SPECIES_ETERNATUS_ETERNAMAX] = 1125,
    [SPECIES_MEWTWO_MEGA_X] = 780,
    [SPECIES_MEWTWO_MEGA_Y] = 780,
    [SPECIES_RAYQUAZA_MEGA] = 780,
    [SPECIES_ZYGARDE_MEGA] = 778,
    [SPECIES_KYOGRE_PRIMAL] = 770,
    [SPECIES_GROUDON_PRIMAL] = 770,
    [SPECIES_NECROZMA_ULTRA] = 754,
    [SPECIES_ARCEUS] = 720,
    [SPECIES_ZYGARDE_COMPLETE] = 708,
    [SPECIES_DRAGONITE_MEGA] = 700,
    [SPECIES_TYRANITAR_MEGA] = 700,
    [SPECIES_SALAMENCE_MEGA] = 700,
    [SPECIES_METAGROSS_MEGA] = 700,
    [SPECIES_LATIAS_MEGA] = 700,
    [SPECIES_LATIOS_MEGA] = 700,
    [SPECIES_GARCHOMP_MEGA] = 700,
    [SPECIES_KYUREM_BLACK] = 700,
    [SPECIES_KYUREM_WHITE] = 700,
    [SPECIES_DIANCIE_MEGA] = 700,
    [SPECIES_ZACIAN_CROWNED] = 700,
    [SPECIES_ZAMAZENTA_CROWNED] = 700,
    [SPECIES_TERAPAGOS_STELLAR] = 700,
    [SPECIES_ETERNATUS] = 690,
    [SPECIES_MEWTWO] = 680,
    [SPECIES_LUGIA] = 680,
    [SPECIES_HO_OH] = 680,
    [SPECIES_RAYQUAZA] = 680,
    [SPECIES_DIALGA] = 680,
    [SPECIES_DIALGA_ORIGIN] = 680,
    [SPECIES_PALKIA] = 680,
    [SPECIES_PALKIA_ORIGIN] = 680,
    [SPECIES_GIRATINA_ALTERED] = 680,
    [SPECIES_GIRATINA_ORIGIN] = 680,
    [SPECIES_RESHIRAM] = 680,
    [SPECIES_ZEKROM] = 680,
    [SPECIES_XERNEAS] = 680,
    [SPECIES_YVELTAL] = 680,
    [SPECIES_HOOPA_UNBOUND] = 680,
    [SPECIES_SOLGALEO] = 680,
    [SPECIES_LUNALA] = 680,
    [SPECIES_NECROZMA_DAWN_WINGS] = 680,
    [SPECIES_NECROZMA_DUSK_MANE] = 680,
    [SPECIES_CALYREX_ICE] = 680,
    [SPECIES_CALYREX_SHADOW] = 680,
    [SPECIES_SLAKING] = 670,
    [SPECIES_KYOGRE] = 670,
    [SPECIES_GROUDON] = 670,
    [SPECIES_REGIGIGAS] = 670,
    [SPECIES_KORAIDON] = 670,
    [SPECIES_MIRAIDON] = 670,
    [SPECIES_STARMIE_MEGA] = 660,
    [SPECIES_KYUREM] = 660,
    [SPECIES_ZACIAN_HERO] = 660,
    [SPECIES_ZAMAZENTA_HERO] = 660,
    [SPECIES_FLOETTE_MEGA] = 651,
    [SPECIES_PALAFIN_HERO] = 650,
    [SPECIES_GYARADOS_MEGA] = 640,
    [SPECIES_GRENINJA_ASH] = 640,
    [SPECIES_SWAMPERT_MEGA] = 635,
    [SPECIES_CHARIZARD_MEGA_X] = 634,
    [SPECIES_CHARIZARD_MEGA_Y] = 634,
    [SPECIES_DELPHOX_MEGA] = 634,
    [SPECIES_BLASTOISE_MEGA] = 630,
    [SPECIES_FERALIGATR_MEGA] = 630,
    [SPECIES_SCEPTILE_MEGA] = 630,
    [SPECIES_BLAZIKEN_MEGA] = 630,
    [SPECIES_AGGRON_MEGA] = 630,
    [SPECIES_CHESNAUGHT_MEGA] = 630,
    [SPECIES_GRENINJA_MEGA] = 630,
    [SPECIES_EMBOAR_MEGA] = 628,
    [SPECIES_VENUSAUR_MEGA] = 625,
    [SPECIES_MEGANIUM_MEGA] = 625,
    [SPECIES_LUCARIO_MEGA] = 625,
    [SPECIES_CHANDELURE_MEGA] = 620,
    [SPECIES_WISHIWASHI_SCHOOL] = 620,
    [SPECIES_GARDEVOIR_MEGA] = 618,
    [SPECIES_GALLADE_MEGA] = 618,
    [SPECIES_AERODACTYL_MEGA] = 615,
    [SPECIES_EELEKTROSS_MEGA] = 615,
    [SPECIES_AMPHAROS_MEGA] = 610,
    [SPECIES_STEELIX_MEGA] = 610,
    [SPECIES_EXCADRILL_MEGA] = 608,
    [SPECIES_PYROAR_MEGA] = 607,
    [SPECIES_ALAKAZAM_MEGA] = 600,
    [SPECIES_GENGAR_MEGA] = 600,
    [SPECIES_PINSIR_MEGA] = 600,
    [SPECIES_DRAGONITE] = 600,
    [SPECIES_MEW] = 600,
    [SPECIES_SCIZOR_MEGA] = 600,
    [SPECIES_HERACROSS_MEGA] = 600,
    [SPECIES_HOUNDOOM_MEGA] = 600,
    [SPECIES_TYRANITAR] = 600,
    [SPECIES_CELEBI] = 600,
    [SPECIES_SALAMENCE] = 600,
    [SPECIES_METAGROSS] = 600,
    [SPECIES_LATIAS] = 600,
    [SPECIES_LATIOS] = 600,
    [SPECIES_JIRACHI] = 600,
    [SPECIES_DEOXYS_ATTACK] = 600,
    [SPECIES_DEOXYS_DEFENSE] = 600,
    [SPECIES_DEOXYS_NORMAL] = 600,
    [SPECIES_DEOXYS_SPEED] = 600,
    [SPECIES_GARCHOMP] = 600,
    [SPECIES_HEATRAN] = 600,
    [SPECIES_MANAPHY] = 600,
    [SPECIES_DARKRAI] = 600,
    [SPECIES_SHAYMIN_LAND] = 600,
    [SPECIES_SHAYMIN_SKY] = 600,
    [SPECIES_VICTINI] = 600,
    [SPECIES_HYDREIGON] = 600,
    [SPECIES_LANDORUS_INCARNATE] = 600,
    [SPECIES_LANDORUS_THERIAN] = 600,
    [SPECIES_MELOETTA_ARIA] = 600,
    [SPECIES_MELOETTA_PIROUETTE] = 600,
    [SPECIES_GENESECT] = 600,
    [SPECIES_BARBARACLE_MEGA] = 600,
    [SPECIES_HAWLUCHA_MEGA] = 600,
    [SPECIES_GOODRA] = 600,
    [SPECIES_GOODRA_HISUI] = 600,
    [SPECIES_ZYGARDE_50_POWER_CONSTRUCT] = 600,
    [SPECIES_DIANCIE] = 600,
    [SPECIES_HOOPA_CONFINED] = 600,
    [SPECIES_VOLCANION] = 600,
    [SPECIES_KOMMO_O] = 600,
    [SPECIES_NECROZMA] = 600,
    [SPECIES_MAGEARNA] = 600,
    [SPECIES_MARSHADOW] = 600,
    [SPECIES_ZERAORA] = 600,
    [SPECIES_MELMETAL] = 600,
    [SPECIES_DRAGAPULT] = 600,
    [SPECIES_ZARUDE] = 600,
    [SPECIES_BAXCALIBUR] = 600,
    [SPECIES_ARCHALUDON] = 600,
    [SPECIES_TERAPAGOS_TERASTAL] = 600,
    [SPECIES_PECHARUNT] = 600,
    [SPECIES_ABOMASNOW_MEGA] = 594,
    [SPECIES_DRAGALGE_MEGA] = 594,
    [SPECIES_VICTREEBEL_MEGA] = 590,
    [SPECIES_SLOWBRO_MEGA] = 590,
    [SPECIES_KANGASKHAN_MEGA] = 590,
    [SPECIES_ALTARIA_MEGA] = 590,
    [SPECIES_ROARING_MOON] = 590,
    [SPECIES_IRON_VALIANT] = 590,
    [SPECIES_WALKING_WAKE] = 590,
    [SPECIES_IRON_LEAVES] = 590,
    [SPECIES_GOUGING_FIRE] = 590,
    [SPECIES_RAGING_BOLT] = 590,
    [SPECIES_IRON_BOULDER] = 590,
    [SPECIES_IRON_CROWN] = 590,
    [SPECIES_SCRAFTY_MEGA] = 588,
    [SPECIES_SCOLIPEDE_MEGA] = 585,
    [SPECIES_DRAMPA_MEGA] = 585,
    [SPECIES_CLEFABLE_MEGA] = 583,
    [SPECIES_MALAMAR_MEGA] = 582,
    [SPECIES_ARTICUNO] = 580,
    [SPECIES_ARTICUNO_GALAR] = 580,
    [SPECIES_ZAPDOS] = 580,
    [SPECIES_ZAPDOS_GALAR] = 580,
    [SPECIES_MOLTRES] = 580,
    [SPECIES_MOLTRES_GALAR] = 580,
    [SPECIES_RAIKOU] = 580,
    [SPECIES_ENTEI] = 580,
    [SPECIES_SUICUNE] = 580,
    [SPECIES_GLALIE_MEGA] = 580,
    [SPECIES_REGIROCK] = 580,
    [SPECIES_REGICE] = 580,
    [SPECIES_REGISTEEL] = 580,
    [SPECIES_LOPUNNY_MEGA] = 580,
    [SPECIES_FROSLASS_MEGA] = 580,
    [SPECIES_UXIE] = 580,
    [SPECIES_MESPRIT] = 580,
    [SPECIES_AZELF] = 580,
    [SPECIES_CRESSELIA] = 580,
    [SPECIES_COBALION] = 580,
    [SPECIES_TERRAKION] = 580,
    [SPECIES_VIRIZION] = 580,
    [SPECIES_TORNADUS_INCARNATE] = 580,
    [SPECIES_TORNADUS_THERIAN] = 580,
    [SPECIES_THUNDURUS_INCARNATE] = 580,
    [SPECIES_THUNDURUS_THERIAN] = 580,
    [SPECIES_KELDEO] = 580,
    [SPECIES_REGIELEKI] = 580,
    [SPECIES_REGIDRAGO] = 580,
    [SPECIES_GLASTRIER] = 580,
    [SPECIES_SPECTRIER] = 580,
    [SPECIES_ENAMORUS_INCARNATE] = 580,
    [SPECIES_ENAMORUS_THERIAN] = 580,
    [SPECIES_PIDGEOT_MEGA] = 579,
    [SPECIES_MANECTRIC_MEGA] = 575,
    [SPECIES_SILVALLY] = 570,
    [SPECIES_TAPU_KOKO] = 570,
    [SPECIES_TAPU_LELE] = 570,
    [SPECIES_TAPU_BULU] = 570,
    [SPECIES_TAPU_FINI] = 570,
    [SPECIES_NIHILEGO] = 570,
    [SPECIES_BUZZWOLE] = 570,
    [SPECIES_PHEROMOSA] = 570,
    [SPECIES_XURKITREE] = 570,
    [SPECIES_CELESTEELA] = 570,
    [SPECIES_KARTANA] = 570,
    [SPECIES_GUZZLORD] = 570,
    [SPECIES_STAKATAKA] = 570,
    [SPECIES_BLACEPHALON] = 570,
    [SPECIES_FALINKS_MEGA] = 570,
    [SPECIES_GREAT_TUSK] = 570,
    [SPECIES_SCREAM_TAIL] = 570,
    [SPECIES_BRUTE_BONNET] = 570,
    [SPECIES_FLUTTER_MANE] = 570,
    [SPECIES_SLITHER_WING] = 570,
    [SPECIES_SANDY_SHOCKS] = 570,
    [SPECIES_IRON_TREADS] = 570,
    [SPECIES_IRON_BUNDLE] = 570,
    [SPECIES_IRON_HANDS] = 570,
    [SPECIES_IRON_JUGULIS] = 570,
    [SPECIES_IRON_MOTH] = 570,
    [SPECIES_IRON_THORNS] = 570,
    [SPECIES_WO_CHIEN] = 570,
    [SPECIES_CHIEN_PAO] = 570,
    [SPECIES_TING_LU] = 570,
    [SPECIES_CHI_YU] = 570,
    [SPECIES_ARCHEOPS] = 567,
    [SPECIES_SKARMORY_MEGA] = 565,
    [SPECIES_ABSOL_MEGA] = 565,
    [SPECIES_SHARPEDO_MEGA] = 560,
    [SPECIES_CAMERUPT_MEGA] = 560,
    [SPECIES_ARCANINE] = 555,
    [SPECIES_ARCANINE_HISUI] = 555,
    [SPECIES_BANETTE_MEGA] = 555,
    [SPECIES_URSALUNA_BLOODMOON] = 555,
    [SPECIES_OKIDOGI] = 555,
    [SPECIES_MUNKIDORI] = 555,
    [SPECIES_FEZANDIPITI] = 555,
    [SPECIES_FLORGES] = 552,
    [SPECIES_FLOETTE_ETERNAL] = 551,
    [SPECIES_VOLCARONA] = 550,
    [SPECIES_URSHIFU] = 550,
    [SPECIES_URSALUNA] = 550,
    [SPECIES_KINGAMBIT] = 550,
    [SPECIES_GHOLDENGO] = 550,
    [SPECIES_OGERPON] = 550,
    [SPECIES_TOGEKISS] = 545,
    [SPECIES_AUDINO_MEGA] = 545,
    [SPECIES_GYARADOS] = 540,
    [SPECIES_SNORLAX] = 540,
    [SPECIES_KINGDRA] = 540,
    [SPECIES_BLISSEY] = 540,
    [SPECIES_MILOTIC] = 540,
    [SPECIES_ELECTIVIRE] = 540,
    [SPECIES_MAGMORTAR] = 540,
    [SPECIES_DARMANITAN_GALAR_ZEN] = 540,
    [SPECIES_DARMANITAN_ZEN] = 540,
    [SPECIES_HAXORUS] = 540,
    [SPECIES_NAGANADEL] = 540,
    [SPECIES_HYDRAPPLE] = 540,
    [SPECIES_LAPRAS] = 535,
    [SPECIES_CROBAT] = 535,
    [SPECIES_SWAMPERT] = 535,
    [SPECIES_MAGNEZONE] = 535,
    [SPECIES_RHYPERIOR] = 535,
    [SPECIES_TANGROWTH] = 535,
    [SPECIES_PORYGON_Z] = 535,
    [SPECIES_VANILLUXE] = 535,
    [SPECIES_NOIVERN] = 535,
    [SPECIES_DURALUDON] = 535,
    [SPECIES_ANNIHILAPE] = 535,
    [SPECIES_CHARIZARD] = 534,
    [SPECIES_TYPHLOSION] = 534,
    [SPECIES_TYPHLOSION_HISUI] = 534,
    [SPECIES_INFERNAPE] = 534,
    [SPECIES_DELPHOX] = 534,
    [SPECIES_TYPE_NULL] = 534,
    [SPECIES_GOGOAT] = 531,
    [SPECIES_BLASTOISE] = 530,
    [SPECIES_EXEGGUTOR] = 530,
    [SPECIES_EXEGGUTOR_ALOLA] = 530,
    [SPECIES_FERALIGATR] = 530,
    [SPECIES_SCEPTILE] = 530,
    [SPECIES_BLAZIKEN] = 530,
    [SPECIES_AGGRON] = 530,
    [SPECIES_WALREIN] = 530,
    [SPECIES_EMPOLEON] = 530,
    [SPECIES_MAMOSWINE] = 530,
    [SPECIES_CHESNAUGHT] = 530,
    [SPECIES_GRENINJA] = 530,
    [SPECIES_DECIDUEYE] = 530,
    [SPECIES_DECIDUEYE_HISUI] = 530,
    [SPECIES_INCINEROAR] = 530,
    [SPECIES_PRIMARINA] = 530,
    [SPECIES_GOLISOPOD] = 530,
    [SPECIES_RILLABOOM] = 530,
    [SPECIES_CINDERACE] = 530,
    [SPECIES_INTELEON] = 530,
    [SPECIES_BASCULEGION_F] = 530,
    [SPECIES_BASCULEGION_M] = 530,
    [SPECIES_MEOWSCARADA] = 530,
    [SPECIES_SKELEDIRGE] = 530,
    [SPECIES_QUAQUAVAL] = 530,
    [SPECIES_DONDOZO] = 530,
    [SPECIES_SERPERIOR] = 528,
    [SPECIES_EMBOAR] = 528,
    [SPECIES_SAMUROTT] = 528,
    [SPECIES_SAMUROTT_HISUI] = 528,
    [SPECIES_VENUSAUR] = 525,
    [SPECIES_CLOYSTER] = 525,
    [SPECIES_VAPOREON] = 525,
    [SPECIES_JOLTEON] = 525,
    [SPECIES_FLAREON] = 525,
    [SPECIES_MEGANIUM] = 525,
    [SPECIES_ESPEON] = 525,
    [SPECIES_UMBREON] = 525,
    [SPECIES_TORTERRA] = 525,
    [SPECIES_LUCARIO] = 525,
    [SPECIES_HIPPOWDON] = 525,
    [SPECIES_LEAFEON] = 525,
    [SPECIES_GLACEON] = 525,
    [SPECIES_PROBOPASS] = 525,
    [SPECIES_DUSKNOIR] = 525,
    [SPECIES_SYLVEON] = 525,
    [SPECIES_CENTISKORCH] = 525,
    [SPECIES_WYRDEER] = 525,
    [SPECIES_ARMAROUGE] = 525,
    [SPECIES_CERULEDGE] = 525,
    [SPECIES_GLIMMORA] = 525,
    [SPECIES_LUXRAY] = 523,
    [SPECIES_TYRANTRUM] = 521,
    [SPECIES_AURORUS] = 521,
    [SPECIES_CETITAN] = 521,
    [SPECIES_STARMIE] = 520,
    [SPECIES_FLYGON] = 520,
    [SPECIES_ROTOM_FAN] = 520,
    [SPECIES_ROTOM_FROST] = 520,
    [SPECIES_ROTOM_HEAT] = 520,
    [SPECIES_ROTOM_MOW] = 520,
    [SPECIES_ROTOM_WASH] = 520,
    [SPECIES_KLINKLANG] = 520,
    [SPECIES_CHANDELURE] = 520,
    [SPECIES_OBSTAGOON] = 520,
    [SPECIES_MR_RIME] = 520,
    [SPECIES_FARIGIRAF] = 520,
    [SPECIES_DUDUNSPARCE] = 520,
    [SPECIES_KROOKODILE] = 519,
    [SPECIES_GARDEVOIR] = 518,
    [SPECIES_GALLADE] = 518,
    [SPECIES_DHELMISE] = 517,
    [SPECIES_TENTACRUEL] = 515,
    [SPECIES_AERODACTYL] = 515,
    [SPECIES_PORYGON2] = 515,
    [SPECIES_ROSERADE] = 515,
    [SPECIES_LICKILICKY] = 515,
    [SPECIES_YANMEGA] = 515,
    [SPECIES_GIGALITH] = 515,
    [SPECIES_EELEKTROSS] = 515,
    [SPECIES_CRYOGONAL] = 515,
    [SPECIES_TOEDSCRUEL] = 515,
    [SPECIES_AVALUGG] = 514,
    [SPECIES_AVALUGG_HISUI] = 514,
    [SPECIES_POLIWRATH] = 510,
    [SPECIES_AMPHAROS] = 510,
    [SPECIES_STEELIX] = 510,
    [SPECIES_MEDICHAM_MEGA] = 510,
    [SPECIES_WEAVILE] = 510,
    [SPECIES_GLISCOR] = 510,
    [SPECIES_ZOROARK] = 510,
    [SPECIES_ZOROARK_HISUI] = 510,
    [SPECIES_MIENSHAO] = 510,
    [SPECIES_BRAVIARY] = 510,
    [SPECIES_BRAVIARY_HISUI] = 510,
    [SPECIES_MANDIBUZZ] = 510,
    [SPECIES_TSAREENA] = 510,
    [SPECIES_COALOSSAL] = 510,
    [SPECIES_SANDACONDA] = 510,
    [SPECIES_HATTERENE] = 510,
    [SPECIES_GRIMMSNARL] = 510,
    [SPECIES_CURSOLA] = 510,
    [SPECIES_SNEASLER] = 510,
    [SPECIES_OVERQWIL] = 510,
    [SPECIES_ARBOLIVA] = 510,
    [SPECIES_SEISMITOAD] = 509,
    [SPECIES_EXCADRILL] = 508,
    [SPECIES_POLTEAGEIST] = 508,
    [SPECIES_SINISTCHA] = 508,
    [SPECIES_PYROAR] = 507,
    [SPECIES_SIRFETCHD] = 507,
    [SPECIES_TINKATON] = 506,
    [SPECIES_NIDOQUEEN] = 505,
    [SPECIES_NIDOKING] = 505,
    [SPECIES_NINETALES] = 505,
    [SPECIES_NINETALES_ALOLA] = 505,
    [SPECIES_MACHAMP] = 505,
    [SPECIES_SHUCKLE] = 505,
    [SPECIES_HONCHKROW] = 505,
    [SPECIES_CONKELDURR] = 505,
    [SPECIES_BEARTIC] = 505,
    [SPECIES_ORBEETLE] = 505,
    [SPECIES_DRACOZOLT] = 505,
    [SPECIES_ARCTOZOLT] = 505,
    [SPECIES_DRACOVISH] = 505,
    [SPECIES_ARCTOVISH] = 505,
    [SPECIES_MABOSSTIFF] = 505,
    [SPECIES_TOXTRICITY] = 502,
    [SPECIES_CYCLIZAR] = 501,
    [SPECIES_GOLDUCK] = 500,
    [SPECIES_ALAKAZAM] = 500,
    [SPECIES_RAPIDASH] = 500,
    [SPECIES_RAPIDASH_GALAR] = 500,
    [SPECIES_MUK] = 500,
    [SPECIES_MUK_ALOLA] = 500,
    [SPECIES_GENGAR] = 500,
    [SPECIES_SCYTHER] = 500,
    [SPECIES_PINSIR] = 500,
    [SPECIES_POLITOED] = 500,
    [SPECIES_SCIZOR] = 500,
    [SPECIES_HERACROSS] = 500,
    [SPECIES_URSARING] = 500,
    [SPECIES_HOUNDOOM] = 500,
    [SPECIES_DONPHAN] = 500,
    [SPECIES_WAILORD] = 500,
    [SPECIES_CLAYDOL] = 500,
    [SPECIES_BRONZONG] = 500,
    [SPECIES_DRAPION] = 500,
    [SPECIES_STOUTLAND] = 500,
    [SPECIES_LEAVANNY] = 500,
    [SPECIES_AEGISLASH_BLADE] = 500,
    [SPECIES_AEGISLASH_SHIELD] = 500,
    [SPECIES_BARBARACLE] = 500,
    [SPECIES_CLAWITZER] = 500,
    [SPECIES_HAWLUCHA] = 500,
    [SPECIES_CARBINK] = 500,
    [SPECIES_VIKAVOLT] = 500,
    [SPECIES_MUDSDALE] = 500,
    [SPECIES_BEWEAR] = 500,
    [SPECIES_MINIOR_CORE] = 500,
    [SPECIES_COPPERAJAH] = 500,
    [SPECIES_CALYREX] = 500,
    [SPECIES_KLEAVOR] = 500,
    [SPECIES_GARGANACL] = 500,
    [SPECIES_REVAVROOM] = 500,
    [SPECIES_FLAMIGO] = 500,
    [SPECIES_TALONFLAME] = 499,
    [SPECIES_DRIFBLIM] = 498,
    [SPECIES_SIMISAGE] = 498,
    [SPECIES_SIMISEAR] = 498,
    [SPECIES_SIMIPOUR] = 498,
    [SPECIES_ZEBSTRIKA] = 497,
    [SPECIES_BEEDRILL_MEGA] = 495,
    [SPECIES_GOLEM] = 495,
    [SPECIES_GOLEM_ALOLA] = 495,
    [SPECIES_MAGMAR] = 495,
    [SPECIES_OMASTAR] = 495,
    [SPECIES_KABUTOPS] = 495,
    [SPECIES_CRADILY] = 495,
    [SPECIES_ARMALDO] = 495,
    [SPECIES_RAMPARDOS] = 495,
    [SPECIES_BASTIODON] = 495,
    [SPECIES_FLOATZEL] = 495,
    [SPECIES_MISMAGIUS] = 495,
    [SPECIES_CARRACOSTA] = 495,
    [SPECIES_ESCAVALIER] = 495,
    [SPECIES_ACCELGOR] = 495,
    [SPECIES_PANGORO] = 495,
    [SPECIES_TOXAPEX] = 495,
    [SPECIES_CORVIKNIGHT] = 495,
    [SPECIES_ALCREMIE] = 495,
    [SPECIES_BELLIBOLT] = 495,
    [SPECIES_ABOMASNOW] = 494,
    [SPECIES_DRAGALGE] = 494,
    [SPECIES_GOURGEIST_AVERAGE] = 494,
    [SPECIES_GOURGEIST_LARGE] = 494,
    [SPECIES_GOURGEIST_SMALL] = 494,
    [SPECIES_GOURGEIST_SUPER] = 494,
    [SPECIES_VILEPLUME] = 490,
    [SPECIES_VICTREEBEL] = 490,
    [SPECIES_SLOWBRO] = 490,
    [SPECIES_SLOWBRO_GALAR] = 490,
    [SPECIES_ELECTRODE] = 490,
    [SPECIES_ELECTRODE_HISUI] = 490,
    [SPECIES_WEEZING] = 490,
    [SPECIES_WEEZING_GALAR] = 490,
    [SPECIES_KANGASKHAN] = 490,
    [SPECIES_ELECTABUZZ] = 490,
    [SPECIES_TAUROS] = 490,
    [SPECIES_TAUROS_PALDEA_AQUA] = 490,
    [SPECIES_TAUROS_PALDEA_BLAZE] = 490,
    [SPECIES_TAUROS_PALDEA_COMBAT] = 490,
    [SPECIES_BELLOSSOM] = 490,
    [SPECIES_SLOWKING] = 490,
    [SPECIES_SLOWKING_GALAR] = 490,
    [SPECIES_MILTANK] = 490,
    [SPECIES_EXPLOUD] = 490,
    [SPECIES_ALTARIA] = 490,
    [SPECIES_TOXICROAK] = 490,
    [SPECIES_SIGILYPH] = 490,
    [SPECIES_GOTHITELLE] = 490,
    [SPECIES_REUNICLUS] = 490,
    [SPECIES_BISHARP] = 490,
    [SPECIES_BOUFFALANT] = 490,
    [SPECIES_ORANGURU] = 490,
    [SPECIES_PASSIMIAN] = 490,
    [SPECIES_DUBWOOL] = 490,
    [SPECIES_BOLTUND] = 490,
    [SPECIES_BARRASKEWDA] = 490,
    [SPECIES_PAWMOT] = 490,
    [SPECIES_KILOWATTREL] = 490,
    [SPECIES_FERROTHORN] = 489,
    [SPECIES_OINKOLOGNE_F] = 489,
    [SPECIES_OINKOLOGNE_M] = 489,
    [SPECIES_UNFEZANT] = 488,
    [SPECIES_SCRAFTY] = 488,
    [SPECIES_HOUNDSTONE] = 488,
    [SPECIES_MUSHARNA] = 487,
    [SPECIES_LYCANROC_DUSK] = 487,
    [SPECIES_LYCANROC_MIDDAY] = 487,
    [SPECIES_LYCANROC_MIDNIGHT] = 487,
    [SPECIES_ZYGARDE_10_POWER_CONSTRUCT] = 486,
    [SPECIES_SCOVILLAIN] = 486,
    [SPECIES_RAICHU] = 485,
    [SPECIES_RAICHU_ALOLA] = 485,
    [SPECIES_RHYDON] = 485,
    [SPECIES_MANTINE] = 485,
    [SPECIES_HUNTAIL] = 485,
    [SPECIES_GOREBYSS] = 485,
    [SPECIES_RELICANTH] = 485,
    [SPECIES_STARAPTOR] = 485,
    [SPECIES_SPIRITOMB] = 485,
    [SPECIES_SCOLIPEDE] = 485,
    [SPECIES_CRUSTLE] = 485,
    [SPECIES_BEHEEYEM] = 485,
    [SPECIES_DRUDDIGON] = 485,
    [SPECIES_TOUCANNON] = 485,
    [SPECIES_COMFEY] = 485,
    [SPECIES_TURTONATOR] = 485,
    [SPECIES_DRAMPA] = 485,
    [SPECIES_DREDNAW] = 485,
    [SPECIES_FLAPPLE] = 485,
    [SPECIES_APPLETUN] = 485,
    [SPECIES_GRAFAIAI] = 485,
    [SPECIES_BOMBIRDIER] = 485,
    [SPECIES_DIPPLIN] = 485,
    [SPECIES_HEATMOR] = 484,
    [SPECIES_DURANT] = 484,
    [SPECIES_CLEFABLE] = 483,
    [SPECIES_HYPNO] = 483,
    [SPECIES_COFAGRIGUS] = 483,
    [SPECIES_GOLURK] = 483,
    [SPECIES_RUNERIGUS] = 483,
    [SPECIES_AMBIPOM] = 482,
    [SPECIES_MALAMAR] = 482,
    [SPECIES_HELIOLISK] = 481,
    [SPECIES_ESPATHRA] = 481,
    [SPECIES_OCTILLERY] = 480,
    [SPECIES_LUDICOLO] = 480,
    [SPECIES_SHIFTRY] = 480,
    [SPECIES_SABLEYE_MEGA] = 480,
    [SPECIES_MAWILE_MEGA] = 480,
    [SPECIES_GLALIE] = 480,
    [SPECIES_LOPUNNY] = 480,
    [SPECIES_FROSLASS] = 480,
    [SPECIES_PHIONE] = 480,
    [SPECIES_WHIMSICOTT] = 480,
    [SPECIES_LILLIGANT] = 480,
    [SPECIES_LILLIGANT_HISUI] = 480,
    [SPECIES_DARMANITAN_GALAR_STANDARD] = 480,
    [SPECIES_DARMANITAN_STANDARD] = 480,
    [SPECIES_JELLICENT] = 480,
    [SPECIES_SLURPUFF] = 480,
    [SPECIES_LURANTIS] = 480,
    [SPECIES_SALAZZLE] = 480,
    [SPECIES_PALOSSAND] = 480,
    [SPECIES_KOMALA] = 480,
    [SPECIES_GRAPPLOCT] = 480,
    [SPECIES_BRAMBLEGHAST] = 480,
    [SPECIES_ORTHWORM] = 480,
    [SPECIES_PIDGEOT] = 479,
    [SPECIES_SKUNTANK] = 479,
    [SPECIES_CRABOMINABLE] = 478,
    [SPECIES_VELUZA] = 478,
    [SPECIES_DACHSBUN] = 477,
    [SPECIES_ORICORIO] = 476,
    [SPECIES_MIMIKYU] = 476,
    [SPECIES_DEWGONG] = 475,
    [SPECIES_KINGLER] = 475,
    [SPECIES_MANECTRIC] = 475,
    [SPECIES_CACTURNE] = 475,
    [SPECIES_GASTRODON] = 475,
    [SPECIES_SAWSBUCK] = 475,
    [SPECIES_BRUXISH] = 475,
    [SPECIES_CRAMORANT] = 475,
    [SPECIES_FROSMOTH] = 475,
    [SPECIES_INDEEDEE_F] = 475,
    [SPECIES_INDEEDEE_M] = 475,
    [SPECIES_TATSUGIRI] = 475,
    [SPECIES_HARIYAMA] = 474,
    [SPECIES_VESPIQUEN] = 474,
    [SPECIES_GARBODOR] = 474,
    [SPECIES_TREVENANT] = 474,
    [SPECIES_SWANNA] = 473,
    [SPECIES_GALVANTULA] = 472,
    [SPECIES_FURFROU] = 472,
    [SPECIES_STUNFISK] = 471,
    [SPECIES_STUNFISK_GALAR] = 471,
    [SPECIES_DODRIO] = 470,
    [SPECIES_XATU] = 470,
    [SPECIES_TORKOAL] = 470,
    [SPECIES_GRUMPIG] = 470,
    [SPECIES_CINCCINO] = 470,
    [SPECIES_ALOMOMOLA] = 470,
    [SPECIES_KLEFKI] = 470,
    [SPECIES_FALINKS] = 470,
    [SPECIES_STONJOURNER] = 470,
    [SPECIES_EISCUE_ICE] = 470,
    [SPECIES_EISCUE_NOICE] = 470,
    [SPECIES_MAUSHOLD] = 470,
    [SPECIES_RABSCA] = 470,
    [SPECIES_WHISCASH] = 468,
    [SPECIES_CRAWDAUNT] = 468,
    [SPECIES_SWALOT] = 467,
    [SPECIES_MEOWSTIC] = 466,
    [SPECIES_MAGNETON] = 465,
    [SPECIES_FORRETRESS] = 465,
    [SPECIES_SKARMORY] = 465,
    [SPECIES_STANTLER] = 465,
    [SPECIES_ABSOL] = 465,
    [SPECIES_THROH] = 465,
    [SPECIES_SAWK] = 465,
    [SPECIES_AMOONGUSS] = 464,
    [SPECIES_RIBOMBEE] = 464,
    [SPECIES_AROMATISSE] = 462,
    [SPECIES_MARACTUS] = 461,
    [SPECIES_MR_MIME] = 460,
    [SPECIES_MR_MIME_GALAR] = 460,
    [SPECIES_LANTURN] = 460,
    [SPECIES_JUMPLUFF] = 460,
    [SPECIES_BRELOOM] = 460,
    [SPECIES_SHARPEDO] = 460,
    [SPECIES_CAMERUPT] = 460,
    [SPECIES_LUNATONE] = 460,
    [SPECIES_SOLROCK] = 460,
    [SPECIES_TROPIUS] = 460,
    [SPECIES_LUMINEON] = 460,
    [SPECIES_BASCULIN] = 460,
    [SPECIES_GREEDENT] = 460,
    [SPECIES_ELDEGOSS] = 460,
    [SPECIES_ZANGOOSE] = 458,
    [SPECIES_SEVIPER] = 458,
    [SPECIES_PALAFIN_ZERO] = 457,
    [SPECIES_NINJASK] = 456,
    [SPECIES_GOLBAT] = 455,
    [SPECIES_PRIMEAPE] = 455,
    [SPECIES_HITMONLEE] = 455,
    [SPECIES_HITMONCHAN] = 455,
    [SPECIES_JYNX] = 455,
    [SPECIES_GIRAFARIG] = 455,
    [SPECIES_HITMONTOP] = 455,
    [SPECIES_SWELLOW] = 455,
    [SPECIES_BANETTE] = 455,
    [SPECIES_DUSCLOPS] = 455,
    [SPECIES_CHIMECHO] = 455,
    [SPECIES_THIEVUL] = 455,
    [SPECIES_MASQUERAIN] = 454,
    [SPECIES_CARNIVINE] = 454,
    [SPECIES_ARAQUANID] = 454,
    [SPECIES_NOCTOWL] = 452,
    [SPECIES_PURUGLY] = 452,
    [SPECIES_SLIGGOO] = 452,
    [SPECIES_SLIGGOO_HISUI] = 452,
    [SPECIES_SANDSLASH] = 450,
    [SPECIES_SANDSLASH_ALOLA] = 450,
    [SPECIES_VENOMOTH] = 450,
    [SPECIES_CHANSEY] = 450,
    [SPECIES_SEAKING] = 450,
    [SPECIES_GRANBULL] = 450,
    [SPECIES_PILOSWINE] = 450,
    [SPECIES_CHERRIM] = 450,
    [SPECIES_LOKIX] = 450,
    [SPECIES_KLAWF] = 450,
    [SPECIES_TERAPAGOS_NORMAL] = 450,
    [SPECIES_ARBOK] = 448,
    [SPECIES_DOUBLADE] = 448,
    [SPECIES_LIEPARD] = 446,
    [SPECIES_AUDINO] = 445,
    [SPECIES_FEAROW] = 442,
    [SPECIES_PERSIAN] = 440,
    [SPECIES_PERSIAN_ALOLA] = 440,
    [SPECIES_SEADRA] = 440,
    [SPECIES_QWILFISH] = 440,
    [SPECIES_QWILFISH_HISUI] = 440,
    [SPECIES_PELIPPER] = 440,
    [SPECIES_VIGOROTH] = 440,
    [SPECIES_KECLEON] = 440,
    [SPECIES_ROTOM] = 440,
    [SPECIES_KLANG] = 440,
    [SPECIES_MINIOR_METEOR] = 440,
    [SPECIES_PERRSERKER] = 440,
    [SPECIES_MORPEKO] = 436,
    [SPECIES_WIGGLYTUFF] = 435,
    [SPECIES_TANGELA] = 435,
    [SPECIES_EEVEE_STARTER] = 435,
    [SPECIES_MISDREAVUS] = 435,
    [SPECIES_TOGEDEMARU] = 435,
    [SPECIES_PINCURCHIN] = 435,
    [SPECIES_DEDENNE] = 431,
    [SPECIES_PIKACHU_STARTER] = 430,
    [SPECIES_QUAGSIRE] = 430,
    [SPECIES_GLIGAR] = 430,
    [SPECIES_SNEASEL] = 430,
    [SPECIES_SNEASEL_HISUI] = 430,
    [SPECIES_MAGCARGO] = 430,
    [SPECIES_LAIRON] = 430,
    [SPECIES_VOLBEAT] = 430,
    [SPECIES_ILLUMISE] = 430,
    [SPECIES_CLODSIRE] = 430,
    [SPECIES_EMOLGA] = 428,
    [SPECIES_DUGTRIO] = 425,
    [SPECIES_DUGTRIO_ALOLA] = 425,
    [SPECIES_MAROWAK] = 425,
    [SPECIES_MAROWAK_ALOLA] = 425,
    [SPECIES_SUNFLORA] = 425,
    [SPECIES_SWOOBAT] = 425,
    [SPECIES_WUGTRIO] = 425,
    [SPECIES_WORMADAM_PLANT] = 424,
    [SPECIES_WORMADAM_SANDY] = 424,
    [SPECIES_WORMADAM_TRASH] = 424,
    [SPECIES_MOTHIM] = 424,
    [SPECIES_DIGGERSBY] = 423,
    [SPECIES_ARCTIBAX] = 423,
    [SPECIES_DRAGONAIR] = 420,
    [SPECIES_AZUMARILL] = 420,
    [SPECIES_MIGHTYENA] = 420,
    [SPECIES_LINOONE] = 420,
    [SPECIES_LINOONE_GALAR] = 420,
    [SPECIES_CASTFORM] = 420,
    [SPECIES_SHELGON] = 420,
    [SPECIES_METANG] = 420,
    [SPECIES_WATCHOG] = 420,
    [SPECIES_ZWEILOUS] = 420,
    [SPECIES_DARTRIX] = 420,
    [SPECIES_TORRACAT] = 420,
    [SPECIES_BRIONNE] = 420,
    [SPECIES_HAKAMO_O] = 420,
    [SPECIES_POIPOLE] = 420,
    [SPECIES_THWACKEY] = 420,
    [SPECIES_RABOOT] = 420,
    [SPECIES_DRIZZILE] = 420,
    [SPECIES_PIGNITE] = 418,
    [SPECIES_GUMSHOOS] = 418,
    [SPECIES_SQUAWKABILLY] = 417,
    [SPECIES_FURRET] = 415,
    [SPECIES_DUNSPARCE] = 415,
    [SPECIES_RATICATE] = 413,
    [SPECIES_RATICATE_ALOLA] = 413,
    [SPECIES_SERVINE] = 413,
    [SPECIES_DEWOTT] = 413,
    [SPECIES_CHATOT] = 411,
    [SPECIES_VIVILLON] = 411,
    [SPECIES_CROCALOR] = 411,
    [SPECIES_PONYTA] = 410,
    [SPECIES_PONYTA_GALAR] = 410,
    [SPECIES_SUDOWOODO] = 410,
    [SPECIES_CORSOLA] = 410,
    [SPECIES_CORSOLA_GALAR] = 410,
    [SPECIES_PUPITAR] = 410,
    [SPECIES_MEDICHAM] = 410,
    [SPECIES_SEALEO] = 410,
    [SPECIES_BIBAREL] = 410,
    [SPECIES_GABITE] = 410,
    [SPECIES_FRAXURE] = 410,
    [SPECIES_PYUKUMUKU] = 410,
    [SPECIES_CARKOL] = 410,
    [SPECIES_DRAKLOAK] = 410,
    [SPECIES_FLORAGATO] = 410,
    [SPECIES_QUAXWELL] = 410,
    [SPECIES_BRAIXEN] = 409,
    [SPECIES_IVYSAUR] = 405,
    [SPECIES_CHARMELEON] = 405,
    [SPECIES_WARTORTLE] = 405,
    [SPECIES_PARASECT] = 405,
    [SPECIES_MACHOKE] = 405,
    [SPECIES_HAUNTER] = 405,
    [SPECIES_BAYLEEF] = 405,
    [SPECIES_QUILAVA] = 405,
    [SPECIES_CROCONAW] = 405,
    [SPECIES_TOGETIC] = 405,
    [SPECIES_MURKROW] = 405,
    [SPECIES_WOBBUFFET] = 405,
    [SPECIES_GROVYLE] = 405,
    [SPECIES_COMBUSKEN] = 405,
    [SPECIES_MARSHTOMP] = 405,
    [SPECIES_PLUSLE] = 405,
    [SPECIES_MINUN] = 405,
    [SPECIES_GROTLE] = 405,
    [SPECIES_MONFERNO] = 405,
    [SPECIES_PRINPLUP] = 405,
    [SPECIES_PACHIRISU] = 405,
    [SPECIES_GURDURR] = 405,
    [SPECIES_EELEKTRIK] = 405,
    [SPECIES_QUILLADIN] = 405,
    [SPECIES_FROGADIER] = 405,
    [SPECIES_SHIINOTIC] = 405,
    [SPECIES_SPIDOPS] = 404,
    [SPECIES_ARCHEN] = 401,
    [SPECIES_KADABRA] = 400,
    [SPECIES_ARIADOS] = 400,
    [SPECIES_DELCATTY] = 400,
    [SPECIES_ROSELIA] = 400,
    [SPECIES_WAILMER] = 400,
    [SPECIES_CHARJABUG] = 400,
    [SPECIES_COSMOEM] = 400,
    [SPECIES_BUTTERFREE] = 395,
    [SPECIES_BEEDRILL] = 395,
    [SPECIES_GLOOM] = 395,
    [SPECIES_PORYGON] = 395,
    [SPECIES_BEAUTIFLY] = 395,
    [SPECIES_VANILLISH] = 395,
    [SPECIES_WEEPINBELL] = 390,
    [SPECIES_GRAVELER] = 390,
    [SPECIES_GRAVELER_ALOLA] = 390,
    [SPECIES_LEDIAN] = 390,
    [SPECIES_YANMA] = 390,
    [SPECIES_MUNCHLAX] = 390,
    [SPECIES_BOLDORE] = 390,
    [SPECIES_GOTHORITA] = 390,
    [SPECIES_POLIWHIRL] = 385,
    [SPECIES_ONIX] = 385,
    [SPECIES_LICKITUNG] = 385,
    [SPECIES_DUSTOX] = 385,
    [SPECIES_MUDBRAY] = 385,
    [SPECIES_KUBFU] = 385,
    [SPECIES_KRICKETUNE] = 384,
    [SPECIES_PALPITOAD] = 384,
    [SPECIES_FLETCHINDER] = 382,
    [SPECIES_SABLEYE] = 380,
    [SPECIES_MAWILE] = 380,
    [SPECIES_SWADLOON] = 380,
    [SPECIES_TINKATUFF] = 380,
    [SPECIES_FARFETCHD] = 377,
    [SPECIES_FARFETCHD_GALAR] = 377,
    [SPECIES_NOSEPASS] = 375,
    [SPECIES_FLOETTE] = 371,
    [SPECIES_HERDIER] = 370,
    [SPECIES_DUOSION] = 370,
    [SPECIES_LAMPENT] = 370,
    [SPECIES_VULLABY] = 370,
    [SPECIES_HATTREM] = 370,
    [SPECIES_MORGREM] = 370,
    [SPECIES_LITLEO] = 369,
    [SPECIES_NIDORINA] = 365,
    [SPECIES_NIDORINO] = 365,
    [SPECIES_FLAAFFY] = 365,
    [SPECIES_MAGBY] = 365,
    [SPECIES_CORVISQUIRE] = 365,
    [SPECIES_LUXIO] = 363,
    [SPECIES_TYRUNT] = 362,
    [SPECIES_AMAURA] = 362,
    [SPECIES_AIPOM] = 360,
    [SPECIES_ELEKID] = 360,
    [SPECIES_LOUDRED] = 360,
    [SPECIES_SPINDA] = 360,
    [SPECIES_WHIRLIPEDE] = 360,
    [SPECIES_LARVESTA] = 360,
    [SPECIES_TRANQUILL] = 358,
    [SPECIES_OMANYTE] = 355,
    [SPECIES_KABUTO] = 355,
    [SPECIES_LILEEP] = 355,
    [SPECIES_ANORITH] = 355,
    [SPECIES_TIRTOUGA] = 355,
    [SPECIES_ESPURR] = 355,
    [SPECIES_TRUMBEAK] = 355,
    [SPECIES_NACLSTACK] = 355,
    [SPECIES_DOLLIV] = 354,
    [SPECIES_KROKOROK] = 351,
    [SPECIES_GROWLITHE] = 350,
    [SPECIES_GROWLITHE_HISUI] = 350,
    [SPECIES_CRANIDOS] = 350,
    [SPECIES_SHIELDON] = 350,
    [SPECIES_BUNEARY] = 350,
    [SPECIES_MIENFOO] = 350,
    [SPECIES_RUFFLET] = 350,
    [SPECIES_SKIDDO] = 350,
    [SPECIES_PAWMO] = 350,
    [SPECIES_GLIMMET] = 350,
    [SPECIES_PIDGEOTTO] = 349,
    [SPECIES_DRIFLOON] = 348,
    [SPECIES_SCRAGGY] = 348,
    [SPECIES_PANCHAM] = 348,
    [SPECIES_RHYHORN] = 345,
    [SPECIES_CLAMPERL] = 345,
    [SPECIES_MANTYKE] = 345,
    [SPECIES_SPRITZEE] = 341,
    [SPECIES_SWIRLIX] = 341,
    [SPECIES_KOFFING] = 340,
    [SPECIES_STARYU] = 340,
    [SPECIES_SKIPLOOM] = 340,
    [SPECIES_LOMBRE] = 340,
    [SPECIES_NUZLEAF] = 340,
    [SPECIES_VIBRAVA] = 340,
    [SPECIES_STARAVIA] = 340,
    [SPECIES_PAWNIARD] = 340,
    [SPECIES_STUFFUL] = 340,
    [SPECIES_MASCHIFF] = 340,
    [SPECIES_CRABRAWLER] = 338,
    [SPECIES_UNOWN] = 336,
    [SPECIES_TENTACOOL] = 335,
    [SPECIES_CACNEA] = 335,
    [SPECIES_DEERLING] = 335,
    [SPECIES_FRILLISH] = 335,
    [SPECIES_ELGYEM] = 335,
    [SPECIES_PUMPKABOO_AVERAGE] = 335,
    [SPECIES_PUMPKABOO_LARGE] = 335,
    [SPECIES_PUMPKABOO_SMALL] = 335,
    [SPECIES_PUMPKABOO_SUPER] = 335,
    [SPECIES_DOTTLER] = 335,
    [SPECIES_TOEDSCOOL] = 335,
    [SPECIES_SNOVER] = 334,
    [SPECIES_CETODDLE] = 334,
    [SPECIES_VOLTORB] = 330,
    [SPECIES_VOLTORB_HISUI] = 330,
    [SPECIES_CHINCHOU] = 330,
    [SPECIES_TEDDIURSA] = 330,
    [SPECIES_DELIBIRD] = 330,
    [SPECIES_HOUNDOUR] = 330,
    [SPECIES_PHANPY] = 330,
    [SPECIES_ARON] = 330,
    [SPECIES_SPOINK] = 330,
    [SPECIES_LUVDISC] = 330,
    [SPECIES_BUIZEL] = 330,
    [SPECIES_HIPPOPOTAS] = 330,
    [SPECIES_SKORUPI] = 330,
    [SPECIES_FINNEON] = 330,
    [SPECIES_ZORUA] = 330,
    [SPECIES_ZORUA_HISUI] = 330,
    [SPECIES_CLAUNCHER] = 330,
    [SPECIES_CUFANT] = 330,
    [SPECIES_STUNKY] = 329,
    [SPECIES_TRUBBISH] = 329,
    [SPECIES_DROWZEE] = 328,
    [SPECIES_DRILBUR] = 328,
    [SPECIES_MAGNEMITE] = 325,
    [SPECIES_SEEL] = 325,
    [SPECIES_GRIMER] = 325,
    [SPECIES_GRIMER_ALOLA] = 325,
    [SPECIES_KRABBY] = 325,
    [SPECIES_EXEGGCUTE] = 325,
    [SPECIES_EEVEE] = 325,
    [SPECIES_SHELLOS] = 325,
    [SPECIES_DWEBBLE] = 325,
    [SPECIES_HONEDGE] = 325,
    [SPECIES_CLEFAIRY] = 323,
    [SPECIES_WOOBAT] = 323,
    [SPECIES_PIKACHU] = 320,
    [SPECIES_ODDISH] = 320,
    [SPECIES_PSYDUCK] = 320,
    [SPECIES_CUBONE] = 320,
    [SPECIES_GOLDEEN] = 320,
    [SPECIES_NATU] = 320,
    [SPECIES_AXEW] = 320,
    [SPECIES_SKRELP] = 320,
    [SPECIES_ROWLET] = 320,
    [SPECIES_LITTEN] = 320,
    [SPECIES_POPPLIO] = 320,
    [SPECIES_SALANDIT] = 320,
    [SPECIES_SANDYGAST] = 320,
    [SPECIES_FRIGIBAX] = 320,
    [SPECIES_JOLTIK] = 319,
    [SPECIES_BULBASAUR] = 318,
    [SPECIES_CHIKORITA] = 318,
    [SPECIES_TURTWIG] = 318,
    [SPECIES_PANSAGE] = 316,
    [SPECIES_PANSEAR] = 316,
    [SPECIES_PANPOUR] = 316,
    [SPECIES_SLOWPOKE] = 315,
    [SPECIES_SLOWPOKE_GALAR] = 315,
    [SPECIES_DARUMAKA] = 315,
    [SPECIES_DARUMAKA_GALAR] = 315,
    [SPECIES_KARRABLAST] = 315,
    [SPECIES_SILICOBRA] = 315,
    [SPECIES_FINIZEN] = 315,
    [SPECIES_SQUIRTLE] = 314,
    [SPECIES_TOTODILE] = 314,
    [SPECIES_PIPLUP] = 314,
    [SPECIES_FROAKIE] = 314,
    [SPECIES_CHESPIN] = 313,
    [SPECIES_FIDOUGH] = 312,
    [SPECIES_ABRA] = 310,
    [SPECIES_DODUO] = 310,
    [SPECIES_GASTLY] = 310,
    [SPECIES_TREECKO] = 310,
    [SPECIES_TORCHIC] = 310,
    [SPECIES_MUDKIP] = 310,
    [SPECIES_SWABLU] = 310,
    [SPECIES_GLAMEOW] = 310,
    [SPECIES_MIME_JR] = 310,
    [SPECIES_SEWADDLE] = 310,
    [SPECIES_GROOKEY] = 310,
    [SPECIES_SCORBUNNY] = 310,
    [SPECIES_SOBBLE] = 310,
    [SPECIES_CLOBBOPUS] = 310,
    [SPECIES_SPRIGATITO] = 310,
    [SPECIES_FUECOCO] = 310,
    [SPECIES_QUAXLY] = 310,
    [SPECIES_CHARMANDER] = 309,
    [SPECIES_CYNDAQUIL] = 309,
    [SPECIES_CHIMCHAR] = 309,
    [SPECIES_PHANTUMP] = 309,
    [SPECIES_CORPHISH] = 308,
    [SPECIES_SNIVY] = 308,
    [SPECIES_TEPIG] = 308,
    [SPECIES_OSHAWOTT] = 308,
    [SPECIES_SINISTEA] = 308,
    [SPECIES_POLTCHAGEIST] = 308,
    [SPECIES_FENNEKIN] = 307,
    [SPECIES_BINACLE] = 306,
    [SPECIES_VENONAT] = 305,
    [SPECIES_MANKEY] = 305,
    [SPECIES_MACHOP] = 305,
    [SPECIES_SHELLDER] = 305,
    [SPECIES_SMOOCHUM] = 305,
    [SPECIES_CARVANHA] = 305,
    [SPECIES_NUMEL] = 305,
    [SPECIES_TIMBURR] = 305,
    [SPECIES_DUCKLETT] = 305,
    [SPECIES_VANILLITE] = 305,
    [SPECIES_FERROSEED] = 305,
    [SPECIES_CUBCHOO] = 305,
    [SPECIES_SHELMET] = 305,
    [SPECIES_MAREANIE] = 305,
    [SPECIES_SIZZLIPEDE] = 305,
    [SPECIES_TANDEMAUS] = 305,
    [SPECIES_BERGMITE] = 304,
    [SPECIES_CUTIEFLY] = 304,
    [SPECIES_CAPSAKID] = 304,
    [SPECIES_YAMASK] = 303,
    [SPECIES_YAMASK_GALAR] = 303,
    [SPECIES_GOLETT] = 303,
    [SPECIES_FLABEBE] = 303,
    [SPECIES_GULPIN] = 302,
    [SPECIES_SANDSHREW] = 300,
    [SPECIES_SANDSHREW_ALOLA] = 300,
    [SPECIES_POLIWAG] = 300,
    [SPECIES_BELLSPROUT] = 300,
    [SPECIES_GEODUDE] = 300,
    [SPECIES_GEODUDE_ALOLA] = 300,
    [SPECIES_DRATINI] = 300,
    [SPECIES_SNUBBULL] = 300,
    [SPECIES_REMORAID] = 300,
    [SPECIES_LARVITAR] = 300,
    [SPECIES_BALTOY] = 300,
    [SPECIES_SNORUNT] = 300,
    [SPECIES_BAGON] = 300,
    [SPECIES_BELDUM] = 300,
    [SPECIES_BRONZOR] = 300,
    [SPECIES_GIBLE] = 300,
    [SPECIES_CROAGUNK] = 300,
    [SPECIES_MINCCINO] = 300,
    [SPECIES_KLINK] = 300,
    [SPECIES_DEINO] = 300,
    [SPECIES_GOOMY] = 300,
    [SPECIES_GRUBBIN] = 300,
    [SPECIES_JANGMO_O] = 300,
    [SPECIES_MELTAN] = 300,
    [SPECIES_VAROOM] = 300,
    [SPECIES_GIMMIGHOUL_CHEST] = 300,
    [SPECIES_GIMMIGHOUL_ROAMING] = 300,
    [SPECIES_VULPIX] = 299,
    [SPECIES_VULPIX_ALOLA] = 299,
    [SPECIES_TINKATINK] = 297,
    [SPECIES_HORSEA] = 295,
    [SPECIES_SHROOMISH] = 295,
    [SPECIES_ELECTRIKE] = 295,
    [SPECIES_SHUPPET] = 295,
    [SPECIES_DUSKULL] = 295,
    [SPECIES_BLITZLE] = 295,
    [SPECIES_TYMPOLE] = 294,
    [SPECIES_FOONGUS] = 294,
    [SPECIES_MUNNA] = 292,
    [SPECIES_SANDILE] = 292,
    [SPECIES_MEOWTH] = 290,
    [SPECIES_MEOWTH_ALOLA] = 290,
    [SPECIES_MEOWTH_GALAR] = 290,
    [SPECIES_PINECO] = 290,
    [SPECIES_TRAPINCH] = 290,
    [SPECIES_SPHEAL] = 290,
    [SPECIES_BONSLY] = 290,
    [SPECIES_GOTHITA] = 290,
    [SPECIES_SOLOSIS] = 290,
    [SPECIES_STEENEE] = 290,
    [SPECIES_SHROODLE] = 290,
    [SPECIES_GREAVARD] = 290,
    [SPECIES_HELIOPTILE] = 289,
    [SPECIES_EKANS] = 288,
    [SPECIES_DITTO] = 288,
    [SPECIES_BARBOACH] = 288,
    [SPECIES_INKAY] = 288,
    [SPECIES_PARAS] = 285,
    [SPECIES_CHINGLING] = 285,
    [SPECIES_RIOLU] = 285,
    [SPECIES_MORELULL] = 285,
    [SPECIES_CHEWTLE] = 284,
    [SPECIES_PURRLOIN] = 281,
    [SPECIES_MAREEP] = 280,
    [SPECIES_SLAKOTH] = 280,
    [SPECIES_MEDITITE] = 280,
    [SPECIES_BUDEW] = 280,
    [SPECIES_ROGGENROLA] = 280,
    [SPECIES_COTTONEE] = 280,
    [SPECIES_PETILIL] = 280,
    [SPECIES_ROCKRUFF] = 280,
    [SPECIES_ARROKUDA] = 280,
    [SPECIES_NACLI] = 280,
    [SPECIES_WATTREL] = 280,
    [SPECIES_KIRLIA] = 278,
    [SPECIES_FLETCHLING] = 278,
    [SPECIES_NIDORAN_F] = 275,
    [SPECIES_CHERUBI] = 275,
    [SPECIES_LILLIPUP] = 275,
    [SPECIES_TYNAMO] = 275,
    [SPECIES_LITWICK] = 275,
    [SPECIES_SKWOVET] = 275,
    [SPECIES_BRAMBLIN] = 275,
    [SPECIES_NIDORAN_M] = 273,
    [SPECIES_TADBULB] = 272,
    [SPECIES_JIGGLYPUFF] = 270,
    [SPECIES_TAILLOW] = 270,
    [SPECIES_WINGULL] = 270,
    [SPECIES_WOOLOO] = 270,
    [SPECIES_YAMPER] = 270,
    [SPECIES_MILCERY] = 270,
    [SPECIES_DREEPY] = 270,
    [SPECIES_RELLOR] = 270,
    [SPECIES_SURSKIT] = 269,
    [SPECIES_DEWPIDER] = 269,
    [SPECIES_NINCADA] = 266,
    [SPECIES_DIGLETT] = 265,
    [SPECIES_DIGLETT_ALOLA] = 265,
    [SPECIES_LEDYBA] = 265,
    [SPECIES_PIKIPEK] = 265,
    [SPECIES_HATENNA] = 265,
    [SPECIES_IMPIDIMP] = 265,
    [SPECIES_PIDOVE] = 264,
    [SPECIES_SHINX] = 263,
    [SPECIES_SPEAROW] = 262,
    [SPECIES_HOOTHOOT] = 262,
    [SPECIES_SKITTY] = 260,
    [SPECIES_WYNAUT] = 260,
    [SPECIES_VENIPEDE] = 260,
    [SPECIES_APPLIN] = 260,
    [SPECIES_SMOLIV] = 260,
    [SPECIES_PATRAT] = 255,
    [SPECIES_CHARCADET] = 255,
    [SPECIES_FLITTLE] = 255,
    [SPECIES_LECHONK] = 254,
    [SPECIES_RATTATA] = 253,
    [SPECIES_RATTATA_ALOLA] = 253,
    [SPECIES_YUNGOOS] = 253,
    [SPECIES_PIDGEY] = 251,
    [SPECIES_SPINARAK] = 250,
    [SPECIES_MARILL] = 250,
    [SPECIES_HOPPIP] = 250,
    [SPECIES_SLUGMA] = 250,
    [SPECIES_SWINUB] = 250,
    [SPECIES_SMEARGLE] = 250,
    [SPECIES_BIDOOF] = 250,
    [SPECIES_FOMANTIS] = 250,
    [SPECIES_GOSSIFLEUR] = 250,
    [SPECIES_ZUBAT] = 245,
    [SPECIES_TOGEPI] = 245,
    [SPECIES_STARLY] = 245,
    [SPECIES_NOIBAT] = 245,
    [SPECIES_ROOKIDEE] = 245,
    [SPECIES_NICKIT] = 245,
    [SPECIES_WIGLETT] = 245,
    [SPECIES_COMBEE] = 244,
    [SPECIES_TOXEL] = 242,
    [SPECIES_ZIGZAGOON] = 240,
    [SPECIES_ZIGZAGOON_GALAR] = 240,
    [SPECIES_WHISMUR] = 240,
    [SPECIES_ROLYCOLY] = 240,
    [SPECIES_PAWMI] = 240,
    [SPECIES_MAKUHITA] = 237,
    [SPECIES_BUNNELBY] = 237,
    [SPECIES_SHEDINJA] = 236,
    [SPECIES_WIMPOD] = 230,
    [SPECIES_BURMY] = 224,
    [SPECIES_POOCHYENA] = 220,
    [SPECIES_LOTAD] = 220,
    [SPECIES_SEEDOT] = 220,
    [SPECIES_HAPPINY] = 220,
    [SPECIES_CLEFFA] = 218,
    [SPECIES_SENTRET] = 215,
    [SPECIES_SPEWPA] = 213,
    [SPECIES_IGGLYBUFF] = 210,
    [SPECIES_WOOPER] = 210,
    [SPECIES_WOOPER_PALDEA] = 210,
    [SPECIES_TYROGUE] = 210,
    [SPECIES_BOUNSWEET] = 210,
    [SPECIES_TAROUNTULA] = 210,
    [SPECIES_NYMBLE] = 210,
    [SPECIES_METAPOD] = 205,
    [SPECIES_KAKUNA] = 205,
    [SPECIES_PICHU] = 205,
    [SPECIES_SILCOON] = 205,
    [SPECIES_CASCOON] = 205,
    [SPECIES_MAGIKARP] = 200,
    [SPECIES_FEEBAS] = 200,
    [SPECIES_SCATTERBUG] = 200,
    [SPECIES_COSMOG] = 200,
    [SPECIES_RALTS] = 198,
    [SPECIES_CATERPIE] = 195,
    [SPECIES_WEEDLE] = 195,
    [SPECIES_WURMPLE] = 195,
    [SPECIES_KRICKETOT] = 194,
    [SPECIES_AZURILL] = 190,
    [SPECIES_SNOM] = 185,
    [SPECIES_SUNKERN] = 180,
    [SPECIES_BLIPBUG] = 180,
    [SPECIES_WISHIWASHI_SOLO] = 175,
};

#define RANDOM_SPECIES_NORMAL_COUNT ARRAY_COUNT(sRandomSpeciesNormalBst)
static const u16 sRandomSpeciesNormalBst[] =
{
    SPECIES_AZURILL,
    SPECIES_IGGLYBUFF,
    SPECIES_SENTRET,
    SPECIES_HAPPINY,
    SPECIES_BUNNELBY,
    SPECIES_WHISMUR,
    SPECIES_ZIGZAGOON_GALAR,
    SPECIES_ZIGZAGOON,
    SPECIES_STARLY,
    SPECIES_BIDOOF,
    SPECIES_SMEARGLE,
    SPECIES_PIDGEY,
    SPECIES_YUNGOOS,
    SPECIES_RATTATA_ALOLA,
    SPECIES_RATTATA,
    SPECIES_LECHONK,
    SPECIES_PATRAT,
    SPECIES_SMOLIV,
    SPECIES_SKITTY,
    SPECIES_HOOTHOOT,
    SPECIES_SPEAROW,
    SPECIES_PIDOVE,
    SPECIES_PIKIPEK,
    SPECIES_WOOLOO,
    SPECIES_TAILLOW,
    SPECIES_JIGGLYPUFF,
    SPECIES_SKWOVET,
    SPECIES_LILLIPUP,
    SPECIES_FLETCHLING,
    SPECIES_SLAKOTH,
    SPECIES_DITTO,
    SPECIES_HELIOPTILE,
    SPECIES_SHROODLE,
    SPECIES_MEOWTH,
    SPECIES_MINCCINO,
    SPECIES_TANDEMAUS,
    SPECIES_GLAMEOW,
    SPECIES_SWABLU,
    SPECIES_DODUO,
    SPECIES_EEVEE,
    SPECIES_ZORUA_HISUI,
    SPECIES_TEDDIURSA,
    SPECIES_DEERLING,
    SPECIES_STUFFUL,
    SPECIES_STARAVIA,
    SPECIES_PIDGEOTTO,
    SPECIES_RUFFLET,
    SPECIES_BUNEARY,
    SPECIES_DOLLIV,
    SPECIES_TRUMBEAK,
    SPECIES_TRANQUILL,
    SPECIES_SPINDA,
    SPECIES_LOUDRED,
    SPECIES_AIPOM,
    SPECIES_LITLEO,
    SPECIES_HERDIER,
    SPECIES_FARFETCHD,
    SPECIES_LICKITUNG,
    SPECIES_MUNCHLAX,
    SPECIES_PORYGON,
    SPECIES_DELCATTY,
    SPECIES_BIBAREL,
    SPECIES_CHATOT,
    SPECIES_RATICATE_ALOLA,
    SPECIES_RATICATE,
    SPECIES_DUNSPARCE,
    SPECIES_FURRET,
    SPECIES_SQUAWKABILLY,
    SPECIES_GUMSHOOS,
    SPECIES_WATCHOG,
    SPECIES_CASTFORM,
    SPECIES_LINOONE_GALAR,
    SPECIES_LINOONE,
    SPECIES_DIGGERSBY,
    SPECIES_EEVEE_STARTER,
    SPECIES_WIGGLYTUFF,
    SPECIES_KECLEON,
    SPECIES_VIGOROTH,
    SPECIES_PERSIAN,
    SPECIES_FEAROW,
    SPECIES_AUDINO,
    SPECIES_TERAPAGOS_NORMAL,
    SPECIES_CHANSEY,
    SPECIES_PURUGLY,
    SPECIES_NOCTOWL,
    SPECIES_SWELLOW,
    SPECIES_GIRAFARIG,
    SPECIES_ZANGOOSE,
    SPECIES_GREEDENT,
    SPECIES_STANTLER,
    SPECIES_MAUSHOLD,
    SPECIES_CINCCINO,
    SPECIES_DODRIO,
    SPECIES_FURFROU,
    SPECIES_INDEEDEE_M,
    SPECIES_INDEEDEE_F,
    SPECIES_SAWSBUCK,
    SPECIES_PIDGEOT,
    SPECIES_KOMALA,
    SPECIES_LOPUNNY,
    SPECIES_HELIOLISK,
    SPECIES_AMBIPOM,
    SPECIES_GRAFAIAI,
    SPECIES_DRAMPA,
    SPECIES_TOUCANNON,
    SPECIES_STARAPTOR,
    SPECIES_UNFEZANT,
    SPECIES_OINKOLOGNE_M,
    SPECIES_OINKOLOGNE_F,
    SPECIES_DUBWOOL,
    SPECIES_ORANGURU,
    SPECIES_BOUFFALANT,
    SPECIES_EXPLOUD,
    SPECIES_MILTANK,
    SPECIES_TAUROS,
    SPECIES_KANGASKHAN,
    SPECIES_BEWEAR,
    SPECIES_STOUTLAND,
    SPECIES_URSARING,
    SPECIES_CYCLIZAR,
    SPECIES_PYROAR,
    SPECIES_ARBOLIVA,
    SPECIES_BRAVIARY,
    SPECIES_ZOROARK_HISUI,
    SPECIES_LICKILICKY,
    SPECIES_PORYGON2,
    SPECIES_DUDUNSPARCE,
    SPECIES_FARIGIRAF,
    SPECIES_OBSTAGOON,
    SPECIES_WYRDEER,
    SPECIES_TYPE_NULL,
    SPECIES_PORYGON_Z,
    SPECIES_BLISSEY,
    SPECIES_SNORLAX,
    SPECIES_AUDINO_MEGA,
    SPECIES_URSALUNA,
    SPECIES_URSALUNA_BLOODMOON,
    SPECIES_SILVALLY,
    SPECIES_PIDGEOT_MEGA,
    SPECIES_LOPUNNY_MEGA,
    SPECIES_KANGASKHAN_MEGA,
    SPECIES_TERAPAGOS_TERASTAL,
    SPECIES_MELOETTA_PIROUETTE,
    SPECIES_MELOETTA_ARIA,
    SPECIES_REGIGIGAS,
    SPECIES_SLAKING,
    SPECIES_TERAPAGOS_STELLAR,
    SPECIES_ARCEUS,
};

#define RANDOM_SPECIES_FIGHTING_COUNT ARRAY_COUNT(sRandomSpeciesFightingBst)
static const u16 sRandomSpeciesFightingBst[] =
{
  SPECIES_TYROGUE,
  SPECIES_MAKUHITA,
  SPECIES_MEDITITE,
  SPECIES_RIOLU,
  SPECIES_CROAGUNK,
  SPECIES_TIMBURR,
  SPECIES_MACHOP,
  SPECIES_MANKEY,
  SPECIES_CLOBBOPUS,
  SPECIES_CRABRAWLER,
  SPECIES_STUFFUL,
  SPECIES_PANCHAM,
  SPECIES_SCRAGGY,
  SPECIES_PAWMO,
  SPECIES_MIENFOO,
  SPECIES_FARFETCHD_GALAR,
  SPECIES_KUBFU,
  SPECIES_GURDURR,
  SPECIES_MONFERNO,
  SPECIES_COMBUSKEN,
  SPECIES_MACHOKE,
  SPECIES_MEDICHAM,
  SPECIES_PIGNITE,
  SPECIES_HAKAMO_O,
  SPECIES_SNEASEL_HISUI,
  SPECIES_HITMONTOP,
  SPECIES_HITMONCHAN,
  SPECIES_HITMONLEE,
  SPECIES_PRIMEAPE,
  SPECIES_BRELOOM,
  SPECIES_SAWK,
  SPECIES_THROH,
  SPECIES_FALINKS,
  SPECIES_HARIYAMA,
  SPECIES_CRABOMINABLE,
  SPECIES_GRAPPLOCT,
  SPECIES_LILLIGANT_HISUI,
  SPECIES_SCRAFTY,
  SPECIES_PAWMOT,
  SPECIES_PASSIMIAN,
  SPECIES_TOXICROAK,
  SPECIES_TAUROS_PALDEA_AQUA,
  SPECIES_TAUROS_PALDEA_BLAZE,
  SPECIES_TAUROS_PALDEA_COMBAT,
  SPECIES_PANGORO,
  SPECIES_FLAMIGO,
  SPECIES_BEWEAR,
  SPECIES_HAWLUCHA,
  SPECIES_HERACROSS,
  SPECIES_CONKELDURR,
  SPECIES_MACHAMP,
  SPECIES_SIRFETCHD,
  SPECIES_SNEASLER,
  SPECIES_MIENSHAO,
  SPECIES_MEDICHAM_MEGA,
  SPECIES_POLIWRATH,
  SPECIES_GALLADE,
  SPECIES_LUCARIO,
  SPECIES_EMBOAR,
  SPECIES_QUAQUAVAL,
  SPECIES_DECIDUEYE_HISUI,
  SPECIES_CHESNAUGHT,
  SPECIES_BLAZIKEN,
  SPECIES_INFERNAPE,
  SPECIES_ANNIHILAPE,
  SPECIES_URSHIFU_RAPID_STRIKE,
  SPECIES_URSHIFU_SINGLE_STRIKE,
  SPECIES_OKIDOGI,
  SPECIES_IRON_HANDS,
  SPECIES_SLITHER_WING,
  SPECIES_GREAT_TUSK,
  SPECIES_PHEROMOSA,
  SPECIES_BUZZWOLE,
  SPECIES_KELDEO_RESOLUTE,
  SPECIES_KELDEO_ORDINARY,
  SPECIES_VIRIZION,
  SPECIES_TERRAKION,
  SPECIES_COBALION,
  SPECIES_LOPUNNY_MEGA,
  SPECIES_ZAPDOS_GALAR,
  SPECIES_IRON_VALIANT,
  SPECIES_MARSHADOW,
  SPECIES_KOMMO_O,
  SPECIES_HAWLUCHA_MEGA,
  SPECIES_MELOETTA_PIROUETTE,
  SPECIES_HERACROSS_MEGA,
  SPECIES_GALLADE_MEGA,
  SPECIES_LUCARIO_MEGA,
  SPECIES_BLAZIKEN_MEGA,
  SPECIES_ZAMAZENTA_HERO,
  SPECIES_KORAIDON,
  SPECIES_ZAMAZENTA_CROWNED,
  SPECIES_MEWTWO_MEGA_X,
};

#define RANDOM_SPECIES_FLYING_COUNT ARRAY_COUNT(sRandomSpeciesFlyingBst)
static const u16 sRandomSpeciesFlyingBst[] =
{
  SPECIES_COMBEE,
  SPECIES_ROOKIDEE,
  SPECIES_NOIBAT,
  SPECIES_STARLY,
  SPECIES_ZUBAT,
  SPECIES_HOPPIP,
  SPECIES_PIDGEY,
  SPECIES_HOOTHOOT,
  SPECIES_SPEAROW,
  SPECIES_PIDOVE,
  SPECIES_PIKIPEK,
  SPECIES_LEDYBA,
  SPECIES_WINGULL,
  SPECIES_TAILLOW,
  SPECIES_FLETCHLING,
  SPECIES_WATTREL,
  SPECIES_DUCKLETT,
  SPECIES_SWABLU,
  SPECIES_DODUO,
  SPECIES_ROWLET,
  SPECIES_NATU,
  SPECIES_WOOBAT,
  SPECIES_DELIBIRD,
  SPECIES_STARAVIA,
  SPECIES_SKIPLOOM,
  SPECIES_MANTYKE,
  SPECIES_DRIFLOON,
  SPECIES_PIDGEOTTO,
  SPECIES_RUFFLET,
  SPECIES_TRUMBEAK,
  SPECIES_TRANQUILL,
  SPECIES_CORVISQUIRE,
  SPECIES_VULLABY,
  SPECIES_FARFETCHD,
  SPECIES_FLETCHINDER,
  SPECIES_YANMA,
  SPECIES_LEDIAN,
  SPECIES_BEAUTIFLY,
  SPECIES_BUTTERFREE,
  SPECIES_ARCHEN,
  SPECIES_MURKROW,
  SPECIES_TOGETIC,
  SPECIES_VIVILLON,
  SPECIES_CHATOT,
  SPECIES_SQUAWKABILLY,
  SPECIES_DARTRIX,
  SPECIES_MOTHIM,
  SPECIES_SWOOBAT,
  SPECIES_EMOLGA,
  SPECIES_GLIGAR,
  SPECIES_MINIOR_METEOR,
  SPECIES_PELIPPER,
  SPECIES_FEAROW,
  SPECIES_NOCTOWL,
  SPECIES_MASQUERAIN,
  SPECIES_SWELLOW,
  SPECIES_GOLBAT,
  SPECIES_NINJASK,
  SPECIES_TROPIUS,
  SPECIES_JUMPLUFF,
  SPECIES_SKARMORY,
  SPECIES_XATU,
  SPECIES_DODRIO,
  SPECIES_SWANNA,
  SPECIES_VESPIQUEN,
  SPECIES_CRAMORANT,
  SPECIES_ORICORIO_SENSU,
  SPECIES_ORICORIO_PAU,
  SPECIES_ORICORIO_POM_POM,
  SPECIES_ORICORIO_BAILE,
  SPECIES_PIDGEOT,
  SPECIES_BOMBIRDIER,
  SPECIES_TOUCANNON,
  SPECIES_STARAPTOR,
  SPECIES_MANTINE,
  SPECIES_UNFEZANT,
  SPECIES_KILOWATTREL,
  SPECIES_SIGILYPH,
  SPECIES_ALTARIA,
  SPECIES_CORVIKNIGHT,
  SPECIES_DRIFBLIM,
  SPECIES_TALONFLAME,
  SPECIES_FLAMIGO,
  SPECIES_MINIOR_CORE,
  SPECIES_HAWLUCHA,
  SPECIES_SCYTHER,
  SPECIES_HONCHKROW,
  SPECIES_MANDIBUZZ,
  SPECIES_BRAVIARY_HISUI,
  SPECIES_BRAVIARY,
  SPECIES_GLISCOR,
  SPECIES_YANMEGA,
  SPECIES_AERODACTYL,
  SPECIES_ROTOM_FAN,
  SPECIES_CHARIZARD,
  SPECIES_NOIVERN,
  SPECIES_CROBAT,
  SPECIES_GYARADOS,
  SPECIES_TOGEKISS,
  SPECIES_ARCHEOPS,
  SPECIES_IRON_JUGULIS,
  SPECIES_CELESTEELA,
  SPECIES_PIDGEOT_MEGA,
  SPECIES_ENAMORUS_THERIAN,
  SPECIES_ENAMORUS_INCARNATE,
  SPECIES_THUNDURUS_THERIAN,
  SPECIES_THUNDURUS_INCARNATE,
  SPECIES_TORNADUS_THERIAN,
  SPECIES_TORNADUS_INCARNATE,
  SPECIES_MOLTRES_GALAR,
  SPECIES_MOLTRES,
  SPECIES_ZAPDOS_GALAR,
  SPECIES_ZAPDOS,
  SPECIES_ARTICUNO_GALAR,
  SPECIES_ARTICUNO,
  SPECIES_HAWLUCHA_MEGA,
  SPECIES_LANDORUS_THERIAN,
  SPECIES_LANDORUS_INCARNATE,
  SPECIES_SHAYMIN_SKY,
  SPECIES_SALAMENCE,
  SPECIES_DRAGONITE,
  SPECIES_PINSIR_MEGA,
  SPECIES_AERODACTYL_MEGA,
  SPECIES_CHARIZARD_MEGA_Y,
  SPECIES_YVELTAL,
  SPECIES_RAYQUAZA,
  SPECIES_HO_OH,
  SPECIES_LUGIA,
  SPECIES_SALAMENCE_MEGA,
  SPECIES_DRAGONITE_MEGA,
  SPECIES_RAYQUAZA_MEGA,
};

#define RANDOM_SPECIES_POISON_COUNT ARRAY_COUNT(sRandomSpeciesPoisonBst)
static const u16 sRandomSpeciesPoisonBst[] =
{
  SPECIES_WEEDLE,
  SPECIES_KAKUNA,
  SPECIES_WOOPER_PALDEA,
  SPECIES_TOXEL,
  SPECIES_ZUBAT,
  SPECIES_SPINARAK,
  SPECIES_VENIPEDE,
  SPECIES_NIDORAN_M,
  SPECIES_NIDORAN_F,
  SPECIES_BUDEW,
  SPECIES_EKANS,
  SPECIES_SHROODLE,
  SPECIES_FOONGUS,
  SPECIES_VAROOM,
  SPECIES_CROAGUNK,
  SPECIES_BELLSPROUT,
  SPECIES_GULPIN,
  SPECIES_MAREANIE,
  SPECIES_VENONAT,
  SPECIES_GASTLY,
  SPECIES_BULBASAUR,
  SPECIES_SALANDIT,
  SPECIES_SKRELP,
  SPECIES_ODDISH,
  SPECIES_GRIMER_ALOLA,
  SPECIES_GRIMER,
  SPECIES_TRUBBISH,
  SPECIES_STUNKY,
  SPECIES_SKORUPI,
  SPECIES_TENTACOOL,
  SPECIES_KOFFING,
  SPECIES_GLIMMET,
  SPECIES_WHIRLIPEDE,
  SPECIES_NIDORINO,
  SPECIES_NIDORINA,
  SPECIES_DUSTOX,
  SPECIES_WEEPINBELL,
  SPECIES_GLOOM,
  SPECIES_BEEDRILL,
  SPECIES_ROSELIA,
  SPECIES_ARIADOS,
  SPECIES_HAUNTER,
  SPECIES_IVYSAUR,
  SPECIES_POIPOLE,
  SPECIES_CLODSIRE,
  SPECIES_SNEASEL_HISUI,
  SPECIES_QWILFISH_HISUI,
  SPECIES_QWILFISH,
  SPECIES_ARBOK,
  SPECIES_VENOMOTH,
  SPECIES_GOLBAT,
  SPECIES_SEVIPER,
  SPECIES_AMOONGUSS,
  SPECIES_SWALOT,
  SPECIES_GARBODOR,
  SPECIES_SKUNTANK,
  SPECIES_SALAZZLE,
  SPECIES_GRAFAIAI,
  SPECIES_SCOLIPEDE,
  SPECIES_TOXICROAK,
  SPECIES_SLOWKING_GALAR,
  SPECIES_WEEZING_GALAR,
  SPECIES_WEEZING,
  SPECIES_SLOWBRO_GALAR,
  SPECIES_VICTREEBEL,
  SPECIES_VILEPLUME,
  SPECIES_DRAGALGE,
  SPECIES_TOXAPEX,
  SPECIES_BEEDRILL_MEGA,
  SPECIES_REVAVROOM,
  SPECIES_DRAPION,
  SPECIES_GENGAR,
  SPECIES_MUK_ALOLA,
  SPECIES_MUK,
  SPECIES_TOXTRICITY_LOW_KEY,
  SPECIES_TOXTRICITY_AMPED,
  SPECIES_NIDOKING,
  SPECIES_NIDOQUEEN,
  SPECIES_OVERQWIL,
  SPECIES_SNEASLER,
  SPECIES_ROSERADE,
  SPECIES_TENTACRUEL,
  SPECIES_GLIMMORA,
  SPECIES_VENUSAUR,
  SPECIES_CROBAT,
  SPECIES_NAGANADEL,
  SPECIES_FEZANDIPITI,
  SPECIES_MUNKIDORI,
  SPECIES_OKIDOGI,
  SPECIES_IRON_MOTH,
  SPECIES_NIHILEGO,
  SPECIES_VICTREEBEL_MEGA,
  SPECIES_PECHARUNT,
  SPECIES_GENGAR_MEGA,
  SPECIES_VENUSAUR_MEGA,
  SPECIES_ETERNATUS,
};

#define RANDOM_SPECIES_GROUND_COUNT ARRAY_COUNT(sRandomSpeciesGroundBst)
static const u16 sRandomSpeciesGroundBst[] =
{
  SPECIES_WOOPER_PALDEA,
  SPECIES_WOOPER,
  SPECIES_SWINUB,
  SPECIES_DIGLETT_ALOLA,
  SPECIES_DIGLETT,
  SPECIES_NINCADA,
  SPECIES_BARBOACH,
  SPECIES_TRAPINCH,
  SPECIES_SANDILE,
  SPECIES_GIBLE,
  SPECIES_BALTOY,
  SPECIES_LARVITAR,
  SPECIES_GEODUDE,
  SPECIES_SANDSHREW,
  SPECIES_GOLETT,
  SPECIES_YAMASK_GALAR,
  SPECIES_NUMEL,
  SPECIES_SILICOBRA,
  SPECIES_SANDYGAST,
  SPECIES_CUBONE,
  SPECIES_DRILBUR,
  SPECIES_HIPPOPOTAS,
  SPECIES_PHANPY,
  SPECIES_TOEDSCOOL,
  SPECIES_VIBRAVA,
  SPECIES_RHYHORN,
  SPECIES_KROKOROK,
  SPECIES_PALPITOAD,
  SPECIES_MUDBRAY,
  SPECIES_ONIX,
  SPECIES_GRAVELER,
  SPECIES_MARSHTOMP,
  SPECIES_GABITE,
  SPECIES_PUPITAR,
  SPECIES_DIGGERSBY,
  SPECIES_WORMADAM_SANDY,
  SPECIES_MAROWAK,
  SPECIES_DUGTRIO_ALOLA,
  SPECIES_DUGTRIO,
  SPECIES_CLODSIRE,
  SPECIES_GLIGAR,
  SPECIES_QUAGSIRE,
  SPECIES_PILOSWINE,
  SPECIES_SANDSLASH,
  SPECIES_CAMERUPT,
  SPECIES_WHISCASH,
  SPECIES_STUNFISK_GALAR,
  SPECIES_STUNFISK,
  SPECIES_GASTRODON,
  SPECIES_PALOSSAND,
  SPECIES_RUNERIGUS,
  SPECIES_GOLURK,
  SPECIES_RHYDON,
  SPECIES_ZYGARDE_10_POWER_CONSTRUCT,
  SPECIES_GOLEM,
  SPECIES_MUDSDALE,
  SPECIES_CLAYDOL,
  SPECIES_DONPHAN,
  SPECIES_NIDOKING,
  SPECIES_NIDOQUEEN,
  SPECIES_EXCADRILL,
  SPECIES_SEISMITOAD,
  SPECIES_SANDACONDA,
  SPECIES_GLISCOR,
  SPECIES_STEELIX,
  SPECIES_TOEDSCRUEL,
  SPECIES_KROOKODILE,
  SPECIES_FLYGON,
  SPECIES_HIPPOWDON,
  SPECIES_TORTERRA,
  SPECIES_MAMOSWINE,
  SPECIES_RHYPERIOR,
  SPECIES_SWAMPERT,
  SPECIES_URSALUNA,
  SPECIES_URSALUNA_BLOODMOON,
  SPECIES_CAMERUPT_MEGA,
  SPECIES_TING_LU,
  SPECIES_IRON_TREADS,
  SPECIES_SANDY_SHOCKS,
  SPECIES_GREAT_TUSK,
  SPECIES_ZYGARDE_50_POWER_CONSTRUCT,
  SPECIES_LANDORUS_THERIAN,
  SPECIES_LANDORUS_INCARNATE,
  SPECIES_GARCHOMP,
  SPECIES_STEELIX_MEGA,
  SPECIES_SWAMPERT_MEGA,
  SPECIES_GROUDON,
  SPECIES_GARCHOMP_MEGA,
  SPECIES_GROUDON_PRIMAL,
};

#define RANDOM_SPECIES_ROCK_COUNT ARRAY_COUNT(sRandomSpeciesRockBst)
static const u16 sRandomSpeciesRockBst[] =
{
  SPECIES_ROLYCOLY,
  SPECIES_NACLI,
  SPECIES_ROCKRUFF_OWN_TEMPO,
  SPECIES_ROCKRUFF,
  SPECIES_ROGGENROLA,
  SPECIES_BONSLY,
  SPECIES_LARVITAR,
  SPECIES_GEODUDE_ALOLA,
  SPECIES_GEODUDE,
  SPECIES_BINACLE,
  SPECIES_DWEBBLE,
  SPECIES_ARON,
  SPECIES_RHYHORN,
  SPECIES_GLIMMET,
  SPECIES_SHIELDON,
  SPECIES_CRANIDOS,
  SPECIES_GROWLITHE_HISUI,
  SPECIES_NACLSTACK,
  SPECIES_TIRTOUGA,
  SPECIES_ANORITH,
  SPECIES_LILEEP,
  SPECIES_KABUTO,
  SPECIES_OMANYTE,
  SPECIES_AMAURA,
  SPECIES_TYRUNT,
  SPECIES_NOSEPASS,
  SPECIES_ONIX,
  SPECIES_BOLDORE,
  SPECIES_GRAVELER_ALOLA,
  SPECIES_GRAVELER,
  SPECIES_ARCHEN,
  SPECIES_CARKOL,
  SPECIES_PUPITAR,
  SPECIES_CORSOLA,
  SPECIES_SUDOWOODO,
  SPECIES_LAIRON,
  SPECIES_MAGCARGO,
  SPECIES_MINIOR_METEOR,
  SPECIES_KLAWF,
  SPECIES_SOLROCK,
  SPECIES_LUNATONE,
  SPECIES_STONJOURNER,
  SPECIES_DREDNAW,
  SPECIES_CRUSTLE,
  SPECIES_RELICANTH,
  SPECIES_RHYDON,
  SPECIES_LYCANROC_DUSK,
  SPECIES_LYCANROC_MIDNIGHT,
  SPECIES_LYCANROC_MIDDAY,
  SPECIES_CARRACOSTA,
  SPECIES_BASTIODON,
  SPECIES_RAMPARDOS,
  SPECIES_ARMALDO,
  SPECIES_CRADILY,
  SPECIES_KABUTOPS,
  SPECIES_OMASTAR,
  SPECIES_GOLEM_ALOLA,
  SPECIES_GOLEM,
  SPECIES_GARGANACL,
  SPECIES_KLEAVOR,
  SPECIES_MINIOR_CORE,
  SPECIES_CARBINK,
  SPECIES_BARBARACLE,
  SPECIES_SHUCKLE,
  SPECIES_COALOSSAL,
  SPECIES_AVALUGG_HISUI,
  SPECIES_GIGALITH,
  SPECIES_AERODACTYL,
  SPECIES_AURORUS,
  SPECIES_TYRANTRUM,
  SPECIES_GLIMMORA,
  SPECIES_PROBOPASS,
  SPECIES_AGGRON,
  SPECIES_RHYPERIOR,
  SPECIES_OGERPON_CORNERSTONE,
  SPECIES_ARCANINE_HISUI,
  SPECIES_ARCHEOPS,
  SPECIES_IRON_THORNS,
  SPECIES_STAKATAKA,
  SPECIES_NIHILEGO,
  SPECIES_TERRAKION,
  SPECIES_REGIROCK,
  SPECIES_IRON_BOULDER,
  SPECIES_DIANCIE,
  SPECIES_TYRANITAR,
  SPECIES_AERODACTYL_MEGA,
  SPECIES_DIANCIE_MEGA,
  SPECIES_TYRANITAR_MEGA,
};

#define RANDOM_SPECIES_BUG_COUNT ARRAY_COUNT(sRandomSpeciesBugBst)
static const u16 sRandomSpeciesBugBst[] =
{
  SPECIES_BLIPBUG,
  SPECIES_SNOM,
  SPECIES_KRICKETOT,
  SPECIES_WURMPLE,
  SPECIES_WEEDLE,
  SPECIES_CATERPIE,
  SPECIES_SCATTERBUG,
  SPECIES_CASCOON,
  SPECIES_SILCOON,
  SPECIES_KAKUNA,
  SPECIES_METAPOD,
  SPECIES_NYMBLE,
  SPECIES_TAROUNTULA,
  SPECIES_SPEWPA,
  SPECIES_BURMY_TRASH,
  SPECIES_BURMY_SANDY,
  SPECIES_BURMY_PLANT,
  SPECIES_WIMPOD,
  SPECIES_SHEDINJA,
  SPECIES_COMBEE,
  SPECIES_SPINARAK,
  SPECIES_VENIPEDE,
  SPECIES_LEDYBA,
  SPECIES_NINCADA,
  SPECIES_DEWPIDER,
  SPECIES_SURSKIT,
  SPECIES_RELLOR,
  SPECIES_PARAS,
  SPECIES_PINECO,
  SPECIES_GRUBBIN,
  SPECIES_CUTIEFLY,
  SPECIES_SIZZLIPEDE,
  SPECIES_SHELMET,
  SPECIES_VENONAT,
  SPECIES_SEWADDLE,
  SPECIES_KARRABLAST,
  SPECIES_JOLTIK,
  SPECIES_DWEBBLE,
  SPECIES_SKORUPI,
  SPECIES_DOTTLER,
  SPECIES_ANORITH,
  SPECIES_LARVESTA,
  SPECIES_WHIRLIPEDE,
  SPECIES_SWADLOON,
  SPECIES_KRICKETUNE,
  SPECIES_DUSTOX,
  SPECIES_YANMA,
  SPECIES_LEDIAN,
  SPECIES_BEAUTIFLY,
  SPECIES_BEEDRILL,
  SPECIES_BUTTERFREE,
  SPECIES_CHARJABUG,
  SPECIES_ARIADOS,
  SPECIES_SPIDOPS,
  SPECIES_PARASECT,
  SPECIES_VIVILLON,
  SPECIES_MOTHIM,
  SPECIES_WORMADAM_TRASH,
  SPECIES_WORMADAM_SANDY,
  SPECIES_WORMADAM_PLANT,
  SPECIES_ILLUMISE,
  SPECIES_VOLBEAT,
  SPECIES_LOKIX,
  SPECIES_VENOMOTH,
  SPECIES_ARAQUANID,
  SPECIES_MASQUERAIN,
  SPECIES_NINJASK,
  SPECIES_RIBOMBEE,
  SPECIES_FORRETRESS,
  SPECIES_RABSCA,
  SPECIES_GALVANTULA,
  SPECIES_VESPIQUEN,
  SPECIES_FROSMOTH,
  SPECIES_DURANT,
  SPECIES_CRUSTLE,
  SPECIES_SCOLIPEDE,
  SPECIES_ACCELGOR,
  SPECIES_ESCAVALIER,
  SPECIES_ARMALDO,
  SPECIES_BEEDRILL_MEGA,
  SPECIES_KLEAVOR,
  SPECIES_VIKAVOLT,
  SPECIES_LEAVANNY,
  SPECIES_HERACROSS,
  SPECIES_SCIZOR,
  SPECIES_PINSIR,
  SPECIES_SCYTHER,
  SPECIES_ORBEETLE,
  SPECIES_SHUCKLE,
  SPECIES_YANMEGA,
  SPECIES_CENTISKORCH,
  SPECIES_GOLISOPOD,
  SPECIES_VOLCARONA,
  SPECIES_SLITHER_WING,
  SPECIES_PHEROMOSA,
  SPECIES_BUZZWOLE,
  SPECIES_GENESECT,
  SPECIES_HERACROSS_MEGA,
  SPECIES_SCIZOR_MEGA,
  SPECIES_PINSIR_MEGA,
};

#define RANDOM_SPECIES_GHOST_COUNT ARRAY_COUNT(sRandomSpeciesGhostBst)
static const u16 sRandomSpeciesGhostBst[] =
{
  SPECIES_DREEPY,
  SPECIES_BRAMBLIN,
  SPECIES_LITWICK,
  SPECIES_GREAVARD,
  SPECIES_DUSKULL,
  SPECIES_SHUPPET,
  SPECIES_GIMMIGHOUL_ROAMING,
  SPECIES_GIMMIGHOUL_CHEST,
  SPECIES_GOLETT,
  SPECIES_YAMASK_GALAR,
  SPECIES_YAMASK,
  SPECIES_POLTCHAGEIST,
  SPECIES_SINISTEA,
  SPECIES_PHANTUMP,
  SPECIES_GASTLY,
  SPECIES_SANDYGAST,
  SPECIES_HONEDGE,
  SPECIES_ZORUA_HISUI,
  SPECIES_PUMPKABOO_SUPER,
  SPECIES_PUMPKABOO_LARGE,
  SPECIES_PUMPKABOO_SMALL,
  SPECIES_PUMPKABOO_AVERAGE,
  SPECIES_FRILLISH,
  SPECIES_DRIFLOON,
  SPECIES_LAMPENT,
  SPECIES_SABLEYE,
  SPECIES_HAUNTER,
  SPECIES_DRAKLOAK,
  SPECIES_CORSOLA_GALAR,
  SPECIES_MAROWAK_ALOLA,
  SPECIES_MISDREAVUS,
  SPECIES_ROTOM,
  SPECIES_DOUBLADE,
  SPECIES_DUSCLOPS,
  SPECIES_BANETTE,
  SPECIES_TREVENANT,
  SPECIES_MIMIKYU,
  SPECIES_ORICORIO_SENSU,
  SPECIES_BRAMBLEGHAST,
  SPECIES_PALOSSAND,
  SPECIES_JELLICENT,
  SPECIES_FROSLASS,
  SPECIES_SABLEYE_MEGA,
  SPECIES_RUNERIGUS,
  SPECIES_GOLURK,
  SPECIES_COFAGRIGUS,
  SPECIES_SPIRITOMB,
  SPECIES_HOUNDSTONE,
  SPECIES_GOURGEIST_AVERAGE,
  SPECIES_MISMAGIUS,
  SPECIES_DRIFBLIM,
  SPECIES_AEGISLASH_BLADE,
  SPECIES_AEGISLASH_SHIELD,
  SPECIES_GENGAR,
  SPECIES_SINISTCHA,
  SPECIES_POLTEAGEIST,
  SPECIES_CURSOLA,
  SPECIES_ZOROARK_HISUI,
  SPECIES_DHELMISE,
  SPECIES_CHANDELURE,
  SPECIES_CERULEDGE,
  SPECIES_DUSKNOIR,
  SPECIES_SKELEDIRGE,
  SPECIES_BASCULEGION_F,
  SPECIES_BASCULEGION_M,
  SPECIES_DECIDUEYE,
  SPECIES_TYPHLOSION_HISUI,
  SPECIES_ANNIHILAPE,
  SPECIES_GHOLDENGO,
  SPECIES_BANETTE_MEGA,
  SPECIES_FLUTTER_MANE,
  SPECIES_BLACEPHALON,
  SPECIES_SPECTRIER,
  SPECIES_PECHARUNT,
  SPECIES_DRAGAPULT,
  SPECIES_MARSHADOW,
  SPECIES_HOOPA_CONFINED,
  SPECIES_GENGAR_MEGA,
  SPECIES_CALYREX_SHADOW,
  SPECIES_NECROZMA_DAWN_WINGS,
  SPECIES_LUNALA,
  SPECIES_GIRATINA_ORIGIN,
  SPECIES_GIRATINA_ALTERED,
};

#define RANDOM_SPECIES_STEEL_COUNT ARRAY_COUNT(sRandomSpeciesSteelBst)
static const u16 sRandomSpeciesSteelBst[] =
{
  SPECIES_DIGLETT_ALOLA,
  SPECIES_MEOWTH_GALAR,
  SPECIES_TINKATINK,
  SPECIES_VAROOM,
  SPECIES_MELTAN,
  SPECIES_KLINK,
  SPECIES_BRONZOR,
  SPECIES_BELDUM,
  SPECIES_SANDSHREW_ALOLA,
  SPECIES_FERROSEED,
  SPECIES_HONEDGE,
  SPECIES_MAGNEMITE,
  SPECIES_CUFANT,
  SPECIES_ARON,
  SPECIES_PAWNIARD,
  SPECIES_SHIELDON,
  SPECIES_TINKATUFF,
  SPECIES_MAWILE,
  SPECIES_METANG,
  SPECIES_WORMADAM_TRASH,
  SPECIES_DUGTRIO_ALOLA,
  SPECIES_LAIRON,
  SPECIES_TOGEDEMARU,
  SPECIES_PERRSERKER,
  SPECIES_KLANG,
  SPECIES_DOUBLADE,
  SPECIES_SANDSLASH_ALOLA,
  SPECIES_SLIGGOO_HISUI,
  SPECIES_SKARMORY,
  SPECIES_FORRETRESS,
  SPECIES_MAGNETON,
  SPECIES_KLEFKI,
  SPECIES_STUNFISK_GALAR,
  SPECIES_ORTHWORM,
  SPECIES_MAWILE_MEGA,
  SPECIES_DURANT,
  SPECIES_FERROTHORN,
  SPECIES_BISHARP,
  SPECIES_CORVIKNIGHT,
  SPECIES_ESCAVALIER,
  SPECIES_BASTIODON,
  SPECIES_REVAVROOM,
  SPECIES_COPPERAJAH,
  SPECIES_AEGISLASH_BLADE,
  SPECIES_AEGISLASH_SHIELD,
  SPECIES_BRONZONG,
  SPECIES_SCIZOR,
  SPECIES_TINKATON,
  SPECIES_EXCADRILL,
  SPECIES_STEELIX,
  SPECIES_KLINKLANG,
  SPECIES_PROBOPASS,
  SPECIES_LUCARIO,
  SPECIES_EMPOLEON,
  SPECIES_AGGRON,
  SPECIES_DURALUDON,
  SPECIES_MAGNEZONE,
  SPECIES_GHOLDENGO,
  SPECIES_KINGAMBIT,
  SPECIES_IRON_TREADS,
  SPECIES_STAKATAKA,
  SPECIES_KARTANA,
  SPECIES_CELESTEELA,
  SPECIES_COBALION,
  SPECIES_REGISTEEL,
  SPECIES_IRON_CROWN,
  SPECIES_ARCHALUDON,
  SPECIES_MELMETAL,
  SPECIES_MAGEARNA,
  SPECIES_GOODRA_HISUI,
  SPECIES_GENESECT,
  SPECIES_HEATRAN,
  SPECIES_JIRACHI,
  SPECIES_METAGROSS,
  SPECIES_SCIZOR_MEGA,
  SPECIES_STEELIX_MEGA,
  SPECIES_LUCARIO_MEGA,
  SPECIES_AGGRON_MEGA,
  SPECIES_NECROZMA_DUSK_MANE,
  SPECIES_SOLGALEO,
  SPECIES_DIALGA_ORIGIN,
  SPECIES_DIALGA,
  SPECIES_ZAMAZENTA_CROWNED,
  SPECIES_ZACIAN_CROWNED,
  SPECIES_METAGROSS_MEGA,
};

#define RANDOM_SPECIES_FIRE_COUNT ARRAY_COUNT(sRandomSpeciesFireBst)
static const u16 sRandomSpeciesFireBst[] =
{
  SPECIES_SLUGMA,
  SPECIES_CHARCADET,
  SPECIES_LITWICK,
  SPECIES_VULPIX,
  SPECIES_SIZZLIPEDE,
  SPECIES_NUMEL,
  SPECIES_FENNEKIN,
  SPECIES_TEPIG,
  SPECIES_CHIMCHAR,
  SPECIES_CYNDAQUIL,
  SPECIES_CHARMANDER,
  SPECIES_FUECOCO,
  SPECIES_SCORBUNNY,
  SPECIES_TORCHIC,
  SPECIES_DARUMAKA,
  SPECIES_PANSEAR,
  SPECIES_SALANDIT,
  SPECIES_LITTEN,
  SPECIES_HOUNDOUR,
  SPECIES_GROWLITHE_HISUI,
  SPECIES_GROWLITHE,
  SPECIES_LARVESTA,
  SPECIES_MAGBY,
  SPECIES_LITLEO,
  SPECIES_LAMPENT,
  SPECIES_FLETCHINDER,
  SPECIES_MONFERNO,
  SPECIES_COMBUSKEN,
  SPECIES_QUILAVA,
  SPECIES_CHARMELEON,
  SPECIES_BRAIXEN,
  SPECIES_CARKOL,
  SPECIES_PONYTA,
  SPECIES_CROCALOR,
  SPECIES_PIGNITE,
  SPECIES_RABOOT,
  SPECIES_TORRACAT,
  SPECIES_CASTFORM_SUNNY,
  SPECIES_MAROWAK_ALOLA,
  SPECIES_MAGCARGO,
  SPECIES_CAMERUPT,
  SPECIES_TORKOAL,
  SPECIES_ORICORIO_BAILE,
  SPECIES_SALAZZLE,
  SPECIES_DARMANITAN_STANDARD,
  SPECIES_HEATMOR,
  SPECIES_TURTONATOR,
  SPECIES_SCOVILLAIN,
  SPECIES_TAUROS_PALDEA_BLAZE,
  SPECIES_MAGMAR,
  SPECIES_SIMISEAR,
  SPECIES_TALONFLAME,
  SPECIES_HOUNDOOM,
  SPECIES_RAPIDASH,
  SPECIES_NINETALES,
  SPECIES_PYROAR,
  SPECIES_COALOSSAL,
  SPECIES_CHANDELURE,
  SPECIES_ROTOM_HEAT,
  SPECIES_CERULEDGE,
  SPECIES_ARMAROUGE,
  SPECIES_CENTISKORCH,
  SPECIES_FLAREON,
  SPECIES_EMBOAR,
  SPECIES_SKELEDIRGE,
  SPECIES_CINDERACE,
  SPECIES_INCINEROAR,
  SPECIES_BLAZIKEN,
  SPECIES_DELPHOX,
  SPECIES_INFERNAPE,
  SPECIES_TYPHLOSION_HISUI,
  SPECIES_TYPHLOSION,
  SPECIES_CHARIZARD,
  SPECIES_MAGMORTAR,
  SPECIES_OGERPON_HEARTHFLAME,
  SPECIES_VOLCARONA,
  SPECIES_ARCANINE_HISUI,
  SPECIES_ARCANINE,
  SPECIES_CAMERUPT_MEGA,
  SPECIES_CHI_YU,
  SPECIES_IRON_MOTH,
  SPECIES_BLACEPHALON,
  SPECIES_ENTEI,
  SPECIES_MOLTRES,
  SPECIES_GOUGING_FIRE,
  SPECIES_VOLCANION,
  SPECIES_VICTINI,
  SPECIES_HEATRAN,
  SPECIES_HOUNDOOM_MEGA,
  SPECIES_BLAZIKEN_MEGA,
  SPECIES_CHARIZARD_MEGA_Y,
  SPECIES_CHARIZARD_MEGA_X,
  SPECIES_RESHIRAM,
  SPECIES_HO_OH,
  SPECIES_GROUDON_PRIMAL,
};

#define RANDOM_SPECIES_WATER_COUNT ARRAY_COUNT(sRandomSpeciesWaterBst)
static const u16 sRandomSpeciesWaterBst[] =
{
  SPECIES_WISHIWASHI_SOLO,
  SPECIES_FEEBAS,
  SPECIES_MAGIKARP,
  SPECIES_WOOPER,
  SPECIES_LOTAD,
  SPECIES_WIMPOD,
  SPECIES_WIGLETT,
  SPECIES_MARILL,
  SPECIES_DEWPIDER,
  SPECIES_SURSKIT,
  SPECIES_WINGULL,
  SPECIES_ARROKUDA,
  SPECIES_CHEWTLE,
  SPECIES_BARBOACH,
  SPECIES_SPHEAL,
  SPECIES_TYMPOLE,
  SPECIES_HORSEA,
  SPECIES_REMORAID,
  SPECIES_POLIWAG,
  SPECIES_MAREANIE,
  SPECIES_DUCKLETT,
  SPECIES_CARVANHA,
  SPECIES_SHELLDER,
  SPECIES_BINACLE,
  SPECIES_OSHAWOTT,
  SPECIES_CORPHISH,
  SPECIES_QUAXLY,
  SPECIES_SOBBLE,
  SPECIES_MUDKIP,
  SPECIES_FROAKIE,
  SPECIES_PIPLUP,
  SPECIES_TOTODILE,
  SPECIES_SQUIRTLE,
  SPECIES_FINIZEN,
  SPECIES_SLOWPOKE,
  SPECIES_PANPOUR,
  SPECIES_POPPLIO,
  SPECIES_SKRELP,
  SPECIES_GOLDEEN,
  SPECIES_PSYDUCK,
  SPECIES_SHELLOS,
  SPECIES_KRABBY,
  SPECIES_SEEL,
  SPECIES_CLAUNCHER,
  SPECIES_FINNEON,
  SPECIES_BUIZEL,
  SPECIES_LUVDISC,
  SPECIES_CHINCHOU,
  SPECIES_FRILLISH,
  SPECIES_TENTACOOL,
  SPECIES_LOMBRE,
  SPECIES_STARYU,
  SPECIES_MANTYKE,
  SPECIES_CLAMPERL,
  SPECIES_TIRTOUGA,
  SPECIES_KABUTO,
  SPECIES_OMANYTE,
  SPECIES_PALPITOAD,
  SPECIES_POLIWHIRL,
  SPECIES_WAILMER,
  SPECIES_FROGADIER,
  SPECIES_PRINPLUP,
  SPECIES_MARSHTOMP,
  SPECIES_CROCONAW,
  SPECIES_WARTORTLE,
  SPECIES_QUAXWELL,
  SPECIES_PYUKUMUKU,
  SPECIES_BIBAREL,
  SPECIES_SEALEO,
  SPECIES_CORSOLA,
  SPECIES_DEWOTT,
  SPECIES_DRIZZILE,
  SPECIES_BRIONNE,
  SPECIES_CASTFORM_RAINY,
  SPECIES_AZUMARILL,
  SPECIES_WUGTRIO,
  SPECIES_QUAGSIRE,
  SPECIES_PELIPPER,
  SPECIES_QWILFISH,
  SPECIES_SEADRA,
  SPECIES_SEAKING,
  SPECIES_ARAQUANID,
  SPECIES_PALAFIN_ZERO,
  SPECIES_BASCULIN,
  SPECIES_LUMINEON,
  SPECIES_SHARPEDO,
  SPECIES_LANTURN,
  SPECIES_CRAWDAUNT,
  SPECIES_WHISCASH,
  SPECIES_ALOMOMOLA,
  SPECIES_SWANNA,
  SPECIES_TATSUGIRI,
  SPECIES_CRAMORANT,
  SPECIES_BRUXISH,
  SPECIES_GASTRODON,
  SPECIES_KINGLER,
  SPECIES_DEWGONG,
  SPECIES_VELUZA,
  SPECIES_JELLICENT,
  SPECIES_PHIONE,
  SPECIES_LUDICOLO,
  SPECIES_OCTILLERY,
  SPECIES_DREDNAW,
  SPECIES_RELICANTH,
  SPECIES_GOREBYSS,
  SPECIES_HUNTAIL,
  SPECIES_MANTINE,
  SPECIES_BARRASKEWDA,
  SPECIES_SLOWKING,
  SPECIES_TAUROS_PALDEA_AQUA,
  SPECIES_SLOWBRO,
  SPECIES_TOXAPEX,
  SPECIES_CARRACOSTA,
  SPECIES_FLOATZEL,
  SPECIES_KABUTOPS,
  SPECIES_OMASTAR,
  SPECIES_SIMIPOUR,
  SPECIES_CLAWITZER,
  SPECIES_BARBARACLE,
  SPECIES_WAILORD,
  SPECIES_POLITOED,
  SPECIES_GOLDUCK,
  SPECIES_ARCTOVISH,
  SPECIES_DRACOVISH,
  SPECIES_SEISMITOAD,
  SPECIES_POLIWRATH,
  SPECIES_TENTACRUEL,
  SPECIES_ROTOM_WASH,
  SPECIES_STARMIE,
  SPECIES_VAPOREON,
  SPECIES_CLOYSTER,
  SPECIES_SAMUROTT_HISUI,
  SPECIES_SAMUROTT,
  SPECIES_DONDOZO,
  SPECIES_QUAQUAVAL,
  SPECIES_BASCULEGION_F,
  SPECIES_BASCULEGION_M,
  SPECIES_INTELEON,
  SPECIES_GOLISOPOD,
  SPECIES_PRIMARINA,
  SPECIES_GRENINJA,
  SPECIES_EMPOLEON,
  SPECIES_WALREIN,
  SPECIES_FERALIGATR,
  SPECIES_BLASTOISE,
  SPECIES_SWAMPERT,
  SPECIES_LAPRAS,
  SPECIES_MILOTIC,
  SPECIES_KINGDRA,
  SPECIES_GYARADOS,
  SPECIES_OGERPON_WELLSPRING,
  SPECIES_URSHIFU_RAPID_STRIKE,
  SPECIES_SHARPEDO_MEGA,
  SPECIES_IRON_BUNDLE,
  SPECIES_TAPU_FINI,
  SPECIES_KELDEO_RESOLUTE,
  SPECIES_KELDEO_ORDINARY,
  SPECIES_SUICUNE,
  SPECIES_WALKING_WAKE,
  SPECIES_SLOWBRO_MEGA,
  SPECIES_VOLCANION,
  SPECIES_MANAPHY,
  SPECIES_WISHIWASHI_SCHOOL,
  SPECIES_BLASTOISE_MEGA,
  SPECIES_SWAMPERT_MEGA,
  SPECIES_GRENINJA_ASH,
  SPECIES_GYARADOS_MEGA,
  SPECIES_PALAFIN_HERO,
  SPECIES_KYOGRE,
  SPECIES_PALKIA_ORIGIN,
  SPECIES_PALKIA,
  SPECIES_KYOGRE_PRIMAL,
};

#define RANDOM_SPECIES_GRASS_COUNT ARRAY_COUNT(sRandomSpeciesGrassBst)
static const u16 sRandomSpeciesGrassBst[] =
{
  SPECIES_SUNKERN,
  SPECIES_BOUNSWEET,
  SPECIES_SEEDOT,
  SPECIES_LOTAD,
  SPECIES_GOSSIFLEUR,
  SPECIES_FOMANTIS,
  SPECIES_HOPPIP,
  SPECIES_SMOLIV,
  SPECIES_APPLIN,
  SPECIES_BRAMBLIN,
  SPECIES_CHERUBI,
  SPECIES_PETILIL,
  SPECIES_COTTONEE,
  SPECIES_BUDEW,
  SPECIES_MORELULL,
  SPECIES_PARAS,
  SPECIES_STEENEE,
  SPECIES_FOONGUS,
  SPECIES_SHROOMISH,
  SPECIES_BELLSPROUT,
  SPECIES_CAPSAKID,
  SPECIES_FERROSEED,
  SPECIES_POLTCHAGEIST,
  SPECIES_SNIVY,
  SPECIES_PHANTUMP,
  SPECIES_SPRIGATITO,
  SPECIES_GROOKEY,
  SPECIES_SEWADDLE,
  SPECIES_TREECKO,
  SPECIES_CHESPIN,
  SPECIES_PANSAGE,
  SPECIES_TURTWIG,
  SPECIES_CHIKORITA,
  SPECIES_BULBASAUR,
  SPECIES_ROWLET,
  SPECIES_ODDISH,
  SPECIES_EXEGGCUTE,
  SPECIES_VOLTORB_HISUI,
  SPECIES_SNOVER,
  SPECIES_TOEDSCOOL,
  SPECIES_PUMPKABOO_AVERAGE,
  SPECIES_DEERLING,
  SPECIES_CACNEA,
  SPECIES_NUZLEAF,
  SPECIES_LOMBRE,
  SPECIES_SKIPLOOM,
  SPECIES_SKIDDO,
  SPECIES_DOLLIV,
  SPECIES_LILEEP,
  SPECIES_SWADLOON,
  SPECIES_WEEPINBELL,
  SPECIES_GLOOM,
  SPECIES_ROSELIA,
  SPECIES_SHIINOTIC,
  SPECIES_QUILLADIN,
  SPECIES_GROTLE,
  SPECIES_GROVYLE,
  SPECIES_BAYLEEF,
  SPECIES_PARASECT,
  SPECIES_IVYSAUR,
  SPECIES_FLORAGATO,
  SPECIES_SERVINE,
  SPECIES_THWACKEY,
  SPECIES_DARTRIX,
  SPECIES_WORMADAM_PLANT,
  SPECIES_SUNFLORA,
  SPECIES_TANGELA,
  SPECIES_CHERRIM,
  SPECIES_CARNIVINE,
  SPECIES_ELDEGOSS,
  SPECIES_TROPIUS,
  SPECIES_BRELOOM,
  SPECIES_JUMPLUFF,
  SPECIES_MARACTUS,
  SPECIES_AMOONGUSS,
  SPECIES_TREVENANT,
  SPECIES_SAWSBUCK,
  SPECIES_CACTURNE,
  SPECIES_BRAMBLEGHAST,
  SPECIES_LURANTIS,
  SPECIES_LILLIGANT_HISUI,
  SPECIES_LILLIGANT,
  SPECIES_WHIMSICOTT,
  SPECIES_SHIFTRY,
  SPECIES_LUDICOLO,
  SPECIES_DIPPLIN,
  SPECIES_APPLETUN,
  SPECIES_FLAPPLE,
  SPECIES_SCOVILLAIN,
  SPECIES_FERROTHORN,
  SPECIES_BELLOSSOM,
  SPECIES_ELECTRODE_HISUI,
  SPECIES_VICTREEBEL,
  SPECIES_VILEPLUME,
  SPECIES_GOURGEIST_AVERAGE,
  SPECIES_ABOMASNOW,
  SPECIES_CRADILY,
  SPECIES_SIMISAGE,
  SPECIES_CALYREX,
  SPECIES_LEAVANNY,
  SPECIES_SINISTCHA,
  SPECIES_ARBOLIVA,
  SPECIES_TSAREENA,
  SPECIES_TOEDSCRUEL,
  SPECIES_ROSERADE,
  SPECIES_DHELMISE,
  SPECIES_ROTOM_MOW,
  SPECIES_LEAFEON,
  SPECIES_TORTERRA,
  SPECIES_MEGANIUM,
  SPECIES_VENUSAUR,
  SPECIES_SERPERIOR,
  SPECIES_MEOWSCARADA,
  SPECIES_RILLABOOM,
  SPECIES_DECIDUEYE_HISUI,
  SPECIES_DECIDUEYE,
  SPECIES_CHESNAUGHT,
  SPECIES_SCEPTILE,
  SPECIES_EXEGGUTOR_ALOLA,
  SPECIES_EXEGGUTOR,
  SPECIES_GOGOAT,
  SPECIES_TANGROWTH,
  SPECIES_HYDRAPPLE,
  SPECIES_OGERPON_CORNERSTONE,
  SPECIES_OGERPON_HEARTHFLAME,
  SPECIES_OGERPON_WELLSPRING,
  SPECIES_OGERPON_TEAL,
  SPECIES_WO_CHIEN,
  SPECIES_BRUTE_BONNET,
  SPECIES_KARTANA,
  SPECIES_TAPU_BULU,
  SPECIES_VIRIZION,
  SPECIES_IRON_LEAVES,
  SPECIES_VICTREEBEL_MEGA,
  SPECIES_ABOMASNOW_MEGA,
  SPECIES_ZARUDE,
  SPECIES_SHAYMIN_SKY,
  SPECIES_SHAYMIN_LAND,
  SPECIES_CELEBI,
  SPECIES_VENUSAUR_MEGA,
  SPECIES_SCEPTILE_MEGA,
};

#define RANDOM_SPECIES_ELECTRIC_COUNT ARRAY_COUNT(sRandomSpeciesElectricBst)
static const u16 sRandomSpeciesElectricBst[] =
{
  SPECIES_PICHU,
  SPECIES_PAWMI,
  SPECIES_TOXEL,
  SPECIES_SHINX,
  SPECIES_YAMPER,
  SPECIES_TADBULB,
  SPECIES_TYNAMO,
  SPECIES_WATTREL,
  SPECIES_MAREEP,
  SPECIES_HELIOPTILE,
  SPECIES_BLITZLE,
  SPECIES_ELECTRIKE,
  SPECIES_GEODUDE_ALOLA,
  SPECIES_JOLTIK,
  SPECIES_PIKACHU,
  SPECIES_MAGNEMITE,
  SPECIES_CHINCHOU,
  SPECIES_VOLTORB_HISUI,
  SPECIES_VOLTORB,
  SPECIES_PAWMO,
  SPECIES_ELEKID,
  SPECIES_LUXIO,
  SPECIES_FLAAFFY,
  SPECIES_GRAVELER_ALOLA,
  SPECIES_CHARJABUG,
  SPECIES_EELEKTRIK,
  SPECIES_PACHIRISU,
  SPECIES_MINUN,
  SPECIES_PLUSLE,
  SPECIES_EMOLGA,
  SPECIES_PIKACHU_PARTNER,
  SPECIES_DEDENNE,
  SPECIES_PINCURCHIN,
  SPECIES_TOGEDEMARU,
  SPECIES_MORPEKO_HANGRY,
  SPECIES_MORPEKO_FULL_BELLY,
  SPECIES_ROTOM,
  SPECIES_LANTURN,
  SPECIES_MAGNETON,
  SPECIES_STUNFISK,
  SPECIES_GALVANTULA,
  SPECIES_MANECTRIC,
  SPECIES_ORICORIO_POM_POM,
  SPECIES_HELIOLISK,
  SPECIES_RAICHU_ALOLA,
  SPECIES_RAICHU,
  SPECIES_KILOWATTREL,
  SPECIES_PAWMOT,
  SPECIES_BOLTUND,
  SPECIES_ELECTABUZZ,
  SPECIES_ELECTRODE_HISUI,
  SPECIES_ELECTRODE,
  SPECIES_BELLIBOLT,
  SPECIES_GOLEM_ALOLA,
  SPECIES_ZEBSTRIKA,
  SPECIES_VIKAVOLT,
  SPECIES_TOXTRICITY_LOW_KEY,
  SPECIES_TOXTRICITY_AMPED,
  SPECIES_ARCTOZOLT,
  SPECIES_DRACOZOLT,
  SPECIES_AMPHAROS,
  SPECIES_EELEKTROSS,
  SPECIES_ROTOM_MOW,
  SPECIES_ROTOM_FAN,
  SPECIES_ROTOM_FROST,
  SPECIES_ROTOM_WASH,
  SPECIES_ROTOM_HEAT,
  SPECIES_LUXRAY,
  SPECIES_JOLTEON,
  SPECIES_MAGNEZONE,
  SPECIES_ELECTIVIRE,
  SPECIES_IRON_THORNS,
  SPECIES_IRON_HANDS,
  SPECIES_SANDY_SHOCKS,
  SPECIES_XURKITREE,
  SPECIES_TAPU_KOKO,
  SPECIES_MANECTRIC_MEGA,
  SPECIES_REGIELEKI,
  SPECIES_THUNDURUS_THERIAN,
  SPECIES_THUNDURUS_INCARNATE,
  SPECIES_RAIKOU,
  SPECIES_ZAPDOS,
  SPECIES_RAGING_BOLT,
  SPECIES_ZERAORA,
  SPECIES_AMPHAROS_MEGA,
  SPECIES_MIRAIDON,
  SPECIES_ZEKROM
};

#define RANDOM_SPECIES_PSYCHIC_COUNT ARRAY_COUNT(sRandomSpeciesPsychicBst)
static const u16 sRandomSpeciesPsychicBst[] =
{
  SPECIES_RALTS,
  SPECIES_COSMOG,
  SPECIES_FLITTLE,
  SPECIES_WYNAUT,
  SPECIES_HATENNA,
  SPECIES_KIRLIA,
  SPECIES_MEDITITE,
  SPECIES_CHINGLING,
  SPECIES_INKAY,
  SPECIES_SOLOSIS,
  SPECIES_GOTHITA,
  SPECIES_MUNNA,
  SPECIES_BRONZOR,
  SPECIES_BELDUM,
  SPECIES_BALTOY,
  SPECIES_SMOOCHUM,
  SPECIES_MIME_JR,
  SPECIES_ABRA,
  SPECIES_SLOWPOKE_GALAR,
  SPECIES_SLOWPOKE,
  SPECIES_NATU,
  SPECIES_WOOBAT,
  SPECIES_EXEGGCUTE,
  SPECIES_DROWZEE,
  SPECIES_SPOINK,
  SPECIES_DOTTLER,
  SPECIES_ELGYEM,
  SPECIES_UNOWN,
  SPECIES_ESPURR,
  SPECIES_HATTREM,
  SPECIES_DUOSION,
  SPECIES_GOTHORITA,
  SPECIES_COSMOEM,
  SPECIES_KADABRA,
  SPECIES_WOBBUFFET,
  SPECIES_MEDICHAM,
  SPECIES_PONYTA_GALAR,
  SPECIES_METANG,
  SPECIES_SWOOBAT,
  SPECIES_CHIMECHO,
  SPECIES_GIRAFARIG,
  SPECIES_JYNX,
  SPECIES_SOLROCK,
  SPECIES_LUNATONE,
  SPECIES_MR_MIME_GALAR,
  SPECIES_MR_MIME,
  SPECIES_MEOWSTIC_F,
  SPECIES_MEOWSTIC_M,
  SPECIES_RABSCA,
  SPECIES_GRUMPIG,
  SPECIES_XATU,
  SPECIES_INDEEDEE_F,
  SPECIES_INDEEDEE_M,
  SPECIES_BRUXISH,
  SPECIES_ORICORIO_PAU,
  SPECIES_VELUZA,
  SPECIES_ESPATHRA,
  SPECIES_MALAMAR,
  SPECIES_HYPNO,
  SPECIES_BEHEEYEM,
  SPECIES_RAICHU_ALOLA,
  SPECIES_MUSHARNA,
  SPECIES_ORANGURU,
  SPECIES_REUNICLUS,
  SPECIES_GOTHITELLE,
  SPECIES_SIGILYPH,
  SPECIES_SLOWKING_GALAR,
  SPECIES_SLOWKING,
  SPECIES_SLOWBRO_GALAR,
  SPECIES_SLOWBRO,
  SPECIES_CALYREX,
  SPECIES_BRONZONG,
  SPECIES_CLAYDOL,
  SPECIES_RAPIDASH_GALAR,
  SPECIES_ALAKAZAM,
  SPECIES_ORBEETLE,
  SPECIES_HATTERENE,
  SPECIES_BRAVIARY_HISUI,
  SPECIES_MEDICHAM_MEGA,
  SPECIES_GALLADE,
  SPECIES_GARDEVOIR,
  SPECIES_FARIGIRAF,
  SPECIES_MR_RIME,
  SPECIES_STARMIE,
  SPECIES_ARMAROUGE,
  SPECIES_WYRDEER,
  SPECIES_ESPEON,
  SPECIES_EXEGGUTOR,
  SPECIES_DELPHOX,
  SPECIES_MUNKIDORI,
  SPECIES_SCREAM_TAIL,
  SPECIES_TAPU_LELE,
  SPECIES_CRESSELIA,
  SPECIES_AZELF,
  SPECIES_MESPRIT,
  SPECIES_UXIE,
  SPECIES_ARTICUNO_GALAR,
  SPECIES_MALAMAR_MEGA,
  SPECIES_IRON_CROWN,
  SPECIES_IRON_BOULDER,
  SPECIES_IRON_LEAVES,
  SPECIES_SLOWBRO_MEGA,
  SPECIES_NECROZMA,
  SPECIES_HOOPA_CONFINED,
  SPECIES_MELOETTA_ARIA,
  SPECIES_VICTINI,
  SPECIES_DEOXYS_SPEED,
  SPECIES_DEOXYS_DEFENSE,
  SPECIES_DEOXYS_ATTACK,
  SPECIES_DEOXYS_NORMAL,
  SPECIES_JIRACHI,
  SPECIES_LATIOS,
  SPECIES_LATIAS,
  SPECIES_METAGROSS,
  SPECIES_CELEBI,
  SPECIES_MEW,
  SPECIES_ALAKAZAM_MEGA,
  SPECIES_GALLADE_MEGA,
  SPECIES_GARDEVOIR_MEGA,
  SPECIES_CALYREX_SHADOW,
  SPECIES_CALYREX_ICE,
  SPECIES_NECROZMA_DAWN_WINGS,
  SPECIES_NECROZMA_DUSK_MANE,
  SPECIES_LUNALA,
  SPECIES_SOLGALEO,
  SPECIES_HOOPA_UNBOUND,
  SPECIES_LUGIA,
  SPECIES_MEWTWO,
  SPECIES_LATIOS_MEGA,
  SPECIES_LATIAS_MEGA,
  SPECIES_METAGROSS_MEGA,
  SPECIES_NECROZMA_ULTRA,
  SPECIES_MEWTWO_MEGA_Y,
  SPECIES_MEWTWO_MEGA_X,
};

#define RANDOM_SPECIES_ICE_COUNT ARRAY_COUNT(sRandomSpeciesIceBst)
static const u16 sRandomSpeciesIceBst[] =
{
  SPECIES_SNOM,
  SPECIES_SWINUB,
  SPECIES_SPHEAL,
  SPECIES_VULPIX_ALOLA,
  SPECIES_SNORUNT,
  SPECIES_SANDSHREW_ALOLA,
  SPECIES_BERGMITE,
  SPECIES_CUBCHOO,
  SPECIES_VANILLITE,
  SPECIES_SMOOCHUM,
  SPECIES_DARUMAKA_GALAR,
  SPECIES_FRIGIBAX,
  SPECIES_DELIBIRD,
  SPECIES_CETODDLE,
  SPECIES_SNOVER,
  SPECIES_AMAURA,
  SPECIES_VANILLISH,
  SPECIES_SEALEO,
  SPECIES_CASTFORM_SNOWY,
  SPECIES_ARCTIBAX,
  SPECIES_SNEASEL,
  SPECIES_PILOSWINE,
  SPECIES_SANDSLASH_ALOLA,
  SPECIES_JYNX,
  SPECIES_MR_MIME_GALAR,
  SPECIES_EISCUE_ICE,
  SPECIES_FROSMOTH,
  SPECIES_DEWGONG,
  SPECIES_CRABOMINABLE,
  SPECIES_DARMANITAN_GALAR,
  SPECIES_FROSLASS,
  SPECIES_GLALIE,
  SPECIES_ABOMASNOW,
  SPECIES_ARCTOVISH,
  SPECIES_ARCTOZOLT,
  SPECIES_BEARTIC,
  SPECIES_NINETALES_ALOLA,
  SPECIES_WEAVILE,
  SPECIES_AVALUGG_HISUI,
  SPECIES_AVALUGG,
  SPECIES_CRYOGONAL,
  SPECIES_MR_RIME,
  SPECIES_ROTOM_FROST,
  SPECIES_CETITAN,
  SPECIES_AURORUS,
  SPECIES_GLACEON,
  SPECIES_CLOYSTER,
  SPECIES_MAMOSWINE,
  SPECIES_WALREIN,
  SPECIES_VANILLUXE,
  SPECIES_LAPRAS,
  SPECIES_CHIEN_PAO,
  SPECIES_IRON_BUNDLE,
  SPECIES_GLASTRIER,
  SPECIES_REGICE,
  SPECIES_GLALIE_MEGA,
  SPECIES_ARTICUNO,
  SPECIES_ABOMASNOW_MEGA,
  SPECIES_BAXCALIBUR,
  SPECIES_KYUREM,
  SPECIES_CALYREX_ICE,
  SPECIES_KYUREM_BLACK,
  SPECIES_KYUREM_WHITE,
};

#define RANDOM_SPECIES_DRAGON_COUNT ARRAY_COUNT(sRandomSpeciesDragonBst)
static const u16 sRandomSpeciesDragonBst[] =
{
  SPECIES_NOIBAT,
  SPECIES_APPLIN,
  SPECIES_DREEPY,
  SPECIES_JANGMO_O,
  SPECIES_GOOMY,
  SPECIES_DEINO,
  SPECIES_GIBLE,
  SPECIES_BAGON,
  SPECIES_DRATINI,
  SPECIES_FRIGIBAX,
  SPECIES_AXEW,
  SPECIES_VIBRAVA,
  SPECIES_TYRUNT,
  SPECIES_DRAKLOAK,
  SPECIES_FRAXURE,
  SPECIES_GABITE,
  SPECIES_HAKAMO_O,
  SPECIES_ZWEILOUS,
  SPECIES_SHELGON,
  SPECIES_DRAGONAIR,
  SPECIES_ARCTIBAX,
  SPECIES_SLIGGOO_HISUI,
  SPECIES_SLIGGOO,
  SPECIES_TATSUGIRI,
  SPECIES_DIPPLIN,
  SPECIES_APPLETUN,
  SPECIES_FLAPPLE,
  SPECIES_DRAMPA,
  SPECIES_TURTONATOR,
  SPECIES_DRUDDIGON,
  SPECIES_ZYGARDE_10_POWER_CONSTRUCT,
  SPECIES_ALTARIA,
  SPECIES_DRAGALGE,
  SPECIES_CYCLIZAR,
  SPECIES_DRACOVISH,
  SPECIES_DRACOZOLT,
  SPECIES_FLYGON,
  SPECIES_TYRANTRUM,
  SPECIES_EXEGGUTOR_ALOLA,
  SPECIES_DURALUDON,
  SPECIES_NOIVERN,
  SPECIES_HYDRAPPLE,
  SPECIES_NAGANADEL,
  SPECIES_HAXORUS,
  SPECIES_KINGDRA,
  SPECIES_GUZZLORD,
  SPECIES_REGIDRAGO,
  SPECIES_RAGING_BOLT,
  SPECIES_GOUGING_FIRE,
  SPECIES_WALKING_WAKE,
  SPECIES_ROARING_MOON,
  SPECIES_ALTARIA_MEGA,
  SPECIES_ARCHALUDON,
  SPECIES_BAXCALIBUR,
  SPECIES_DRAGAPULT,
  SPECIES_KOMMO_O,
  SPECIES_ZYGARDE_50_POWER_CONSTRUCT,
  SPECIES_GOODRA_HISUI,
  SPECIES_GOODRA,
  SPECIES_HYDREIGON,
  SPECIES_GARCHOMP,
  SPECIES_LATIOS,
  SPECIES_LATIAS,
  SPECIES_SALAMENCE,
  SPECIES_DRAGONITE,
  SPECIES_AMPHAROS_MEGA,
  SPECIES_SCEPTILE_MEGA,
  SPECIES_CHARIZARD_MEGA_X,
  SPECIES_KYUREM,
  SPECIES_MIRAIDON,
  SPECIES_KORAIDON,
  SPECIES_ZEKROM,
  SPECIES_RESHIRAM,
  SPECIES_GIRATINA_ORIGIN,
  SPECIES_GIRATINA_ALTERED,
  SPECIES_PALKIA_ORIGIN,
  SPECIES_PALKIA,
  SPECIES_DIALGA_ORIGIN,
  SPECIES_DIALGA,
  SPECIES_RAYQUAZA,
  SPECIES_ETERNATUS,
  SPECIES_KYUREM_BLACK,
  SPECIES_KYUREM_WHITE,
  SPECIES_GARCHOMP_MEGA,
  SPECIES_LATIOS_MEGA,
  SPECIES_LATIAS_MEGA,
  SPECIES_SALAMENCE_MEGA,
  SPECIES_DRAGONITE_MEGA,
  SPECIES_NECROZMA_ULTRA,
  SPECIES_RAYQUAZA_MEGA,
};

#define RANDOM_SPECIES_DARK_COUNT ARRAY_COUNT(sRandomSpeciesDarkBst)
static const u16 sRandomSpeciesDarkBst[] =
{
  SPECIES_POOCHYENA,
  SPECIES_ZIGZAGOON_GALAR,
  SPECIES_NICKIT,
  SPECIES_RATTATA_ALOLA,
  SPECIES_IMPIDIMP,
  SPECIES_PURRLOIN,
  SPECIES_INKAY,
  SPECIES_MEOWTH_ALOLA,
  SPECIES_SANDILE,
  SPECIES_DEINO,
  SPECIES_CARVANHA,
  SPECIES_GRIMER_ALOLA,
  SPECIES_STUNKY,
  SPECIES_ZORUA,
  SPECIES_HOUNDOUR,
  SPECIES_MASCHIFF,
  SPECIES_PAWNIARD,
  SPECIES_NUZLEAF,
  SPECIES_SCRAGGY,
  SPECIES_KROKOROK,
  SPECIES_MORGREM,
  SPECIES_VULLABY,
  SPECIES_SABLEYE,
  SPECIES_MURKROW,
  SPECIES_RATICATE_ALOLA,
  SPECIES_ZWEILOUS,
  SPECIES_LINOONE_GALAR,
  SPECIES_MIGHTYENA,
  SPECIES_SNEASEL,
  SPECIES_MORPEKO_HANGRY,
  SPECIES_MORPEKO_FULL_BELLY,
  SPECIES_QWILFISH_HISUI,
  SPECIES_PERSIAN_ALOLA,
  SPECIES_LIEPARD,
  SPECIES_LOKIX,
  SPECIES_THIEVUL,
  SPECIES_SHARPEDO,
  SPECIES_ABSOL,
  SPECIES_CRAWDAUNT,
  SPECIES_CACTURNE,
  SPECIES_SKUNTANK,
  SPECIES_SABLEYE_MEGA,
  SPECIES_SHIFTRY,
  SPECIES_MALAMAR,
  SPECIES_BOMBIRDIER,
  SPECIES_SPIRITOMB,
  SPECIES_SCRAFTY,
  SPECIES_BISHARP,
  SPECIES_PANGORO,
  SPECIES_DRAPION,
  SPECIES_HOUNDOOM,
  SPECIES_MUK_ALOLA,
  SPECIES_MABOSSTIFF,
  SPECIES_HONCHKROW,
  SPECIES_OVERQWIL,
  SPECIES_GRIMMSNARL,
  SPECIES_MANDIBUZZ,
  SPECIES_ZOROARK,
  SPECIES_WEAVILE,
  SPECIES_KROOKODILE,
  SPECIES_OBSTAGOON,
  SPECIES_UMBREON,
  SPECIES_SAMUROTT_HISUI,
  SPECIES_MEOWSCARADA,
  SPECIES_INCINEROAR,
  SPECIES_GRENINJA,
  SPECIES_KINGAMBIT,
  SPECIES_URSHIFU_SINGLE_STRIKE,
  SPECIES_SHARPEDO_MEGA,
  SPECIES_ABSOL_MEGA,
  SPECIES_CHI_YU,
  SPECIES_TING_LU,
  SPECIES_CHIEN_PAO,
  SPECIES_WO_CHIEN,
  SPECIES_IRON_JUGULIS,
  SPECIES_BRUTE_BONNET,
  SPECIES_GUZZLORD,
  SPECIES_MOLTRES_GALAR,
  SPECIES_MALAMAR_MEGA,
  SPECIES_ROARING_MOON,
  SPECIES_ZARUDE,
  SPECIES_HYDREIGON,
  SPECIES_DARKRAI,
  SPECIES_TYRANITAR,
  SPECIES_HOUNDOOM_MEGA,
  SPECIES_GRENINJA_ASH,
  SPECIES_GYARADOS_MEGA,
  SPECIES_HOOPA_UNBOUND,
  SPECIES_YVELTAL,
  SPECIES_TYRANITAR_MEGA,
};

#define RANDOM_SPECIES_FAIRY_COUNT ARRAY_COUNT(sRandomSpeciesFairyBst)
static const u16 sRandomSpeciesFairyBst[] =
{
  SPECIES_AZURILL,
  SPECIES_RALTS,
  SPECIES_IGGLYBUFF,
  SPECIES_CLEFFA,
  SPECIES_TOGEPI,
  SPECIES_MARILL,
  SPECIES_IMPIDIMP,
  SPECIES_MILCERY,
  SPECIES_JIGGLYPUFF,
  SPECIES_KIRLIA,
  SPECIES_COTTONEE,
  SPECIES_MORELULL,
  SPECIES_TINKATINK,
  SPECIES_SNUBBULL,
  SPECIES_FLABEBE,
  SPECIES_CUTIEFLY,
  SPECIES_MIME_JR,
  SPECIES_FIDOUGH,
  SPECIES_CLEFAIRY,
  SPECIES_SWIRLIX,
  SPECIES_SPRITZEE,
  SPECIES_MORGREM,
  SPECIES_FLOETTE,
  SPECIES_TINKATUFF,
  SPECIES_MAWILE,
  SPECIES_SHIINOTIC,
  SPECIES_TOGETIC,
  SPECIES_AZUMARILL,
  SPECIES_DEDENNE,
  SPECIES_WIGGLYTUFF,
  SPECIES_GRANBULL,
  SPECIES_MR_MIME,
  SPECIES_AROMATISSE,
  SPECIES_RIBOMBEE,
  SPECIES_KLEFKI,
  SPECIES_MIMIKYU,
  SPECIES_DACHSBUN,
  SPECIES_SLURPUFF,
  SPECIES_WHIMSICOTT,
  SPECIES_MAWILE_MEGA,
  SPECIES_CLEFABLE,
  SPECIES_COMFEY,
  SPECIES_WEEZING_GALAR,
  SPECIES_ALCREMIE,
  SPECIES_CARBINK,
  SPECIES_RAPIDASH_GALAR,
  SPECIES_NINETALES_ALOLA,
  SPECIES_TINKATON,
  SPECIES_GRIMMSNARL,
  SPECIES_HATTERENE,
  SPECIES_GARDEVOIR,
  SPECIES_SYLVEON,
  SPECIES_PRIMARINA,
  SPECIES_AUDINO_MEGA,
  SPECIES_TOGEKISS,
  SPECIES_FLORGES,
  SPECIES_FEZANDIPITI,
  SPECIES_FLUTTER_MANE,
  SPECIES_SCREAM_TAIL,
  SPECIES_TAPU_FINI,
  SPECIES_TAPU_BULU,
  SPECIES_TAPU_LELE,
  SPECIES_TAPU_KOKO,
  SPECIES_ENAMORUS_THERIAN,
  SPECIES_ENAMORUS_INCARNATE,
  SPECIES_IRON_VALIANT,
  SPECIES_ALTARIA_MEGA,
  SPECIES_MAGEARNA,
  SPECIES_DIANCIE,
  SPECIES_GARDEVOIR_MEGA,
  SPECIES_ZACIAN_HERO,
  SPECIES_XERNEAS,
  SPECIES_ZACIAN_CROWNED,
  SPECIES_DIANCIE_MEGA,
};

static const u8 sRandomTypeCounts[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL] = RANDOM_SPECIES_NORMAL_COUNT,
    [TYPE_FIGHTING] = RANDOM_SPECIES_FIGHTING_COUNT,
    [TYPE_FLYING] = RANDOM_SPECIES_FLYING_COUNT,
    [TYPE_POISON] = RANDOM_SPECIES_POISON_COUNT,
    [TYPE_GROUND] = RANDOM_SPECIES_GROUND_COUNT,
    [TYPE_ROCK] = RANDOM_SPECIES_ROCK_COUNT,
    [TYPE_BUG] = RANDOM_SPECIES_BUG_COUNT,
    [TYPE_GHOST] = RANDOM_SPECIES_GHOST_COUNT,
    [TYPE_STEEL] = RANDOM_SPECIES_STEEL_COUNT,
    [TYPE_FIRE] = RANDOM_SPECIES_FIRE_COUNT,
    [TYPE_WATER] = RANDOM_SPECIES_WATER_COUNT,
    [TYPE_GRASS] = RANDOM_SPECIES_GRASS_COUNT,
    [TYPE_ELECTRIC] = RANDOM_SPECIES_ELECTRIC_COUNT,
    [TYPE_PSYCHIC] = RANDOM_SPECIES_PSYCHIC_COUNT,
    [TYPE_ICE] = RANDOM_SPECIES_ICE_COUNT,
    [TYPE_DRAGON] = RANDOM_SPECIES_DRAGON_COUNT,
    [TYPE_DARK] = RANDOM_SPECIES_DARK_COUNT,
    [TYPE_FAIRY] = RANDOM_SPECIES_FAIRY_COUNT,
};

static const u16 *sRandomTypeLists[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL] = sRandomSpeciesNormalBst,
    [TYPE_FIGHTING] = sRandomSpeciesFightingBst,
    [TYPE_FLYING] = sRandomSpeciesFlyingBst,
    [TYPE_POISON] = sRandomSpeciesPoisonBst,
    [TYPE_GROUND] = sRandomSpeciesGroundBst,
    [TYPE_ROCK] = sRandomSpeciesRockBst,
    [TYPE_BUG] = sRandomSpeciesBugBst,
    [TYPE_GHOST] = sRandomSpeciesGhostBst,
    [TYPE_STEEL] = sRandomSpeciesSteelBst,
    [TYPE_FIRE] = sRandomSpeciesFireBst,
    [TYPE_WATER] = sRandomSpeciesWaterBst,
    [TYPE_GRASS] = sRandomSpeciesGrassBst,
    [TYPE_ELECTRIC] = sRandomSpeciesElectricBst,
    [TYPE_PSYCHIC] = sRandomSpeciesPsychicBst,
    [TYPE_ICE] = sRandomSpeciesIceBst,
    [TYPE_DRAGON] = sRandomSpeciesDragonBst,
    [TYPE_DARK] = sRandomSpeciesDarkBst,
    [TYPE_FAIRY] = sRandomSpeciesFairyBst,
};

// static const u16 sRandomSpeciesAllMap[RANDOM_SPECIES_ALL_COUNT] =
// {
//     [SPECIES_WISHIWASHI_SOLO] = 0
//     [SPECIES_BLIPBUG] = 
//     [SPECIES_SUNKERN] = 
//     [SPECIES_SNOM] = 
//     [SPECIES_AZURILL] = 
//     [SPECIES_KRICKETOT] = 
//     [SPECIES_WURMPLE] = 
//     [SPECIES_WEEDLE] = 
//     [SPECIES_CATERPIE] = 
//     [SPECIES_RALTS] = 
//     [SPECIES_COSMOG] = 
//     [SPECIES_SCATTERBUG] = 
//     [SPECIES_FEEBAS] = 
//     [SPECIES_MAGIKARP] = 
//     [SPECIES_CASCOON] = 
//     [SPECIES_SILCOON] = 
//     [SPECIES_PICHU] = 
//     [SPECIES_KAKUNA] = 
//     [SPECIES_METAPOD] = 
//     [SPECIES_NYMBLE] = 
//     [SPECIES_TAROUNTULA] = 
//     [SPECIES_BOUNSWEET] = 
//     [SPECIES_TYROGUE] = 
//     [SPECIES_WOOPER_PALDEA] = 
//     [SPECIES_WOOPER] = 
//     [SPECIES_IGGLYBUFF] = 
//     [SPECIES_SPEWPA] = 
//     [SPECIES_SENTRET] = 
//     [SPECIES_CLEFFA] = 
//     [SPECIES_HAPPINY] = 
//     [SPECIES_SEEDOT] = 
//     [SPECIES_LOTAD] = 
//     [SPECIES_POOCHYENA] = 
//     [SPECIES_BURMY] = 
//     [SPECIES_WIMPOD] = 
//     [SPECIES_SHEDINJA] = 
//     [SPECIES_BUNNELBY] = 
//     [SPECIES_MAKUHITA] = 
//     [SPECIES_PAWMI] = 
//     [SPECIES_ROLYCOLY] = 
//     [SPECIES_WHISMUR] = 
//     [SPECIES_ZIGZAGOON_GALAR] = 
//     [SPECIES_ZIGZAGOON] = 
//     [SPECIES_TOXEL] = 
//     [SPECIES_COMBEE] = 
//     [SPECIES_WIGLETT] = 
//     [SPECIES_NICKIT] = 
//     [SPECIES_ROOKIDEE] = 
//     [SPECIES_NOIBAT] = 
//     [SPECIES_STARLY] = 
//     [SPECIES_TOGEPI] = 
//     [SPECIES_ZUBAT] = 
//     [SPECIES_GOSSIFLEUR] = 
//     [SPECIES_FOMANTIS] = 
//     [SPECIES_BIDOOF] = 
//     [SPECIES_SMEARGLE] = 
//     [SPECIES_SWINUB] = 
//     [SPECIES_SLUGMA] = 
//     [SPECIES_HOPPIP] = 
//     [SPECIES_MARILL] = 
//     [SPECIES_SPINARAK] = 
//     [SPECIES_PIDGEY] = 
//     [SPECIES_YUNGOOS] = 
//     [SPECIES_RATTATA_ALOLA] = 
//     [SPECIES_RATTATA] = 
//     [SPECIES_LECHONK] = 
//     [SPECIES_FLITTLE] = 
//     [SPECIES_CHARCADET] = 
//     [SPECIES_PATRAT] = 
//     [SPECIES_SMOLIV] = 
//     [SPECIES_APPLIN] = 
//     [SPECIES_VENIPEDE] = 
//     [SPECIES_WYNAUT] = 
//     [SPECIES_SKITTY] = 
//     [SPECIES_HOOTHOOT] = 
//     [SPECIES_SPEAROW] = 
//     [SPECIES_SHINX] = 
//     [SPECIES_PIDOVE] = 
//     [SPECIES_IMPIDIMP] = 
//     [SPECIES_HATENNA] = 
//     [SPECIES_PIKIPEK] = 
//     [SPECIES_LEDYBA] = 
//     [SPECIES_DIGLETT_ALOLA] = 
//     [SPECIES_DIGLETT] = 
//     [SPECIES_NINCADA] = 
//     [SPECIES_DEWPIDER] = 
//     [SPECIES_SURSKIT] = 
//     [SPECIES_RELLOR] = 
//     [SPECIES_DREEPY] = 
//     [SPECIES_MILCERY] = 
//     [SPECIES_YAMPER] = 
//     [SPECIES_WOOLOO] = 
//     [SPECIES_WINGULL] = 
//     [SPECIES_TAILLOW] = 
//     [SPECIES_JIGGLYPUFF] = 
//     [SPECIES_TADBULB] = 
//     [SPECIES_NIDORAN_M] = 
//     [SPECIES_BRAMBLIN] = 
//     [SPECIES_SKWOVET] = 
//     [SPECIES_LITWICK] = 
//     [SPECIES_TYNAMO] = 
//     [SPECIES_LILLIPUP] = 
//     [SPECIES_CHERUBI] = 
//     [SPECIES_NIDORAN_F] = 
//     [SPECIES_FLETCHLING] = 
//     [SPECIES_KIRLIA] = 
//     [SPECIES_WATTREL] = 
//     [SPECIES_NACLI] = 
//     [SPECIES_ARROKUDA] = 
//     [SPECIES_ROCKRUFF] = 
//     [SPECIES_PETILIL] = 
//     [SPECIES_COTTONEE] = 
//     [SPECIES_ROGGENROLA] = 
//     [SPECIES_BUDEW] = 
//     [SPECIES_MEDITITE] = 
//     [SPECIES_SLAKOTH] = 
//     [SPECIES_MAREEP] = 
//     [SPECIES_PURRLOIN] = 
//     [SPECIES_CHEWTLE] = 
//     [SPECIES_MORELULL] = 
//     [SPECIES_RIOLU] = 
//     [SPECIES_CHINGLING] = 
//     [SPECIES_PARAS] = 
//     [SPECIES_INKAY] = 
//     [SPECIES_BARBOACH] = 
//     [SPECIES_DITTO] = 
//     [SPECIES_EKANS] = 
//     [SPECIES_HELIOPTILE] = 
//     [SPECIES_GREAVARD] = 
//     [SPECIES_SHROODLE] = 
//     [SPECIES_STEENEE] = 
//     [SPECIES_SOLOSIS] = 
//     [SPECIES_GOTHITA] = 
//     [SPECIES_BONSLY] = 
//     [SPECIES_SPHEAL] = 
//     [SPECIES_TRAPINCH] = 
//     [SPECIES_PINECO] = 
//     [SPECIES_MEOWTH_GALAR] = 
//     [SPECIES_MEOWTH_ALOLA] = 
//     [SPECIES_MEOWTH] = 
//     [SPECIES_SANDILE] = 
//     [SPECIES_MUNNA] = 
//     [SPECIES_FOONGUS] = 
//     [SPECIES_TYMPOLE] = 
//     [SPECIES_BLITZLE] = 
//     [SPECIES_DUSKULL] = 
//     [SPECIES_SHUPPET] = 
//     [SPECIES_ELECTRIKE] = 
//     [SPECIES_SHROOMISH] = 
//     [SPECIES_HORSEA] = 
//     [SPECIES_TINKATINK] = 
//     [SPECIES_VULPIX_ALOLA] = 
//     [SPECIES_VULPIX] = 
//     [SPECIES_GIMMIGHOUL_ROAMING] = 
//     [SPECIES_GIMMIGHOUL_CHEST] = 
//     [SPECIES_VAROOM] = 
//     [SPECIES_MELTAN] = 
//     [SPECIES_JANGMO_O] = 
//     [SPECIES_GRUBBIN] = 
//     [SPECIES_GOOMY] = 
//     [SPECIES_DEINO] = 
//     [SPECIES_KLINK] = 
//     [SPECIES_MINCCINO] = 
//     [SPECIES_CROAGUNK] = 
//     [SPECIES_GIBLE] = 
//     [SPECIES_BRONZOR] = 
//     [SPECIES_BELDUM] = 
//     [SPECIES_BAGON] = 
//     [SPECIES_SNORUNT] = 
//     [SPECIES_BALTOY] = 
//     [SPECIES_LARVITAR] = 
//     [SPECIES_REMORAID] = 
//     [SPECIES_SNUBBULL] = 
//     [SPECIES_DRATINI] = 
//     [SPECIES_GEODUDE_ALOLA] = 
//     [SPECIES_GEODUDE] = 
//     [SPECIES_BELLSPROUT] = 
//     [SPECIES_POLIWAG] = 
//     [SPECIES_SANDSHREW_ALOLA] = 
//     [SPECIES_SANDSHREW] = 
//     [SPECIES_GULPIN] = 
//     [SPECIES_FLABEBE] = 
//     [SPECIES_GOLETT] = 
//     [SPECIES_YAMASK_GALAR] = 
//     [SPECIES_YAMASK] = 
//     [SPECIES_CAPSAKID] = 
//     [SPECIES_CUTIEFLY] = 
//     [SPECIES_BERGMITE] = 
//     [SPECIES_TANDEMAUS] = 
//     [SPECIES_SIZZLIPEDE] = 
//     [SPECIES_MAREANIE] = 
//     [SPECIES_SHELMET] = 
//     [SPECIES_CUBCHOO] = 
//     [SPECIES_FERROSEED] = 
//     [SPECIES_VANILLITE] = 
//     [SPECIES_DUCKLETT] = 
//     [SPECIES_TIMBURR] = 
//     [SPECIES_NUMEL] = 
//     [SPECIES_CARVANHA] = 
//     [SPECIES_SMOOCHUM] = 
//     [SPECIES_SHELLDER] = 
//     [SPECIES_MACHOP] = 
//     [SPECIES_MANKEY] = 
//     [SPECIES_VENONAT] = 
//     [SPECIES_BINACLE] = 
//     [SPECIES_FENNEKIN] = 
//     [SPECIES_POLTCHAGEIST] = 
//     [SPECIES_SINISTEA] = 
//     [SPECIES_OSHAWOTT] = 
//     [SPECIES_TEPIG] = 
//     [SPECIES_SNIVY] = 
//     [SPECIES_CORPHISH] = 
//     [SPECIES_PHANTUMP] = 
//     [SPECIES_CHIMCHAR] = 
//     [SPECIES_CYNDAQUIL] = 
//     [SPECIES_CHARMANDER] = 
//     [SPECIES_QUAXLY] = 
//     [SPECIES_FUECOCO] = 
//     [SPECIES_SPRIGATITO] = 
//     [SPECIES_CLOBBOPUS] = 
//     [SPECIES_SOBBLE] = 
//     [SPECIES_SCORBUNNY] = 
//     [SPECIES_GROOKEY] = 
//     [SPECIES_SEWADDLE] = 
//     [SPECIES_MIME_JR] = 
//     [SPECIES_GLAMEOW] = 
//     [SPECIES_SWABLU] = 
//     [SPECIES_MUDKIP] = 
//     [SPECIES_TORCHIC] = 
//     [SPECIES_TREECKO] = 
//     [SPECIES_GASTLY] = 
//     [SPECIES_DODUO] = 
//     [SPECIES_ABRA] = 
//     [SPECIES_FIDOUGH] = 
//     [SPECIES_CHESPIN] = 
//     [SPECIES_FROAKIE] = 
//     [SPECIES_PIPLUP] = 
//     [SPECIES_TOTODILE] = 
//     [SPECIES_SQUIRTLE] = 
//     [SPECIES_FINIZEN] = 
//     [SPECIES_SILICOBRA] = 
//     [SPECIES_KARRABLAST] = 
//     [SPECIES_DARUMAKA_GALAR] = 
//     [SPECIES_DARUMAKA] = 
//     [SPECIES_SLOWPOKE_GALAR] = 
//     [SPECIES_SLOWPOKE] = 
//     [SPECIES_PANPOUR] = 
//     [SPECIES_PANSEAR] = 
//     [SPECIES_PANSAGE] = 
//     [SPECIES_TURTWIG] = 
//     [SPECIES_CHIKORITA] = 
//     [SPECIES_BULBASAUR] = 
//     [SPECIES_JOLTIK] = 
//     [SPECIES_FRIGIBAX] = 
//     [SPECIES_SANDYGAST] = 
//     [SPECIES_SALANDIT] = 
//     [SPECIES_POPPLIO] = 
//     [SPECIES_LITTEN] = 
//     [SPECIES_ROWLET] = 
//     [SPECIES_SKRELP] = 
//     [SPECIES_AXEW] = 
//     [SPECIES_NATU] = 
//     [SPECIES_GOLDEEN] = 
//     [SPECIES_CUBONE] = 
//     [SPECIES_PSYDUCK] = 
//     [SPECIES_ODDISH] = 
//     [SPECIES_PIKACHU] = 
//     [SPECIES_WOOBAT] = 
//     [SPECIES_CLEFAIRY] = 
//     [SPECIES_HONEDGE] = 
//     [SPECIES_DWEBBLE] = 
//     [SPECIES_SHELLOS] = 
//     [SPECIES_EEVEE] = 
//     [SPECIES_EXEGGCUTE] = 
//     [SPECIES_KRABBY] = 
//     [SPECIES_GRIMER_ALOLA] = 
//     [SPECIES_GRIMER] = 
//     [SPECIES_SEEL] = 
//     [SPECIES_MAGNEMITE] = 
//     [SPECIES_DRILBUR] = 
//     [SPECIES_DROWZEE] = 
//     [SPECIES_TRUBBISH] = 
//     [SPECIES_STUNKY] = 
//     [SPECIES_CUFANT] = 
//     [SPECIES_CLAUNCHER] = 
//     [SPECIES_ZORUA_HISUI] = 
//     [SPECIES_ZORUA] = 
//     [SPECIES_FINNEON] = 
//     [SPECIES_SKORUPI] = 
//     [SPECIES_HIPPOPOTAS] = 
//     [SPECIES_BUIZEL] = 
//     [SPECIES_LUVDISC] = 
//     [SPECIES_SPOINK] = 
//     [SPECIES_ARON] = 
//     [SPECIES_PHANPY] = 
//     [SPECIES_HOUNDOUR] = 
//     [SPECIES_DELIBIRD] = 
//     [SPECIES_TEDDIURSA] = 
//     [SPECIES_CHINCHOU] = 
//     [SPECIES_VOLTORB_HISUI] = 
//     [SPECIES_VOLTORB] = 
//     [SPECIES_CETODDLE] = 
//     [SPECIES_SNOVER] = 
//     [SPECIES_TOEDSCOOL] = 
//     [SPECIES_DOTTLER] = 
//     [SPECIES_PUMPKABOO_SUPER] = 
//     [SPECIES_PUMPKABOO_SMALL] = 
//     [SPECIES_PUMPKABOO_LARGE] = 
//     [SPECIES_PUMPKABOO_AVERAGE] = 
//     [SPECIES_ELGYEM] = 
//     [SPECIES_FRILLISH] = 
//     [SPECIES_DEERLING] = 
//     [SPECIES_CACNEA] = 
//     [SPECIES_TENTACOOL] = 
//     [SPECIES_UNOWN] = 
//     [SPECIES_CRABRAWLER] = 
//     [SPECIES_MASCHIFF] = 
//     [SPECIES_STUFFUL] = 
//     [SPECIES_PAWNIARD] = 
//     [SPECIES_STARAVIA] = 
//     [SPECIES_VIBRAVA] = 
//     [SPECIES_NUZLEAF] = 
//     [SPECIES_LOMBRE] = 
//     [SPECIES_SKIPLOOM] = 
//     [SPECIES_STARYU] = 
//     [SPECIES_KOFFING] = 
//     [SPECIES_SWIRLIX] = 
//     [SPECIES_SPRITZEE] = 
//     [SPECIES_MANTYKE] = 
//     [SPECIES_CLAMPERL] = 
//     [SPECIES_RHYHORN] = 
//     [SPECIES_PANCHAM] = 
//     [SPECIES_SCRAGGY] = 
//     [SPECIES_DRIFLOON] = 
//     [SPECIES_PIDGEOTTO] = 
//     [SPECIES_GLIMMET] = 
//     [SPECIES_PAWMO] = 
//     [SPECIES_SKIDDO] = 
//     [SPECIES_RUFFLET] = 
//     [SPECIES_MIENFOO] = 
//     [SPECIES_BUNEARY] = 
//     [SPECIES_SHIELDON] = 
//     [SPECIES_CRANIDOS] = 
//     [SPECIES_GROWLITHE_HISUI] = 
//     [SPECIES_GROWLITHE] = 
//     [SPECIES_KROKOROK] = 
//     [SPECIES_DOLLIV] = 
//     [SPECIES_NACLSTACK] = 
//     [SPECIES_TRUMBEAK] = 
//     [SPECIES_ESPURR] = 
//     [SPECIES_TIRTOUGA] = 
//     [SPECIES_ANORITH] = 
//     [SPECIES_LILEEP] = 
//     [SPECIES_KABUTO] = 
//     [SPECIES_OMANYTE] = 
//     [SPECIES_TRANQUILL] = 
//     [SPECIES_LARVESTA] = 
//     [SPECIES_WHIRLIPEDE] = 
//     [SPECIES_SPINDA] = 
//     [SPECIES_LOUDRED] = 
//     [SPECIES_ELEKID] = 
//     [SPECIES_AIPOM] = 
//     [SPECIES_AMAURA] = 
//     [SPECIES_TYRUNT] = 
//     [SPECIES_LUXIO] = 
//     [SPECIES_CORVISQUIRE] = 
//     [SPECIES_MAGBY] = 
//     [SPECIES_FLAAFFY] = 
//     [SPECIES_NIDORINO] = 
//     [SPECIES_NIDORINA] = 
//     [SPECIES_LITLEO] = 
//     [SPECIES_MORGREM] = 
//     [SPECIES_HATTREM] = 
//     [SPECIES_VULLABY] = 
//     [SPECIES_LAMPENT] = 
//     [SPECIES_DUOSION] = 
//     [SPECIES_HERDIER] = 
//     [SPECIES_FLOETTE] = 
//     [SPECIES_NOSEPASS] = 
//     [SPECIES_FARFETCHD_GALAR] = 
//     [SPECIES_FARFETCHD] = 
//     [SPECIES_TINKATUFF] = 
//     [SPECIES_SWADLOON] = 
//     [SPECIES_MAWILE] = 
//     [SPECIES_SABLEYE] = 
//     [SPECIES_FLETCHINDER] = 
//     [SPECIES_PALPITOAD] = 
//     [SPECIES_KRICKETUNE] = 
//     [SPECIES_KUBFU] = 
//     [SPECIES_MUDBRAY] = 
//     [SPECIES_DUSTOX] = 
//     [SPECIES_LICKITUNG] = 
//     [SPECIES_ONIX] = 
//     [SPECIES_POLIWHIRL] = 
//     [SPECIES_GOTHORITA] = 
//     [SPECIES_BOLDORE] = 
//     [SPECIES_MUNCHLAX] = 
//     [SPECIES_YANMA] = 
//     [SPECIES_LEDIAN] = 
//     [SPECIES_GRAVELER_ALOLA] = 
//     [SPECIES_GRAVELER] = 
//     [SPECIES_WEEPINBELL] = 
//     [SPECIES_VANILLISH] = 
//     [SPECIES_BEAUTIFLY] = 
//     [SPECIES_PORYGON] = 
//     [SPECIES_GLOOM] = 
//     [SPECIES_BEEDRILL] = 
//     [SPECIES_BUTTERFREE] = 
//     [SPECIES_COSMOEM] = 
//     [SPECIES_CHARJABUG] = 
//     [SPECIES_WAILMER] = 
//     [SPECIES_ROSELIA] = 
//     [SPECIES_DELCATTY] = 
//     [SPECIES_ARIADOS] = 
//     [SPECIES_KADABRA] = 
//     [SPECIES_ARCHEN] = 
//     [SPECIES_SPIDOPS] = 
//     [SPECIES_SHIINOTIC] = 
//     [SPECIES_FROGADIER] = 
//     [SPECIES_QUILLADIN] = 
//     [SPECIES_EELEKTRIK] = 
//     [SPECIES_GURDURR] = 
//     [SPECIES_PACHIRISU] = 
//     [SPECIES_PRINPLUP] = 
//     [SPECIES_MONFERNO] = 
//     [SPECIES_GROTLE] = 
//     [SPECIES_MINUN] = 
//     [SPECIES_PLUSLE] = 
//     [SPECIES_MARSHTOMP] = 
//     [SPECIES_COMBUSKEN] = 
//     [SPECIES_GROVYLE] = 
//     [SPECIES_WOBBUFFET] = 
//     [SPECIES_MURKROW] = 
//     [SPECIES_TOGETIC] = 
//     [SPECIES_CROCONAW] = 
//     [SPECIES_QUILAVA] = 
//     [SPECIES_BAYLEEF] = 
//     [SPECIES_HAUNTER] = 
//     [SPECIES_MACHOKE] = 
//     [SPECIES_PARASECT] = 
//     [SPECIES_WARTORTLE] = 
//     [SPECIES_CHARMELEON] = 
//     [SPECIES_IVYSAUR] = 
//     [SPECIES_BRAIXEN] = 
//     [SPECIES_QUAXWELL] = 
//     [SPECIES_FLORAGATO] = 
//     [SPECIES_DRAKLOAK] = 
//     [SPECIES_CARKOL] = 
//     [SPECIES_PYUKUMUKU] = 
//     [SPECIES_FRAXURE] = 
//     [SPECIES_GABITE] = 
//     [SPECIES_BIBAREL] = 
//     [SPECIES_SEALEO] = 
//     [SPECIES_MEDICHAM] = 
//     [SPECIES_PUPITAR] = 
//     [SPECIES_CORSOLA_GALAR] = 
//     [SPECIES_CORSOLA] = 
//     [SPECIES_SUDOWOODO] = 
//     [SPECIES_PONYTA_GALAR] = 
//     [SPECIES_PONYTA] = 
//     [SPECIES_CROCALOR] = 
//     [SPECIES_VIVILLON] = 
//     [SPECIES_CHATOT] = 
//     [SPECIES_DEWOTT] = 
//     [SPECIES_SERVINE] = 
//     [SPECIES_RATICATE_ALOLA] = 
//     [SPECIES_RATICATE] = 
//     [SPECIES_DUNSPARCE] = 
//     [SPECIES_FURRET] = 
//     [SPECIES_SQUAWKABILLY] = 
//     [SPECIES_GUMSHOOS] = 
//     [SPECIES_PIGNITE] = 
//     [SPECIES_DRIZZILE] = 
//     [SPECIES_RABOOT] = 
//     [SPECIES_THWACKEY] = 
//     [SPECIES_POIPOLE] = 
//     [SPECIES_HAKAMO_O] = 
//     [SPECIES_BRIONNE] = 
//     [SPECIES_TORRACAT] = 
//     [SPECIES_DARTRIX] = 
//     [SPECIES_ZWEILOUS] = 
//     [SPECIES_WATCHOG] = 
//     [SPECIES_METANG] = 
//     [SPECIES_SHELGON] = 
//     [SPECIES_CASTFORM] = 
//     [SPECIES_LINOONE_GALAR] = 
//     [SPECIES_LINOONE] = 
//     [SPECIES_MIGHTYENA] = 
//     [SPECIES_AZUMARILL] = 
//     [SPECIES_DRAGONAIR] = 
//     [SPECIES_ARCTIBAX] = 
//     [SPECIES_DIGGERSBY] = 
//     [SPECIES_MOTHIM] = 
//     [SPECIES_WORMADAM_TRASH] = 
//     [SPECIES_WORMADAM_SANDY] = 
//     [SPECIES_WORMADAM_PLANT] = 
//     [SPECIES_WUGTRIO] = 
//     [SPECIES_SWOOBAT] = 
//     [SPECIES_SUNFLORA] = 
//     [SPECIES_MAROWAK_ALOLA] = 
//     [SPECIES_MAROWAK] = 
//     [SPECIES_DUGTRIO_ALOLA] = 
//     [SPECIES_DUGTRIO] = 
//     [SPECIES_EMOLGA] = 
//     [SPECIES_CLODSIRE] = 
//     [SPECIES_ILLUMISE] = 
//     [SPECIES_VOLBEAT] = 
//     [SPECIES_LAIRON] = 
//     [SPECIES_MAGCARGO] = 
//     [SPECIES_SNEASEL_HISUI] = 
//     [SPECIES_SNEASEL] = 
//     [SPECIES_GLIGAR] = 
//     [SPECIES_QUAGSIRE] = 
//     [SPECIES_PIKACHU_STARTER] = 
//     [SPECIES_DEDENNE] = 
//     [SPECIES_PINCURCHIN] = 
//     [SPECIES_TOGEDEMARU] = 
//     [SPECIES_MISDREAVUS] = 
//     [SPECIES_EEVEE_STARTER] = 
//     [SPECIES_TANGELA] = 
//     [SPECIES_WIGGLYTUFF] = 
//     [SPECIES_MORPEKO] = 
//     [SPECIES_PERRSERKER] = 
//     [SPECIES_MINIOR_METEOR] = 
//     [SPECIES_KLANG] = 
//     [SPECIES_ROTOM] = 
//     [SPECIES_KECLEON] = 
//     [SPECIES_VIGOROTH] = 
//     [SPECIES_PELIPPER] = 
//     [SPECIES_QWILFISH_HISUI] = 
//     [SPECIES_QWILFISH] = 
//     [SPECIES_SEADRA] = 
//     [SPECIES_PERSIAN_ALOLA] = 
//     [SPECIES_PERSIAN] = 
//     [SPECIES_FEAROW] = 
//     [SPECIES_AUDINO] = 
//     [SPECIES_LIEPARD] = 
//     [SPECIES_DOUBLADE] = 
//     [SPECIES_ARBOK] = 
//     [SPECIES_TERAPAGOS_NORMAL] = 
//     [SPECIES_KLAWF] = 
//     [SPECIES_LOKIX] = 
//     [SPECIES_CHERRIM] = 
//     [SPECIES_PILOSWINE] = 
//     [SPECIES_GRANBULL] = 
//     [SPECIES_SEAKING] = 
//     [SPECIES_CHANSEY] = 
//     [SPECIES_VENOMOTH] = 
//     [SPECIES_SANDSLASH_ALOLA] = 
//     [SPECIES_SANDSLASH] = 
//     [SPECIES_SLIGGOO_HISUI] = 
//     [SPECIES_SLIGGOO] = 
//     [SPECIES_PURUGLY] = 
//     [SPECIES_NOCTOWL] = 
//     [SPECIES_ARAQUANID] = 
//     [SPECIES_CARNIVINE] = 
//     [SPECIES_MASQUERAIN] = 
//     [SPECIES_THIEVUL] = 
//     [SPECIES_CHIMECHO] = 
//     [SPECIES_DUSCLOPS] = 
//     [SPECIES_BANETTE] = 
//     [SPECIES_SWELLOW] = 
//     [SPECIES_HITMONTOP] = 
//     [SPECIES_GIRAFARIG] = 
//     [SPECIES_JYNX] = 
//     [SPECIES_HITMONCHAN] = 
//     [SPECIES_HITMONLEE] = 
//     [SPECIES_PRIMEAPE] = 
//     [SPECIES_GOLBAT] = 
//     [SPECIES_NINJASK] = 
//     [SPECIES_PALAFIN_ZERO] = 
//     [SPECIES_SEVIPER] = 
//     [SPECIES_ZANGOOSE] = 
//     [SPECIES_ELDEGOSS] = 
//     [SPECIES_GREEDENT] = 
//     [SPECIES_BASCULIN] = 
//     [SPECIES_LUMINEON] = 
//     [SPECIES_TROPIUS] = 
//     [SPECIES_SOLROCK] = 
//     [SPECIES_LUNATONE] = 
//     [SPECIES_CAMERUPT] = 
//     [SPECIES_SHARPEDO] = 
//     [SPECIES_BRELOOM] = 
//     [SPECIES_JUMPLUFF] = 
//     [SPECIES_LANTURN] = 
//     [SPECIES_MR_MIME_GALAR] = 
//     [SPECIES_MR_MIME] = 
//     [SPECIES_MARACTUS] = 
//     [SPECIES_AROMATISSE] = 
//     [SPECIES_RIBOMBEE] = 
//     [SPECIES_AMOONGUSS] = 
//     [SPECIES_SAWK] = 
//     [SPECIES_THROH] = 
//     [SPECIES_ABSOL] = 
//     [SPECIES_STANTLER] = 
//     [SPECIES_SKARMORY] = 
//     [SPECIES_FORRETRESS] = 
//     [SPECIES_MAGNETON] = 
//     [SPECIES_MEOWSTIC] = 
//     [SPECIES_SWALOT] = 
//     [SPECIES_CRAWDAUNT] = 
//     [SPECIES_WHISCASH] = 
//     [SPECIES_RABSCA] = 
//     [SPECIES_MAUSHOLD] = 
//     [SPECIES_EISCUE_NOICE] = 
//     [SPECIES_EISCUE_ICE] = 
//     [SPECIES_STONJOURNER] = 
//     [SPECIES_FALINKS] = 
//     [SPECIES_KLEFKI] = 
//     [SPECIES_ALOMOMOLA] = 
//     [SPECIES_CINCCINO] = 
//     [SPECIES_GRUMPIG] = 
//     [SPECIES_TORKOAL] = 
//     [SPECIES_XATU] = 
//     [SPECIES_DODRIO] = 
//     [SPECIES_STUNFISK_GALAR] = 
//     [SPECIES_STUNFISK] = 
//     [SPECIES_FURFROU] = 
//     [SPECIES_GALVANTULA] = 
//     [SPECIES_SWANNA] = 
//     [SPECIES_TREVENANT] = 
//     [SPECIES_GARBODOR] = 
//     [SPECIES_VESPIQUEN] = 
//     [SPECIES_HARIYAMA] = 
//     [SPECIES_TATSUGIRI] = 
//     [SPECIES_INDEEDEE_M] = 
//     [SPECIES_INDEEDEE_F] = 
//     [SPECIES_FROSMOTH] = 
//     [SPECIES_CRAMORANT] = 
//     [SPECIES_BRUXISH] = 
//     [SPECIES_SAWSBUCK] = 
//     [SPECIES_GASTRODON] = 
//     [SPECIES_CACTURNE] = 
//     [SPECIES_MANECTRIC] = 
//     [SPECIES_KINGLER] = 
//     [SPECIES_DEWGONG] = 
//     [SPECIES_MIMIKYU] = 
//     [SPECIES_ORICORIO] = 
//     [SPECIES_DACHSBUN] = 
//     [SPECIES_VELUZA] = 
//     [SPECIES_CRABOMINABLE] = 
//     [SPECIES_SKUNTANK] = 
//     [SPECIES_PIDGEOT] = 
//     [SPECIES_ORTHWORM] = 
//     [SPECIES_BRAMBLEGHAST] = 
//     [SPECIES_GRAPPLOCT] = 
//     [SPECIES_KOMALA] = 
//     [SPECIES_PALOSSAND] = 
//     [SPECIES_SALAZZLE] = 
//     [SPECIES_LURANTIS] = 
//     [SPECIES_SLURPUFF] = 
//     [SPECIES_JELLICENT] = 
//     [SPECIES_DARMANITAN_STANDARD] = 
//     [SPECIES_DARMANITAN_GALAR_STANDARD] = 
//     [SPECIES_LILLIGANT_HISUI] = 
//     [SPECIES_LILLIGANT] = 
//     [SPECIES_WHIMSICOTT] = 
//     [SPECIES_PHIONE] = 
//     [SPECIES_FROSLASS] = 
//     [SPECIES_LOPUNNY] = 
//     [SPECIES_GLALIE] = 
//     [SPECIES_MAWILE_MEGA] = 
//     [SPECIES_SABLEYE_MEGA] = 
//     [SPECIES_SHIFTRY] = 
//     [SPECIES_LUDICOLO] = 
//     [SPECIES_OCTILLERY] = 
//     [SPECIES_ESPATHRA] = 
//     [SPECIES_HELIOLISK] = 
//     [SPECIES_MALAMAR] = 
//     [SPECIES_AMBIPOM] = 
//     [SPECIES_RUNERIGUS] = 
//     [SPECIES_GOLURK] = 
//     [SPECIES_COFAGRIGUS] = 
//     [SPECIES_HYPNO] = 
//     [SPECIES_CLEFABLE] = 
//     [SPECIES_DURANT] = 
//     [SPECIES_HEATMOR] = 
//     [SPECIES_DIPPLIN] = 
//     [SPECIES_BOMBIRDIER] = 
//     [SPECIES_GRAFAIAI] = 
//     [SPECIES_APPLETUN] = 
//     [SPECIES_FLAPPLE] = 
//     [SPECIES_DREDNAW] = 
//     [SPECIES_DRAMPA] = 
//     [SPECIES_TURTONATOR] = 
//     [SPECIES_COMFEY] = 
//     [SPECIES_TOUCANNON] = 
//     [SPECIES_DRUDDIGON] = 
//     [SPECIES_BEHEEYEM] = 
//     [SPECIES_CRUSTLE] = 
//     [SPECIES_SCOLIPEDE] = 
//     [SPECIES_SPIRITOMB] = 
//     [SPECIES_STARAPTOR] = 
//     [SPECIES_RELICANTH] = 
//     [SPECIES_GOREBYSS] = 
//     [SPECIES_HUNTAIL] = 
//     [SPECIES_MANTINE] = 
//     [SPECIES_RHYDON] = 
//     [SPECIES_RAICHU_ALOLA] = 
//     [SPECIES_RAICHU] = 
//     [SPECIES_SCOVILLAIN] = 
//     [SPECIES_ZYGARDE_10_POWER_CONSTRUCT] = 
//     [SPECIES_LYCANROC_MIDNIGHT] = 
//     [SPECIES_LYCANROC_MIDDAY] = 
//     [SPECIES_LYCANROC_DUSK] = 
//     [SPECIES_MUSHARNA] = 
//     [SPECIES_HOUNDSTONE] = 
//     [SPECIES_SCRAFTY] = 
//     [SPECIES_UNFEZANT] = 
//     [SPECIES_OINKOLOGNE_M] = 
//     [SPECIES_OINKOLOGNE_F] = 
//     [SPECIES_FERROTHORN] = 
//     [SPECIES_KILOWATTREL] = 
//     [SPECIES_PAWMOT] = 
//     [SPECIES_BARRASKEWDA] = 
//     [SPECIES_BOLTUND] = 
//     [SPECIES_DUBWOOL] = 
//     [SPECIES_PASSIMIAN] = 
//     [SPECIES_ORANGURU] = 
//     [SPECIES_BOUFFALANT] = 
//     [SPECIES_BISHARP] = 
//     [SPECIES_REUNICLUS] = 
//     [SPECIES_GOTHITELLE] = 
//     [SPECIES_SIGILYPH] = 
//     [SPECIES_TOXICROAK] = 
//     [SPECIES_ALTARIA] = 
//     [SPECIES_EXPLOUD] = 
//     [SPECIES_MILTANK] = 
//     [SPECIES_SLOWKING_GALAR] = 
//     [SPECIES_SLOWKING] = 
//     [SPECIES_BELLOSSOM] = 
//     [SPECIES_TAUROS_PALDEA_COMBAT] = 
//     [SPECIES_TAUROS_PALDEA_BLAZE] = 
//     [SPECIES_TAUROS_PALDEA_AQUA] = 
//     [SPECIES_TAUROS] = 
//     [SPECIES_ELECTABUZZ] = 
//     [SPECIES_KANGASKHAN] = 
//     [SPECIES_WEEZING_GALAR] = 
//     [SPECIES_WEEZING] = 
//     [SPECIES_ELECTRODE_HISUI] = 
//     [SPECIES_ELECTRODE] = 
//     [SPECIES_SLOWBRO_GALAR] = 
//     [SPECIES_SLOWBRO] = 
//     [SPECIES_VICTREEBEL] = 
//     [SPECIES_VILEPLUME] = 
//     [SPECIES_GOURGEIST_SUPER] = 
//     [SPECIES_GOURGEIST_SMALL] = 
//     [SPECIES_GOURGEIST_LARGE] = 
//     [SPECIES_GOURGEIST_AVERAGE] = 
//     [SPECIES_DRAGALGE] = 
//     [SPECIES_ABOMASNOW] = 
//     [SPECIES_BELLIBOLT] = 
//     [SPECIES_ALCREMIE] = 
//     [SPECIES_CORVIKNIGHT] = 
//     [SPECIES_TOXAPEX] = 
//     [SPECIES_PANGORO] = 
//     [SPECIES_ACCELGOR] = 
//     [SPECIES_ESCAVALIER] = 
//     [SPECIES_CARRACOSTA] = 
//     [SPECIES_MISMAGIUS] = 
//     [SPECIES_FLOATZEL] = 
//     [SPECIES_BASTIODON] = 
//     [SPECIES_RAMPARDOS] = 
//     [SPECIES_ARMALDO] = 
//     [SPECIES_CRADILY] = 
//     [SPECIES_KABUTOPS] = 
//     [SPECIES_OMASTAR] = 
//     [SPECIES_MAGMAR] = 
//     [SPECIES_GOLEM_ALOLA] = 
//     [SPECIES_GOLEM] = 
//     [SPECIES_BEEDRILL_MEGA] = 
//     [SPECIES_ZEBSTRIKA] = 
//     [SPECIES_SIMIPOUR] = 
//     [SPECIES_SIMISEAR] = 
//     [SPECIES_SIMISAGE] = 
//     [SPECIES_DRIFBLIM] = 
//     [SPECIES_TALONFLAME] = 
//     [SPECIES_FLAMIGO] = 
//     [SPECIES_REVAVROOM] = 
//     [SPECIES_GARGANACL] = 
//     [SPECIES_KLEAVOR] = 
//     [SPECIES_CALYREX] = 
//     [SPECIES_COPPERAJAH] = 
//     [SPECIES_MINIOR_CORE] = 
//     [SPECIES_BEWEAR] = 
//     [SPECIES_MUDSDALE] = 
//     [SPECIES_VIKAVOLT] = 
//     [SPECIES_CARBINK] = 
//     [SPECIES_HAWLUCHA] = 
//     [SPECIES_CLAWITZER] = 
//     [SPECIES_BARBARACLE] = 
//     [SPECIES_AEGISLASH_SHIELD] = 
//     [SPECIES_AEGISLASH_BLADE] = 
//     [SPECIES_LEAVANNY] = 
//     [SPECIES_STOUTLAND] = 
//     [SPECIES_DRAPION] = 
//     [SPECIES_BRONZONG] = 
//     [SPECIES_CLAYDOL] = 
//     [SPECIES_WAILORD] = 
//     [SPECIES_DONPHAN] = 
//     [SPECIES_HOUNDOOM] = 
//     [SPECIES_URSARING] = 
//     [SPECIES_HERACROSS] = 
//     [SPECIES_SCIZOR] = 
//     [SPECIES_POLITOED] = 
//     [SPECIES_PINSIR] = 
//     [SPECIES_SCYTHER] = 
//     [SPECIES_GENGAR] = 
//     [SPECIES_MUK_ALOLA] = 
//     [SPECIES_MUK] = 
//     [SPECIES_RAPIDASH_GALAR] = 
//     [SPECIES_RAPIDASH] = 
//     [SPECIES_ALAKAZAM] = 
//     [SPECIES_GOLDUCK] = 
//     [SPECIES_CYCLIZAR] = 
//     [SPECIES_TOXTRICITY] = 
//     [SPECIES_MABOSSTIFF] = 
//     [SPECIES_ARCTOVISH] = 
//     [SPECIES_DRACOVISH] = 
//     [SPECIES_ARCTOZOLT] = 
//     [SPECIES_DRACOZOLT] = 
//     [SPECIES_ORBEETLE] = 
//     [SPECIES_BEARTIC] = 
//     [SPECIES_CONKELDURR] = 
//     [SPECIES_HONCHKROW] = 
//     [SPECIES_SHUCKLE] = 
//     [SPECIES_MACHAMP] = 
//     [SPECIES_NINETALES_ALOLA] = 
//     [SPECIES_NINETALES] = 
//     [SPECIES_NIDOKING] = 
//     [SPECIES_NIDOQUEEN] = 
//     [SPECIES_TINKATON] = 
//     [SPECIES_SIRFETCHD] = 
//     [SPECIES_PYROAR] = 
//     [SPECIES_SINISTCHA] = 
//     [SPECIES_POLTEAGEIST] = 
//     [SPECIES_EXCADRILL] = 
//     [SPECIES_SEISMITOAD] = 
//     [SPECIES_ARBOLIVA] = 
//     [SPECIES_OVERQWIL] = 
//     [SPECIES_SNEASLER] = 
//     [SPECIES_CURSOLA] = 
//     [SPECIES_GRIMMSNARL] = 
//     [SPECIES_HATTERENE] = 
//     [SPECIES_SANDACONDA] = 
//     [SPECIES_COALOSSAL] = 
//     [SPECIES_TSAREENA] = 
//     [SPECIES_MANDIBUZZ] = 
//     [SPECIES_BRAVIARY_HISUI] = 
//     [SPECIES_BRAVIARY] = 
//     [SPECIES_MIENSHAO] = 
//     [SPECIES_ZOROARK_HISUI] = 
//     [SPECIES_ZOROARK] = 
//     [SPECIES_GLISCOR] = 
//     [SPECIES_WEAVILE] = 
//     [SPECIES_MEDICHAM_MEGA] = 
//     [SPECIES_STEELIX] = 
//     [SPECIES_AMPHAROS] = 
//     [SPECIES_POLIWRATH] = 
//     [SPECIES_AVALUGG_HISUI] = 
//     [SPECIES_AVALUGG] = 
//     [SPECIES_TOEDSCRUEL] = 
//     [SPECIES_CRYOGONAL] = 
//     [SPECIES_EELEKTROSS] = 
//     [SPECIES_GIGALITH] = 
//     [SPECIES_YANMEGA] = 
//     [SPECIES_LICKILICKY] = 
//     [SPECIES_ROSERADE] = 
//     [SPECIES_PORYGON2] = 
//     [SPECIES_AERODACTYL] = 
//     [SPECIES_TENTACRUEL] = 
//     [SPECIES_DHELMISE] = 
//     [SPECIES_GALLADE] = 
//     [SPECIES_GARDEVOIR] = 
//     [SPECIES_KROOKODILE] = 
//     [SPECIES_DUDUNSPARCE] = 
//     [SPECIES_FARIGIRAF] = 
//     [SPECIES_MR_RIME] = 
//     [SPECIES_OBSTAGOON] = 
//     [SPECIES_CHANDELURE] = 
//     [SPECIES_KLINKLANG] = 
//     [SPECIES_ROTOM_WASH] = 
//     [SPECIES_ROTOM_MOW] = 
//     [SPECIES_ROTOM_HEAT] = 
//     [SPECIES_ROTOM_FROST] = 
//     [SPECIES_ROTOM_FAN] = 
//     [SPECIES_FLYGON] = 
//     [SPECIES_STARMIE] = 
//     [SPECIES_CETITAN] = 
//     [SPECIES_AURORUS] = 
//     [SPECIES_TYRANTRUM] = 
//     [SPECIES_LUXRAY] = 
//     [SPECIES_GLIMMORA] = 
//     [SPECIES_CERULEDGE] = 
//     [SPECIES_ARMAROUGE] = 
//     [SPECIES_WYRDEER] = 
//     [SPECIES_CENTISKORCH] = 
//     [SPECIES_SYLVEON] = 
//     [SPECIES_DUSKNOIR] = 
//     [SPECIES_PROBOPASS] = 
//     [SPECIES_GLACEON] = 
//     [SPECIES_LEAFEON] = 
//     [SPECIES_HIPPOWDON] = 
//     [SPECIES_LUCARIO] = 
//     [SPECIES_TORTERRA] = 
//     [SPECIES_UMBREON] = 
//     [SPECIES_ESPEON] = 
//     [SPECIES_MEGANIUM] = 
//     [SPECIES_FLAREON] = 
//     [SPECIES_JOLTEON] = 
//     [SPECIES_VAPOREON] = 
//     [SPECIES_CLOYSTER] = 
//     [SPECIES_VENUSAUR] = 
//     [SPECIES_SAMUROTT_HISUI] = 
//     [SPECIES_SAMUROTT] = 
//     [SPECIES_EMBOAR] = 
//     [SPECIES_SERPERIOR] = 
//     [SPECIES_DONDOZO] = 
//     [SPECIES_QUAQUAVAL] = 
//     [SPECIES_SKELEDIRGE] = 
//     [SPECIES_MEOWSCARADA] = 
//     [SPECIES_BASCULEGION_M] = 
//     [SPECIES_BASCULEGION_F] = 
//     [SPECIES_INTELEON] = 
//     [SPECIES_CINDERACE] = 
//     [SPECIES_RILLABOOM] = 
//     [SPECIES_GOLISOPOD] = 
//     [SPECIES_PRIMARINA] = 
//     [SPECIES_INCINEROAR] = 
//     [SPECIES_DECIDUEYE_HISUI] = 
//     [SPECIES_DECIDUEYE] = 
//     [SPECIES_GRENINJA] = 
//     [SPECIES_CHESNAUGHT] = 
//     [SPECIES_MAMOSWINE] = 
//     [SPECIES_EMPOLEON] = 
//     [SPECIES_WALREIN] = 
//     [SPECIES_AGGRON] = 
//     [SPECIES_BLAZIKEN] = 
//     [SPECIES_SCEPTILE] = 
//     [SPECIES_FERALIGATR] = 
//     [SPECIES_EXEGGUTOR_ALOLA] = 
//     [SPECIES_EXEGGUTOR] = 
//     [SPECIES_BLASTOISE] = 
//     [SPECIES_GOGOAT] = 
//     [SPECIES_TYPE_NULL] = 
//     [SPECIES_DELPHOX] = 
//     [SPECIES_INFERNAPE] = 
//     [SPECIES_TYPHLOSION_HISUI] = 
//     [SPECIES_TYPHLOSION] = 
//     [SPECIES_CHARIZARD] = 
//     [SPECIES_ANNIHILAPE] = 
//     [SPECIES_DURALUDON] = 
//     [SPECIES_NOIVERN] = 
//     [SPECIES_VANILLUXE] = 
//     [SPECIES_PORYGON_Z] = 
//     [SPECIES_TANGROWTH] = 
//     [SPECIES_RHYPERIOR] = 
//     [SPECIES_MAGNEZONE] = 
//     [SPECIES_SWAMPERT] = 
//     [SPECIES_CROBAT] = 
//     [SPECIES_LAPRAS] = 
//     [SPECIES_HYDRAPPLE] = 
//     [SPECIES_NAGANADEL] = 
//     [SPECIES_HAXORUS] = 
//     [SPECIES_DARMANITAN_ZEN] = 
//     [SPECIES_DARMANITAN_GALAR_ZEN] = 
//     [SPECIES_MAGMORTAR] = 
//     [SPECIES_ELECTIVIRE] = 
//     [SPECIES_MILOTIC] = 
//     [SPECIES_BLISSEY] = 
//     [SPECIES_KINGDRA] = 
//     [SPECIES_SNORLAX] = 
//     [SPECIES_GYARADOS] = 
//     [SPECIES_AUDINO_MEGA] = 
//     [SPECIES_TOGEKISS] = 
//     [SPECIES_OGERPON] = 
//     [SPECIES_GHOLDENGO] = 
//     [SPECIES_KINGAMBIT] = 
//     [SPECIES_URSALUNA] = 
//     [SPECIES_URSHIFU] = 
//     [SPECIES_VOLCARONA] = 
//     [SPECIES_FLOETTE_ETERNAL] = 
//     [SPECIES_FLORGES] = 
//     [SPECIES_FEZANDIPITI] = 
//     [SPECIES_MUNKIDORI] = 
//     [SPECIES_OKIDOGI] = 
//     [SPECIES_URSALUNA_BLOODMOON] = 
//     [SPECIES_BANETTE_MEGA] = 
//     [SPECIES_ARCANINE_HISUI] = 
//     [SPECIES_ARCANINE] = 
//     [SPECIES_CAMERUPT_MEGA] = 
//     [SPECIES_SHARPEDO_MEGA] = 
//     [SPECIES_ABSOL_MEGA] = 
//     [SPECIES_SKARMORY_MEGA] = 
//     [SPECIES_ARCHEOPS] = 
//     [SPECIES_CHI_YU] = 
//     [SPECIES_TING_LU] = 
//     [SPECIES_CHIEN_PAO] = 
//     [SPECIES_WO_CHIEN] = 
//     [SPECIES_IRON_THORNS] = 
//     [SPECIES_IRON_MOTH] = 
//     [SPECIES_IRON_JUGULIS] = 
//     [SPECIES_IRON_HANDS] = 
//     [SPECIES_IRON_BUNDLE] = 
//     [SPECIES_IRON_TREADS] = 
//     [SPECIES_SANDY_SHOCKS] = 
//     [SPECIES_SLITHER_WING] = 
//     [SPECIES_FLUTTER_MANE] = 
//     [SPECIES_BRUTE_BONNET] = 
//     [SPECIES_SCREAM_TAIL] = 
//     [SPECIES_GREAT_TUSK] = 
//     [SPECIES_FALINKS_MEGA] = 
//     [SPECIES_BLACEPHALON] = 
//     [SPECIES_STAKATAKA] = 
//     [SPECIES_GUZZLORD] = 
//     [SPECIES_KARTANA] = 
//     [SPECIES_CELESTEELA] = 
//     [SPECIES_XURKITREE] = 
//     [SPECIES_PHEROMOSA] = 
//     [SPECIES_BUZZWOLE] = 
//     [SPECIES_NIHILEGO] = 
//     [SPECIES_TAPU_FINI] = 
//     [SPECIES_TAPU_BULU] = 
//     [SPECIES_TAPU_LELE] = 
//     [SPECIES_TAPU_KOKO] = 
//     [SPECIES_SILVALLY] = 
//     [SPECIES_MANECTRIC_MEGA] = 
//     [SPECIES_PIDGEOT_MEGA] = 
//     [SPECIES_ENAMORUS_THERIAN] = 
//     [SPECIES_ENAMORUS_INCARNATE] = 
//     [SPECIES_SPECTRIER] = 
//     [SPECIES_GLASTRIER] = 
//     [SPECIES_REGIDRAGO] = 
//     [SPECIES_REGIELEKI] = 
//     [SPECIES_KELDEO] = 
//     [SPECIES_THUNDURUS_THERIAN] = 
//     [SPECIES_THUNDURUS_INCARNATE] = 
//     [SPECIES_TORNADUS_THERIAN] = 
//     [SPECIES_TORNADUS_INCARNATE] = 
//     [SPECIES_VIRIZION] = 
//     [SPECIES_TERRAKION] = 
//     [SPECIES_COBALION] = 
//     [SPECIES_CRESSELIA] = 
//     [SPECIES_AZELF] = 
//     [SPECIES_MESPRIT] = 
//     [SPECIES_UXIE] = 
//     [SPECIES_FROSLASS_MEGA] = 
//     [SPECIES_LOPUNNY_MEGA] = 
//     [SPECIES_REGISTEEL] = 
//     [SPECIES_REGICE] = 
//     [SPECIES_REGIROCK] = 
//     [SPECIES_GLALIE_MEGA] = 
//     [SPECIES_SUICUNE] = 
//     [SPECIES_ENTEI] = 
//     [SPECIES_RAIKOU] = 
//     [SPECIES_MOLTRES_GALAR] = 
//     [SPECIES_MOLTRES] = 
//     [SPECIES_ZAPDOS_GALAR] = 
//     [SPECIES_ZAPDOS] = 
//     [SPECIES_ARTICUNO_GALAR] = 
//     [SPECIES_ARTICUNO] = 
//     [SPECIES_MALAMAR_MEGA] = 
//     [SPECIES_CLEFABLE_MEGA] = 
//     [SPECIES_DRAMPA_MEGA] = 
//     [SPECIES_SCOLIPEDE_MEGA] = 
//     [SPECIES_SCRAFTY_MEGA] = 
//     [SPECIES_IRON_CROWN] = 
//     [SPECIES_IRON_BOULDER] = 
//     [SPECIES_RAGING_BOLT] = 
//     [SPECIES_GOUGING_FIRE] = 
//     [SPECIES_IRON_LEAVES] = 
//     [SPECIES_WALKING_WAKE] = 
//     [SPECIES_IRON_VALIANT] = 
//     [SPECIES_ROARING_MOON] = 
//     [SPECIES_ALTARIA_MEGA] = 
//     [SPECIES_KANGASKHAN_MEGA] = 
//     [SPECIES_SLOWBRO_MEGA] = 
//     [SPECIES_VICTREEBEL_MEGA] = 
//     [SPECIES_DRAGALGE_MEGA] = 
//     [SPECIES_ABOMASNOW_MEGA] = 
//     [SPECIES_PECHARUNT] = 
//     [SPECIES_TERAPAGOS_TERASTAL] = 
//     [SPECIES_ARCHALUDON] = 
//     [SPECIES_BAXCALIBUR] = 
//     [SPECIES_ZARUDE] = 
//     [SPECIES_DRAGAPULT] = 
//     [SPECIES_MELMETAL] = 
//     [SPECIES_ZERAORA] = 
//     [SPECIES_MARSHADOW] = 
//     [SPECIES_MAGEARNA] = 
//     [SPECIES_NECROZMA] = 
//     [SPECIES_KOMMO_O] = 
//     [SPECIES_VOLCANION] = 
//     [SPECIES_HOOPA_CONFINED] = 
//     [SPECIES_DIANCIE] = 
//     [SPECIES_ZYGARDE_50_POWER_CONSTRUCT] = 
//     [SPECIES_GOODRA_HISUI] = 
//     [SPECIES_GOODRA] = 
//     [SPECIES_HAWLUCHA_MEGA] = 
//     [SPECIES_BARBARACLE_MEGA] = 
//     [SPECIES_GENESECT] = 
//     [SPECIES_MELOETTA_PIROUETTE] = 
//     [SPECIES_MELOETTA_ARIA] = 
//     [SPECIES_LANDORUS_THERIAN] = 
//     [SPECIES_LANDORUS_INCARNATE] = 
//     [SPECIES_HYDREIGON] = 
//     [SPECIES_VICTINI] = 
//     [SPECIES_SHAYMIN_SKY] = 
//     [SPECIES_SHAYMIN_LAND] = 
//     [SPECIES_DARKRAI] = 
//     [SPECIES_MANAPHY] = 
//     [SPECIES_HEATRAN] = 
//     [SPECIES_GARCHOMP] = 
//     [SPECIES_DEOXYS_SPEED] = 
//     [SPECIES_DEOXYS_NORMAL] = 
//     [SPECIES_DEOXYS_DEFENSE] = 
//     [SPECIES_DEOXYS_ATTACK] = 
//     [SPECIES_JIRACHI] = 
//     [SPECIES_LATIOS] = 
//     [SPECIES_LATIAS] = 
//     [SPECIES_METAGROSS] = 
//     [SPECIES_SALAMENCE] = 
//     [SPECIES_CELEBI] = 
//     [SPECIES_TYRANITAR] = 
//     [SPECIES_HOUNDOOM_MEGA] = 
//     [SPECIES_HERACROSS_MEGA] = 
//     [SPECIES_SCIZOR_MEGA] = 
//     [SPECIES_MEW] = 
//     [SPECIES_DRAGONITE] = 
//     [SPECIES_PINSIR_MEGA] = 
//     [SPECIES_GENGAR_MEGA] = 
//     [SPECIES_ALAKAZAM_MEGA] = 
//     [SPECIES_PYROAR_MEGA] = 
//     [SPECIES_EXCADRILL_MEGA] = 
//     [SPECIES_STEELIX_MEGA] = 
//     [SPECIES_AMPHAROS_MEGA] = 
//     [SPECIES_EELEKTROSS_MEGA] = 
//     [SPECIES_AERODACTYL_MEGA] = 
//     [SPECIES_GALLADE_MEGA] = 
//     [SPECIES_GARDEVOIR_MEGA] = 
//     [SPECIES_WISHIWASHI_SCHOOL] = 
//     [SPECIES_CHANDELURE_MEGA] = 
//     [SPECIES_LUCARIO_MEGA] = 
//     [SPECIES_MEGANIUM_MEGA] = 
//     [SPECIES_VENUSAUR_MEGA] = 
//     [SPECIES_EMBOAR_MEGA] = 
//     [SPECIES_GRENINJA_MEGA] = 
//     [SPECIES_CHESNAUGHT_MEGA] = 
//     [SPECIES_AGGRON_MEGA] = 
//     [SPECIES_BLAZIKEN_MEGA] = 
//     [SPECIES_SCEPTILE_MEGA] = 
//     [SPECIES_FERALIGATR_MEGA] = 
//     [SPECIES_BLASTOISE_MEGA] = 
//     [SPECIES_DELPHOX_MEGA] = 
//     [SPECIES_CHARIZARD_MEGA_Y] = 
//     [SPECIES_CHARIZARD_MEGA_X] = 
//     [SPECIES_SWAMPERT_MEGA] = 
//     [SPECIES_GRENINJA_ASH] = 
//     [SPECIES_GYARADOS_MEGA] = 
//     [SPECIES_PALAFIN_HERO] = 
//     [SPECIES_FLOETTE_MEGA] = 
//     [SPECIES_ZAMAZENTA_HERO] = 
//     [SPECIES_ZACIAN_HERO] = 
//     [SPECIES_KYUREM] = 
//     [SPECIES_STARMIE_MEGA] = 
//     [SPECIES_MIRAIDON] = 
//     [SPECIES_KORAIDON] = 
//     [SPECIES_REGIGIGAS] = 
//     [SPECIES_GROUDON] = 
//     [SPECIES_KYOGRE] = 
//     [SPECIES_SLAKING] = 
//     [SPECIES_CALYREX_SHADOW] = 
//     [SPECIES_CALYREX_ICE] = 
//     [SPECIES_NECROZMA_DUSK_MANE] = 
//     [SPECIES_NECROZMA_DAWN_WINGS] = 
//     [SPECIES_LUNALA] = 
//     [SPECIES_SOLGALEO] = 
//     [SPECIES_HOOPA_UNBOUND] = 
//     [SPECIES_YVELTAL] = 
//     [SPECIES_XERNEAS] = 
//     [SPECIES_ZEKROM] = 
//     [SPECIES_RESHIRAM] = 
//     [SPECIES_GIRATINA_ORIGIN] = 
//     [SPECIES_GIRATINA_ALTERED] = 
//     [SPECIES_PALKIA_ORIGIN] = 
//     [SPECIES_PALKIA] = 
//     [SPECIES_DIALGA_ORIGIN] = 
//     [SPECIES_DIALGA] = 
//     [SPECIES_RAYQUAZA] = 
//     [SPECIES_HO_OH] = 
//     [SPECIES_LUGIA] = 
//     [SPECIES_MEWTWO] = 
//     [SPECIES_ETERNATUS] = 
//     [SPECIES_TERAPAGOS_STELLAR] = 
//     [SPECIES_ZAMAZENTA_CROWNED] = 
//     [SPECIES_ZACIAN_CROWNED] = 
//     [SPECIES_DIANCIE_MEGA] = 
//     [SPECIES_KYUREM_WHITE] = 
//     [SPECIES_KYUREM_BLACK] = 
//     [SPECIES_GARCHOMP_MEGA] = 
//     [SPECIES_LATIOS_MEGA] = 
//     [SPECIES_LATIAS_MEGA] = 
//     [SPECIES_METAGROSS_MEGA] = 
//     [SPECIES_SALAMENCE_MEGA] = 
//     [SPECIES_TYRANITAR_MEGA] = 
//     [SPECIES_DRAGONITE_MEGA] = 
//     [SPECIES_ZYGARDE_COMPLETE] = 
//     [SPECIES_ARCEUS] = 
//     [SPECIES_NECROZMA_ULTRA] = 
//     [SPECIES_GROUDON_PRIMAL] = 
//     [SPECIES_KYOGRE_PRIMAL] = 
//     [SPECIES_ZYGARDE_MEGA] = 
//     [SPECIES_RAYQUAZA_MEGA] = 
//     [SPECIES_MEWTWO_MEGA_Y] = 
//     [SPECIES_MEWTWO_MEGA_X] = 
//     // SPECIES_ETERNATUS_ETERNAMAX,
// };

#define RANDOM_SPECIES_WILD_COUNT ARRAY_COUNT(sRandomSpeciesWildBst)
static const u16 sRandomSpeciesWildBst[] =
{
    SPECIES_WISHIWASHI_SOLO,
    SPECIES_BLIPBUG,
    SPECIES_SUNKERN,
    SPECIES_SNOM,
    SPECIES_AZURILL,
    SPECIES_KRICKETOT,
    SPECIES_WURMPLE,
    SPECIES_WEEDLE,
    SPECIES_CATERPIE,
    SPECIES_RALTS,
    // SPECIES_COSMOG,
    SPECIES_SCATTERBUG,
    SPECIES_FEEBAS,
    SPECIES_MAGIKARP,
    SPECIES_CASCOON,
    SPECIES_SILCOON,
    SPECIES_PICHU,
    SPECIES_KAKUNA,
    SPECIES_METAPOD,
    SPECIES_NYMBLE,
    SPECIES_TAROUNTULA,
    SPECIES_BOUNSWEET,
    SPECIES_TYROGUE,
    SPECIES_WOOPER_PALDEA,
    SPECIES_WOOPER,
    SPECIES_IGGLYBUFF,
    SPECIES_SPEWPA,
    SPECIES_SENTRET,
    SPECIES_CLEFFA,
    SPECIES_HAPPINY,
    SPECIES_SEEDOT,
    SPECIES_LOTAD,
    SPECIES_POOCHYENA,
    SPECIES_BURMY,
    SPECIES_WIMPOD,
    SPECIES_SHEDINJA,
    SPECIES_BUNNELBY,
    SPECIES_MAKUHITA,
    SPECIES_PAWMI,
    SPECIES_ROLYCOLY,
    SPECIES_WHISMUR,
    SPECIES_ZIGZAGOON_GALAR,
    SPECIES_ZIGZAGOON,
    SPECIES_TOXEL,
    SPECIES_COMBEE,
    SPECIES_WIGLETT,
    SPECIES_NICKIT,
    SPECIES_ROOKIDEE,
    SPECIES_NOIBAT,
    SPECIES_STARLY,
    SPECIES_TOGEPI,
    SPECIES_ZUBAT,
    SPECIES_GOSSIFLEUR,
    SPECIES_FOMANTIS,
    SPECIES_BIDOOF,
    SPECIES_SMEARGLE,
    SPECIES_SWINUB,
    SPECIES_SLUGMA,
    SPECIES_HOPPIP,
    SPECIES_MARILL,
    SPECIES_SPINARAK,
    SPECIES_PIDGEY,
    SPECIES_YUNGOOS,
    SPECIES_RATTATA_ALOLA,
    SPECIES_RATTATA,
    SPECIES_LECHONK,
    SPECIES_FLITTLE,
    SPECIES_CHARCADET,
    SPECIES_PATRAT,
    SPECIES_SMOLIV,
    SPECIES_APPLIN,
    SPECIES_VENIPEDE,
    SPECIES_WYNAUT,
    SPECIES_SKITTY,
    SPECIES_HOOTHOOT,
    SPECIES_SPEAROW,
    SPECIES_SHINX,
    SPECIES_PIDOVE,
    SPECIES_IMPIDIMP,
    SPECIES_HATENNA,
    SPECIES_PIKIPEK,
    SPECIES_LEDYBA,
    SPECIES_DIGLETT_ALOLA,
    SPECIES_DIGLETT,
    SPECIES_NINCADA,
    SPECIES_DEWPIDER,
    SPECIES_SURSKIT,
    SPECIES_RELLOR,
    SPECIES_DREEPY,
    SPECIES_MILCERY,
    SPECIES_YAMPER,
    SPECIES_WOOLOO,
    SPECIES_WINGULL,
    SPECIES_TAILLOW,
    SPECIES_JIGGLYPUFF,
    SPECIES_TADBULB,
    SPECIES_NIDORAN_M,
    SPECIES_BRAMBLIN,
    SPECIES_SKWOVET,
    SPECIES_LITWICK,
    SPECIES_TYNAMO,
    SPECIES_LILLIPUP,
    SPECIES_CHERUBI,
    SPECIES_NIDORAN_F,
    SPECIES_FLETCHLING,
    SPECIES_KIRLIA,
    SPECIES_WATTREL,
    SPECIES_NACLI,
    SPECIES_ARROKUDA,
    SPECIES_ROCKRUFF,
    SPECIES_PETILIL,
    SPECIES_COTTONEE,
    SPECIES_ROGGENROLA,
    SPECIES_BUDEW,
    SPECIES_MEDITITE,
    SPECIES_SLAKOTH,
    SPECIES_MAREEP,
    SPECIES_PURRLOIN,
    SPECIES_CHEWTLE,
    SPECIES_MORELULL,
    SPECIES_RIOLU,
    SPECIES_CHINGLING,
    SPECIES_PARAS,
    SPECIES_INKAY,
    SPECIES_BARBOACH,
    SPECIES_DITTO,
    SPECIES_EKANS,
    SPECIES_HELIOPTILE,
    SPECIES_GREAVARD,
    SPECIES_SHROODLE,
    SPECIES_STEENEE,
    SPECIES_SOLOSIS,
    SPECIES_GOTHITA,
    SPECIES_BONSLY,
    SPECIES_SPHEAL,
    SPECIES_TRAPINCH,
    SPECIES_PINECO,
    SPECIES_MEOWTH_GALAR,
    SPECIES_MEOWTH_ALOLA,
    SPECIES_MEOWTH,
    SPECIES_SANDILE,
    SPECIES_MUNNA,
    SPECIES_FOONGUS,
    SPECIES_TYMPOLE,
    SPECIES_BLITZLE,
    SPECIES_DUSKULL,
    SPECIES_SHUPPET,
    SPECIES_ELECTRIKE,
    SPECIES_SHROOMISH,
    SPECIES_HORSEA,
    SPECIES_TINKATINK,
    SPECIES_VULPIX_ALOLA,
    SPECIES_VULPIX,
    SPECIES_VAROOM,
    SPECIES_MELTAN,
    SPECIES_JANGMO_O,
    SPECIES_GRUBBIN,
    SPECIES_GOOMY,
    SPECIES_DEINO,
    SPECIES_KLINK,
    SPECIES_MINCCINO,
    SPECIES_CROAGUNK,
    SPECIES_GIBLE,
    SPECIES_BRONZOR,
    SPECIES_BELDUM,
    SPECIES_BAGON,
    SPECIES_SNORUNT,
    SPECIES_BALTOY,
    SPECIES_LARVITAR,
    SPECIES_REMORAID,
    SPECIES_SNUBBULL,
    SPECIES_DRATINI,
    SPECIES_GEODUDE_ALOLA,
    SPECIES_GEODUDE,
    SPECIES_BELLSPROUT,
    SPECIES_POLIWAG,
    SPECIES_SANDSHREW_ALOLA,
    SPECIES_SANDSHREW,
    SPECIES_GULPIN,
    SPECIES_FLABEBE,
    SPECIES_GOLETT,
    SPECIES_YAMASK_GALAR,
    SPECIES_YAMASK,
    SPECIES_CAPSAKID,
    SPECIES_CUTIEFLY,
    SPECIES_BERGMITE,
    SPECIES_TANDEMAUS,
    SPECIES_SIZZLIPEDE,
    SPECIES_MAREANIE,
    SPECIES_SHELMET,
    SPECIES_CUBCHOO,
    SPECIES_FERROSEED,
    SPECIES_VANILLITE,
    SPECIES_DUCKLETT,
    SPECIES_TIMBURR,
    SPECIES_NUMEL,
    SPECIES_CARVANHA,
    SPECIES_SMOOCHUM,
    SPECIES_SHELLDER,
    SPECIES_MACHOP,
    SPECIES_MANKEY,
    SPECIES_VENONAT,
    SPECIES_BINACLE,
    SPECIES_FENNEKIN,
    SPECIES_POLTCHAGEIST,
    SPECIES_SINISTEA,
    SPECIES_OSHAWOTT,
    SPECIES_TEPIG,
    SPECIES_SNIVY,
    SPECIES_CORPHISH,
    SPECIES_PHANTUMP,
    SPECIES_CHIMCHAR,
    SPECIES_CYNDAQUIL,
    SPECIES_CHARMANDER,
    SPECIES_QUAXLY,
    SPECIES_FUECOCO,
    SPECIES_SPRIGATITO,
    SPECIES_CLOBBOPUS,
    SPECIES_SOBBLE,
    SPECIES_SCORBUNNY,
    SPECIES_GROOKEY,
    SPECIES_SEWADDLE,
    SPECIES_MIME_JR,
    SPECIES_GLAMEOW,
    SPECIES_SWABLU,
    SPECIES_MUDKIP,
    SPECIES_TORCHIC,
    SPECIES_TREECKO,
    SPECIES_GASTLY,
    SPECIES_DODUO,
    SPECIES_ABRA,
    SPECIES_FIDOUGH,
    SPECIES_CHESPIN,
    SPECIES_FROAKIE,
    SPECIES_PIPLUP,
    SPECIES_TOTODILE,
    SPECIES_SQUIRTLE,
    SPECIES_FINIZEN,
    SPECIES_SILICOBRA,
    SPECIES_KARRABLAST,
    SPECIES_DARUMAKA_GALAR,
    SPECIES_DARUMAKA,
    SPECIES_SLOWPOKE_GALAR,
    SPECIES_SLOWPOKE,
    SPECIES_PANPOUR,
    SPECIES_PANSEAR,
    SPECIES_PANSAGE,
    SPECIES_TURTWIG,
    SPECIES_CHIKORITA,
    SPECIES_BULBASAUR,
    SPECIES_JOLTIK,
    SPECIES_FRIGIBAX,
    SPECIES_SANDYGAST,
    SPECIES_SALANDIT,
    SPECIES_POPPLIO,
    SPECIES_LITTEN,
    SPECIES_ROWLET,
    SPECIES_SKRELP,
    SPECIES_AXEW,
    SPECIES_NATU,
    SPECIES_GOLDEEN,
    SPECIES_CUBONE,
    SPECIES_PSYDUCK,
    SPECIES_ODDISH,
    SPECIES_PIKACHU,
    SPECIES_WOOBAT,
    SPECIES_CLEFAIRY,
    SPECIES_HONEDGE,
    SPECIES_DWEBBLE,
    SPECIES_SHELLOS,
    SPECIES_EEVEE,
    SPECIES_EXEGGCUTE,
    SPECIES_KRABBY,
    SPECIES_GRIMER_ALOLA,
    SPECIES_GRIMER,
    SPECIES_SEEL,
    SPECIES_MAGNEMITE,
    SPECIES_DRILBUR,
    SPECIES_DROWZEE,
    SPECIES_TRUBBISH,
    SPECIES_STUNKY,
    SPECIES_CUFANT,
    SPECIES_CLAUNCHER,
    SPECIES_ZORUA_HISUI,
    SPECIES_ZORUA,
    SPECIES_FINNEON,
    SPECIES_SKORUPI,
    SPECIES_HIPPOPOTAS,
    SPECIES_BUIZEL,
    SPECIES_LUVDISC,
    SPECIES_SPOINK,
    SPECIES_ARON,
    SPECIES_PHANPY,
    SPECIES_HOUNDOUR,
    SPECIES_DELIBIRD,
    SPECIES_TEDDIURSA,
    SPECIES_CHINCHOU,
    SPECIES_VOLTORB_HISUI,
    SPECIES_VOLTORB,
    SPECIES_CETODDLE,
    SPECIES_SNOVER,
    SPECIES_TOEDSCOOL,
    SPECIES_DOTTLER,
    SPECIES_PUMPKABOO_SUPER,
    SPECIES_PUMPKABOO_SMALL,
    SPECIES_PUMPKABOO_LARGE,
    SPECIES_PUMPKABOO_AVERAGE,
    SPECIES_ELGYEM,
    SPECIES_FRILLISH,
    SPECIES_DEERLING,
    SPECIES_CACNEA,
    SPECIES_TENTACOOL,
    SPECIES_UNOWN,
    SPECIES_CRABRAWLER,
    SPECIES_MASCHIFF,
    SPECIES_STUFFUL,
    SPECIES_PAWNIARD,
    SPECIES_STARAVIA,
    SPECIES_VIBRAVA,
    SPECIES_NUZLEAF,
    SPECIES_LOMBRE,
    SPECIES_SKIPLOOM,
    SPECIES_STARYU,
    SPECIES_KOFFING,
    SPECIES_SWIRLIX,
    SPECIES_SPRITZEE,
    SPECIES_MANTYKE,
    SPECIES_CLAMPERL,
    SPECIES_RHYHORN,
    SPECIES_PANCHAM,
    SPECIES_SCRAGGY,
    SPECIES_DRIFLOON,
    SPECIES_PIDGEOTTO,
    SPECIES_GLIMMET,
    SPECIES_PAWMO,
    SPECIES_SKIDDO,
    SPECIES_RUFFLET,
    SPECIES_MIENFOO,
    SPECIES_BUNEARY,
    SPECIES_SHIELDON,
    SPECIES_CRANIDOS,
    SPECIES_GROWLITHE_HISUI,
    SPECIES_GROWLITHE,
    SPECIES_KROKOROK,
    SPECIES_DOLLIV,
    SPECIES_NACLSTACK,
    SPECIES_TRUMBEAK,
    SPECIES_ESPURR,
    SPECIES_TIRTOUGA,
    SPECIES_ANORITH,
    SPECIES_LILEEP,
    SPECIES_KABUTO,
    SPECIES_OMANYTE,
    SPECIES_TRANQUILL,
    SPECIES_LARVESTA,
    SPECIES_WHIRLIPEDE,
    SPECIES_SPINDA,
    SPECIES_LOUDRED,
    SPECIES_ELEKID,
    SPECIES_AIPOM,
    SPECIES_AMAURA,
    SPECIES_TYRUNT,
    SPECIES_LUXIO,
    SPECIES_CORVISQUIRE,
    SPECIES_MAGBY,
    SPECIES_FLAAFFY,
    SPECIES_NIDORINO,
    SPECIES_NIDORINA,
    SPECIES_LITLEO,
    SPECIES_MORGREM,
    SPECIES_HATTREM,
    SPECIES_VULLABY,
    SPECIES_LAMPENT,
    SPECIES_DUOSION,
    SPECIES_HERDIER,
    SPECIES_FLOETTE,
    SPECIES_NOSEPASS,
    SPECIES_FARFETCHD_GALAR,
    SPECIES_FARFETCHD,
    SPECIES_TINKATUFF,
    SPECIES_SWADLOON,
    SPECIES_MAWILE,
    SPECIES_SABLEYE,
    SPECIES_FLETCHINDER,
    SPECIES_PALPITOAD,
    SPECIES_KRICKETUNE,
    SPECIES_KUBFU,
    SPECIES_MUDBRAY,
    SPECIES_DUSTOX,
    SPECIES_LICKITUNG,
    SPECIES_ONIX,
    SPECIES_POLIWHIRL,
    SPECIES_GOTHORITA,
    SPECIES_BOLDORE,
    SPECIES_MUNCHLAX,
    SPECIES_YANMA,
    SPECIES_LEDIAN,
    SPECIES_GRAVELER_ALOLA,
    SPECIES_GRAVELER,
    SPECIES_WEEPINBELL,
    SPECIES_VANILLISH,
    SPECIES_BEAUTIFLY,
    SPECIES_PORYGON,
    SPECIES_GLOOM,
    SPECIES_BEEDRILL,
    SPECIES_BUTTERFREE,
    SPECIES_COSMOEM,
    SPECIES_CHARJABUG,
    SPECIES_WAILMER,
    SPECIES_ROSELIA,
    SPECIES_DELCATTY,
    SPECIES_ARIADOS,
    SPECIES_KADABRA,
    SPECIES_ARCHEN,
    SPECIES_SPIDOPS,
    SPECIES_SHIINOTIC,
    SPECIES_FROGADIER,
    SPECIES_QUILLADIN,
    SPECIES_EELEKTRIK,
    SPECIES_GURDURR,
    SPECIES_PACHIRISU,
    SPECIES_PRINPLUP,
    SPECIES_MONFERNO,
    SPECIES_GROTLE,
    SPECIES_MINUN,
    SPECIES_PLUSLE,
    SPECIES_MARSHTOMP,
    SPECIES_COMBUSKEN,
    SPECIES_GROVYLE,
    SPECIES_WOBBUFFET,
    SPECIES_MURKROW,
    SPECIES_TOGETIC,
    SPECIES_CROCONAW,
    SPECIES_QUILAVA,
    SPECIES_BAYLEEF,
    SPECIES_HAUNTER,
    SPECIES_MACHOKE,
    SPECIES_PARASECT,
    SPECIES_WARTORTLE,
    SPECIES_CHARMELEON,
    SPECIES_IVYSAUR,
    SPECIES_BRAIXEN,
    SPECIES_QUAXWELL,
    SPECIES_FLORAGATO,
    SPECIES_DRAKLOAK,
    SPECIES_CARKOL,
    SPECIES_PYUKUMUKU,
    SPECIES_FRAXURE,
    SPECIES_GABITE,
    SPECIES_BIBAREL,
    SPECIES_SEALEO,
    SPECIES_MEDICHAM,
    SPECIES_PUPITAR,
    SPECIES_CORSOLA_GALAR,
    SPECIES_CORSOLA,
    SPECIES_SUDOWOODO,
    SPECIES_PONYTA_GALAR,
    SPECIES_PONYTA,
    SPECIES_CROCALOR,
    SPECIES_VIVILLON,
    SPECIES_CHATOT,
    SPECIES_DEWOTT,
    SPECIES_SERVINE,
    SPECIES_RATICATE_ALOLA,
    SPECIES_RATICATE,
    SPECIES_DUNSPARCE,
    SPECIES_FURRET,
    SPECIES_SQUAWKABILLY,
    SPECIES_GUMSHOOS,
    SPECIES_PIGNITE,
    SPECIES_DRIZZILE,
    SPECIES_RABOOT,
    SPECIES_THWACKEY,
    SPECIES_POIPOLE,
    SPECIES_HAKAMO_O,
    SPECIES_BRIONNE,
    SPECIES_TORRACAT,
    SPECIES_DARTRIX,
    SPECIES_ZWEILOUS,
    SPECIES_WATCHOG,
    SPECIES_METANG,
    SPECIES_SHELGON,
    SPECIES_CASTFORM,
    SPECIES_LINOONE_GALAR,
    SPECIES_LINOONE,
    SPECIES_MIGHTYENA,
    SPECIES_AZUMARILL,
    SPECIES_DRAGONAIR,
    SPECIES_ARCTIBAX,
    SPECIES_DIGGERSBY,
    SPECIES_MOTHIM,
    SPECIES_WORMADAM_TRASH,
    SPECIES_WORMADAM_SANDY,
    SPECIES_WORMADAM_PLANT,
    SPECIES_WUGTRIO,
    SPECIES_SWOOBAT,
    SPECIES_SUNFLORA,
    SPECIES_MAROWAK_ALOLA,
    SPECIES_MAROWAK,
    SPECIES_DUGTRIO_ALOLA,
    SPECIES_DUGTRIO,
    SPECIES_EMOLGA,
    SPECIES_CLODSIRE,
    SPECIES_ILLUMISE,
    SPECIES_VOLBEAT,
    SPECIES_LAIRON,
    SPECIES_MAGCARGO,
    SPECIES_SNEASEL_HISUI,
    SPECIES_SNEASEL,
    SPECIES_GLIGAR,
    SPECIES_QUAGSIRE,
    SPECIES_PIKACHU_STARTER,
    SPECIES_DEDENNE,
    SPECIES_PINCURCHIN,
    SPECIES_TOGEDEMARU,
    SPECIES_MISDREAVUS,
    SPECIES_EEVEE_STARTER,
    SPECIES_TANGELA,
    SPECIES_WIGGLYTUFF,
    SPECIES_MORPEKO,
    SPECIES_PERRSERKER,
    SPECIES_MINIOR_METEOR,
    SPECIES_KLANG,
    SPECIES_ROTOM,
    SPECIES_KECLEON,
    SPECIES_VIGOROTH,
    SPECIES_PELIPPER,
    SPECIES_QWILFISH_HISUI,
    SPECIES_QWILFISH,
    SPECIES_SEADRA,
    SPECIES_PERSIAN_ALOLA,
    SPECIES_PERSIAN,
    SPECIES_FEAROW,
    SPECIES_AUDINO,
    SPECIES_LIEPARD,
    SPECIES_DOUBLADE,
    SPECIES_ARBOK,
    SPECIES_KLAWF,
    SPECIES_LOKIX,
    SPECIES_CHERRIM,
    SPECIES_PILOSWINE,
    SPECIES_GRANBULL,
    SPECIES_SEAKING,
    SPECIES_CHANSEY,
    SPECIES_VENOMOTH,
    SPECIES_SANDSLASH_ALOLA,
    SPECIES_SANDSLASH,
    SPECIES_SLIGGOO_HISUI,
    SPECIES_SLIGGOO,
    SPECIES_PURUGLY,
    SPECIES_NOCTOWL,
    SPECIES_ARAQUANID,
    SPECIES_CARNIVINE,
    SPECIES_MASQUERAIN,
    SPECIES_THIEVUL,
    SPECIES_CHIMECHO,
    SPECIES_DUSCLOPS,
    SPECIES_BANETTE,
    SPECIES_SWELLOW,
    SPECIES_HITMONTOP,
    SPECIES_GIRAFARIG,
    SPECIES_JYNX,
    SPECIES_HITMONCHAN,
    SPECIES_HITMONLEE,
    SPECIES_PRIMEAPE,
    SPECIES_GOLBAT,
    SPECIES_NINJASK,
    SPECIES_PALAFIN_ZERO,
    SPECIES_SEVIPER,
    SPECIES_ZANGOOSE,
    SPECIES_ELDEGOSS,
    SPECIES_GREEDENT,
    SPECIES_BASCULIN,
    SPECIES_LUMINEON,
    SPECIES_TROPIUS,
    SPECIES_SOLROCK,
    SPECIES_LUNATONE,
    SPECIES_CAMERUPT,
    SPECIES_SHARPEDO,
    SPECIES_BRELOOM,
    SPECIES_JUMPLUFF,
    SPECIES_LANTURN,
    SPECIES_MR_MIME_GALAR,
    SPECIES_MR_MIME,
    SPECIES_MARACTUS,
    SPECIES_AROMATISSE,
    SPECIES_RIBOMBEE,
    SPECIES_AMOONGUSS,
    SPECIES_SAWK,
    SPECIES_THROH,
    SPECIES_ABSOL,
    SPECIES_STANTLER,
    SPECIES_SKARMORY,
    SPECIES_FORRETRESS,
    SPECIES_MAGNETON,
    SPECIES_MEOWSTIC,
    SPECIES_SWALOT,
    SPECIES_CRAWDAUNT,
    SPECIES_WHISCASH,
    SPECIES_RABSCA,
    SPECIES_MAUSHOLD,
    SPECIES_EISCUE_NOICE,
    SPECIES_EISCUE_ICE,
    SPECIES_STONJOURNER,
    SPECIES_FALINKS,
    SPECIES_KLEFKI,
    SPECIES_ALOMOMOLA,
    SPECIES_CINCCINO,
    SPECIES_GRUMPIG,
    SPECIES_TORKOAL,
    SPECIES_XATU,
    SPECIES_DODRIO,
    SPECIES_STUNFISK_GALAR,
    SPECIES_STUNFISK,
    SPECIES_FURFROU,
    SPECIES_GALVANTULA,
    SPECIES_SWANNA,
    SPECIES_TREVENANT,
    SPECIES_GARBODOR,
    SPECIES_VESPIQUEN,
    SPECIES_HARIYAMA,
    SPECIES_TATSUGIRI,
    SPECIES_INDEEDEE_M,
    SPECIES_INDEEDEE_F,
    SPECIES_FROSMOTH,
    SPECIES_CRAMORANT,
    SPECIES_BRUXISH,
    SPECIES_SAWSBUCK,
    SPECIES_GASTRODON,
    SPECIES_CACTURNE,
    SPECIES_MANECTRIC,
    SPECIES_KINGLER,
    SPECIES_DEWGONG,
    SPECIES_MIMIKYU,
    SPECIES_ORICORIO,
    SPECIES_DACHSBUN,
    SPECIES_VELUZA,
    SPECIES_CRABOMINABLE,
    SPECIES_SKUNTANK,
    SPECIES_PIDGEOT,
    SPECIES_ORTHWORM,
    SPECIES_BRAMBLEGHAST,
    SPECIES_GRAPPLOCT,
    SPECIES_KOMALA,
    SPECIES_PALOSSAND,
    SPECIES_SALAZZLE,
    SPECIES_LURANTIS,
    SPECIES_SLURPUFF,
    SPECIES_JELLICENT,
    SPECIES_DARMANITAN_STANDARD,
    SPECIES_DARMANITAN_GALAR_STANDARD,
    SPECIES_LILLIGANT_HISUI,
    SPECIES_LILLIGANT,
    SPECIES_WHIMSICOTT,
    SPECIES_PHIONE,
    SPECIES_FROSLASS,
    SPECIES_LOPUNNY,
    SPECIES_GLALIE,
    SPECIES_SHIFTRY,
    SPECIES_LUDICOLO,
    SPECIES_OCTILLERY,
    SPECIES_ESPATHRA,
    SPECIES_HELIOLISK,
    SPECIES_MALAMAR,
    SPECIES_AMBIPOM,
    SPECIES_RUNERIGUS,
    SPECIES_GOLURK,
    SPECIES_COFAGRIGUS,
    SPECIES_HYPNO,
    SPECIES_CLEFABLE,
    SPECIES_DURANT,
    SPECIES_HEATMOR,
    SPECIES_DIPPLIN,
    SPECIES_BOMBIRDIER,
    SPECIES_GRAFAIAI,
    SPECIES_APPLETUN,
    SPECIES_FLAPPLE,
    SPECIES_DREDNAW,
    SPECIES_DRAMPA,
    SPECIES_TURTONATOR,
    SPECIES_COMFEY,
    SPECIES_TOUCANNON,
    SPECIES_DRUDDIGON,
    SPECIES_BEHEEYEM,
    SPECIES_CRUSTLE,
    SPECIES_SCOLIPEDE,
    SPECIES_SPIRITOMB,
    SPECIES_STARAPTOR,
    SPECIES_RELICANTH,
    SPECIES_GOREBYSS,
    SPECIES_HUNTAIL,
    SPECIES_MANTINE,
    SPECIES_RHYDON,
    SPECIES_RAICHU_ALOLA,
    SPECIES_RAICHU,
    SPECIES_SCOVILLAIN,
    SPECIES_ZYGARDE_10_POWER_CONSTRUCT,
    SPECIES_LYCANROC_MIDNIGHT,
    SPECIES_LYCANROC_MIDDAY,
    SPECIES_LYCANROC_DUSK,
    SPECIES_MUSHARNA,
    SPECIES_HOUNDSTONE,
    SPECIES_SCRAFTY,
    SPECIES_UNFEZANT,
    SPECIES_OINKOLOGNE_M,
    SPECIES_OINKOLOGNE_F,
    SPECIES_FERROTHORN,
    SPECIES_KILOWATTREL,
    SPECIES_PAWMOT,
    SPECIES_BARRASKEWDA,
    SPECIES_BOLTUND,
    SPECIES_DUBWOOL,
    SPECIES_PASSIMIAN,
    SPECIES_ORANGURU,
    SPECIES_BOUFFALANT,
    SPECIES_BISHARP,
    SPECIES_REUNICLUS,
    SPECIES_GOTHITELLE,
    SPECIES_SIGILYPH,
    SPECIES_TOXICROAK,
    SPECIES_ALTARIA,
    SPECIES_EXPLOUD,
    SPECIES_MILTANK,
    SPECIES_SLOWKING_GALAR,
    SPECIES_SLOWKING,
    SPECIES_BELLOSSOM,
    SPECIES_TAUROS_PALDEA_COMBAT,
    SPECIES_TAUROS_PALDEA_BLAZE,
    SPECIES_TAUROS_PALDEA_AQUA,
    SPECIES_TAUROS,
    SPECIES_ELECTABUZZ,
    SPECIES_KANGASKHAN,
    SPECIES_WEEZING_GALAR,
    SPECIES_WEEZING,
    SPECIES_ELECTRODE_HISUI,
    SPECIES_ELECTRODE,
    SPECIES_SLOWBRO_GALAR,
    SPECIES_SLOWBRO,
    SPECIES_VICTREEBEL,
    SPECIES_VILEPLUME,
    SPECIES_GOURGEIST_SUPER,
    SPECIES_GOURGEIST_SMALL,
    SPECIES_GOURGEIST_LARGE,
    SPECIES_GOURGEIST_AVERAGE,
    SPECIES_DRAGALGE,
    SPECIES_ABOMASNOW,
    SPECIES_BELLIBOLT,
    SPECIES_ALCREMIE,
    SPECIES_CORVIKNIGHT,
    SPECIES_TOXAPEX,
    SPECIES_PANGORO,
    SPECIES_ACCELGOR,
    SPECIES_ESCAVALIER,
    SPECIES_CARRACOSTA,
    SPECIES_MISMAGIUS,
    SPECIES_FLOATZEL,
    SPECIES_BASTIODON,
    SPECIES_RAMPARDOS,
    SPECIES_ARMALDO,
    SPECIES_CRADILY,
    SPECIES_KABUTOPS,
    SPECIES_OMASTAR,
    SPECIES_MAGMAR,
    SPECIES_GOLEM_ALOLA,
    SPECIES_GOLEM,
    SPECIES_ZEBSTRIKA,
    SPECIES_SIMIPOUR,
    SPECIES_SIMISEAR,
    SPECIES_SIMISAGE,
    SPECIES_DRIFBLIM,
    SPECIES_TALONFLAME,
    SPECIES_FLAMIGO,
    SPECIES_REVAVROOM,
    SPECIES_GARGANACL,
    SPECIES_KLEAVOR,
    SPECIES_CALYREX,
    SPECIES_COPPERAJAH,
    SPECIES_MINIOR_CORE,
    SPECIES_BEWEAR,
    SPECIES_MUDSDALE,
    SPECIES_VIKAVOLT,
    SPECIES_CARBINK,
    SPECIES_HAWLUCHA,
    SPECIES_CLAWITZER,
    SPECIES_BARBARACLE,
    SPECIES_AEGISLASH_SHIELD,
    SPECIES_AEGISLASH_BLADE,
    SPECIES_LEAVANNY,
    SPECIES_STOUTLAND,
    SPECIES_DRAPION,
    SPECIES_BRONZONG,
    SPECIES_CLAYDOL,
    SPECIES_WAILORD,
    SPECIES_DONPHAN,
    SPECIES_HOUNDOOM,
    SPECIES_URSARING,
    SPECIES_HERACROSS,
    SPECIES_SCIZOR,
    SPECIES_POLITOED,
    SPECIES_PINSIR,
    SPECIES_SCYTHER,
    SPECIES_GENGAR,
    SPECIES_MUK_ALOLA,
    SPECIES_MUK,
    SPECIES_RAPIDASH_GALAR,
    SPECIES_RAPIDASH,
    SPECIES_ALAKAZAM,
    SPECIES_GOLDUCK,
    SPECIES_CYCLIZAR,
    SPECIES_TOXTRICITY,
    SPECIES_MABOSSTIFF,
    SPECIES_ARCTOVISH,
    SPECIES_DRACOVISH,
    SPECIES_ARCTOZOLT,
    SPECIES_DRACOZOLT,
    SPECIES_ORBEETLE,
    SPECIES_BEARTIC,
    SPECIES_CONKELDURR,
    SPECIES_HONCHKROW,
    SPECIES_SHUCKLE,
    SPECIES_MACHAMP,
    SPECIES_NINETALES_ALOLA,
    SPECIES_NINETALES,
    SPECIES_NIDOKING,
    SPECIES_NIDOQUEEN,
    SPECIES_TINKATON,
    SPECIES_SIRFETCHD,
    SPECIES_PYROAR,
    SPECIES_SINISTCHA,
    SPECIES_POLTEAGEIST,
    SPECIES_EXCADRILL,
    SPECIES_SEISMITOAD,
    SPECIES_ARBOLIVA,
    SPECIES_OVERQWIL,
    SPECIES_SNEASLER,
    SPECIES_CURSOLA,
    SPECIES_GRIMMSNARL,
    SPECIES_HATTERENE,
    SPECIES_SANDACONDA,
    SPECIES_COALOSSAL,
    SPECIES_TSAREENA,
    SPECIES_MANDIBUZZ,
    SPECIES_BRAVIARY_HISUI,
    SPECIES_BRAVIARY,
    SPECIES_MIENSHAO,
    SPECIES_ZOROARK_HISUI,
    SPECIES_ZOROARK,
    SPECIES_GLISCOR,
    SPECIES_WEAVILE,
    SPECIES_STEELIX,
    SPECIES_AMPHAROS,
    SPECIES_POLIWRATH,
    SPECIES_AVALUGG_HISUI,
    SPECIES_AVALUGG,
    SPECIES_TOEDSCRUEL,
    SPECIES_CRYOGONAL,
    SPECIES_EELEKTROSS,
    SPECIES_GIGALITH,
    SPECIES_YANMEGA,
    SPECIES_LICKILICKY,
    SPECIES_ROSERADE,
    SPECIES_PORYGON2,
    SPECIES_AERODACTYL,
    SPECIES_TENTACRUEL,
    SPECIES_DHELMISE,
    SPECIES_GALLADE,
    SPECIES_GARDEVOIR,
    SPECIES_KROOKODILE,
    SPECIES_DUDUNSPARCE,
    SPECIES_FARIGIRAF,
    SPECIES_MR_RIME,
    SPECIES_OBSTAGOON,
    SPECIES_CHANDELURE,
    SPECIES_KLINKLANG,
    SPECIES_ROTOM_WASH,
    SPECIES_ROTOM_MOW,
    SPECIES_ROTOM_HEAT,
    SPECIES_ROTOM_FROST,
    SPECIES_ROTOM_FAN,
    SPECIES_FLYGON,
    SPECIES_STARMIE,
    SPECIES_CETITAN,
    SPECIES_AURORUS,
    SPECIES_TYRANTRUM,
    SPECIES_LUXRAY,
    SPECIES_GLIMMORA,
    SPECIES_CERULEDGE,
    SPECIES_ARMAROUGE,
    SPECIES_WYRDEER,
    SPECIES_CENTISKORCH,
    SPECIES_SYLVEON,
    SPECIES_DUSKNOIR,
    SPECIES_PROBOPASS,
    SPECIES_GLACEON,
    SPECIES_LEAFEON,
    SPECIES_HIPPOWDON,
    SPECIES_LUCARIO,
    SPECIES_TORTERRA,
    SPECIES_UMBREON,
    SPECIES_ESPEON,
    SPECIES_MEGANIUM,
    SPECIES_FLAREON,
    SPECIES_JOLTEON,
    SPECIES_VAPOREON,
    SPECIES_CLOYSTER,
    SPECIES_VENUSAUR,
    SPECIES_SAMUROTT_HISUI,
    SPECIES_SAMUROTT,
    SPECIES_EMBOAR,
    SPECIES_SERPERIOR,
    SPECIES_DONDOZO,
    SPECIES_QUAQUAVAL,
    SPECIES_SKELEDIRGE,
    SPECIES_MEOWSCARADA,
    SPECIES_BASCULEGION_M,
    SPECIES_BASCULEGION_F,
    SPECIES_INTELEON,
    SPECIES_CINDERACE,
    SPECIES_RILLABOOM,
    SPECIES_GOLISOPOD,
    SPECIES_PRIMARINA,
    SPECIES_INCINEROAR,
    SPECIES_DECIDUEYE_HISUI,
    SPECIES_DECIDUEYE,
    SPECIES_GRENINJA,
    SPECIES_CHESNAUGHT,
    SPECIES_MAMOSWINE,
    SPECIES_EMPOLEON,
    SPECIES_WALREIN,
    SPECIES_AGGRON,
    SPECIES_BLAZIKEN,
    SPECIES_SCEPTILE,
    SPECIES_FERALIGATR,
    SPECIES_EXEGGUTOR_ALOLA,
    SPECIES_EXEGGUTOR,
    SPECIES_BLASTOISE,
    SPECIES_GOGOAT,
    SPECIES_TYPE_NULL,
    SPECIES_DELPHOX,
    SPECIES_INFERNAPE,
    SPECIES_TYPHLOSION_HISUI,
    SPECIES_TYPHLOSION,
    SPECIES_CHARIZARD,
    SPECIES_ANNIHILAPE,
    SPECIES_DURALUDON,
    SPECIES_NOIVERN,
    SPECIES_VANILLUXE,
    SPECIES_PORYGON_Z,
    SPECIES_TANGROWTH,
    SPECIES_RHYPERIOR,
    SPECIES_MAGNEZONE,
    SPECIES_SWAMPERT,
    SPECIES_CROBAT,
    SPECIES_LAPRAS,
    SPECIES_HYDRAPPLE,
    SPECIES_NAGANADEL,
    SPECIES_HAXORUS,
    SPECIES_DARMANITAN_ZEN,
    SPECIES_DARMANITAN_GALAR_ZEN,
    SPECIES_MAGMORTAR,
    SPECIES_ELECTIVIRE,
    SPECIES_MILOTIC,
    SPECIES_BLISSEY,
    SPECIES_KINGDRA,
    SPECIES_SNORLAX,
    SPECIES_GYARADOS,
    SPECIES_TOGEKISS,
    SPECIES_OGERPON,
    SPECIES_GHOLDENGO,
    SPECIES_KINGAMBIT,
    SPECIES_URSALUNA,
    SPECIES_URSHIFU,
    SPECIES_VOLCARONA,
    SPECIES_FLOETTE_ETERNAL,
    SPECIES_FLORGES,
    SPECIES_FEZANDIPITI,
    SPECIES_MUNKIDORI,
    SPECIES_OKIDOGI,
    SPECIES_URSALUNA_BLOODMOON,
    SPECIES_ARCANINE_HISUI,
    SPECIES_ARCANINE,
    SPECIES_ARCHEOPS,
    SPECIES_IRON_THORNS,
    SPECIES_IRON_MOTH,
    SPECIES_IRON_JUGULIS,
    SPECIES_IRON_HANDS,
    SPECIES_IRON_BUNDLE,
    SPECIES_IRON_TREADS,
    SPECIES_SANDY_SHOCKS,
    SPECIES_SLITHER_WING,
    SPECIES_FLUTTER_MANE,
    SPECIES_BRUTE_BONNET,
    SPECIES_SCREAM_TAIL,
    SPECIES_GREAT_TUSK,
    SPECIES_BLACEPHALON,
    SPECIES_STAKATAKA,
    SPECIES_GUZZLORD,
    SPECIES_KARTANA,
    SPECIES_CELESTEELA,
    SPECIES_XURKITREE,
    SPECIES_PHEROMOSA,
    SPECIES_BUZZWOLE,
    SPECIES_NIHILEGO,
    SPECIES_IRON_CROWN,
    SPECIES_IRON_BOULDER,
    SPECIES_RAGING_BOLT,
    SPECIES_GOUGING_FIRE,
    SPECIES_IRON_LEAVES,
    SPECIES_WALKING_WAKE,
    SPECIES_IRON_VALIANT,
    SPECIES_ROARING_MOON,
    SPECIES_ARCHALUDON,
    SPECIES_BAXCALIBUR,
    SPECIES_DRAGAPULT,
    SPECIES_KOMMO_O,
    SPECIES_GOODRA_HISUI,
    SPECIES_GOODRA,
    SPECIES_HYDREIGON,
    SPECIES_GARCHOMP,
    SPECIES_METAGROSS,
    SPECIES_SALAMENCE,
    SPECIES_TYRANITAR,
    SPECIES_DRAGONITE,
    SPECIES_WISHIWASHI_SCHOOL,
    SPECIES_GRENINJA_ASH,
    SPECIES_PALAFIN_HERO,
    SPECIES_SLAKING,
};

EWRAM_DATA static u16 sRandomSpeciesWildMap[RANDOM_SPECIES_WILD_COUNT] = {0};
EWRAM_DATA static bool8 bRandomSpeciesWildMapIsSetup = FALSE;
void SetUpSpeciesWildLookup() {
    if (bRandomSpeciesWildMapIsSetup != TRUE) {
        for (u16 i = 0; i < RANDOM_SPECIES_WILD_COUNT; ++i)
        {
            sRandomSpeciesWildMap[sRandomSpeciesWildBst[i]] = i;
        }
        bRandomSpeciesWildMapIsSetup = TRUE;
    }
}

// #define RANDOM_SPECIES_EVO_0_COUNT ARRAY_COUNT(sRandomSpeciesEvo0)
// static const u16 sRandomSpeciesEvo0[] =
// {
//     SPECIES_BULBASAUR       ,    //= EVO_TYPE_0,
//     SPECIES_CHARMANDER      ,    //= EVO_TYPE_0,
//     SPECIES_SQUIRTLE        ,    //= EVO_TYPE_0,
//     SPECIES_CATERPIE        ,    //= EVO_TYPE_0,
//     SPECIES_WEEDLE          ,    //= EVO_TYPE_0,
//     SPECIES_PIDGEY          ,    //= EVO_TYPE_0,
//     SPECIES_RATTATA         ,    //= EVO_TYPE_0,
//     SPECIES_SPEAROW         ,    //= EVO_TYPE_0,
//     SPECIES_EKANS           ,    //= EVO_TYPE_0,
//     SPECIES_SANDSHREW       ,    //= EVO_TYPE_0,
//     SPECIES_NIDORAN_F       ,    //= EVO_TYPE_0,
//     SPECIES_NIDORAN_M       ,    //= EVO_TYPE_0,
//     SPECIES_VULPIX          ,    //= EVO_TYPE_0,
//     SPECIES_ZUBAT           ,    //= EVO_TYPE_0,
//     SPECIES_ODDISH          ,    //= EVO_TYPE_0,
//     SPECIES_PARAS           ,    //= EVO_TYPE_0,
//     SPECIES_VENONAT         ,    //= EVO_TYPE_0,
//     SPECIES_DIGLETT         ,    //= EVO_TYPE_0,
//     SPECIES_MEOWTH          ,    //= EVO_TYPE_0,
//     SPECIES_PSYDUCK         ,    //= EVO_TYPE_0,
//     SPECIES_MANKEY          ,    //= EVO_TYPE_0,
//     SPECIES_GROWLITHE       ,    //= EVO_TYPE_0,
//     SPECIES_POLIWAG         ,    //= EVO_TYPE_0,
//     SPECIES_ABRA            ,    //= EVO_TYPE_0,
//     SPECIES_MACHOP          ,    //= EVO_TYPE_0,
//     SPECIES_BELLSPROUT      ,    //= EVO_TYPE_0,
//     SPECIES_TENTACOOL       ,    //= EVO_TYPE_0,
//     SPECIES_GEODUDE         ,    //= EVO_TYPE_0,
//     SPECIES_PONYTA          ,    //= EVO_TYPE_0,
//     SPECIES_SLOWPOKE        ,    //= EVO_TYPE_0,
//     SPECIES_MAGNEMITE       ,    //= EVO_TYPE_0,
//     SPECIES_FARFETCHD       ,    //= EVO_TYPE_0,
//     SPECIES_DODUO           ,    //= EVO_TYPE_0,
//     SPECIES_SEEL            ,    //= EVO_TYPE_0,
//     SPECIES_GRIMER          ,    //= EVO_TYPE_0,
//     SPECIES_SHELLDER        ,    //= EVO_TYPE_0,
//     SPECIES_GASTLY          ,    //= EVO_TYPE_0,
//     SPECIES_ONIX            ,    //= EVO_TYPE_0,
//     SPECIES_DROWZEE         ,    //= EVO_TYPE_0,
//     SPECIES_KRABBY          ,    //= EVO_TYPE_0,
//     SPECIES_VOLTORB         ,    //= EVO_TYPE_0,
//     SPECIES_EXEGGCUTE       ,    //= EVO_TYPE_0,
//     SPECIES_CUBONE          ,    //= EVO_TYPE_0,
//     SPECIES_LICKITUNG       ,    //= EVO_TYPE_0,
//     SPECIES_KOFFING         ,    //= EVO_TYPE_0,
//     SPECIES_RHYHORN         ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_CHANSEY         ,    //= EVO_TYPE_0,
//     #endif
//     SPECIES_TANGELA         ,    //= EVO_TYPE_0,
//     SPECIES_KANGASKHAN      ,    //= EVO_TYPE_0,
//     SPECIES_HORSEA          ,    //= EVO_TYPE_0,
//     SPECIES_GOLDEEN         ,    //= EVO_TYPE_0,
//     SPECIES_STARYU          ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_MR_MIME         ,    //= EVO_TYPE_0,
//     #endif
//     SPECIES_SCYTHER         ,    //= EVO_TYPE_0,
//     SPECIES_PINSIR          ,    //= EVO_TYPE_0,
//     SPECIES_TAUROS          ,    //= EVO_TYPE_0,
//     SPECIES_MAGIKARP        ,    //= EVO_TYPE_0,
//     SPECIES_LAPRAS          ,    //= EVO_TYPE_0,
//     SPECIES_DITTO           ,    //= EVO_TYPE_0,
//     SPECIES_EEVEE           ,    //= EVO_TYPE_0,
//     SPECIES_PORYGON         ,    //= EVO_TYPE_0,
//     SPECIES_OMANYTE         ,    //= EVO_TYPE_0,
//     SPECIES_KABUTO          ,    //= EVO_TYPE_0,
//     SPECIES_AERODACTYL      ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_SNORLAX         ,    //= EVO_TYPE_0,
//     #endif
//     SPECIES_DRATINI         ,    //= EVO_TYPE_0,
//     SPECIES_CHIKORITA       ,    //= EVO_TYPE_0,
//     SPECIES_CYNDAQUIL       ,    //= EVO_TYPE_0,
//     SPECIES_TOTODILE        ,    //= EVO_TYPE_0,
//     SPECIES_SENTRET         ,    //= EVO_TYPE_0,
//     SPECIES_HOOTHOOT        ,    //= EVO_TYPE_0,
//     SPECIES_LEDYBA          ,    //= EVO_TYPE_0,
//     SPECIES_SPINARAK        ,    //= EVO_TYPE_0,
//     SPECIES_CHINCHOU        ,    //= EVO_TYPE_0,
//     SPECIES_PICHU           ,    //= EVO_TYPE_0,
//     SPECIES_CLEFFA          ,    //= EVO_TYPE_0,
//     SPECIES_IGGLYBUFF       ,    //= EVO_TYPE_0,
//     SPECIES_TOGEPI          ,    //= EVO_TYPE_0,
//     SPECIES_NATU            ,    //= EVO_TYPE_0,
//     SPECIES_MAREEP          ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_SUDOWOODO       ,    //= EVO_TYPE_0,
//     #endif
//     SPECIES_HOPPIP          ,    //= EVO_TYPE_0,
//     SPECIES_AIPOM           ,    //= EVO_TYPE_0,
//     SPECIES_SUNKERN         ,    //= EVO_TYPE_0,
//     SPECIES_YANMA           ,    //= EVO_TYPE_0,
//     SPECIES_WOOPER          ,    //= EVO_TYPE_0,
//     SPECIES_MURKROW         ,    //= EVO_TYPE_0,
//     SPECIES_MISDREAVUS      ,    //= EVO_TYPE_0,
//     SPECIES_UNOWN           ,    //= EVO_TYPE_0,
//     SPECIES_GIRAFARIG       ,    //= EVO_TYPE_0,
//     SPECIES_PINECO          ,    //= EVO_TYPE_0,
//     SPECIES_DUNSPARCE       ,    //= EVO_TYPE_0,
//     SPECIES_GLIGAR          ,    //= EVO_TYPE_0,
//     SPECIES_SNUBBULL        ,    //= EVO_TYPE_0,
//     SPECIES_QWILFISH        ,    //= EVO_TYPE_0,
//     SPECIES_SHUCKLE         ,    //= EVO_TYPE_0,
//     SPECIES_HERACROSS       ,    //= EVO_TYPE_0,
//     SPECIES_SNEASEL         ,    //= EVO_TYPE_0,
//     SPECIES_TEDDIURSA       ,    //= EVO_TYPE_0,
//     SPECIES_SLUGMA          ,    //= EVO_TYPE_0,
//     SPECIES_SWINUB          ,    //= EVO_TYPE_0,
//     SPECIES_CORSOLA         ,    //= EVO_TYPE_0,
//     SPECIES_REMORAID        ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_MANTINE         ,    //= EVO_TYPE_0,
//     #endif
//     SPECIES_DELIBIRD        ,    //= EVO_TYPE_0,
//     SPECIES_SKARMORY        ,    //= EVO_TYPE_0,
//     SPECIES_HOUNDOUR        ,    //= EVO_TYPE_0,
//     SPECIES_PHANPY          ,    //= EVO_TYPE_0,
//     SPECIES_STANTLER        ,    //= EVO_TYPE_0,
//     SPECIES_SMEARGLE        ,    //= EVO_TYPE_0,
//     SPECIES_TYROGUE         ,    //= EVO_TYPE_0,
//     SPECIES_SMOOCHUM        ,    //= EVO_TYPE_0,
//     SPECIES_ELEKID          ,    //= EVO_TYPE_0,
//     SPECIES_MAGBY           ,    //= EVO_TYPE_0,
//     SPECIES_MILTANK         ,    //= EVO_TYPE_0,
//     SPECIES_LARVITAR        ,    //= EVO_TYPE_0,
//     SPECIES_TREECKO         ,    //= EVO_TYPE_0,
//     SPECIES_TORCHIC         ,    //= EVO_TYPE_0,
//     SPECIES_MUDKIP          ,    //= EVO_TYPE_0,
//     SPECIES_POOCHYENA       ,    //= EVO_TYPE_0,
//     SPECIES_ZIGZAGOON       ,    //= EVO_TYPE_0,
//     SPECIES_WURMPLE         ,    //= EVO_TYPE_0,
//     SPECIES_LOTAD           ,    //= EVO_TYPE_0,
//     SPECIES_SEEDOT          ,    //= EVO_TYPE_0,
//     SPECIES_NINCADA         ,    //= EVO_TYPE_0,
//     SPECIES_TAILLOW         ,    //= EVO_TYPE_0,
//     SPECIES_SHROOMISH       ,    //= EVO_TYPE_0,
//     SPECIES_SPINDA          ,    //= EVO_TYPE_0,
//     SPECIES_WINGULL         ,    //= EVO_TYPE_0,
//     SPECIES_SURSKIT         ,    //= EVO_TYPE_0,
//     SPECIES_WAILMER         ,    //= EVO_TYPE_0,
//     SPECIES_SKITTY          ,    //= EVO_TYPE_0,
//     SPECIES_KECLEON         ,    //= EVO_TYPE_0,
//     SPECIES_BALTOY          ,    //= EVO_TYPE_0,
//     SPECIES_NOSEPASS        ,    //= EVO_TYPE_0,
//     SPECIES_TORKOAL         ,    //= EVO_TYPE_0,
//     SPECIES_SABLEYE         ,    //= EVO_TYPE_0,
//     SPECIES_BARBOACH        ,    //= EVO_TYPE_0,
//     SPECIES_LUVDISC         ,    //= EVO_TYPE_0,
//     SPECIES_CORPHISH        ,    //= EVO_TYPE_0,
//     SPECIES_FEEBAS          ,    //= EVO_TYPE_0,
//     SPECIES_CARVANHA        ,    //= EVO_TYPE_0,
//     SPECIES_TRAPINCH        ,    //= EVO_TYPE_0,
//     SPECIES_MAKUHITA        ,    //= EVO_TYPE_0,
//     SPECIES_ELECTRIKE       ,    //= EVO_TYPE_0,
//     SPECIES_NUMEL           ,    //= EVO_TYPE_0,
//     SPECIES_SPHEAL          ,    //= EVO_TYPE_0,
//     SPECIES_CACNEA          ,    //= EVO_TYPE_0,
//     SPECIES_SNORUNT         ,    //= EVO_TYPE_0,
//     SPECIES_LUNATONE        ,    //= EVO_TYPE_0,
//     SPECIES_SOLROCK         ,    //= EVO_TYPE_0,
//     SPECIES_AZURILL         ,    //= EVO_TYPE_0,
//     SPECIES_SPOINK          ,    //= EVO_TYPE_0,
//     SPECIES_PLUSLE          ,    //= EVO_TYPE_0,
//     SPECIES_MINUN           ,    //= EVO_TYPE_0,
//     SPECIES_MAWILE          ,    //= EVO_TYPE_0,
//     SPECIES_MEDITITE        ,    //= EVO_TYPE_0,
//     SPECIES_SWABLU          ,    //= EVO_TYPE_0,
//     SPECIES_WYNAUT          ,    //= EVO_TYPE_0,
//     SPECIES_DUSKULL         ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_ROSELIA         ,    //= EVO_TYPE_0,
//     #endif
//     SPECIES_SLAKOTH         ,    //= EVO_TYPE_0,
//     SPECIES_GULPIN          ,    //= EVO_TYPE_0,
//     SPECIES_TROPIUS         ,    //= EVO_TYPE_0,
//     SPECIES_WHISMUR         ,    //= EVO_TYPE_0,
//     SPECIES_CLAMPERL        ,    //= EVO_TYPE_0,
//     SPECIES_ABSOL           ,    //= EVO_TYPE_0,
//     SPECIES_SHUPPET         ,    //= EVO_TYPE_0,
//     SPECIES_SEVIPER         ,    //= EVO_TYPE_0,
//     SPECIES_ZANGOOSE        ,    //= EVO_TYPE_0,
//     SPECIES_RELICANTH       ,    //= EVO_TYPE_0,
//     SPECIES_ARON            ,    //= EVO_TYPE_0,
//     SPECIES_LILEEP          ,    //= EVO_TYPE_0,
//     SPECIES_ANORITH         ,    //= EVO_TYPE_0,
//     SPECIES_RALTS           ,    //= EVO_TYPE_0,
//     SPECIES_BAGON           ,    //= EVO_TYPE_0,
//     SPECIES_BELDUM          ,    //= EVO_TYPE_0,
//     #ifndef POKEMON_EXPANSION
//     SPECIES_CHIMECHO        ,    //= EVO_TYPE_0,
//     #else
//     SPECIES_TURTWIG           , //= EVO_TYPE_0,
//     SPECIES_CHIMCHAR          , //= EVO_TYPE_0,
//     SPECIES_PIPLUP            , //= EVO_TYPE_0,
//     SPECIES_STARLY            , //= EVO_TYPE_0,
//     SPECIES_BIDOOF            , //= EVO_TYPE_0,
//     SPECIES_KRICKETOT         , //= EVO_TYPE_0,
//     SPECIES_SHINX             , //= EVO_TYPE_0,
//     SPECIES_BUDEW             , //= EVO_TYPE_0,
//     SPECIES_CRANIDOS          , //= EVO_TYPE_0,
//     SPECIES_SHIELDON          , //= EVO_TYPE_0,
//     SPECIES_BURMY             , //= EVO_TYPE_0,
//     SPECIES_COMBEE            , //= EVO_TYPE_0,
//     SPECIES_PACHIRISU         , //= EVO_TYPE_0,
//     SPECIES_BUIZEL            , //= EVO_TYPE_0,
//     SPECIES_CHERUBI           , //= EVO_TYPE_0,
//     SPECIES_SHELLOS           , //= EVO_TYPE_0,
//     SPECIES_DRIFLOON          , //= EVO_TYPE_0,
//     SPECIES_BUNEARY           , //= EVO_TYPE_0,
//     SPECIES_GLAMEOW           , //= EVO_TYPE_0,
//     SPECIES_CHINGLING         , //= EVO_TYPE_0,
//     SPECIES_STUNKY            , //= EVO_TYPE_0,
//     SPECIES_BRONZOR           , //= EVO_TYPE_0,
//     SPECIES_BONSLY            , //= EVO_TYPE_0,
//     SPECIES_MIME_JR           , //= EVO_TYPE_0,
//     SPECIES_HAPPINY           , //= EVO_TYPE_0,
//     SPECIES_CHATOT            , //= EVO_TYPE_0,
//     SPECIES_SPIRITOMB         , //= EVO_TYPE_0,
//     SPECIES_GIBLE             , //= EVO_TYPE_0,
//     SPECIES_MUNCHLAX          , //= EVO_TYPE_0,
//     SPECIES_RIOLU             , //= EVO_TYPE_0,
//     SPECIES_HIPPOPOTAS        , //= EVO_TYPE_0,
//     SPECIES_SKORUPI           , //= EVO_TYPE_0,
//     SPECIES_CROAGUNK          , //= EVO_TYPE_0,
//     SPECIES_CARNIVINE         , //= EVO_TYPE_0,
//     SPECIES_FINNEON           , //= EVO_TYPE_0,
//     SPECIES_MANTYKE           , //= EVO_TYPE_0,
//     SPECIES_SNOVER            , //= EVO_TYPE_0,
//     SPECIES_ROTOM             , //= EVO_TYPE_0,
//     SPECIES_SNIVY             , //= EVO_TYPE_0,
//     SPECIES_TEPIG             , //= EVO_TYPE_0,
//     SPECIES_OSHAWOTT          , //= EVO_TYPE_0,
//     SPECIES_PATRAT            , //= EVO_TYPE_0,
//     SPECIES_LILLIPUP          , //= EVO_TYPE_0,
//     SPECIES_PURRLOIN          , //= EVO_TYPE_0,
//     SPECIES_PANSAGE           , //= EVO_TYPE_0,
//     SPECIES_PANSEAR           , //= EVO_TYPE_0,
//     SPECIES_PANPOUR           , //= EVO_TYPE_0,
//     SPECIES_MUNNA             , //= EVO_TYPE_0,
//     SPECIES_PIDOVE            , //= EVO_TYPE_0,
//     SPECIES_BLITZLE           , //= EVO_TYPE_0,
//     SPECIES_ROGGENROLA        , //= EVO_TYPE_0,
//     SPECIES_WOOBAT            , //= EVO_TYPE_0,
//     SPECIES_DRILBUR           , //= EVO_TYPE_0,
//     SPECIES_AUDINO            , //= EVO_TYPE_0,
//     SPECIES_TIMBURR           , //= EVO_TYPE_0,
//     SPECIES_TYMPOLE           , //= EVO_TYPE_0,
//     SPECIES_THROH             , //= EVO_TYPE_0,
//     SPECIES_SAWK              , //= EVO_TYPE_0,
//     SPECIES_SEWADDLE          , //= EVO_TYPE_0,
//     SPECIES_VENIPEDE          , //= EVO_TYPE_0,
//     SPECIES_COTTONEE          , //= EVO_TYPE_0,
//     SPECIES_PETILIL           , //= EVO_TYPE_0,
//     SPECIES_BASCULIN          , //= EVO_TYPE_0,
//     SPECIES_SANDILE           , //= EVO_TYPE_0,
//     SPECIES_DARUMAKA          , //= EVO_TYPE_0,
//     SPECIES_MARACTUS          , //= EVO_TYPE_0,
//     SPECIES_DWEBBLE           , //= EVO_TYPE_0,
//     SPECIES_SCRAGGY           , //= EVO_TYPE_0,
//     SPECIES_SIGILYPH          , //= EVO_TYPE_0,
//     SPECIES_YAMASK            , //= EVO_TYPE_0,
//     SPECIES_TIRTOUGA          , //= EVO_TYPE_0,
//     SPECIES_ARCHEN            , //= EVO_TYPE_0,
//     SPECIES_TRUBBISH          , //= EVO_TYPE_0,
//     SPECIES_ZORUA             , //= EVO_TYPE_0,
//     SPECIES_MINCCINO          , //= EVO_TYPE_0,
//     SPECIES_GOTHITA           , //= EVO_TYPE_0,
//     SPECIES_SOLOSIS           , //= EVO_TYPE_0,
//     SPECIES_DUCKLETT          , //= EVO_TYPE_0,
//     SPECIES_VANILLITE         , //= EVO_TYPE_0,
//     SPECIES_DEERLING          , //= EVO_TYPE_0,
//     SPECIES_EMOLGA            , //= EVO_TYPE_0,
//     SPECIES_KARRABLAST        , //= EVO_TYPE_0,
//     SPECIES_FOONGUS           , //= EVO_TYPE_0,
//     SPECIES_FRILLISH          , //= EVO_TYPE_0,
//     SPECIES_ALOMOMOLA         , //= EVO_TYPE_0,
//     SPECIES_JOLTIK            , //= EVO_TYPE_0,
//     SPECIES_FERROSEED         , //= EVO_TYPE_0,
//     SPECIES_KLINK             , //= EVO_TYPE_0,
//     SPECIES_TYNAMO            , //= EVO_TYPE_0,
//     SPECIES_ELGYEM            , //= EVO_TYPE_0,
//     SPECIES_LITWICK           , //= EVO_TYPE_0,
//     SPECIES_AXEW              , //= EVO_TYPE_0,
//     SPECIES_CUBCHOO           , //= EVO_TYPE_0,
//     SPECIES_CRYOGONAL         , //= EVO_TYPE_0,
//     SPECIES_SHELMET           , //= EVO_TYPE_0,
//     SPECIES_STUNFISK          , //= EVO_TYPE_0,
//     SPECIES_MIENFOO           , //= EVO_TYPE_0,
//     SPECIES_DRUDDIGON         , //= EVO_TYPE_0,
//     SPECIES_GOLETT            , //= EVO_TYPE_0,
//     SPECIES_PAWNIARD          , //= EVO_TYPE_0,
//     SPECIES_BOUFFALANT        , //= EVO_TYPE_0,
//     SPECIES_RUFFLET           , //= EVO_TYPE_0,
//     SPECIES_VULLABY           , //= EVO_TYPE_0,
//     SPECIES_HEATMOR           , //= EVO_TYPE_0,
//     SPECIES_DURANT            , //= EVO_TYPE_0,
//     SPECIES_DEINO             , //= EVO_TYPE_0,
//     SPECIES_LARVESTA          , //= EVO_TYPE_0,
//     SPECIES_CHESPIN           , //= EVO_TYPE_0,
//     SPECIES_FENNEKIN          , //= EVO_TYPE_0,
//     SPECIES_FROAKIE           , //= EVO_TYPE_0,
//     SPECIES_BUNNELBY          , //= EVO_TYPE_0,
//     SPECIES_FLETCHLING        , //= EVO_TYPE_0,
//     SPECIES_SCATTERBUG        , //= EVO_TYPE_0,
//     SPECIES_LITLEO            , //= EVO_TYPE_0,
//     SPECIES_FLABEBE           , //= EVO_TYPE_0,
//     SPECIES_SKIDDO            , //= EVO_TYPE_0,
//     SPECIES_PANCHAM           , //= EVO_TYPE_0,
//     SPECIES_FURFROU           , //= EVO_TYPE_0,
//     SPECIES_ESPURR            , //= EVO_TYPE_0,
//     SPECIES_HONEDGE           , //= EVO_TYPE_0,
//     SPECIES_SPRITZEE          , //= EVO_TYPE_0,
//     SPECIES_SWIRLIX           , //= EVO_TYPE_0,
//     SPECIES_INKAY             , //= EVO_TYPE_0,
//     SPECIES_BINACLE           , //= EVO_TYPE_0,
//     SPECIES_SKRELP            , //= EVO_TYPE_0,
//     SPECIES_CLAUNCHER         , //= EVO_TYPE_0,
//     SPECIES_HELIOPTILE        , //= EVO_TYPE_0,
//     SPECIES_TYRUNT            , //= EVO_TYPE_0,
//     SPECIES_AMAURA            , //= EVO_TYPE_0,
//     SPECIES_HAWLUCHA          , //= EVO_TYPE_0,
//     SPECIES_DEDENNE           , //= EVO_TYPE_0,
//     SPECIES_CARBINK           , //= EVO_TYPE_0,
//     SPECIES_GOOMY             , //= EVO_TYPE_0,
//     SPECIES_KLEFKI            , //= EVO_TYPE_0,
//     SPECIES_PHANTUMP          , //= EVO_TYPE_0,
//     SPECIES_PUMPKABOO         , //= EVO_TYPE_0,
//     SPECIES_BERGMITE          , //= EVO_TYPE_0,
//     SPECIES_NOIBAT            , //= EVO_TYPE_0,
//     SPECIES_ROWLET            , //= EVO_TYPE_0,
//     SPECIES_LITTEN            , //= EVO_TYPE_0,
//     SPECIES_POPPLIO           , //= EVO_TYPE_0,
//     SPECIES_PIKIPEK           , //= EVO_TYPE_0,
//     SPECIES_YUNGOOS           , //= EVO_TYPE_0,
//     SPECIES_GRUBBIN           , //= EVO_TYPE_0,
//     SPECIES_CRABRAWLER        , //= EVO_TYPE_0,
//     SPECIES_ORICORIO          , //= EVO_TYPE_0,
//     SPECIES_CUTIEFLY          , //= EVO_TYPE_0,
//     SPECIES_ROCKRUFF          , //= EVO_TYPE_0,
//     SPECIES_WISHIWASHI        , //= EVO_TYPE_0,
//     SPECIES_MAREANIE          , //= EVO_TYPE_0,
//     SPECIES_MUDBRAY           , //= EVO_TYPE_0,
//     SPECIES_DEWPIDER          , //= EVO_TYPE_0,
//     SPECIES_FOMANTIS          , //= EVO_TYPE_0,
//     SPECIES_MORELULL          , //= EVO_TYPE_0,
//     SPECIES_SALANDIT          , //= EVO_TYPE_0,
//     SPECIES_STUFFUL           , //= EVO_TYPE_0,
//     SPECIES_BOUNSWEET         , //= EVO_TYPE_0,
//     SPECIES_COMFEY            , //= EVO_TYPE_0,
//     SPECIES_ORANGURU          , //= EVO_TYPE_0,
//     SPECIES_PASSIMIAN         , //= EVO_TYPE_0,
//     SPECIES_WIMPOD            , //= EVO_TYPE_0,
//     SPECIES_SANDYGAST         , //= EVO_TYPE_0,
//     SPECIES_PYUKUMUKU         , //= EVO_TYPE_0,
//     SPECIES_MINIOR            , //= EVO_TYPE_0,
//     SPECIES_KOMALA            , //= EVO_TYPE_0,
//     SPECIES_TURTONATOR        , //= EVO_TYPE_0,
//     SPECIES_TOGEDEMARU        , //= EVO_TYPE_0,
//     SPECIES_MIMIKYU           , //= EVO_TYPE_0,
//     SPECIES_BRUXISH           , //= EVO_TYPE_0,
//     SPECIES_DRAMPA            , //= EVO_TYPE_0,
//     SPECIES_DHELMISE          , //= EVO_TYPE_0,
//     SPECIES_JANGMO_O          , //= EVO_TYPE_0,
//     SPECIES_GROOKEY           , //= EVO_TYPE_0,
//     SPECIES_SCORBUNNY         , //= EVO_TYPE_0,
//     SPECIES_SOBBLE            , //= EVO_TYPE_0,
//     SPECIES_SKWOVET           , //= EVO_TYPE_0,
//     SPECIES_ROOKIDEE          , //= EVO_TYPE_0,
//     SPECIES_BLIPBUG           , //= EVO_TYPE_0,
//     SPECIES_NICKIT            , //= EVO_TYPE_0,
//     SPECIES_GOSSIFLEUR        , //= EVO_TYPE_0,
//     SPECIES_WOOLOO            , //= EVO_TYPE_0,
//     SPECIES_CHEWTLE           , //= EVO_TYPE_0,
//     SPECIES_YAMPER            , //= EVO_TYPE_0,
//     SPECIES_ROLYCOLY          , //= EVO_TYPE_0,
//     SPECIES_APPLIN            , //= EVO_TYPE_0,
//     SPECIES_SILICOBRA         , //= EVO_TYPE_0,
//     SPECIES_CRAMORANT         , //= EVO_TYPE_0,
//     SPECIES_ARROKUDA          , //= EVO_TYPE_0,
//     SPECIES_TOXEL             , //= EVO_TYPE_0,
//     SPECIES_SIZZLIPEDE        , //= EVO_TYPE_0,
//     SPECIES_CLOBBOPUS         , //= EVO_TYPE_0,
//     SPECIES_SINISTEA          , //= EVO_TYPE_0,
//     SPECIES_HATENNA           , //= EVO_TYPE_0,
//     SPECIES_IMPIDIMP          , //= EVO_TYPE_0,
//     SPECIES_MILCERY           , //= EVO_TYPE_0,
//     SPECIES_FALINKS           , //= EVO_TYPE_0,
//     SPECIES_PINCURCHIN        , //= EVO_TYPE_0,
//     SPECIES_SNOM              , //= EVO_TYPE_0,
//     SPECIES_STONJOURNER       , //= EVO_TYPE_0,
//     SPECIES_EISCUE            , //= EVO_TYPE_0,
//     SPECIES_INDEEDEE          , //= EVO_TYPE_0,
//     SPECIES_MORPEKO           , //= EVO_TYPE_0,
//     SPECIES_CUFANT            , //= EVO_TYPE_0,
//     SPECIES_DRACOZOLT         , //= EVO_TYPE_0,
//     SPECIES_ARCTOZOLT         , //= EVO_TYPE_0,
//     SPECIES_DRACOVISH         , //= EVO_TYPE_0,
//     SPECIES_ARCTOVISH         , //= EVO_TYPE_0,
//     SPECIES_DURALUDON         , //= EVO_TYPE_0,
//     SPECIES_DREEPY            , //= EVO_TYPE_0,
//     SPECIES_RATTATA_ALOLA     , //= EVO_TYPE_0,
//     SPECIES_SANDSHREW_ALOLA   , //= EVO_TYPE_0,
//     SPECIES_VULPIX_ALOLA      , //= EVO_TYPE_0,
//     SPECIES_DIGLETT_ALOLA     , //= EVO_TYPE_0,
//     SPECIES_MEOWTH_ALOLA      , //= EVO_TYPE_0,
//     SPECIES_GEODUDE_ALOLA     , //= EVO_TYPE_0,
//     SPECIES_GRIMER_ALOLA      , //= EVO_TYPE_0,
//     SPECIES_MEOWTH_GALAR    , //= EVO_TYPE_0,
//     SPECIES_PONYTA_GALAR    , //= EVO_TYPE_0,
//     SPECIES_SLOWPOKE_GALAR  , //= EVO_TYPE_0,
//     SPECIES_FARFETCHD_GALAR , //= EVO_TYPE_0,
//     SPECIES_ZIGZAGOON_GALAR , //= EVO_TYPE_0,
//     SPECIES_DARUMAKA_GALAR  , //= EVO_TYPE_0,
//     SPECIES_YAMASK_GALAR    , //= EVO_TYPE_0,
//     SPECIES_STUNFISK_GALAR  , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_COSPLAY    , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_ROCK_STAR  , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_BELLE      , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_POP_STAR   , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_PHD       , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_LIBRE      , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_ORIGINAL , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_HOENN  , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_SINNOH , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_UNOVA  , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_KALOS  , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_ALOLA  , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_PARTNER , //= EVO_TYPE_0,
//     SPECIES_PIKACHU_WORLD  , //= EVO_TYPE_0,
//     SPECIES_PICHU_SPIKY_EARED  , //= EVO_TYPE_0,
//     SPECIES_BURMY_SANDY  , //= EVO_TYPE_0,
//     SPECIES_BURMY_TRASH  , //= EVO_TYPE_0,
//     SPECIES_SHELLOS_EAST   , //= EVO_TYPE_0,
//     SPECIES_ROTOM_HEAT         , //= EVO_TYPE_0,
//     SPECIES_ROTOM_WASH         , //= EVO_TYPE_0,
//     SPECIES_ROTOM_FROST        , //= EVO_TYPE_0,
//     SPECIES_ROTOM_FAN          , //= EVO_TYPE_0,
//     SPECIES_ROTOM_MOW          , //= EVO_TYPE_0,
//     SPECIES_BASCULIN_BLUE_STRIPED , //= EVO_TYPE_0,
//     SPECIES_DEERLING_SUMMER    , //= EVO_TYPE_0,
//     SPECIES_DEERLING_AUTUMN    , //= EVO_TYPE_0,
//     SPECIES_DEERLING_WINTER    , //= EVO_TYPE_0,
//     SPECIES_FLABEBE_YELLOW , //= EVO_TYPE_0,
//     SPECIES_FLABEBE_ORANGE , //= EVO_TYPE_0,
//     SPECIES_FLABEBE_BLUE , //= EVO_TYPE_0,
//     SPECIES_FLABEBE_WHITE , //= EVO_TYPE_0,
//     SPECIES_FURFROU_HEART , //= EVO_TYPE_0,
//     SPECIES_FURFROU_STAR  , //= EVO_TYPE_0,
//     SPECIES_FURFROU_DIAMOND , //= EVO_TYPE_0,
//     SPECIES_FURFROU_DEBUTANTE , //= EVO_TYPE_0,
//     SPECIES_FURFROU_MATRON , //= EVO_TYPE_0,
//     SPECIES_FURFROU_DANDY , //= EVO_TYPE_0,
//     SPECIES_FURFROU_LA_REINE , //= EVO_TYPE_0,
//     SPECIES_FURFROU_KABUKI , //= EVO_TYPE_0,
//     SPECIES_FURFROU_PHARAOH , //= EVO_TYPE_0,
//     SPECIES_PUMPKABOO_SMALL    , //= EVO_TYPE_0,
//     SPECIES_PUMPKABOO_LARGE    , //= EVO_TYPE_0,
//     SPECIES_PUMPKABOO_SUPER    , //= EVO_TYPE_0,
//     SPECIES_ORICORIO_POM_POM   , //= EVO_TYPE_0,
//     SPECIES_ORICORIO_PAU       , //= EVO_TYPE_0,
//     SPECIES_ORICORIO_SENSU     , //= EVO_TYPE_0,
//     SPECIES_ROCKRUFF_OWN_TEMPO , //= EVO_TYPE_0,
//     SPECIES_WISHIWASHI_SCHOOL  , //= EVO_TYPE_0,
//     SPECIES_MINIOR_METEOR_ORANGE , //= EVO_TYPE_0,
//     SPECIES_MINIOR_METEOR_YELLOW , //= EVO_TYPE_0,
//     SPECIES_MINIOR_METEOR_GREEN , //= EVO_TYPE_0,
//     SPECIES_MINIOR_METEOR_BLUE , //= EVO_TYPE_0,
//     SPECIES_MINIOR_METEOR_INDIGO , //= EVO_TYPE_0,
//     SPECIES_MINIOR_METEOR_VIOLET , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_RED    , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_ORANGE , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_YELLOW , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_GREEN  , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_BLUE   , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_INDIGO , //= EVO_TYPE_0,
//     SPECIES_MINIOR_CORE_VIOLET , //= EVO_TYPE_0,
//     SPECIES_MIMIKYU_BUSTED     , //= EVO_TYPE_0,
//     SPECIES_CRAMORANT_GULPING  , //= EVO_TYPE_0,
//     SPECIES_CRAMORANT_GORGING  , //= EVO_TYPE_0,
//     SPECIES_SINISTEA_ANTIQUE   , //= EVO_TYPE_0,
//     SPECIES_EISCUE_NOICE  , //= EVO_TYPE_0,
//     SPECIES_MORPEKO_HANGRY     , //= EVO_TYPE_0,
//     #endif
// };
// #define RANDOM_SPECIES_EVO_1_COUNT ARRAY_COUNT(sRandomSpeciesEvo1)
// static const u16 sRandomSpeciesEvo1[] =
// {
//     SPECIES_IVYSAUR         , //= EVO_TYPE_1,
//     SPECIES_CHARMELEON      , //= EVO_TYPE_1,
//     SPECIES_WARTORTLE       , //= EVO_TYPE_1,
//     SPECIES_METAPOD         , //= EVO_TYPE_1,
//     SPECIES_KAKUNA          , //= EVO_TYPE_1,
//     SPECIES_PIDGEOTTO       , //= EVO_TYPE_1,
//     SPECIES_RATICATE        , //= EVO_TYPE_1,
//     SPECIES_FEAROW          , //= EVO_TYPE_1,
//     SPECIES_ARBOK           , //= EVO_TYPE_1,
//     SPECIES_PIKACHU         , //= EVO_TYPE_1,
//     SPECIES_SANDSLASH       , //= EVO_TYPE_1,
//     SPECIES_NIDORINA        , //= EVO_TYPE_1,
//     SPECIES_NIDORINO        , //= EVO_TYPE_1,
//     SPECIES_CLEFAIRY        , //= EVO_TYPE_1,
//     SPECIES_NINETALES       , //= EVO_TYPE_1,
//     SPECIES_JIGGLYPUFF      , //= EVO_TYPE_1,
//     SPECIES_GOLBAT          , //= EVO_TYPE_1,
//     SPECIES_GLOOM           , //= EVO_TYPE_1,
//     SPECIES_PARASECT        , //= EVO_TYPE_1,
//     SPECIES_VENOMOTH        , //= EVO_TYPE_1,
//     SPECIES_DUGTRIO         , //= EVO_TYPE_1,
//     SPECIES_PERSIAN         , //= EVO_TYPE_1,
//     SPECIES_GOLDUCK         , //= EVO_TYPE_1,
//     SPECIES_PRIMEAPE        , //= EVO_TYPE_1,
//     SPECIES_ARCANINE        , //= EVO_TYPE_1,
//     SPECIES_POLIWHIRL       , //= EVO_TYPE_1,
//     SPECIES_KADABRA         , //= EVO_TYPE_1,
//     SPECIES_MACHOKE         , //= EVO_TYPE_1,
//     SPECIES_WEEPINBELL      , //= EVO_TYPE_1,
//     SPECIES_TENTACRUEL      , //= EVO_TYPE_1,
//     SPECIES_GRAVELER        , //= EVO_TYPE_1,
//     SPECIES_RAPIDASH        , //= EVO_TYPE_1,
//     SPECIES_MAGNETON        , //= EVO_TYPE_1,
//     SPECIES_DODRIO          , //= EVO_TYPE_1,
//     SPECIES_DEWGONG         , //= EVO_TYPE_1,
//     SPECIES_MUK             , //= EVO_TYPE_1,
//     SPECIES_CLOYSTER        , //= EVO_TYPE_1,
//     SPECIES_HAUNTER         , //= EVO_TYPE_1,
//     SPECIES_HYPNO           , //= EVO_TYPE_1,
//     SPECIES_KINGLER         , //= EVO_TYPE_1,
//     SPECIES_ELECTRODE       , //= EVO_TYPE_1,
//     SPECIES_EXEGGUTOR       , //= EVO_TYPE_1,
//     SPECIES_MAROWAK         , //= EVO_TYPE_1,
//     SPECIES_HITMONLEE       , //= EVO_TYPE_1,
//     SPECIES_HITMONCHAN      , //= EVO_TYPE_1,
//     SPECIES_WEEZING         , //= EVO_TYPE_1,
//     SPECIES_RHYDON          , //= EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_CHANSEY         , //= EVO_TYPE_1,
//     #endif
//     SPECIES_SEADRA          , //= EVO_TYPE_1,
//     SPECIES_SEAKING         , //= EVO_TYPE_1,
//     SPECIES_STARMIE         , //= EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_MR_MIME         , //= EVO_TYPE_1,
//     #endif
//     SPECIES_JYNX            , //= EVO_TYPE_1,
//     SPECIES_ELECTABUZZ      , //= EVO_TYPE_1,
//     SPECIES_MAGMAR          , //= EVO_TYPE_1,
//     SPECIES_VAPOREON        , //= EVO_TYPE_1,
//     SPECIES_JOLTEON         , //= EVO_TYPE_1,
//     SPECIES_FLAREON         , //= EVO_TYPE_1,
//     SPECIES_OMASTAR         , //= EVO_TYPE_1,
//     SPECIES_KABUTOPS        , //= EVO_TYPE_1,
//     SPECIES_DRAGONAIR       , //= EVO_TYPE_1,
//     SPECIES_BAYLEEF         , //= EVO_TYPE_1,
//     SPECIES_QUILAVA         , //= EVO_TYPE_1,
//     SPECIES_CROCONAW        , //= EVO_TYPE_1,
//     SPECIES_FURRET          , //= EVO_TYPE_1,
//     SPECIES_NOCTOWL         , //= EVO_TYPE_1,
//     SPECIES_LEDIAN          , //= EVO_TYPE_1,
//     SPECIES_ARIADOS         , //= EVO_TYPE_1,
//     SPECIES_LANTURN         , //= EVO_TYPE_1,
//     SPECIES_TOGETIC         , //= EVO_TYPE_1,
//     SPECIES_XATU            , //= EVO_TYPE_1,
//     SPECIES_FLAAFFY         , //= EVO_TYPE_1,
//     SPECIES_MARILL          , //= EVO_TYPE_1,
//     SPECIES_SKIPLOOM        , //= EVO_TYPE_1,
//     SPECIES_SUNFLORA        , //= EVO_TYPE_1,
//     SPECIES_QUAGSIRE        , //= EVO_TYPE_1,
//     SPECIES_ESPEON          , //= EVO_TYPE_1,
//     SPECIES_UMBREON         , //= EVO_TYPE_1,
//     SPECIES_WOBBUFFET       , //= EVO_TYPE_1,
//     SPECIES_FORRETRESS      , //= EVO_TYPE_1,
//     SPECIES_STEELIX         , //= EVO_TYPE_1,
//     SPECIES_GRANBULL        , //= EVO_TYPE_1,
//     SPECIES_SCIZOR          , //= EVO_TYPE_1,
//     SPECIES_URSARING        , //= EVO_TYPE_1,
//     SPECIES_MAGCARGO        , //= EVO_TYPE_1,
//     SPECIES_PILOSWINE       , //= EVO_TYPE_1,
//     SPECIES_OCTILLERY       , //= EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_MANTINE         , //= EVO_TYPE_1,
//     #endif
//     SPECIES_HOUNDOOM        , //= EVO_TYPE_1,
//     SPECIES_DONPHAN         , //= EVO_TYPE_1,
//     SPECIES_PORYGON2        , //= EVO_TYPE_1,
//     SPECIES_HITMONTOP       , //= EVO_TYPE_1,
//     SPECIES_PUPITAR         , //= EVO_TYPE_1,
//     SPECIES_GROVYLE         , //= EVO_TYPE_1,
//     SPECIES_COMBUSKEN       , //= EVO_TYPE_1,
//     SPECIES_MARSHTOMP       , //= EVO_TYPE_1,
//     SPECIES_MIGHTYENA       , //= EVO_TYPE_1,
//     SPECIES_LINOONE         , //= EVO_TYPE_1,
//     SPECIES_SILCOON         , //= EVO_TYPE_1,
//     SPECIES_CASCOON         , //= EVO_TYPE_1,
//     SPECIES_LOMBRE          , //= EVO_TYPE_1,
//     SPECIES_NUZLEAF         , //= EVO_TYPE_1,
//     SPECIES_NINJASK         , //= EVO_TYPE_1,
//     SPECIES_SHEDINJA        , //= EVO_TYPE_1,
//     SPECIES_SWELLOW         , //= EVO_TYPE_1,
//     SPECIES_BRELOOM         , //= EVO_TYPE_1,
//     SPECIES_PELIPPER        , //= EVO_TYPE_1,
//     SPECIES_MASQUERAIN      , //= EVO_TYPE_1,
//     SPECIES_WAILORD         , //= EVO_TYPE_1,
//     SPECIES_DELCATTY        , //= EVO_TYPE_1,
//     SPECIES_CLAYDOL         , //= EVO_TYPE_1,
//     SPECIES_WHISCASH        , //= EVO_TYPE_1,
//     SPECIES_CRAWDAUNT       , //= EVO_TYPE_1,
//     SPECIES_MILOTIC         , //= EVO_TYPE_1,
//     SPECIES_SHARPEDO        , //= EVO_TYPE_1,
//     SPECIES_VIBRAVA         , //= EVO_TYPE_1,
//     SPECIES_HARIYAMA        , //= EVO_TYPE_1,
//     SPECIES_MANECTRIC       , //= EVO_TYPE_1,
//     SPECIES_CAMERUPT        , //= EVO_TYPE_1,
//     SPECIES_SEALEO          , //= EVO_TYPE_1,
//     SPECIES_CACTURNE        , //= EVO_TYPE_1,
//     SPECIES_GLALIE          , //= EVO_TYPE_1,
//     SPECIES_GRUMPIG         , //= EVO_TYPE_1,
//     SPECIES_MEDICHAM        , //= EVO_TYPE_1,
//     SPECIES_ALTARIA         , //= EVO_TYPE_1,
//     SPECIES_DUSCLOPS        , //= EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_ROSELIA         , //= EVO_TYPE_1,
//     #endif
//     SPECIES_VIGOROTH        , //= EVO_TYPE_1,
//     SPECIES_SWALOT          , //= EVO_TYPE_1,
//     SPECIES_LOUDRED         , //= EVO_TYPE_1,
//     SPECIES_HUNTAIL         , //= EVO_TYPE_1,
//     SPECIES_GOREBYSS        , //= EVO_TYPE_1,
//     SPECIES_BANETTE         , //= EVO_TYPE_1,
//     SPECIES_LAIRON          , //= EVO_TYPE_1,
//     SPECIES_VOLBEAT         , //= EVO_TYPE_1,
//     SPECIES_ILLUMISE        , //= EVO_TYPE_1,
//     SPECIES_CRADILY         , //= EVO_TYPE_1,
//     SPECIES_ARMALDO         , //= EVO_TYPE_1,
//     SPECIES_KIRLIA          , //= EVO_TYPE_1,
//     SPECIES_SHELGON         , //= EVO_TYPE_1,
//     SPECIES_METANG          , //= EVO_TYPE_1,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_CHIMECHO        , //= EVO_TYPE_1,
//     SPECIES_GROTLE            , //= EVO_TYPE_1,
//     SPECIES_MONFERNO          , //= EVO_TYPE_1,
//     SPECIES_PRINPLUP          , //= EVO_TYPE_1,
//     SPECIES_STARAVIA          , //= EVO_TYPE_1,
//     SPECIES_BIBAREL           , //= EVO_TYPE_1,
//     SPECIES_KRICKETUNE        , //= EVO_TYPE_1,
//     SPECIES_LUXIO             , //= EVO_TYPE_1,
//     SPECIES_RAMPARDOS         , //= EVO_TYPE_1,
//     SPECIES_BASTIODON         , //= EVO_TYPE_1,
//     SPECIES_WORMADAM          , //= EVO_TYPE_1,
//     SPECIES_MOTHIM            , //= EVO_TYPE_1,
//     SPECIES_VESPIQUEN         , //= EVO_TYPE_1,
//     SPECIES_FLOATZEL          , //= EVO_TYPE_1,
//     SPECIES_CHERRIM           , //= EVO_TYPE_1,
//     SPECIES_GASTRODON         , //= EVO_TYPE_1,
//     SPECIES_AMBIPOM           , //= EVO_TYPE_1,
//     SPECIES_DRIFBLIM          , //= EVO_TYPE_1,
//     SPECIES_LOPUNNY           , //= EVO_TYPE_1,
//     SPECIES_MISMAGIUS         , //= EVO_TYPE_1,
//     SPECIES_HONCHKROW         , //= EVO_TYPE_1,
//     SPECIES_PURUGLY           , //= EVO_TYPE_1,
//     SPECIES_SKUNTANK          , //= EVO_TYPE_1,
//     SPECIES_BRONZONG          , //= EVO_TYPE_1,
//     SPECIES_GABITE            , //= EVO_TYPE_1,
//     SPECIES_LUCARIO           , //= EVO_TYPE_1,
//     SPECIES_HIPPOWDON         , //= EVO_TYPE_1,
//     SPECIES_DRAPION           , //= EVO_TYPE_1,
//     SPECIES_TOXICROAK         , //= EVO_TYPE_1,
//     SPECIES_LUMINEON          , //= EVO_TYPE_1,
//     SPECIES_ABOMASNOW         , //= EVO_TYPE_1,
//     SPECIES_WEAVILE           , //= EVO_TYPE_1,
//     SPECIES_LICKILICKY        , //= EVO_TYPE_1,
//     SPECIES_TANGROWTH         , //= EVO_TYPE_1,
//     SPECIES_YANMEGA           , //= EVO_TYPE_1,
//     SPECIES_LEAFEON           , //= EVO_TYPE_1,
//     SPECIES_GLACEON           , //= EVO_TYPE_1,
//     SPECIES_GLISCOR           , //= EVO_TYPE_1,
//     SPECIES_PROBOPASS         , //= EVO_TYPE_1,
//     SPECIES_FROSLASS          , //= EVO_TYPE_1,
//     SPECIES_SERVINE           , //= EVO_TYPE_1,
//     SPECIES_PIGNITE           , //= EVO_TYPE_1,
//     SPECIES_DEWOTT            , //= EVO_TYPE_1,
//     SPECIES_WATCHOG           , //= EVO_TYPE_1,
//     SPECIES_HERDIER           , //= EVO_TYPE_1,
//     SPECIES_LIEPARD           , //= EVO_TYPE_1,
//     SPECIES_SIMISAGE          , //= EVO_TYPE_1,
//     SPECIES_SIMISEAR          , //= EVO_TYPE_1,
//     SPECIES_SIMIPOUR          , //= EVO_TYPE_1,
//     SPECIES_MUSHARNA          , //= EVO_TYPE_1,
//     SPECIES_TRANQUILL         , //= EVO_TYPE_1,
//     SPECIES_ZEBSTRIKA         , //= EVO_TYPE_1,
//     SPECIES_BOLDORE           , //= EVO_TYPE_1,
//     SPECIES_SWOOBAT           , //= EVO_TYPE_1,
//     SPECIES_EXCADRILL         , //= EVO_TYPE_1,
//     SPECIES_GURDURR           , //= EVO_TYPE_1,
//     SPECIES_PALPITOAD         , //= EVO_TYPE_1,
//     SPECIES_SWADLOON          , //= EVO_TYPE_1,
//     SPECIES_WHIRLIPEDE        , //= EVO_TYPE_1,
//     SPECIES_WHIMSICOTT        , //= EVO_TYPE_1,
//     SPECIES_LILLIGANT         , //= EVO_TYPE_1,
//     SPECIES_KROKOROK          , //= EVO_TYPE_1,
//     SPECIES_DARMANITAN        , //= EVO_TYPE_1,
//     SPECIES_CRUSTLE           , //= EVO_TYPE_1,
//     SPECIES_SCRAFTY           , //= EVO_TYPE_1,
//     SPECIES_COFAGRIGUS        , //= EVO_TYPE_1,
//     SPECIES_CARRACOSTA        , //= EVO_TYPE_1,
//     SPECIES_ARCHEOPS          , //= EVO_TYPE_1,
//     SPECIES_GARBODOR          , //= EVO_TYPE_1,
//     SPECIES_ZOROARK           , //= EVO_TYPE_1,
//     SPECIES_CINCCINO          , //= EVO_TYPE_1,
//     SPECIES_GOTHORITA         , //= EVO_TYPE_1,
//     SPECIES_DUOSION           , //= EVO_TYPE_1,
//     SPECIES_SWANNA            , //= EVO_TYPE_1,
//     SPECIES_VANILLISH         , //= EVO_TYPE_1,
//     SPECIES_SAWSBUCK          , //= EVO_TYPE_1,
//     SPECIES_ESCAVALIER        , //= EVO_TYPE_1,
//     SPECIES_AMOONGUSS         , //= EVO_TYPE_1,
//     SPECIES_JELLICENT         , //= EVO_TYPE_1,
//     SPECIES_GALVANTULA        , //= EVO_TYPE_1,
//     SPECIES_FERROTHORN        , //= EVO_TYPE_1,
//     SPECIES_KLANG             , //= EVO_TYPE_1,
//     SPECIES_EELEKTRIK         , //= EVO_TYPE_1,
//     SPECIES_BEHEEYEM          , //= EVO_TYPE_1,
//     SPECIES_LAMPENT           , //= EVO_TYPE_1,
//     SPECIES_FRAXURE           , //= EVO_TYPE_1,
//     SPECIES_BEARTIC           , //= EVO_TYPE_1,
//     SPECIES_ACCELGOR          , //= EVO_TYPE_1,
//     SPECIES_MIENSHAO          , //= EVO_TYPE_1,
//     SPECIES_GOLURK            , //= EVO_TYPE_1,
//     SPECIES_BISHARP           , //= EVO_TYPE_1,
//     SPECIES_BRAVIARY          , //= EVO_TYPE_1,
//     SPECIES_MANDIBUZZ         , //= EVO_TYPE_1,
//     SPECIES_ZWEILOUS          , //= EVO_TYPE_1,
//     SPECIES_VOLCARONA         , //= EVO_TYPE_1,
//     SPECIES_QUILLADIN         , //= EVO_TYPE_1,
//     SPECIES_BRAIXEN           , //= EVO_TYPE_1,
//     SPECIES_FROGADIER         , //= EVO_TYPE_1,
//     SPECIES_DIGGERSBY         , //= EVO_TYPE_1,
//     SPECIES_FLETCHINDER       , //= EVO_TYPE_1,
//     SPECIES_SPEWPA            , //= EVO_TYPE_1,
//     SPECIES_PYROAR            , //= EVO_TYPE_1,
//     SPECIES_FLOETTE           , //= EVO_TYPE_1,
//     SPECIES_GOGOAT            , //= EVO_TYPE_1,
//     SPECIES_PANGORO           , //= EVO_TYPE_1,
//     SPECIES_MEOWSTIC          , //= EVO_TYPE_1,
//     SPECIES_DOUBLADE          , //= EVO_TYPE_1,
//     SPECIES_AROMATISSE        , //= EVO_TYPE_1,
//     SPECIES_SLURPUFF          , //= EVO_TYPE_1,
//     SPECIES_MALAMAR           , //= EVO_TYPE_1,
//     SPECIES_BARBARACLE        , //= EVO_TYPE_1,
//     SPECIES_DRAGALGE          , //= EVO_TYPE_1,
//     SPECIES_CLAWITZER         , //= EVO_TYPE_1,
//     SPECIES_HELIOLISK         , //= EVO_TYPE_1,
//     SPECIES_TYRANTRUM         , //= EVO_TYPE_1,
//     SPECIES_AURORUS           , //= EVO_TYPE_1,
//     SPECIES_SYLVEON           , //= EVO_TYPE_1,
//     SPECIES_SLIGGOO           , //= EVO_TYPE_1,
//     SPECIES_TREVENANT         , //= EVO_TYPE_1,
//     SPECIES_GOURGEIST         , //= EVO_TYPE_1,
//     SPECIES_AVALUGG           , //= EVO_TYPE_1,
//     SPECIES_NOIVERN           , //= EVO_TYPE_1,
//     SPECIES_DARTRIX           , //= EVO_TYPE_1,
//     SPECIES_TORRACAT          , //= EVO_TYPE_1,
//     SPECIES_BRIONNE           , //= EVO_TYPE_1,
//     SPECIES_TRUMBEAK          , //= EVO_TYPE_1,
//     SPECIES_GUMSHOOS          , //= EVO_TYPE_1,
//     SPECIES_CHARJABUG         , //= EVO_TYPE_1,
//     SPECIES_CRABOMINABLE      , //= EVO_TYPE_1,
//     SPECIES_RIBOMBEE          , //= EVO_TYPE_1,
//     SPECIES_LYCANROC          , //= EVO_TYPE_1,
//     SPECIES_TOXAPEX           , //= EVO_TYPE_1,
//     SPECIES_MUDSDALE          , //= EVO_TYPE_1,
//     SPECIES_ARAQUANID         , //= EVO_TYPE_1,
//     SPECIES_LURANTIS          , //= EVO_TYPE_1,
//     SPECIES_SHIINOTIC         , //= EVO_TYPE_1,
//     SPECIES_SALAZZLE          , //= EVO_TYPE_1,
//     SPECIES_BEWEAR            , //= EVO_TYPE_1,
//     SPECIES_STEENEE           , //= EVO_TYPE_1,
//     SPECIES_GOLISOPOD         , //= EVO_TYPE_1,
//     SPECIES_PALOSSAND         , //= EVO_TYPE_1,
//     SPECIES_HAKAMO_O          , //= EVO_TYPE_1,
//     SPECIES_THWACKEY          , //= EVO_TYPE_1,
//     SPECIES_RABOOT            , //= EVO_TYPE_1,
//     SPECIES_DRIZZILE          , //= EVO_TYPE_1,
//     SPECIES_GREEDENT          , //= EVO_TYPE_1,
//     SPECIES_CORVISQUIRE       , //= EVO_TYPE_1,
//     SPECIES_DOTTLER           , //= EVO_TYPE_1,
//     SPECIES_THIEVUL           , //= EVO_TYPE_1,
//     SPECIES_ELDEGOSS          , //= EVO_TYPE_1,
//     SPECIES_DUBWOOL           , //= EVO_TYPE_1,
//     SPECIES_DREDNAW           , //= EVO_TYPE_1,
//     SPECIES_BOLTUND           , //= EVO_TYPE_1,
//     SPECIES_CARKOL            , //= EVO_TYPE_1,
//     SPECIES_FLAPPLE           , //= EVO_TYPE_1,
//     SPECIES_APPLETUN          , //= EVO_TYPE_1,
//     SPECIES_SANDACONDA        , //= EVO_TYPE_1,
//     SPECIES_BARRASKEWDA       , //= EVO_TYPE_1,
//     SPECIES_TOXTRICITY        , //= EVO_TYPE_1,
//     SPECIES_CENTISKORCH       , //= EVO_TYPE_1,
//     SPECIES_GRAPPLOCT         , //= EVO_TYPE_1,
//     SPECIES_POLTEAGEIST       , //= EVO_TYPE_1,
//     SPECIES_HATTREM           , //= EVO_TYPE_1,
//     SPECIES_MORGREM           , //= EVO_TYPE_1,
//     SPECIES_PERRSERKER        , //= EVO_TYPE_1,
//     SPECIES_CURSOLA           , //= EVO_TYPE_1,
//     SPECIES_SIRFETCHD         , //= EVO_TYPE_1,
//     SPECIES_RUNERIGUS         , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE          , //= EVO_TYPE_1,
//     SPECIES_FROSMOTH          , //= EVO_TYPE_1,
//     SPECIES_COPPERAJAH        , //= EVO_TYPE_1,
//     SPECIES_DRAKLOAK          , //= EVO_TYPE_1,
//     SPECIES_RATICATE_ALOLA    , //= EVO_TYPE_1,
//     SPECIES_SANDSLASH_ALOLA   , //= EVO_TYPE_1,
//     SPECIES_NINETALES_ALOLA   , //= EVO_TYPE_1,
//     SPECIES_DUGTRIO_ALOLA     , //= EVO_TYPE_1,
//     SPECIES_PERSIAN_ALOLA     , //= EVO_TYPE_1,
//     SPECIES_GRAVELER_ALOLA    , //= EVO_TYPE_1,
//     SPECIES_MUK_ALOLA         , //= EVO_TYPE_1,
//     SPECIES_EXEGGUTOR_ALOLA   , //= EVO_TYPE_1,
//     SPECIES_MAROWAK_ALOLA     , //= EVO_TYPE_1,
//     SPECIES_RAPIDASH_GALAR  , //= EVO_TYPE_1,
//     SPECIES_SLOWBRO_GALAR   , //= EVO_TYPE_1,
//     SPECIES_WEEZING_GALAR   , //= EVO_TYPE_1,
//     SPECIES_MR_MIME_GALAR   , //= EVO_TYPE_1,
//     SPECIES_SLOWKING_GALAR  , //= EVO_TYPE_1,
//     SPECIES_CORSOLA_GALAR   , //= EVO_TYPE_1,
//     SPECIES_LINOONE_GALAR   , //= EVO_TYPE_1,
//     SPECIES_DARMANITAN_GALAR , //= EVO_TYPE_1,
//     SPECIES_WORMADAM_SANDY , //= EVO_TYPE_1,
//     SPECIES_WORMADAM_TRASH , //= EVO_TYPE_1,
//     SPECIES_CHERRIM_SUNSHINE   , //= EVO_TYPE_1,
//     SPECIES_GASTRODON_EAST , //= EVO_TYPE_1,
//     SPECIES_DARMANITAN_ZEN , //= EVO_TYPE_1,
//     SPECIES_DARMANITAN_GALAR_ZEN , //= EVO_TYPE_1,
//     SPECIES_SAWSBUCK_SUMMER    , //= EVO_TYPE_1,
//     SPECIES_SAWSBUCK_AUTUMN    , //= EVO_TYPE_1,
//     SPECIES_SAWSBUCK_WINTER    , //= EVO_TYPE_1,
//     SPECIES_FLOETTE_YELLOW , //= EVO_TYPE_1,
//     SPECIES_FLOETTE_ORANGE , //= EVO_TYPE_1,
//     SPECIES_FLOETTE_BLUE , //= EVO_TYPE_1,
//     SPECIES_FLOETTE_WHITE , //= EVO_TYPE_1,
//     SPECIES_MEOWSTIC_F    , //= EVO_TYPE_1,
//     SPECIES_GOURGEIST_SMALL    , //= EVO_TYPE_1,
//     SPECIES_GOURGEIST_LARGE    , //= EVO_TYPE_1,
//     SPECIES_GOURGEIST_SUPER    , //= EVO_TYPE_1,
//     SPECIES_LYCANROC_MIDNIGHT  , //= EVO_TYPE_1,
//     SPECIES_LYCANROC_DUSK      , //= EVO_TYPE_1,
//     SPECIES_TOXTRICITY_LOW_KEY , //= EVO_TYPE_1,
//     SPECIES_POLTEAGEIST_ANTIQUE , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_RUBY_CREAM , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_MATCHA_CREAM , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_MINT_CREAM , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_LEMON_CREAM , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_SALTED_CREAM , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_RUBY_SWIRL , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_CARAMEL_SWIRL , //= EVO_TYPE_1,
//     SPECIES_ALCREMIE_RAINBOW_SWIRL , //= EVO_TYPE_1,
//     SPECIES_INDEEDEE_F    , //= EVO_TYPE_1,
//     #endif
// };
// #define RANDOM_SPECIES_EVO_2_COUNT ARRAY_COUNT(sRandomSpeciesEvo2)
// static const u16 sRandomSpeciesEvo2[] =
// {
//     SPECIES_VENUSAUR        , //= EVO_TYPE_2,
//     SPECIES_CHARIZARD       , //= EVO_TYPE_2,
//     SPECIES_BLASTOISE       , //= EVO_TYPE_2,
//     SPECIES_BUTTERFREE      , //= EVO_TYPE_2,
//     SPECIES_BEEDRILL        , //= EVO_TYPE_2,
//     SPECIES_PIDGEOT         , //= EVO_TYPE_2,
//     SPECIES_RAICHU          , //= EVO_TYPE_2,
//     SPECIES_NIDOQUEEN       , //= EVO_TYPE_2,
//     SPECIES_NIDOKING        , //= EVO_TYPE_2,
//     SPECIES_CLEFABLE        , //= EVO_TYPE_2,
//     SPECIES_WIGGLYTUFF      , //= EVO_TYPE_2,
//     SPECIES_VILEPLUME       , //= EVO_TYPE_2,
//     SPECIES_POLIWRATH       , //= EVO_TYPE_2,
//     SPECIES_ALAKAZAM        , //= EVO_TYPE_2,
//     SPECIES_MACHAMP         , //= EVO_TYPE_2,
//     SPECIES_VICTREEBEL      , //= EVO_TYPE_2,
//     SPECIES_GOLEM           , //= EVO_TYPE_2,
//     SPECIES_SLOWBRO         , //= EVO_TYPE_2,
//     SPECIES_GENGAR          , //= EVO_TYPE_2,
//     SPECIES_GYARADOS        , //= EVO_TYPE_2,
//     SPECIES_DRAGONITE       , //= EVO_TYPE_2,
//     SPECIES_MEGANIUM        , //= EVO_TYPE_2,
//     SPECIES_TYPHLOSION      , //= EVO_TYPE_2,
//     SPECIES_FERALIGATR      , //= EVO_TYPE_2,
//     SPECIES_CROBAT          , //= EVO_TYPE_2,
//     SPECIES_AMPHAROS        , //= EVO_TYPE_2,
//     SPECIES_BELLOSSOM       , //= EVO_TYPE_2,
//     SPECIES_AZUMARILL       , //= EVO_TYPE_2,
//     SPECIES_POLITOED        , //= EVO_TYPE_2,
//     SPECIES_JUMPLUFF        , //= EVO_TYPE_2,
//     SPECIES_SLOWKING        , //= EVO_TYPE_2,
//     SPECIES_KINGDRA         , //= EVO_TYPE_2,
//     SPECIES_BLISSEY         , //= EVO_TYPE_2,
//     SPECIES_TYRANITAR       , //= EVO_TYPE_2,
//     SPECIES_SCEPTILE        , //= EVO_TYPE_2,
//     SPECIES_BLAZIKEN        , //= EVO_TYPE_2,
//     SPECIES_SWAMPERT        , //= EVO_TYPE_2,
//     SPECIES_BEAUTIFLY       , //= EVO_TYPE_2,
//     SPECIES_DUSTOX          , //= EVO_TYPE_2,
//     SPECIES_LUDICOLO        , //= EVO_TYPE_2,
//     SPECIES_SHIFTRY         , //= EVO_TYPE_2,
//     SPECIES_FLYGON          , //= EVO_TYPE_2,
//     SPECIES_WALREIN         , //= EVO_TYPE_2,
//     SPECIES_SLAKING         , //= EVO_TYPE_2,
//     SPECIES_EXPLOUD         , //= EVO_TYPE_2,
//     SPECIES_AGGRON          , //= EVO_TYPE_2,
//     SPECIES_GARDEVOIR       , //= EVO_TYPE_2,
//     SPECIES_SALAMENCE       , //= EVO_TYPE_2,
//     SPECIES_METAGROSS       , //= EVO_TYPE_2,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_TORTERRA          , //= EVO_TYPE_2,
//     SPECIES_INFERNAPE         , //= EVO_TYPE_2,
//     SPECIES_EMPOLEON          , //= EVO_TYPE_2,
//     SPECIES_STARAPTOR         , //= EVO_TYPE_2,
//     SPECIES_LUXRAY            , //= EVO_TYPE_2,
//     SPECIES_ROSERADE          , //= EVO_TYPE_2,
//     SPECIES_GARCHOMP          , //= EVO_TYPE_2,
//     SPECIES_MAGNEZONE         , //= EVO_TYPE_2,
//     SPECIES_RHYPERIOR         , //= EVO_TYPE_2,
//     SPECIES_ELECTIVIRE        , //= EVO_TYPE_2,
//     SPECIES_MAGMORTAR         , //= EVO_TYPE_2,
//     SPECIES_TOGEKISS          , //= EVO_TYPE_2,
//     SPECIES_MAMOSWINE         , //= EVO_TYPE_2,
//     SPECIES_PORYGON_Z         , //= EVO_TYPE_2,
//     SPECIES_GALLADE           , //= EVO_TYPE_2,
//     SPECIES_DUSKNOIR          , //= EVO_TYPE_2,
//     SPECIES_SERPERIOR         , //= EVO_TYPE_2,
//     SPECIES_SAMUROTT          , //= EVO_TYPE_2,
//     SPECIES_STOUTLAND         , //= EVO_TYPE_2,
//     SPECIES_UNFEZANT          , //= EVO_TYPE_2,
//     SPECIES_GIGALITH          , //= EVO_TYPE_2,
//     SPECIES_CONKELDURR        , //= EVO_TYPE_2,
//     SPECIES_SEISMITOAD        , //= EVO_TYPE_2,
//     SPECIES_LEAVANNY          , //= EVO_TYPE_2,
//     SPECIES_SCOLIPEDE         , //= EVO_TYPE_2,
//     SPECIES_KROOKODILE        , //= EVO_TYPE_2,
//     SPECIES_GOTHITELLE        , //= EVO_TYPE_2,
//     SPECIES_REUNICLUS         , //= EVO_TYPE_2,
//     SPECIES_VANILLUXE         , //= EVO_TYPE_2,
//     SPECIES_KLINKLANG         , //= EVO_TYPE_2,
//     SPECIES_EELEKTROSS        , //= EVO_TYPE_2,
//     SPECIES_CHANDELURE        , //= EVO_TYPE_2,
//     SPECIES_HAXORUS           , //= EVO_TYPE_2,
//     SPECIES_HYDREIGON         , //= EVO_TYPE_2,
//     SPECIES_CHESNAUGHT        , //= EVO_TYPE_2,
//     SPECIES_DELPHOX           , //= EVO_TYPE_2,
//     SPECIES_GRENINJA          , //= EVO_TYPE_2,
//     SPECIES_TALONFLAME        , //= EVO_TYPE_2,
//     SPECIES_VIVILLON          , //= EVO_TYPE_2,
//     SPECIES_FLORGES           , //= EVO_TYPE_2,
//     SPECIES_AEGISLASH         , //= EVO_TYPE_2,
//     SPECIES_GOODRA            , //= EVO_TYPE_2,
//     SPECIES_DECIDUEYE         , //= EVO_TYPE_2,
//     SPECIES_INCINEROAR        , //= EVO_TYPE_2,
//     SPECIES_PRIMARINA         , //= EVO_TYPE_2,
//     SPECIES_TOUCANNON         , //= EVO_TYPE_2,
//     SPECIES_VIKAVOLT          , //= EVO_TYPE_2,
//     SPECIES_TSAREENA          , //= EVO_TYPE_2,
//     SPECIES_KOMMO_O           , //= EVO_TYPE_2,
//     SPECIES_RILLABOOM         , //= EVO_TYPE_2,
//     SPECIES_CINDERACE         , //= EVO_TYPE_2,
//     SPECIES_INTELEON          , //= EVO_TYPE_2,
//     SPECIES_CORVIKNIGHT       , //= EVO_TYPE_2,
//     SPECIES_ORBEETLE          , //= EVO_TYPE_2,
//     SPECIES_COALOSSAL         , //= EVO_TYPE_2,
//     SPECIES_HATTERENE         , //= EVO_TYPE_2,
//     SPECIES_GRIMMSNARL        , //= EVO_TYPE_2,
//     SPECIES_OBSTAGOON         , //= EVO_TYPE_2,
//     SPECIES_MR_RIME           , //= EVO_TYPE_2,
//     SPECIES_DRAGAPULT         , //= EVO_TYPE_2,
//     SPECIES_RAICHU_ALOLA      , //= EVO_TYPE_2,
//     SPECIES_GOLEM_ALOLA       , //= EVO_TYPE_2,
//     SPECIES_GRENINJA_BATTLE_BOND , //= EVO_TYPE_2,
//     SPECIES_GRENINJA_ASH       , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_POLAR     , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_TUNDRA    , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_CONTINENTAL , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_GARDEN    , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_ELEGANT   , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_MEADOW    , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_MODERN    , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_MARINE    , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_ARCHIPELAGO , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_HIGH_PLAINS , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_SANDSTORM , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_RIVER     , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_MONSOON   , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_SAVANNA   , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_SUN       , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_OCEAN     , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_JUNGLE    , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_FANCY     , //= EVO_TYPE_2,
//     SPECIES_VIVILLON_POKEBALL , //= EVO_TYPE_2,
//     SPECIES_FLORGES_YELLOW , //= EVO_TYPE_2,
//     SPECIES_FLORGES_ORANGE , //= EVO_TYPE_2,
//     SPECIES_FLORGES_BLUE , //= EVO_TYPE_2,
//     SPECIES_FLORGES_WHITE , //= EVO_TYPE_2,
//     SPECIES_AEGISLASH_BLADE    , //= EVO_TYPE_2,
//     #endif
// };
// #define RANDOM_SPECIES_EVO_LEGENDARY_COUNT ARRAY_COUNT(sRandomSpeciesEvoLegendary)
// static const u16 sRandomSpeciesEvoLegendary[] =
// {
//     SPECIES_ARTICUNO                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZAPDOS                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MOLTRES                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MEWTWO                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MEW                           , //= EVO_TYPE_LEGENDARY,
//     SPECIES_RAIKOU                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ENTEI                         , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SUICUNE                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_LUGIA                         , //= EVO_TYPE_LEGENDARY,
//     SPECIES_HO_OH                         , //= EVO_TYPE_LEGENDARY,
//     SPECIES_CELEBI                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_REGIROCK                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_REGICE                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_REGISTEEL                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KYOGRE                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GROUDON                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_RAYQUAZA                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_LATIAS                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_LATIOS                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_JIRACHI                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_DEOXYS                        , //= EVO_TYPE_LEGENDARY,
//     #ifdef POKEMON_EXPANSION
//     SPECIES_UXIE                          , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MESPRIT                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_AZELF                         , //= EVO_TYPE_LEGENDARY,
//     SPECIES_DIALGA                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_PALKIA                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_HEATRAN                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_REGIGIGAS                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GIRATINA                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_CRESSELIA                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_PHIONE                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MANAPHY                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_DARKRAI                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SHAYMIN                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_VICTINI                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_COBALION                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TERRAKION                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_VIRIZION                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TORNADUS                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_THUNDURUS                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_RESHIRAM                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZEKROM                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_LANDORUS                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KYUREM                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KELDEO                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MELOETTA                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GENESECT                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_XERNEAS                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_YVELTAL                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZYGARDE                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_DIANCIE                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_HOOPA                         , //= EVO_TYPE_LEGENDARY,
//     SPECIES_VOLCANION                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TYPE_NULL                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TAPU_KOKO                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TAPU_LELE                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TAPU_BULU                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TAPU_FINI                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_COSMOG                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_COSMOEM                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SOLGALEO                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_LUNALA                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_NIHILEGO                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_BUZZWOLE                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_PHEROMOSA                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_XURKITREE                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_CELESTEELA                    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KARTANA                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GUZZLORD                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_NECROZMA                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MAGEARNA                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MARSHADOW                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_POIPOLE                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_NAGANADEL                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_STAKATAKA                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_BLACEPHALON                   , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZERAORA                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MELTAN                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MELMETAL                      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZACIAN                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZAMAZENTA                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ETERNATUS                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KUBFU                         , //= EVO_TYPE_LEGENDARY,
//     SPECIES_URSHIFU                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZARUDE                        , //= EVO_TYPE_LEGENDARY,
//     SPECIES_REGIELEKI                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_REGIDRAGO                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GLASTRIER                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SPECTRIER                     , //= EVO_TYPE_LEGENDARY,
//     SPECIES_CALYREX                       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARTICUNO_GALAR             , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZAPDOS_GALAR               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MOLTRES_GALAR              , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GIRATINA_ORIGIN               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SHAYMIN_SKY                   , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_FIGHTING               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_FLYING                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_POISON                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_GROUND                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_ROCK                   , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_BUG                    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_GHOST                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_STEEL                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_FIRE                   , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_WATER                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_GRASS                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_ELECTRIC               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_PSYCHIC                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_ICE                    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_DRAGON                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_DARK                   , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ARCEUS_FAIRY                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_TORNADUS_THERIAN              , //= EVO_TYPE_LEGENDARY,
//     SPECIES_THUNDURUS_THERIAN             , //= EVO_TYPE_LEGENDARY,
//     SPECIES_LANDORUS_THERIAN              , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KYUREM_WHITE                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KYUREM_BLACK                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_KELDEO_RESOLUTE               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MELOETTA_PIROUETTE            , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GENESECT_DOUSE          , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GENESECT_SHOCK          , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GENESECT_BURN           , //= EVO_TYPE_LEGENDARY,
//     SPECIES_GENESECT_CHILL          , //= EVO_TYPE_LEGENDARY,
//     SPECIES_XERNEAS_ACTIVE                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZYGARDE_10                    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZYGARDE_10_POWER_CONSTRUCT    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZYGARDE_50_POWER_CONSTRUCT    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZYGARDE_COMPLETE              , //= EVO_TYPE_LEGENDARY,
//     SPECIES_HOOPA_UNBOUND                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_FIGHTING             , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_FLYING               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_POISON               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_GROUND               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_ROCK                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_BUG                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_GHOST                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_STEEL                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_FIRE                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_WATER                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_GRASS                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_ELECTRIC             , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_PSYCHIC              , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_ICE                  , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_DRAGON               , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_DARK                 , //= EVO_TYPE_LEGENDARY,
//     SPECIES_SILVALLY_FAIRY                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_NECROZMA_DUSK_MANE            , //= EVO_TYPE_LEGENDARY,
//     SPECIES_NECROZMA_DAWN_WINGS           , //= EVO_TYPE_LEGENDARY,
//     SPECIES_NECROZMA_ULTRA                , //= EVO_TYPE_LEGENDARY,
//     SPECIES_MAGEARNA_ORIGINAL       , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZACIAN_CROWNED          , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZAMAZENTA_CROWNED      , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ETERNATUS_ETERNAMAX           , //= EVO_TYPE_LEGENDARY,
//     SPECIES_URSHIFU_RAPID_STRIKE    , //= EVO_TYPE_LEGENDARY,
//     SPECIES_ZARUDE_DADA                   , //= EVO_TYPE_LEGENDARY,
//     SPECIES_CALYREX_ICE             , //= EVO_TYPE_LEGENDARY,
//     SPECIES_CALYREX_SHADOW          , //= EVO_TYPE_LEGENDARY,
//     #endif
// };

// const u16 gEvolutionLines[NUM_SPECIES][EVOS_PER_LINE] =
// {
//     [SPECIES_BULBASAUR ... SPECIES_VENUSAUR]    = {SPECIES_BULBASAUR, SPECIES_IVYSAUR, SPECIES_VENUSAUR},
//     [SPECIES_CHARMANDER ... SPECIES_CHARIZARD]  = {SPECIES_CHARMANDER, SPECIES_CHARMELEON, SPECIES_CHARIZARD},
//     [SPECIES_SQUIRTLE ... SPECIES_BLASTOISE]    = {SPECIES_SQUIRTLE, SPECIES_WARTORTLE, SPECIES_BLASTOISE},
//     [SPECIES_CATERPIE ... SPECIES_BUTTERFREE]   = {SPECIES_CATERPIE, SPECIES_METAPOD, SPECIES_BUTTERFREE},
//     [SPECIES_WEEDLE ... SPECIES_BEEDRILL]       = {SPECIES_WEEDLE, SPECIES_KAKUNA, SPECIES_BEEDRILL},
//     [SPECIES_PIDGEY ... SPECIES_PIDGEOT]        = {SPECIES_PIDGEY, SPECIES_PIDGEOTTO, SPECIES_PIDGEOT},
//     [SPECIES_RATTATA ... SPECIES_RATICATE]      = {SPECIES_RATTATA, SPECIES_RATICATE},
//     [SPECIES_SPEAROW ... SPECIES_FEAROW]        = {SPECIES_SPEAROW, SPECIES_FEAROW},
//     [SPECIES_EKANS ... SPECIES_ARBOK]           = {SPECIES_EKANS, SPECIES_ARBOK},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_PIKACHU ... SPECIES_RAICHU]        = {SPECIES_PICHU, SPECIES_PIKACHU, SPECIES_RAICHU, SPECIES_RAICHU_ALOLA}, 
//     [SPECIES_PICHU]                             = {SPECIES_PICHU, SPECIES_PIKACHU, SPECIES_RAICHU, SPECIES_RAICHU_ALOLA}, 
//     [SPECIES_RAICHU_ALOLA]                     = {SPECIES_PICHU, SPECIES_PIKACHU, SPECIES_RAICHU, SPECIES_RAICHU_ALOLA},
//     #else
//     [SPECIES_PIKACHU ... SPECIES_RAICHU]        = {SPECIES_PICHU, SPECIES_PIKACHU, SPECIES_RAICHU}, 
//     [SPECIES_PICHU]                             = {SPECIES_PICHU, SPECIES_PIKACHU, SPECIES_RAICHU},
//     #endif
//     [SPECIES_SANDSHREW ... SPECIES_SANDSLASH]   = {SPECIES_SANDSHREW, SPECIES_SANDSLASH},
//     [SPECIES_NIDORAN_F ... SPECIES_NIDOQUEEN]   = {SPECIES_NIDORAN_F, SPECIES_NIDORINA, SPECIES_NIDOQUEEN},
//     [SPECIES_NIDORAN_M ... SPECIES_NIDOKING]    = {SPECIES_NIDORAN_M, SPECIES_NIDORINO, SPECIES_NIDOKING},
//     [SPECIES_CLEFAIRY ... SPECIES_CLEFABLE]     = {SPECIES_CLEFFA, SPECIES_CLEFAIRY, SPECIES_CLEFABLE},
//     [SPECIES_CLEFFA]                            = {SPECIES_CLEFFA, SPECIES_CLEFAIRY, SPECIES_CLEFABLE},
//     [SPECIES_VULPIX ... SPECIES_NINETALES]      = {SPECIES_VULPIX, SPECIES_NINETALES},
//     [SPECIES_JIGGLYPUFF ... SPECIES_WIGGLYTUFF] = {SPECIES_IGGLYBUFF, SPECIES_JIGGLYPUFF, SPECIES_WIGGLYTUFF},
//     [SPECIES_IGGLYBUFF]                         = {SPECIES_IGGLYBUFF, SPECIES_JIGGLYPUFF, SPECIES_WIGGLYTUFF},
//     [SPECIES_ZUBAT ... SPECIES_GOLBAT]          = {SPECIES_ZUBAT, SPECIES_GOLBAT, SPECIES_CROBAT},
//     [SPECIES_CROBAT]                            = {SPECIES_ZUBAT, SPECIES_GOLBAT, SPECIES_CROBAT},
//     [SPECIES_ODDISH ... SPECIES_VILEPLUME]      = {SPECIES_ODDISH, SPECIES_GLOOM, SPECIES_VILEPLUME, SPECIES_BELLOSSOM},
//     [SPECIES_BELLOSSOM]                         = {SPECIES_ODDISH, SPECIES_GLOOM, SPECIES_VILEPLUME, SPECIES_BELLOSSOM},
//     [SPECIES_PARAS ... SPECIES_PARASECT]        = {SPECIES_PARAS, SPECIES_PARASECT},
//     [SPECIES_VENONAT ... SPECIES_VENOMOTH]      = {SPECIES_VENONAT, SPECIES_VENOMOTH},
//     [SPECIES_DIGLETT ... SPECIES_DUGTRIO]       = {SPECIES_DIGLETT, SPECIES_DUGTRIO},
//     [SPECIES_MEOWTH ... SPECIES_PERSIAN]        = {SPECIES_MEOWTH, SPECIES_PERSIAN},
//     [SPECIES_PSYDUCK ... SPECIES_GOLDUCK]       = {SPECIES_PSYDUCK, SPECIES_GOLDUCK},
//     [SPECIES_MANKEY ... SPECIES_PRIMEAPE]       = {SPECIES_MANKEY, SPECIES_PRIMEAPE},
//     [SPECIES_GROWLITHE ... SPECIES_ARCANINE]    = {SPECIES_GROWLITHE, SPECIES_ARCANINE},
//     [SPECIES_POLIWAG ... SPECIES_POLIWRATH]     = {SPECIES_POLIWAG, SPECIES_POLIWHIRL, SPECIES_POLIWRATH, SPECIES_POLITOED},
//     [SPECIES_POLITOED]                          = {SPECIES_POLIWAG, SPECIES_POLIWHIRL, SPECIES_POLIWRATH, SPECIES_POLITOED},
//     [SPECIES_ABRA ... SPECIES_ALAKAZAM]         = {SPECIES_ABRA, SPECIES_KADABRA, SPECIES_ALAKAZAM},
//     [SPECIES_MACHOP ... SPECIES_MACHAMP]        = {SPECIES_MACHOP, SPECIES_MACHOKE, SPECIES_MACHAMP},
//     [SPECIES_BELLSPROUT ... SPECIES_VICTREEBEL] = {SPECIES_BELLSPROUT, SPECIES_WEEPINBELL, SPECIES_VICTREEBEL},
//     [SPECIES_TENTACOOL ... SPECIES_TENTACRUEL]  = {SPECIES_TENTACOOL, SPECIES_TENTACRUEL},
//     [SPECIES_GEODUDE ... SPECIES_GOLEM]         = {SPECIES_GEODUDE, SPECIES_GRAVELER, SPECIES_GOLEM},
//     [SPECIES_PONYTA ... SPECIES_RAPIDASH]       = {SPECIES_PONYTA, SPECIES_RAPIDASH},
//     [SPECIES_SLOWPOKE ... SPECIES_SLOWBRO]      = {SPECIES_SLOWPOKE, SPECIES_SLOWBRO, SPECIES_SLOWKING},
//     [SPECIES_SLOWKING]                          = {SPECIES_SLOWPOKE, SPECIES_SLOWBRO, SPECIES_SLOWKING},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_MAGNEMITE ... SPECIES_MAGNETON]    = {SPECIES_MAGNEMITE, SPECIES_MAGNETON, SPECIES_MAGNEZONE},
//     [SPECIES_MAGNEZONE]                         = {SPECIES_MAGNEMITE, SPECIES_MAGNETON, SPECIES_MAGNEZONE},
//     #else
//     [SPECIES_MAGNEMITE ... SPECIES_MAGNETON]    = {SPECIES_MAGNEMITE, SPECIES_MAGNETON},
//     #endif
//     [SPECIES_DODUO ... SPECIES_DODRIO]          = {SPECIES_DODUO, SPECIES_DODRIO},
//     [SPECIES_SEEL ... SPECIES_DEWGONG]          = {SPECIES_SEEL, SPECIES_DEWGONG},
//     [SPECIES_GRIMER ... SPECIES_MUK]            = {SPECIES_GRIMER, SPECIES_MUK},
//     [SPECIES_SHELLDER ... SPECIES_CLOYSTER]     = {SPECIES_SHELLDER, SPECIES_CLOYSTER},
//     [SPECIES_GASTLY ... SPECIES_GENGAR]         = {SPECIES_GASTLY, SPECIES_HAUNTER, SPECIES_GENGAR},
//     [SPECIES_ONIX]                              = {SPECIES_ONIX, SPECIES_STEELIX},
//     [SPECIES_STEELIX]                           = {SPECIES_ONIX, SPECIES_STEELIX},
//     [SPECIES_DROWZEE ... SPECIES_HYPNO]         = {SPECIES_DROWZEE, SPECIES_HYPNO},
//     [SPECIES_KRABBY ... SPECIES_KINGLER]        = {SPECIES_KRABBY, SPECIES_KINGLER},
//     [SPECIES_VOLTORB ... SPECIES_ELECTRODE]     = {SPECIES_VOLTORB, SPECIES_ELECTRODE},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_EXEGGCUTE ... SPECIES_EXEGGUTOR]   = {SPECIES_EXEGGCUTE, SPECIES_EXEGGUTOR, SPECIES_EXEGGUTOR_ALOLA},
//     [SPECIES_EXEGGUTOR_ALOLA]                  = {SPECIES_EXEGGCUTE, SPECIES_EXEGGUTOR, SPECIES_EXEGGUTOR_ALOLA},
//     [SPECIES_CUBONE ... SPECIES_MAROWAK]        = {SPECIES_CUBONE, SPECIES_MAROWAK, SPECIES_MAROWAK_ALOLA},
//     [SPECIES_MAROWAK_ALOLA]                    = {SPECIES_CUBONE, SPECIES_MAROWAK, SPECIES_MAROWAK_ALOLA},
//     #else
//     [SPECIES_EXEGGCUTE ... SPECIES_EXEGGUTOR]   = {SPECIES_EXEGGCUTE, SPECIES_EXEGGUTOR},
//     [SPECIES_CUBONE ... SPECIES_MAROWAK]        = {SPECIES_CUBONE, SPECIES_MAROWAK},
//     #endif
//     [SPECIES_HITMONLEE ... SPECIES_HITMONCHAN]  = {SPECIES_TYROGUE, SPECIES_HITMONCHAN, SPECIES_HITMONLEE, SPECIES_HITMONTOP},
//     [SPECIES_TYROGUE ... SPECIES_HITMONTOP]     = {SPECIES_TYROGUE, SPECIES_HITMONCHAN, SPECIES_HITMONLEE, SPECIES_HITMONTOP},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_LICKITUNG]                         = {SPECIES_LICKITUNG, SPECIES_LICKILICKY},
//     [SPECIES_LICKILICKY]                        = {SPECIES_LICKITUNG, SPECIES_LICKILICKY},
//     [SPECIES_KOFFING ... SPECIES_WEEZING]       = {SPECIES_KOFFING, SPECIES_WEEZING, SPECIES_WEEZING_GALAR},
//     [SPECIES_WEEZING_GALAR]                  = {SPECIES_KOFFING, SPECIES_WEEZING, SPECIES_WEEZING_GALAR},
//     [SPECIES_RHYHORN ... SPECIES_RHYDON]        = {SPECIES_RHYHORN, SPECIES_RHYDON, SPECIES_RHYPERIOR},
//     [SPECIES_RHYPERIOR]                         = {SPECIES_RHYHORN, SPECIES_RHYDON, SPECIES_RHYPERIOR},
//     [SPECIES_CHANSEY]                           = {SPECIES_HAPPINY, SPECIES_CHANSEY, SPECIES_BLISSEY},
//     [SPECIES_BLISSEY]                           = {SPECIES_HAPPINY, SPECIES_CHANSEY, SPECIES_BLISSEY},
//     [SPECIES_HAPPINY]                           = {SPECIES_HAPPINY, SPECIES_CHANSEY, SPECIES_BLISSEY},
//     [SPECIES_TANGELA]                           = {SPECIES_TANGELA, SPECIES_TANGROWTH},
//     [SPECIES_TANGROWTH]                         = {SPECIES_TANGELA, SPECIES_TANGROWTH},
//     #else
//     [SPECIES_KOFFING ... SPECIES_WEEZING]       = {SPECIES_KOFFING, SPECIES_WEEZING},
//     [SPECIES_RHYHORN ... SPECIES_RHYDON]        = {SPECIES_RHYHORN, SPECIES_RHYDON},
//     [SPECIES_CHANSEY]                           = {SPECIES_CHANSEY, SPECIES_BLISSEY},
//     [SPECIES_BLISSEY]                           = {SPECIES_CHANSEY, SPECIES_BLISSEY},
//     #endif
//     [SPECIES_HORSEA ... SPECIES_SEADRA]         = {SPECIES_HORSEA, SPECIES_SEADRA, SPECIES_KINGDRA},
//     [SPECIES_KINGDRA]                           = {SPECIES_HORSEA, SPECIES_SEADRA, SPECIES_KINGDRA},
//     [SPECIES_GOLDEEN ... SPECIES_SEAKING]       = {SPECIES_GOLDEEN, SPECIES_SEAKING},
//     [SPECIES_STARYU ... SPECIES_STARMIE]        = {SPECIES_STARYU, SPECIES_STARMIE},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_MR_MIME]                           = {SPECIES_MIME_JR, SPECIES_MR_MIME, SPECIES_MR_MIME_GALAR, SPECIES_MR_RIME},
//     [SPECIES_MIME_JR]                           = {SPECIES_MIME_JR, SPECIES_MR_MIME, SPECIES_MR_MIME_GALAR, SPECIES_MR_RIME},
//     [SPECIES_MR_MIME_GALAR]                  = {SPECIES_MIME_JR, SPECIES_MR_MIME, SPECIES_MR_MIME_GALAR, SPECIES_MR_RIME},
//     [SPECIES_MR_RIME]                           = {SPECIES_MIME_JR, SPECIES_MR_MIME, SPECIES_MR_MIME_GALAR, SPECIES_MR_RIME},
//     #endif
//     [SPECIES_SCYTHER]                           = {SPECIES_SCYTHER, SPECIES_SCIZOR},
//     [SPECIES_JYNX]                              = {SPECIES_SMOOCHUM, SPECIES_JYNX},
//     [SPECIES_SMOOCHUM]                          = {SPECIES_SMOOCHUM, SPECIES_JYNX},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_ELECTABUZZ]                        = {SPECIES_ELEKID, SPECIES_ELECTABUZZ, SPECIES_ELECTIVIRE},
//     [SPECIES_ELEKID]                            = {SPECIES_ELEKID, SPECIES_ELECTABUZZ, SPECIES_ELECTIVIRE},
//     [SPECIES_ELECTIVIRE]                        = {SPECIES_ELEKID, SPECIES_ELECTABUZZ, SPECIES_ELECTIVIRE},
//     [SPECIES_MAGBY]                             = {SPECIES_MAGBY, SPECIES_MAGMAR, SPECIES_MAGMORTAR},
//     [SPECIES_MAGMAR]                            = {SPECIES_MAGBY, SPECIES_MAGMAR, SPECIES_MAGMORTAR},
//     [SPECIES_MAGMORTAR]                         = {SPECIES_MAGBY, SPECIES_MAGMAR, SPECIES_MAGMORTAR},
//     #else
//     [SPECIES_ELECTABUZZ]                        = {SPECIES_ELEKID, SPECIES_ELECTABUZZ},
//     [SPECIES_ELEKID]                            = {SPECIES_ELEKID, SPECIES_ELECTABUZZ},
//     [SPECIES_MAGBY]                             = {SPECIES_MAGBY, SPECIES_MAGMAR},
//     [SPECIES_MAGMAR]                            = {SPECIES_MAGBY, SPECIES_MAGMAR},
//     #endif
//     [SPECIES_SCIZOR]                            = {SPECIES_SCYTHER, SPECIES_SCIZOR},
//     [SPECIES_MAGIKARP ... SPECIES_GYARADOS]     = {SPECIES_MAGIKARP, SPECIES_GYARADOS},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_EEVEE ... SPECIES_FLAREON]         = {SPECIES_EEVEE, SPECIES_JOLTEON, SPECIES_VAPOREON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON, SPECIES_LEAFEON, SPECIES_GLACEON, SPECIES_SYLVEON},
//     [SPECIES_ESPEON ... SPECIES_UMBREON]        = {SPECIES_EEVEE, SPECIES_JOLTEON, SPECIES_VAPOREON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON, SPECIES_LEAFEON, SPECIES_GLACEON, SPECIES_SYLVEON},
//     [SPECIES_LEAFEON ... SPECIES_GLACEON]       = {SPECIES_EEVEE, SPECIES_JOLTEON, SPECIES_VAPOREON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON, SPECIES_LEAFEON, SPECIES_GLACEON, SPECIES_SYLVEON},
//     [SPECIES_SYLVEON]                           = {SPECIES_EEVEE, SPECIES_JOLTEON, SPECIES_VAPOREON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON, SPECIES_LEAFEON, SPECIES_GLACEON, SPECIES_SYLVEON},
//     [SPECIES_PORYGON]                           = {SPECIES_PORYGON, SPECIES_PORYGON2, SPECIES_PORYGON_Z},
//     [SPECIES_PORYGON2]                          = {SPECIES_PORYGON, SPECIES_PORYGON2, SPECIES_PORYGON_Z},
//     [SPECIES_PORYGON_Z]                         = {SPECIES_PORYGON, SPECIES_PORYGON2, SPECIES_PORYGON_Z},
//     #else
//     [SPECIES_EEVEE ... SPECIES_FLAREON]         = {SPECIES_EEVEE, SPECIES_JOLTEON, SPECIES_VAPOREON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON},
//     [SPECIES_ESPEON ... SPECIES_UMBREON]        = {SPECIES_EEVEE, SPECIES_JOLTEON, SPECIES_VAPOREON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON},
//     [SPECIES_PORYGON]                           = {SPECIES_PORYGON, SPECIES_PORYGON2},
//     [SPECIES_PORYGON2]                          = {SPECIES_PORYGON, SPECIES_PORYGON2},
//     #endif
//     [SPECIES_OMANYTE ... SPECIES_OMASTAR]       = {SPECIES_OMANYTE, SPECIES_OMASTAR},
//     [SPECIES_KABUTO ... SPECIES_KABUTOPS]       = {SPECIES_KABUTO, SPECIES_KABUTOPS},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SNORLAX]                           = {SPECIES_MUNCHLAX, SPECIES_SNORLAX},
//     [SPECIES_MUNCHLAX]                          = {SPECIES_MUNCHLAX, SPECIES_SNORLAX},
//     #endif
//     [SPECIES_DRATINI ... SPECIES_DRAGONITE]     = {SPECIES_DRATINI, SPECIES_DRAGONAIR, SPECIES_DRAGONITE},
//     [SPECIES_CHIKORITA ... SPECIES_MEGANIUM]    = {SPECIES_CHIKORITA, SPECIES_BAYLEEF, SPECIES_MEGANIUM},
//     [SPECIES_CYNDAQUIL ... SPECIES_TYPHLOSION]  = {SPECIES_CYNDAQUIL, SPECIES_QUILAVA, SPECIES_TYPHLOSION},
//     [SPECIES_TOTODILE ... SPECIES_FERALIGATR]   = {SPECIES_TOTODILE, SPECIES_CROCONAW, SPECIES_FERALIGATR},
//     [SPECIES_SENTRET ... SPECIES_FURRET]        = {SPECIES_SENTRET, SPECIES_FURRET},
//     [SPECIES_HOOTHOOT ... SPECIES_NOCTOWL]      = {SPECIES_HOOTHOOT, SPECIES_NOCTOWL},
//     [SPECIES_LEDYBA ... SPECIES_LEDIAN]         = {SPECIES_LEDYBA, SPECIES_LEDIAN},
//     [SPECIES_SPINARAK ... SPECIES_ARIADOS]      = {SPECIES_SPINARAK, SPECIES_ARIADOS},
//     [SPECIES_CHINCHOU ... SPECIES_LANTURN]      = {SPECIES_CHINCHOU, SPECIES_LANTURN},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_TOGEPI ... SPECIES_TOGETIC]        = {SPECIES_TOGEPI, SPECIES_TOGETIC, SPECIES_TOGEKISS},
//     [SPECIES_TOGEKISS]                          = {SPECIES_TOGEPI, SPECIES_TOGETIC, SPECIES_TOGEKISS},
//     #else
//     [SPECIES_TOGEPI ... SPECIES_TOGETIC]        = {SPECIES_TOGEPI, SPECIES_TOGETIC},
//     #endif
//     [SPECIES_NATU ... SPECIES_XATU]             = {SPECIES_NATU, SPECIES_XATU},
//     [SPECIES_MAREEP ... SPECIES_AMPHAROS]       = {SPECIES_MAREEP, SPECIES_FLAAFFY, SPECIES_AMPHAROS},
//     [SPECIES_MARILL ... SPECIES_AZUMARILL]      = {SPECIES_AZURILL, SPECIES_MARILL, SPECIES_AZUMARILL},
//     [SPECIES_AZURILL]                           = {SPECIES_AZURILL, SPECIES_MARILL, SPECIES_AZUMARILL},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SUDOWOODO]                         = {SPECIES_BONSLY, SPECIES_SUDOWOODO},
//     [SPECIES_BONSLY]                            = {SPECIES_BONSLY, SPECIES_SUDOWOODO},
//     #endif
//     [SPECIES_HOPPIP ... SPECIES_JUMPLUFF]       = {SPECIES_HOPPIP, SPECIES_SKIPLOOM, SPECIES_JUMPLUFF},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_AIPOM]                             = {SPECIES_AIPOM, SPECIES_AMBIPOM},
//     [SPECIES_AMBIPOM]                           = {SPECIES_AIPOM, SPECIES_AMBIPOM},
//     #endif
//     [SPECIES_SUNKERN ... SPECIES_SUNFLORA]      = {SPECIES_SUNKERN, SPECIES_SUNFLORA},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_YANMA]                             = {SPECIES_YANMA, SPECIES_YANMEGA},
//     [SPECIES_YANMEGA]                           = {SPECIES_YANMA, SPECIES_YANMEGA},
//     #endif
//     [SPECIES_WOOPER ... SPECIES_QUAGSIRE]       = {SPECIES_WOOPER, SPECIES_QUAGSIRE},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_MURKROW]                           = {SPECIES_MURKROW, SPECIES_HONCHKROW},
//     [SPECIES_HONCHKROW]                         = {SPECIES_MURKROW, SPECIES_HONCHKROW},
//     [SPECIES_MISDREAVUS]                        = {SPECIES_MISDREAVUS, SPECIES_MISMAGIUS},
//     [SPECIES_MISMAGIUS]                         = {SPECIES_MISDREAVUS, SPECIES_MISMAGIUS},
//     #endif
//     [SPECIES_WOBBUFFET]                         = {SPECIES_WYNAUT, SPECIES_WOBBUFFET},
//     [SPECIES_WYNAUT]                            = {SPECIES_WYNAUT, SPECIES_WOBBUFFET},
//     [SPECIES_PINECO ... SPECIES_FORRETRESS]     = {SPECIES_PINECO, SPECIES_FORRETRESS},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_GLIGAR]                            = {SPECIES_GLIGAR, SPECIES_GLISCOR},
//     [SPECIES_GLISCOR]                           = {SPECIES_GLIGAR, SPECIES_GLISCOR},
//     #endif
//     [SPECIES_SNUBBULL ... SPECIES_GRANBULL]     = {SPECIES_SNUBBULL, SPECIES_GRANBULL},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SNEASEL]                           = {SPECIES_SNEASEL, SPECIES_WEAVILE},
//     [SPECIES_WEAVILE]                           = {SPECIES_SNEASEL, SPECIES_WEAVILE},
//     #endif
//     [SPECIES_TEDDIURSA ... SPECIES_URSARING]    = {SPECIES_TEDDIURSA, SPECIES_URSARING},
//     [SPECIES_SLUGMA ... SPECIES_MAGCARGO]       = {SPECIES_SLUGMA, SPECIES_MAGCARGO},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SWINUB ... SPECIES_PILOSWINE]      = {SPECIES_SWINUB, SPECIES_PILOSWINE, SPECIES_MAMOSWINE},
//     [SPECIES_MAMOSWINE]                         = {SPECIES_SWINUB, SPECIES_PILOSWINE, SPECIES_MAMOSWINE},
//     #else
//     [SPECIES_SWINUB ... SPECIES_PILOSWINE]      = {SPECIES_SWINUB, SPECIES_PILOSWINE},
//     #endif
//     [SPECIES_REMORAID ... SPECIES_OCTILLERY]    = {SPECIES_REMORAID, SPECIES_OCTILLERY},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_MANTINE]                           = {SPECIES_MANTYKE, SPECIES_MANTINE},
//     [SPECIES_MANTYKE]                           = {SPECIES_MANTYKE, SPECIES_MANTINE},
//     #endif
//     [SPECIES_HOUNDOUR ... SPECIES_HOUNDOOM]     = {SPECIES_HOUNDOUR, SPECIES_HOUNDOOM},
//     [SPECIES_PHANPY ... SPECIES_DONPHAN]        = {SPECIES_PHANPY, SPECIES_DONPHAN},
//     [SPECIES_LARVITAR ... SPECIES_TYRANITAR]    = {SPECIES_LARVITAR, SPECIES_PUPITAR, SPECIES_TYRANITAR},
//     [SPECIES_TREECKO ... SPECIES_SCEPTILE]      = {SPECIES_TREECKO, SPECIES_GROVYLE, SPECIES_SCEPTILE},
//     [SPECIES_TORCHIC ... SPECIES_BLAZIKEN]      = {SPECIES_TORCHIC, SPECIES_COMBUSKEN, SPECIES_BLAZIKEN},
//     [SPECIES_MUDKIP ... SPECIES_SWAMPERT]       = {SPECIES_MUDKIP, SPECIES_MARSHTOMP, SPECIES_SWAMPERT},
//     [SPECIES_POOCHYENA ... SPECIES_MIGHTYENA]   = {SPECIES_POOCHYENA, SPECIES_MIGHTYENA},
//     [SPECIES_ZIGZAGOON ... SPECIES_LINOONE]     = {SPECIES_ZIGZAGOON, SPECIES_LINOONE},
//     [SPECIES_WURMPLE ... SPECIES_DUSTOX]        = {SPECIES_WURMPLE, SPECIES_SILCOON, SPECIES_BEAUTIFLY, SPECIES_CASCOON, SPECIES_DUSTOX},
//     [SPECIES_LOTAD ... SPECIES_LUDICOLO]        = {SPECIES_LOTAD, SPECIES_LOMBRE, SPECIES_LUDICOLO},
//     [SPECIES_SEEDOT ... SPECIES_SHIFTRY]        = {SPECIES_SEEDOT, SPECIES_NUZLEAF, SPECIES_SHIFTRY},
//     [SPECIES_NINCADA ... SPECIES_SHEDINJA]      = {SPECIES_NINCADA, SPECIES_NINJASK, SPECIES_SHEDINJA},
//     [SPECIES_TAILLOW ... SPECIES_SWELLOW]       = {SPECIES_TAILLOW, SPECIES_SWELLOW},
//     [SPECIES_SHROOMISH ... SPECIES_BRELOOM]     = {SPECIES_SHROOMISH, SPECIES_BRELOOM},
//     [SPECIES_WINGULL ... SPECIES_PELIPPER]      = {SPECIES_WINGULL, SPECIES_PELIPPER},
//     [SPECIES_SURSKIT ... SPECIES_MASQUERAIN]    = {SPECIES_SURSKIT, SPECIES_MASQUERAIN},
//     [SPECIES_WAILMER ... SPECIES_WAILORD]       = {SPECIES_WAILMER, SPECIES_WAILORD},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_NOSEPASS]                          = {SPECIES_NOSEPASS, SPECIES_PROBOPASS},
//     [SPECIES_PROBOPASS]                         = {SPECIES_NOSEPASS, SPECIES_PROBOPASS},
//     #endif
//     [SPECIES_SKITTY ... SPECIES_DELCATTY]       = {SPECIES_SKITTY, SPECIES_DELCATTY},
//     [SPECIES_BALTOY ... SPECIES_CLAYDOL]        = {SPECIES_BALTOY, SPECIES_CLAYDOL},
//     [SPECIES_BARBOACH ... SPECIES_WHISCASH]     = {SPECIES_BARBOACH, SPECIES_WHISCASH},
//     [SPECIES_CORPHISH ... SPECIES_CRAWDAUNT]    = {SPECIES_CORPHISH, SPECIES_CRAWDAUNT},
//     [SPECIES_FEEBAS ... SPECIES_MILOTIC]        = {SPECIES_FEEBAS, SPECIES_MILOTIC},
//     [SPECIES_CARVANHA ... SPECIES_SHARPEDO]     = {SPECIES_CARVANHA, SPECIES_SHARPEDO},
//     [SPECIES_TRAPINCH ... SPECIES_FLYGON]       = {SPECIES_TRAPINCH, SPECIES_VIBRAVA, SPECIES_FLYGON},
//     [SPECIES_MAKUHITA ... SPECIES_HARIYAMA]     = {SPECIES_MAKUHITA, SPECIES_HARIYAMA},
//     [SPECIES_ELECTRIKE ... SPECIES_MANECTRIC]   = {SPECIES_ELECTRIKE, SPECIES_MANECTRIC},
//     [SPECIES_NUMEL ... SPECIES_CAMERUPT]        = {SPECIES_NUMEL, SPECIES_CAMERUPT},
//     [SPECIES_SPHEAL ... SPECIES_WALREIN]        = {SPECIES_SPHEAL, SPECIES_SEALEO, SPECIES_WALREIN},
//     [SPECIES_CACNEA ... SPECIES_CACTURNE]       = {SPECIES_CACNEA, SPECIES_CACTURNE},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_SNORUNT ... SPECIES_GLALIE]        = {SPECIES_SNORUNT, SPECIES_GLALIE, SPECIES_FROSLASS},
//     [SPECIES_FROSLASS]                          = {SPECIES_SNORUNT, SPECIES_GLALIE, SPECIES_FROSLASS},
//     #else
//     [SPECIES_SNORUNT ... SPECIES_GLALIE]        = {SPECIES_SNORUNT, SPECIES_GLALIE},
//     #endif
//     [SPECIES_SPOINK ... SPECIES_GRUMPIG]        = {SPECIES_SPOINK, SPECIES_GRUMPIG},
//     [SPECIES_MEDITITE ... SPECIES_MEDICHAM]     = {SPECIES_MEDITITE, SPECIES_MEDICHAM},
//     [SPECIES_SWABLU ... SPECIES_ALTARIA]        = {SPECIES_SWABLU, SPECIES_ALTARIA},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_DUSKULL ... SPECIES_DUSCLOPS]      = {SPECIES_DUSKULL, SPECIES_DUSCLOPS, SPECIES_DUSKNOIR},
//     [SPECIES_DUSKNOIR]                          = {SPECIES_DUSKULL, SPECIES_DUSCLOPS, SPECIES_DUSKNOIR},
//     #else
//     [SPECIES_DUSKULL ... SPECIES_DUSCLOPS]      = {SPECIES_DUSKULL, SPECIES_DUSCLOPS},
//     #endif
//     [SPECIES_SLAKOTH ... SPECIES_SLAKING]       = {SPECIES_SLAKOTH, SPECIES_VIGOROTH, SPECIES_SLAKING},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_ROSELIA]                           = {SPECIES_BUDEW, SPECIES_ROSELIA, SPECIES_ROSERADE},
//     [SPECIES_BUDEW ... SPECIES_ROSERADE]        = {SPECIES_BUDEW, SPECIES_ROSELIA, SPECIES_ROSERADE},
//     #endif
//     [SPECIES_GULPIN ... SPECIES_SWALOT]         = {SPECIES_GULPIN, SPECIES_SWALOT},
//     [SPECIES_WHISMUR ... SPECIES_EXPLOUD]       = {SPECIES_WHISMUR, SPECIES_LOUDRED, SPECIES_EXPLOUD},
//     [SPECIES_CLAMPERL ... SPECIES_GOREBYSS]     = {SPECIES_CLAMPERL, SPECIES_HUNTAIL, SPECIES_GOREBYSS},
//     [SPECIES_SHUPPET ... SPECIES_BANETTE]       = {SPECIES_SHUPPET, SPECIES_BANETTE},
//     [SPECIES_ARON ... SPECIES_AGGRON]           = {SPECIES_ARON, SPECIES_LAIRON, SPECIES_AGGRON},
//     [SPECIES_LILEEP ... SPECIES_CRADILY]        = {SPECIES_LILEEP, SPECIES_CRADILY},
//     [SPECIES_ANORITH ... SPECIES_ARMALDO]       = {SPECIES_ANORITH, SPECIES_ARMALDO},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_RALTS ... SPECIES_GARDEVOIR]       = {SPECIES_RALTS, SPECIES_KIRLIA, SPECIES_GARDEVOIR, SPECIES_GALLADE},
//     [SPECIES_GALLADE]                           = {SPECIES_RALTS, SPECIES_KIRLIA, SPECIES_GARDEVOIR, SPECIES_GALLADE},
//     #else
//     [SPECIES_RALTS ... SPECIES_GARDEVOIR]       = {SPECIES_RALTS, SPECIES_KIRLIA, SPECIES_GARDEVOIR},
//     #endif
//     [SPECIES_BAGON ... SPECIES_SALAMENCE]       = {SPECIES_BAGON, SPECIES_SHELGON, SPECIES_SALAMENCE},
//     [SPECIES_BELDUM ... SPECIES_METAGROSS]      = {SPECIES_BELDUM, SPECIES_METANG, SPECIES_METAGROSS},
//     #ifdef POKEMON_EXPANSION
//     [SPECIES_DEOXYS]                            = {SPECIES_DEOXYS, SPECIES_DEOXYS_ATTACK, SPECIES_DEOXYS_DEFENSE, SPECIES_DEOXYS_SPEED},
//     [SPECIES_DEOXYS_ATTACK ... SPECIES_DEOXYS_SPEED] = {SPECIES_DEOXYS, SPECIES_DEOXYS_ATTACK, SPECIES_DEOXYS_DEFENSE, SPECIES_DEOXYS_SPEED},
//     [SPECIES_CHIMECHO]                          = {SPECIES_CHINGLING, SPECIES_CHIMECHO},
//     [SPECIES_CHINGLING]                         = {SPECIES_CHINGLING, SPECIES_CHIMECHO},
//     [SPECIES_TURTWIG ... SPECIES_TORTERRA]      = {SPECIES_TURTWIG, SPECIES_GROTLE, SPECIES_TORTERRA},
//     [SPECIES_CHIMCHAR ... SPECIES_INFERNAPE]    = {SPECIES_CHIMCHAR, SPECIES_MONFERNO, SPECIES_INFERNAPE},
//     [SPECIES_PIPLUP ... SPECIES_EMPOLEON]       = {SPECIES_PIPLUP, SPECIES_PRINPLUP, SPECIES_EMPOLEON},
//     [SPECIES_STARLY ... SPECIES_STARAPTOR]      = {SPECIES_STARLY, SPECIES_STARAVIA, SPECIES_STARAPTOR},
//     [SPECIES_BIDOOF ... SPECIES_BIBAREL]        = {SPECIES_BIDOOF, SPECIES_BIBAREL},
//     [SPECIES_KRICKETOT ... SPECIES_KRICKETUNE]  = {SPECIES_KRICKETOT, SPECIES_KRICKETUNE},
//     [SPECIES_SHINX ... SPECIES_LUXRAY]          = {SPECIES_SHINX, SPECIES_LUXIO, SPECIES_LUXRAY},
//     [SPECIES_CRANIDOS ... SPECIES_RAMPARDOS]    = {SPECIES_CRANIDOS, SPECIES_RAMPARDOS},
//     [SPECIES_SHIELDON ... SPECIES_BASTIODON]    = {SPECIES_SHIELDON, SPECIES_BASTIODON},
//     [SPECIES_BURMY ... SPECIES_MOTHIM]          = {SPECIES_BURMY, SPECIES_WORMADAM, SPECIES_MOTHIM,
//                                                     SPECIES_BURMY_SANDY, SPECIES_BURMY_TRASH,
//                                                     SPECIES_WORMADAM_SANDY, SPECIES_WORMADAM_TRASH},
//     [SPECIES_BURMY_SANDY ... SPECIES_WORMADAM_TRASH] = {SPECIES_BURMY, SPECIES_WORMADAM, SPECIES_MOTHIM,
//                                                     SPECIES_BURMY_SANDY, SPECIES_BURMY_TRASH,
//                                                     SPECIES_WORMADAM_SANDY, SPECIES_WORMADAM_TRASH},
//     [SPECIES_COMBEE ... SPECIES_VESPIQUEN]      = {SPECIES_COMBEE, SPECIES_VESPIQUEN},
//     [SPECIES_BUIZEL ... SPECIES_FLOATZEL]       = {SPECIES_BUIZEL, SPECIES_FLOATZEL},
//     [SPECIES_CHERUBI ... SPECIES_CHERRIM]       = {SPECIES_CHERUBI, SPECIES_CHERRIM, SPECIES_CHERRIM_SUNSHINE},
//     [SPECIES_CHERRIM_SUNSHINE]                  = {SPECIES_CHERUBI, SPECIES_CHERRIM, SPECIES_CHERRIM_SUNSHINE},
//     [SPECIES_SHELLOS ... SPECIES_GASTRODON]     = {SPECIES_SHELLOS, SPECIES_GASTRODON, SPECIES_CHERRIM_SUNSHINE},
//     [SPECIES_SHELLOS_EAST ... SPECIES_GASTRODON_EAST] = {SPECIES_SHELLOS, SPECIES_GASTRODON, SPECIES_SHELLOS_EAST, SPECIES_GASTRODON_EAST},
//     [SPECIES_DRIFLOON ... SPECIES_DRIFBLIM]     = {SPECIES_DRIFLOON, SPECIES_DRIFBLIM},
//     [SPECIES_BUNEARY ... SPECIES_LOPUNNY]       = {SPECIES_BUNEARY, SPECIES_LOPUNNY},
//     [SPECIES_GLAMEOW ... SPECIES_PURUGLY]       = {SPECIES_GLAMEOW, SPECIES_PURUGLY},
//     [SPECIES_STUNKY ... SPECIES_SKUNTANK]       = {SPECIES_STUNKY, SPECIES_SKUNTANK},
//     [SPECIES_BRONZOR ... SPECIES_BRONZONG]      = {SPECIES_BRONZOR, SPECIES_BRONZONG},
//     [SPECIES_GIBLE ... SPECIES_GARCHOMP]        = {SPECIES_GIBLE, SPECIES_GABITE, SPECIES_GARCHOMP},
//     [SPECIES_RIOLU ... SPECIES_LUCARIO]         = {SPECIES_RIOLU, SPECIES_LUCARIO},
//     [SPECIES_HIPPOPOTAS ... SPECIES_HIPPOWDON]  = {SPECIES_HIPPOPOTAS, SPECIES_HIPPOWDON},
//     [SPECIES_SKORUPI ... SPECIES_DRAPION]       = {SPECIES_SKORUPI, SPECIES_DRAPION},
//     [SPECIES_CROAGUNK ... SPECIES_TOXICROAK]    = {SPECIES_CROAGUNK, SPECIES_TOXICROAK},
//     [SPECIES_FINNEON ... SPECIES_LUMINEON]      = {SPECIES_FINNEON, SPECIES_LUMINEON},
//     [SPECIES_SNOVER ... SPECIES_ABOMASNOW]      = {SPECIES_SNOVER, SPECIES_ABOMASNOW},
//     [SPECIES_ROTOM]                             = {SPECIES_ROTOM, SPECIES_ROTOM_HEAT, SPECIES_ROTOM_WASH, SPECIES_ROTOM_FROST, SPECIES_ROTOM_FAN, SPECIES_ROTOM_MOW},
//     [SPECIES_ROTOM_HEAT ... SPECIES_ROTOM_MOW]  = {SPECIES_ROTOM, SPECIES_ROTOM_HEAT, SPECIES_ROTOM_WASH, SPECIES_ROTOM_FROST, SPECIES_ROTOM_FAN, SPECIES_ROTOM_MOW},
//     [SPECIES_GIRATINA]                          = {SPECIES_GIRATINA, SPECIES_GIRATINA_ORIGIN},
//     [SPECIES_GIRATINA_ORIGIN]                   = {SPECIES_GIRATINA, SPECIES_GIRATINA_ORIGIN},
//     [SPECIES_SHAYMIN]                           = {SPECIES_SHAYMIN, SPECIES_SHAYMIN_SKY},
//     [SPECIES_SHAYMIN_SKY]                       = {SPECIES_SHAYMIN, SPECIES_SHAYMIN_SKY},
//     [SPECIES_ARCEUS]                            = {SPECIES_ARCEUS,
//                                                     SPECIES_ARCEUS_FIGHTING,
//                                                     SPECIES_ARCEUS_FLYING,
//                                                     SPECIES_ARCEUS_POISON,
//                                                     SPECIES_ARCEUS_GROUND,
//                                                     SPECIES_ARCEUS_ROCK,
//                                                     SPECIES_ARCEUS_BUG,
//                                                     SPECIES_ARCEUS_GHOST,
//                                                     SPECIES_ARCEUS_STEEL,
//                                                     SPECIES_ARCEUS_FIRE,
//                                                     SPECIES_ARCEUS_WATER,
//                                                     SPECIES_ARCEUS_GRASS,
//                                                     SPECIES_ARCEUS_ELECTRIC,
//                                                     SPECIES_ARCEUS_PSYCHIC,
//                                                     SPECIES_ARCEUS_ICE,
//                                                     SPECIES_ARCEUS_DRAGON,
//                                                     SPECIES_ARCEUS_DARK,
//                                                     SPECIES_ARCEUS_FAIRY},
//     [SPECIES_ARCEUS_FIGHTING ... SPECIES_ARCEUS_FAIRY] = {SPECIES_ARCEUS,
//                                                     SPECIES_ARCEUS_FIGHTING,
//                                                     SPECIES_ARCEUS_FLYING,
//                                                     SPECIES_ARCEUS_POISON,
//                                                     SPECIES_ARCEUS_GROUND,
//                                                     SPECIES_ARCEUS_ROCK,
//                                                     SPECIES_ARCEUS_BUG,
//                                                     SPECIES_ARCEUS_GHOST,
//                                                     SPECIES_ARCEUS_STEEL,
//                                                     SPECIES_ARCEUS_FIRE,
//                                                     SPECIES_ARCEUS_WATER,
//                                                     SPECIES_ARCEUS_GRASS,
//                                                     SPECIES_ARCEUS_ELECTRIC,
//                                                     SPECIES_ARCEUS_PSYCHIC,
//                                                     SPECIES_ARCEUS_ICE,
//                                                     SPECIES_ARCEUS_DRAGON,
//                                                     SPECIES_ARCEUS_DARK,
//                                                     SPECIES_ARCEUS_FAIRY},
//     [SPECIES_SNIVY ... SPECIES_SERPERIOR]       = {SPECIES_SNIVY, SPECIES_SERVINE, SPECIES_SERPERIOR},
//     [SPECIES_TEPIG ... SPECIES_EMBOAR]          = {SPECIES_TEPIG, SPECIES_PIGNITE, SPECIES_EMBOAR},
//     [SPECIES_OSHAWOTT ... SPECIES_SAMUROTT]     = {SPECIES_OSHAWOTT, SPECIES_DEWOTT, SPECIES_SAMUROTT},
//     [SPECIES_PATRAT ... SPECIES_WATCHOG]        = {SPECIES_PATRAT, SPECIES_WATCHOG},
//     [SPECIES_LILLIPUP ... SPECIES_STOUTLAND]    = {SPECIES_LILLIPUP, SPECIES_HERDIER, SPECIES_STOUTLAND},
//     [SPECIES_PURRLOIN ... SPECIES_LIEPARD]      = {SPECIES_PURRLOIN, SPECIES_LIEPARD},
//     [SPECIES_PANSAGE ... SPECIES_SIMISAGE]      = {SPECIES_PANSAGE, SPECIES_SIMISAGE},
//     [SPECIES_PANSEAR ... SPECIES_SIMISEAR]      = {SPECIES_PANSEAR, SPECIES_SIMISEAR},
//     [SPECIES_PANPOUR ... SPECIES_SIMIPOUR]      = {SPECIES_PANPOUR, SPECIES_SIMIPOUR},
//     [SPECIES_MUNNA ... SPECIES_MUSHARNA]        = {SPECIES_MUNNA, SPECIES_MUSHARNA},
//     [SPECIES_PIDOVE ... SPECIES_UNFEZANT]       = {SPECIES_PIDOVE, SPECIES_TRANQUILL, SPECIES_UNFEZANT},
//     [SPECIES_BLITZLE ... SPECIES_ZEBSTRIKA]     = {SPECIES_BLITZLE, SPECIES_ZEBSTRIKA},
//     [SPECIES_ROGGENROLA ... SPECIES_GIGALITH]   = {SPECIES_ROGGENROLA, SPECIES_BOLDORE, SPECIES_GIGALITH},
//     [SPECIES_WOOBAT ... SPECIES_SWOOBAT]        = {SPECIES_WOOBAT, SPECIES_SWOOBAT},
//     [SPECIES_DRILBUR ... SPECIES_EXCADRILL]     = {SPECIES_DRILBUR, SPECIES_EXCADRILL},
//     [SPECIES_TIMBURR ... SPECIES_CONKELDURR]    = {SPECIES_TIMBURR, SPECIES_GURDURR, SPECIES_CONKELDURR},
//     [SPECIES_TYMPOLE ... SPECIES_SEISMITOAD]    = {SPECIES_TYMPOLE, SPECIES_PALPITOAD, SPECIES_SEISMITOAD},
//     [SPECIES_SEWADDLE ... SPECIES_LEAVANNY]     = {SPECIES_SEWADDLE, SPECIES_SWADLOON, SPECIES_LEAVANNY},
//     [SPECIES_VENIPEDE ... SPECIES_SCOLIPEDE]    = {SPECIES_VENIPEDE, SPECIES_WHIRLIPEDE, SPECIES_SCOLIPEDE},
//     [SPECIES_COTTONEE ... SPECIES_WHIMSICOTT]   = {SPECIES_COTTONEE, SPECIES_WHIMSICOTT},
//     [SPECIES_PETILIL ... SPECIES_LILLIGANT]     = {SPECIES_PETILIL, SPECIES_LILLIGANT},
//     [SPECIES_BASCULIN]                          = {SPECIES_BASCULIN, SPECIES_BASCULIN_BLUE_STRIPED},
//     [SPECIES_BASCULIN_BLUE_STRIPED]             = {SPECIES_BASCULIN, SPECIES_BASCULIN_BLUE_STRIPED},
//     [SPECIES_SANDILE ... SPECIES_KROOKODILE]    = {SPECIES_SANDILE, SPECIES_KROKOROK, SPECIES_KROOKODILE},
//     [SPECIES_DARUMAKA ... SPECIES_DARMANITAN]   = {SPECIES_DARUMAKA, SPECIES_DARMANITAN, SPECIES_DARMANITAN_ZEN},
//     [SPECIES_DWEBBLE ... SPECIES_CRUSTLE]       = {SPECIES_DWEBBLE, SPECIES_CRUSTLE},
//     [SPECIES_SCRAGGY ... SPECIES_SCRAFTY]       = {SPECIES_SCRAGGY, SPECIES_SCRAFTY},
//     [SPECIES_YAMASK ... SPECIES_COFAGRIGUS]     = {SPECIES_YAMASK, SPECIES_COFAGRIGUS},
//     [SPECIES_TIRTOUGA ... SPECIES_CARRACOSTA]   = {SPECIES_TIRTOUGA, SPECIES_CARRACOSTA},
//     [SPECIES_ARCHEN ... SPECIES_ARCHEOPS]       = {SPECIES_ARCHEN, SPECIES_ARCHEOPS},
//     [SPECIES_TRUBBISH ... SPECIES_GARBODOR]     = {SPECIES_TRUBBISH, SPECIES_GARBODOR},
//     [SPECIES_ZORUA ... SPECIES_ZOROARK]         = {SPECIES_ZORUA, SPECIES_ZOROARK},
//     [SPECIES_MINCCINO ... SPECIES_CINCCINO]     = {SPECIES_MINCCINO, SPECIES_CINCCINO},
//     [SPECIES_GOTHITA ... SPECIES_GOTHITELLE]    = {SPECIES_GOTHITA, SPECIES_GOTHORITA, SPECIES_GOTHITELLE},
//     [SPECIES_SOLOSIS ... SPECIES_REUNICLUS]     = {SPECIES_SOLOSIS, SPECIES_DUOSION, SPECIES_REUNICLUS},
//     [SPECIES_DUCKLETT ... SPECIES_SWANNA]       = {SPECIES_DUCKLETT, SPECIES_SWANNA},
//     [SPECIES_VANILLITE ... SPECIES_VANILLUXE]   = {SPECIES_VANILLITE, SPECIES_VANILLISH, SPECIES_VANILLUXE},
//     [SPECIES_DEERLING ... SPECIES_SAWSBUCK]       = {SPECIES_DEERLING, SPECIES_SAWSBUCK,
//                                                     SPECIES_DEERLING_SUMMER, SPECIES_SAWSBUCK_SUMMER,
//                                                     SPECIES_DEERLING_AUTUMN, SPECIES_SAWSBUCK_AUTUMN,
//                                                     SPECIES_DEERLING_WINTER, SPECIES_SAWSBUCK_WINTER},
//     [SPECIES_DEERLING_SUMMER ... SPECIES_SAWSBUCK_WINTER] = {SPECIES_DEERLING, SPECIES_SAWSBUCK,
//                                                     SPECIES_DEERLING_SUMMER, SPECIES_SAWSBUCK_SUMMER,
//                                                     SPECIES_DEERLING_AUTUMN, SPECIES_SAWSBUCK_AUTUMN,
//                                                     SPECIES_DEERLING_WINTER, SPECIES_SAWSBUCK_WINTER},
//     [SPECIES_KARRABLAST ... SPECIES_ESCAVALIER] = {SPECIES_KARRABLAST, SPECIES_ESCAVALIER},
//     [SPECIES_FOONGUS ... SPECIES_AMOONGUSS]     = {SPECIES_FOONGUS, SPECIES_AMOONGUSS},
//     [SPECIES_FRILLISH ... SPECIES_JELLICENT]    = {SPECIES_FRILLISH, SPECIES_JELLICENT},
//     [SPECIES_JOLTIK ... SPECIES_GALVANTULA]     = {SPECIES_JOLTIK, SPECIES_GALVANTULA},
//     [SPECIES_FERROSEED ... SPECIES_FERROTHORN]  = {SPECIES_FERROSEED, SPECIES_FERROTHORN},
//     [SPECIES_KLINK ... SPECIES_KLINKLANG]       = {SPECIES_KLINK, SPECIES_KLANG, SPECIES_KLINKLANG},
//     [SPECIES_TYNAMO ... SPECIES_EELEKTROSS]     = {SPECIES_TYNAMO, SPECIES_EELEKTRIK, SPECIES_EELEKTROSS},
//     [SPECIES_ELGYEM ... SPECIES_BEHEEYEM]       = {SPECIES_ELGYEM, SPECIES_BEHEEYEM},
//     [SPECIES_LITWICK ... SPECIES_CHANDELURE]    = {SPECIES_LITWICK, SPECIES_LAMPENT, SPECIES_CHANDELURE},
//     [SPECIES_AXEW ... SPECIES_HAXORUS]          = {SPECIES_AXEW, SPECIES_FRAXURE, SPECIES_HAXORUS},
//     [SPECIES_CUBCHOO ... SPECIES_BEARTIC]       = {SPECIES_CUBCHOO, SPECIES_BEARTIC},
//     [SPECIES_SHELMET ... SPECIES_ACCELGOR]      = {SPECIES_SHELMET, SPECIES_ACCELGOR},
//     [SPECIES_MIENFOO ... SPECIES_MIENSHAO]      = {SPECIES_MIENFOO, SPECIES_MIENSHAO},
//     [SPECIES_GOLETT ... SPECIES_GOLURK]         = {SPECIES_GOLETT, SPECIES_GOLURK},
//     [SPECIES_PAWNIARD ... SPECIES_BISHARP]      = {SPECIES_PAWNIARD, SPECIES_BISHARP},
//     [SPECIES_RUFFLET ... SPECIES_BRAVIARY]      = {SPECIES_RUFFLET, SPECIES_BRAVIARY},
//     [SPECIES_VULLABY ... SPECIES_MANDIBUZZ]     = {SPECIES_VULLABY, SPECIES_MANDIBUZZ},
//     [SPECIES_DEINO ... SPECIES_HYDREIGON]       = {SPECIES_DEINO, SPECIES_ZWEILOUS, SPECIES_HYDREIGON},
//     [SPECIES_LARVESTA ... SPECIES_VOLCARONA]    = {SPECIES_LARVESTA, SPECIES_VOLCARONA},
//     [SPECIES_TORNADUS]                          = {SPECIES_TORNADUS, SPECIES_TORNADUS_THERIAN},
//     [SPECIES_TORNADUS_THERIAN]                  = {SPECIES_TORNADUS, SPECIES_TORNADUS_THERIAN},
//     [SPECIES_THUNDURUS]                         = {SPECIES_THUNDURUS, SPECIES_THUNDURUS_THERIAN},
//     [SPECIES_THUNDURUS_THERIAN]                 = {SPECIES_THUNDURUS, SPECIES_THUNDURUS_THERIAN},
//     [SPECIES_LANDORUS]                          = {SPECIES_LANDORUS, SPECIES_LANDORUS_THERIAN},
//     [SPECIES_LANDORUS_THERIAN]                  = {SPECIES_LANDORUS, SPECIES_LANDORUS_THERIAN},
//     [SPECIES_KYUREM]                            = {SPECIES_KYUREM, SPECIES_KYUREM_WHITE},
//     [SPECIES_KYUREM_WHITE ... SPECIES_KYUREM_BLACK] = {SPECIES_KYUREM, SPECIES_KYUREM_BLACK},
//     [SPECIES_KELDEO]                            = {SPECIES_KELDEO, SPECIES_KELDEO_RESOLUTE},
//     [SPECIES_KELDEO_RESOLUTE]                   = {SPECIES_KELDEO, SPECIES_KELDEO_RESOLUTE},
//     [SPECIES_MELOETTA]                          = {SPECIES_MELOETTA, SPECIES_MELOETTA_PIROUETTE},
//     [SPECIES_MELOETTA_PIROUETTE]                = {SPECIES_MELOETTA, SPECIES_MELOETTA_PIROUETTE},
//     [SPECIES_GENESECT]                          = {SPECIES_GENESECT, SPECIES_GENESECT_DOUSE, SPECIES_GENESECT_SHOCK, SPECIES_GENESECT_BURN, SPECIES_GENESECT_CHILL},
//     [SPECIES_GENESECT_DOUSE ... SPECIES_GENESECT_CHILL] = {SPECIES_GENESECT, SPECIES_GENESECT_DOUSE, SPECIES_GENESECT_SHOCK, SPECIES_GENESECT_BURN, SPECIES_GENESECT_CHILL},
//     [SPECIES_CHESPIN ... SPECIES_CHESNAUGHT]    = {SPECIES_CHESPIN, SPECIES_QUILLADIN, SPECIES_CHESNAUGHT},
//     [SPECIES_FENNEKIN ... SPECIES_DELPHOX]      = {SPECIES_FENNEKIN, SPECIES_BRAIXEN, SPECIES_DELPHOX},
//     [SPECIES_FROAKIE ... SPECIES_GRENINJA]      = {SPECIES_FROAKIE, SPECIES_FROGADIER, SPECIES_GRENINJA, SPECIES_GRENINJA_BATTLE_BOND, SPECIES_GRENINJA_ASH},
//     [SPECIES_GRENINJA_BATTLE_BOND ... SPECIES_GRENINJA_ASH] = {SPECIES_FROAKIE, SPECIES_FROGADIER, SPECIES_GRENINJA, SPECIES_GRENINJA_BATTLE_BOND, SPECIES_GRENINJA_ASH},
//     [SPECIES_BUNNELBY ... SPECIES_DIGGERSBY]    = {SPECIES_BUNNELBY, SPECIES_DIGGERSBY},
//     [SPECIES_FLETCHLING ... SPECIES_TALONFLAME] = {SPECIES_FLETCHLING, SPECIES_FLETCHINDER, SPECIES_TALONFLAME},
//     [SPECIES_SCATTERBUG ... SPECIES_VIVILLON]   = {SPECIES_SCATTERBUG, SPECIES_SPEWPA,
//                                                     SPECIES_VIVILLON,
//                                                     SPECIES_VIVILLON_POLAR,
//                                                     SPECIES_VIVILLON_TUNDRA,
//                                                     SPECIES_VIVILLON_CONTINENTAL,
//                                                     SPECIES_VIVILLON_GARDEN,
//                                                     SPECIES_VIVILLON_ELEGANT,
//                                                     SPECIES_VIVILLON_MEADOW,
//                                                     SPECIES_VIVILLON_MODERN,
//                                                     SPECIES_VIVILLON_MARINE,
//                                                     SPECIES_VIVILLON_ARCHIPELAGO,
//                                                     SPECIES_VIVILLON_HIGH_PLAINS,
//                                                     SPECIES_VIVILLON_SANDSTORM,
//                                                     SPECIES_VIVILLON_RIVER,
//                                                     SPECIES_VIVILLON_MONSOON,
//                                                     SPECIES_VIVILLON_SAVANNA,
//                                                     SPECIES_VIVILLON_SUN,
//                                                     SPECIES_VIVILLON_OCEAN,
//                                                     SPECIES_VIVILLON_JUNGLE,
//                                                     SPECIES_VIVILLON_FANCY,
//                                                     SPECIES_VIVILLON_POKEBALL},
//     [SPECIES_VIVILLON_POLAR ... SPECIES_VIVILLON_POKEBALL] = {SPECIES_SCATTERBUG, SPECIES_SPEWPA,
//                                                     SPECIES_VIVILLON,
//                                                     SPECIES_VIVILLON_POLAR,
//                                                     SPECIES_VIVILLON_TUNDRA,
//                                                     SPECIES_VIVILLON_CONTINENTAL,
//                                                     SPECIES_VIVILLON_GARDEN,
//                                                     SPECIES_VIVILLON_ELEGANT,
//                                                     SPECIES_VIVILLON_MEADOW,
//                                                     SPECIES_VIVILLON_MODERN,
//                                                     SPECIES_VIVILLON_MARINE,
//                                                     SPECIES_VIVILLON_ARCHIPELAGO,
//                                                     SPECIES_VIVILLON_HIGH_PLAINS,
//                                                     SPECIES_VIVILLON_SANDSTORM,
//                                                     SPECIES_VIVILLON_RIVER,
//                                                     SPECIES_VIVILLON_MONSOON,
//                                                     SPECIES_VIVILLON_SAVANNA,
//                                                     SPECIES_VIVILLON_SUN,
//                                                     SPECIES_VIVILLON_OCEAN,
//                                                     SPECIES_VIVILLON_JUNGLE,
//                                                     SPECIES_VIVILLON_FANCY,
//                                                     SPECIES_VIVILLON_POKEBALL},
//     [SPECIES_LITLEO ... SPECIES_PYROAR]         = {SPECIES_LITLEO, SPECIES_PYROAR},
//     [SPECIES_FLABEBE ... SPECIES_FLORGES]       = {SPECIES_FLABEBE, SPECIES_FLOETTE, SPECIES_FLORGES,
//                                                     SPECIES_FLABEBE_YELLOW, SPECIES_FLOETTE_YELLOW, SPECIES_FLORGES_YELLOW,
//                                                     SPECIES_FLABEBE_ORANGE, SPECIES_FLOETTE_ORANGE, SPECIES_FLORGES_ORANGE,
//                                                     SPECIES_FLABEBE_BLUE, SPECIES_FLOETTE_BLUE, SPECIES_FLORGES_BLUE,
//                                                     SPECIES_FLABEBE_WHITE, SPECIES_FLOETTE_WHITE, SPECIES_FLORGES_WHITE,
//                                                     SPECIES_FLOETTE_ETERNAL},
//     [SPECIES_FLABEBE_YELLOW ... SPECIES_FLORGES_WHITE] = {SPECIES_FLABEBE, SPECIES_FLOETTE, SPECIES_FLORGES,
//                                                     SPECIES_FLABEBE_YELLOW, SPECIES_FLOETTE_YELLOW, SPECIES_FLORGES_YELLOW,
//                                                     SPECIES_FLABEBE_ORANGE, SPECIES_FLOETTE_ORANGE, SPECIES_FLORGES_ORANGE,
//                                                     SPECIES_FLABEBE_BLUE, SPECIES_FLOETTE_BLUE, SPECIES_FLORGES_BLUE,
//                                                     SPECIES_FLABEBE_WHITE, SPECIES_FLOETTE_WHITE, SPECIES_FLORGES_WHITE,
//                                                     SPECIES_FLOETTE_ETERNAL},
//     [SPECIES_SKIDDO ... SPECIES_GOGOAT]         = {SPECIES_SKIDDO, SPECIES_GOGOAT},
//     [SPECIES_PANCHAM ... SPECIES_PANGORO]       = {SPECIES_PANCHAM, SPECIES_PANGORO},
//     [SPECIES_FURFROU]                           = {SPECIES_FURFROU,
//                                                     SPECIES_FURFROU_HEART,
//                                                     SPECIES_FURFROU_STAR,
//                                                     SPECIES_FURFROU_DIAMOND,
//                                                     SPECIES_FURFROU_DEBUTANTE,
//                                                     SPECIES_FURFROU_MATRON,
//                                                     SPECIES_FURFROU_DANDY,
//                                                     SPECIES_FURFROU_LA_REINE,
//                                                     SPECIES_FURFROU_KABUKI,
//                                                     SPECIES_FURFROU_PHARAOH},
//     [SPECIES_FURFROU_HEART ... SPECIES_FURFROU_PHARAOH] = {SPECIES_FURFROU,
//                                                     SPECIES_FURFROU_HEART,
//                                                     SPECIES_FURFROU_STAR,
//                                                     SPECIES_FURFROU_DIAMOND,
//                                                     SPECIES_FURFROU_DEBUTANTE,
//                                                     SPECIES_FURFROU_MATRON,
//                                                     SPECIES_FURFROU_DANDY,
//                                                     SPECIES_FURFROU_LA_REINE,
//                                                     SPECIES_FURFROU_KABUKI,
//                                                     SPECIES_FURFROU_PHARAOH},
//     [SPECIES_ESPURR ... SPECIES_MEOWSTIC]       = {SPECIES_ESPURR, SPECIES_MEOWSTIC, SPECIES_MEOWSTIC_F},
//     [SPECIES_MEOWSTIC_F]                   = {SPECIES_ESPURR, SPECIES_MEOWSTIC, SPECIES_MEOWSTIC_F},
//     [SPECIES_HONEDGE ... SPECIES_AEGISLASH]       = {SPECIES_HONEDGE, SPECIES_DOUBLADE, SPECIES_AEGISLASH, SPECIES_AEGISLASH_BLADE},
//     [SPECIES_AEGISLASH_BLADE]                   = {SPECIES_HONEDGE, SPECIES_DOUBLADE, SPECIES_AEGISLASH, SPECIES_AEGISLASH_BLADE},
//     [SPECIES_SPRITZEE ... SPECIES_AROMATISSE]   = {SPECIES_SPRITZEE, SPECIES_AROMATISSE},
//     [SPECIES_INKAY ... SPECIES_MALAMAR]         = {SPECIES_INKAY, SPECIES_MALAMAR},
//     [SPECIES_BINACLE ... SPECIES_BARBARACLE]    = {SPECIES_BINACLE, SPECIES_BARBARACLE},
//     [SPECIES_SKRELP ... SPECIES_DRAGALGE]       = {SPECIES_SKRELP, SPECIES_DRAGALGE},
//     [SPECIES_CLAUNCHER ... SPECIES_CLAWITZER]   = {SPECIES_CLAUNCHER, SPECIES_CLAWITZER},
//     [SPECIES_HELIOPTILE ... SPECIES_HELIOLISK]  = {SPECIES_HELIOPTILE, SPECIES_HELIOLISK},
//     [SPECIES_TYRUNT ... SPECIES_TYRANTRUM]      = {SPECIES_TYRUNT, SPECIES_TYRANTRUM},
//     [SPECIES_AMAURA ... SPECIES_AURORUS]        = {SPECIES_AMAURA, SPECIES_AURORUS},
//     [SPECIES_GOOMY ... SPECIES_GOODRA]          = {SPECIES_GOOMY, SPECIES_SLIGGOO, SPECIES_GOODRA},
//     [SPECIES_PHANTUMP ... SPECIES_TREVENANT]    = {SPECIES_PHANTUMP, SPECIES_TREVENANT},
//     [SPECIES_PUMPKABOO ... SPECIES_GOURGEIST]   = {SPECIES_PUMPKABOO, SPECIES_GOURGEIST,
//                                                     SPECIES_PUMPKABOO_SMALL, SPECIES_GOURGEIST_SMALL,
//                                                     SPECIES_PUMPKABOO_LARGE, SPECIES_GOURGEIST_LARGE,
//                                                     SPECIES_PUMPKABOO_SUPER, SPECIES_GOURGEIST_SUPER},
//     [SPECIES_PUMPKABOO_SMALL ... SPECIES_GOURGEIST_SUPER] = {SPECIES_PUMPKABOO, SPECIES_GOURGEIST,
//                                                     SPECIES_PUMPKABOO_SMALL, SPECIES_GOURGEIST_SMALL,
//                                                     SPECIES_PUMPKABOO_LARGE, SPECIES_GOURGEIST_LARGE,
//                                                     SPECIES_PUMPKABOO_SUPER, SPECIES_GOURGEIST_SUPER},
//     [SPECIES_BERGMITE ... SPECIES_AVALUGG]      = {SPECIES_BERGMITE, SPECIES_AVALUGG},
//     [SPECIES_NOIBAT ... SPECIES_NOIVERN]        = {SPECIES_NOIBAT, SPECIES_NOIVERN},
//     [SPECIES_ZYGARDE]                           = {SPECIES_ZYGARDE,
//                                                     SPECIES_ZYGARDE_10,
//                                                     SPECIES_ZYGARDE_10_POWER_CONSTRUCT,
//                                                     SPECIES_ZYGARDE_50_POWER_CONSTRUCT,
//                                                     SPECIES_ZYGARDE_COMPLETE},
//     [SPECIES_ZYGARDE_10 ... SPECIES_ZYGARDE_COMPLETE] = {SPECIES_ZYGARDE,
//                                                     SPECIES_ZYGARDE_10,
//                                                     SPECIES_ZYGARDE_10_POWER_CONSTRUCT,
//                                                     SPECIES_ZYGARDE_50_POWER_CONSTRUCT,
//                                                     SPECIES_ZYGARDE_COMPLETE},
//     [SPECIES_HOOPA]                             = {SPECIES_HOOPA, SPECIES_HOOPA_UNBOUND},
//     [SPECIES_HOOPA_UNBOUND]                     = {SPECIES_HOOPA, SPECIES_HOOPA_UNBOUND},
//     [SPECIES_ROWLET ... SPECIES_DECIDUEYE]      = {SPECIES_ROWLET, SPECIES_DARTRIX, SPECIES_DECIDUEYE},
//     [SPECIES_LITTEN ... SPECIES_INCINEROAR]     = {SPECIES_LITTEN, SPECIES_TORRACAT, SPECIES_INCINEROAR},
//     [SPECIES_POPPLIO ... SPECIES_PRIMARINA]     = {SPECIES_POPPLIO, SPECIES_BRIONNE, SPECIES_PRIMARINA},
//     [SPECIES_PIKIPEK ... SPECIES_TOUCANNON]     = {SPECIES_PIKIPEK, SPECIES_TRUMBEAK, SPECIES_TOUCANNON},
//     [SPECIES_YUNGOOS ... SPECIES_GUMSHOOS]      = {SPECIES_YUNGOOS, SPECIES_GUMSHOOS},
//     [SPECIES_GRUBBIN ... SPECIES_VIKAVOLT]      = {SPECIES_GRUBBIN, SPECIES_CHARJABUG, SPECIES_VIKAVOLT},
//     [SPECIES_CRABRAWLER ... SPECIES_CRABOMINABLE] = {SPECIES_CRABRAWLER, SPECIES_CRABOMINABLE},
//     [SPECIES_ORICORIO]                          = {SPECIES_ORICORIO, SPECIES_ORICORIO_POM_POM, SPECIES_ORICORIO_PAU, SPECIES_ORICORIO_SENSU},
//     [SPECIES_ORICORIO_POM_POM ... SPECIES_ORICORIO_SENSU] = {SPECIES_ORICORIO, SPECIES_ORICORIO_POM_POM, SPECIES_ORICORIO_PAU, SPECIES_ORICORIO_SENSU},
//     [SPECIES_CUTIEFLY ... SPECIES_RIBOMBEE]     = {SPECIES_CUTIEFLY, SPECIES_RIBOMBEE},
//     [SPECIES_ROCKRUFF ... SPECIES_LYCANROC] = {SPECIES_ROCKRUFF, SPECIES_LYCANROC, SPECIES_ROCKRUFF_OWN_TEMPO, SPECIES_LYCANROC_MIDNIGHT, SPECIES_LYCANROC_DUSK},
//     [SPECIES_ROCKRUFF_OWN_TEMPO ... SPECIES_LYCANROC_DUSK] = {SPECIES_ROCKRUFF, SPECIES_LYCANROC, SPECIES_ROCKRUFF_OWN_TEMPO, SPECIES_LYCANROC_MIDNIGHT, SPECIES_LYCANROC_DUSK},
//     [SPECIES_MAREANIE ... SPECIES_TOXAPEX]      = {SPECIES_MAREANIE, SPECIES_TOXAPEX},
//     [SPECIES_MUDBRAY ... SPECIES_MUDSDALE]      = {SPECIES_MUDBRAY, SPECIES_MUDSDALE},
//     [SPECIES_DEWPIDER ... SPECIES_ARAQUANID]    = {SPECIES_DEWPIDER, SPECIES_ARAQUANID},
//     [SPECIES_FOMANTIS ... SPECIES_LURANTIS]     = {SPECIES_FOMANTIS, SPECIES_LURANTIS},
//     [SPECIES_MORELULL ... SPECIES_SHIINOTIC]    = {SPECIES_MORELULL, SPECIES_SHIINOTIC},
//     [SPECIES_SALANDIT ... SPECIES_SALAZZLE]     = {SPECIES_SALANDIT, SPECIES_SALAZZLE},
//     [SPECIES_STUFFUL ... SPECIES_BEWEAR]        = {SPECIES_STUFFUL, SPECIES_BEWEAR},
//     [SPECIES_BOUNSWEET ... SPECIES_TSAREENA]    = {SPECIES_BOUNSWEET, SPECIES_STEENEE, SPECIES_TSAREENA},
//     [SPECIES_WIMPOD ... SPECIES_GOLISOPOD]      = {SPECIES_WIMPOD, SPECIES_GOLISOPOD},
//     [SPECIES_SANDYGAST ... SPECIES_PALOSSAND]   = {SPECIES_SANDYGAST, SPECIES_PALOSSAND},
//     [SPECIES_TYPE_NULL ... SPECIES_SILVALLY]    = {SPECIES_TYPE_NULL,
//                                                     SPECIES_SILVALLY,
//                                                     SPECIES_SILVALLY_FIGHTING,
//                                                     SPECIES_SILVALLY_FLYING,
//                                                     SPECIES_SILVALLY_POISON,
//                                                     SPECIES_SILVALLY_GROUND,
//                                                     SPECIES_SILVALLY_ROCK,
//                                                     SPECIES_SILVALLY_BUG,
//                                                     SPECIES_SILVALLY_GHOST,
//                                                     SPECIES_SILVALLY_STEEL,
//                                                     SPECIES_SILVALLY_FIRE,
//                                                     SPECIES_SILVALLY_WATER,
//                                                     SPECIES_SILVALLY_GRASS,
//                                                     SPECIES_SILVALLY_ELECTRIC,
//                                                     SPECIES_SILVALLY_PSYCHIC,
//                                                     SPECIES_SILVALLY_ICE,
//                                                     SPECIES_SILVALLY_DRAGON,
//                                                     SPECIES_SILVALLY_DARK,
//                                                     SPECIES_SILVALLY_FAIRY},
//     [SPECIES_SILVALLY_FIGHTING ... SPECIES_SILVALLY_FAIRY] = {SPECIES_TYPE_NULL,
//                                                     SPECIES_SILVALLY,
//                                                     SPECIES_SILVALLY_FIGHTING,
//                                                     SPECIES_SILVALLY_FLYING,
//                                                     SPECIES_SILVALLY_POISON,
//                                                     SPECIES_SILVALLY_GROUND,
//                                                     SPECIES_SILVALLY_ROCK,
//                                                     SPECIES_SILVALLY_BUG,
//                                                     SPECIES_SILVALLY_GHOST,
//                                                     SPECIES_SILVALLY_STEEL,
//                                                     SPECIES_SILVALLY_FIRE,
//                                                     SPECIES_SILVALLY_WATER,
//                                                     SPECIES_SILVALLY_GRASS,
//                                                     SPECIES_SILVALLY_ELECTRIC,
//                                                     SPECIES_SILVALLY_PSYCHIC,
//                                                     SPECIES_SILVALLY_ICE,
//                                                     SPECIES_SILVALLY_DRAGON,
//                                                     SPECIES_SILVALLY_DARK,
//                                                     SPECIES_SILVALLY_FAIRY},
//     [SPECIES_MINIOR_METEOR_ORANGE ... SPECIES_MINIOR_CORE_VIOLET] = {SPECIES_MINIOR,
//                                                     SPECIES_MINIOR_METEOR_ORANGE,
//                                                     SPECIES_MINIOR_METEOR_YELLOW,
//                                                     SPECIES_MINIOR_METEOR_GREEN,
//                                                     SPECIES_MINIOR_METEOR_BLUE,
//                                                     SPECIES_MINIOR_METEOR_INDIGO,
//                                                     SPECIES_MINIOR_METEOR_VIOLET,
//                                                     SPECIES_MINIOR_CORE_RED,
//                                                     SPECIES_MINIOR_CORE_ORANGE,
//                                                     SPECIES_MINIOR_CORE_YELLOW,
//                                                     SPECIES_MINIOR_CORE_GREEN,
//                                                     SPECIES_MINIOR_CORE_BLUE,
//                                                     SPECIES_MINIOR_CORE_INDIGO,
//                                                     SPECIES_MINIOR_CORE_VIOLET},
//     [SPECIES_MINIOR]                            = {SPECIES_MINIOR,
//                                                     SPECIES_MINIOR_METEOR_ORANGE,
//                                                     SPECIES_MINIOR_METEOR_YELLOW,
//                                                     SPECIES_MINIOR_METEOR_GREEN,
//                                                     SPECIES_MINIOR_METEOR_BLUE,
//                                                     SPECIES_MINIOR_METEOR_INDIGO,
//                                                     SPECIES_MINIOR_METEOR_VIOLET,
//                                                     SPECIES_MINIOR_CORE_RED,
//                                                     SPECIES_MINIOR_CORE_ORANGE,
//                                                     SPECIES_MINIOR_CORE_YELLOW,
//                                                     SPECIES_MINIOR_CORE_GREEN,
//                                                     SPECIES_MINIOR_CORE_BLUE,
//                                                     SPECIES_MINIOR_CORE_INDIGO,
//                                                     SPECIES_MINIOR_CORE_VIOLET},
//     [SPECIES_MIMIKYU]                           = {SPECIES_MIMIKYU, SPECIES_MIMIKYU_BUSTED},
//     [SPECIES_MIMIKYU_BUSTED]                    = {SPECIES_MIMIKYU, SPECIES_MIMIKYU_BUSTED},
//     [SPECIES_JANGMO_O ... SPECIES_KOMMO_O]      = {SPECIES_JANGMO_O, SPECIES_HAKAMO_O, SPECIES_KOMMO_O},
//     [SPECIES_COSMOG ... SPECIES_LUNALA]         = {SPECIES_COSMOG, SPECIES_COSMOEM, SPECIES_SOLGALEO, SPECIES_LUNALA},
//     [SPECIES_POIPOLE ... SPECIES_NAGANADEL]     = {SPECIES_POIPOLE, SPECIES_NAGANADEL},
//     [SPECIES_MELTAN ... SPECIES_MELMETAL]       = {SPECIES_MELTAN, SPECIES_MELMETAL},
//     [SPECIES_GROOKEY ... SPECIES_RILLABOOM]     = {SPECIES_GROOKEY, SPECIES_THWACKEY, SPECIES_RILLABOOM},
//     [SPECIES_SCORBUNNY ... SPECIES_CINDERACE]     = {SPECIES_SCORBUNNY, SPECIES_RABOOT, SPECIES_CINDERACE},
//     [SPECIES_SOBBLE ... SPECIES_INTELEON]       = {SPECIES_SOBBLE, SPECIES_DRIZZILE, SPECIES_INTELEON},
//     [SPECIES_SKWOVET ... SPECIES_GREEDENT]      = {SPECIES_SKWOVET, SPECIES_GREEDENT},
//     [SPECIES_ROOKIDEE ... SPECIES_CORVIKNIGHT]  = {SPECIES_ROOKIDEE, SPECIES_CORVISQUIRE, SPECIES_CORVIKNIGHT},
//     [SPECIES_BLIPBUG ... SPECIES_ORBEETLE]      = {SPECIES_BLIPBUG, SPECIES_DOTTLER, SPECIES_ORBEETLE},
//     [SPECIES_NICKIT ... SPECIES_THIEVUL]        = {SPECIES_NICKIT, SPECIES_THIEVUL},
//     [SPECIES_GOSSIFLEUR ... SPECIES_ELDEGOSS]   = {SPECIES_GOSSIFLEUR, SPECIES_ELDEGOSS},
//     [SPECIES_WOOLOO ... SPECIES_DUBWOOL]        = {SPECIES_WOOLOO, SPECIES_DUBWOOL},
//     [SPECIES_CHEWTLE ... SPECIES_DREDNAW]       = {SPECIES_CHEWTLE, SPECIES_DREDNAW},
//     [SPECIES_YAMPER ... SPECIES_BOLTUND]        = {SPECIES_YAMPER, SPECIES_BOLTUND},
//     [SPECIES_ROLYCOLY ... SPECIES_COALOSSAL]    = {SPECIES_ROLYCOLY, SPECIES_CARKOL, SPECIES_COALOSSAL},
//     [SPECIES_APPLIN ... SPECIES_APPLETUN]       = {SPECIES_APPLIN, SPECIES_FLAPPLE, SPECIES_APPLETUN},
//     [SPECIES_SILICOBRA ... SPECIES_SANDACONDA]  = {SPECIES_SILICOBRA, SPECIES_SANDACONDA},
//     [SPECIES_CRAMORANT]                         = {SPECIES_CRAMORANT, SPECIES_CRAMORANT_GULPING, SPECIES_CRAMORANT_GORGING},
//     [SPECIES_CRAMORANT_GULPING ... SPECIES_CRAMORANT_GORGING] = {SPECIES_CRAMORANT, SPECIES_CRAMORANT_GULPING, SPECIES_CRAMORANT_GORGING},
//     [SPECIES_ARROKUDA ... SPECIES_BARRASKEWDA]  = {SPECIES_ARROKUDA, SPECIES_BARRASKEWDA},
//     [SPECIES_TOXEL ... SPECIES_TOXTRICITY]      = {SPECIES_TOXEL, SPECIES_TOXTRICITY, SPECIES_TOXTRICITY_LOW_KEY},
//     [SPECIES_TOXTRICITY_LOW_KEY]                = {SPECIES_TOXEL, SPECIES_TOXTRICITY, SPECIES_TOXTRICITY_LOW_KEY},
//     [SPECIES_SIZZLIPEDE ... SPECIES_CENTISKORCH] = {SPECIES_SIZZLIPEDE, SPECIES_CENTISKORCH},
//     [SPECIES_CLOBBOPUS ... SPECIES_GRAPPLOCT]   = {SPECIES_CLOBBOPUS, SPECIES_GRAPPLOCT},
//     [SPECIES_SINISTEA ... SPECIES_POLTEAGEIST]  = {SPECIES_SINISTEA, SPECIES_POLTEAGEIST, SPECIES_SINISTEA_ANTIQUE, SPECIES_POLTEAGEIST_ANTIQUE},
//     [SPECIES_SINISTEA_ANTIQUE ... SPECIES_POLTEAGEIST_ANTIQUE] = {SPECIES_SINISTEA, SPECIES_POLTEAGEIST, SPECIES_SINISTEA_ANTIQUE, SPECIES_POLTEAGEIST_ANTIQUE},
//     [SPECIES_HATENNA ... SPECIES_HATTERENE]     = {SPECIES_HATENNA, SPECIES_HATTREM, SPECIES_HATTERENE},
//     [SPECIES_IMPIDIMP ... SPECIES_GRIMMSNARL]   = {SPECIES_IMPIDIMP, SPECIES_MORGREM, SPECIES_GRIMMSNARL},
//     [SPECIES_OBSTAGOON]                         = {SPECIES_ZIGZAGOON_GALAR, SPECIES_LINOONE_GALAR, SPECIES_OBSTAGOON},
//     [SPECIES_ZIGZAGOON_GALAR ... SPECIES_LINOONE_GALAR] = {SPECIES_ZIGZAGOON_GALAR, SPECIES_LINOONE_GALAR, SPECIES_OBSTAGOON},
//     [SPECIES_PERRSERKER]                        = {SPECIES_MEOWTH_GALAR, SPECIES_PERRSERKER},
//     [SPECIES_MEOWTH_GALAR]                   = {SPECIES_MEOWTH_GALAR, SPECIES_PERRSERKER},
//     [SPECIES_CURSOLA]                           = {SPECIES_CORSOLA_GALAR, SPECIES_CURSOLA},
//     [SPECIES_CORSOLA_GALAR]                  = {SPECIES_CORSOLA_GALAR, SPECIES_CURSOLA},
//     [SPECIES_SIRFETCHD]                         = {SPECIES_FARFETCHD_GALAR, SPECIES_SIRFETCHD},
//     [SPECIES_FARFETCHD_GALAR]                = {SPECIES_FARFETCHD_GALAR, SPECIES_SIRFETCHD},
//     [SPECIES_RUNERIGUS]                         = {SPECIES_YAMASK_GALAR, SPECIES_RUNERIGUS},
//     [SPECIES_YAMASK_GALAR]                   = {SPECIES_YAMASK_GALAR, SPECIES_RUNERIGUS},
//     [SPECIES_MILCERY ... SPECIES_ALCREMIE]      = {SPECIES_MILCERY,
//                                                     SPECIES_ALCREMIE,
//                                                     SPECIES_ALCREMIE_RUBY_CREAM,
//                                                     SPECIES_ALCREMIE_MATCHA_CREAM,
//                                                     SPECIES_ALCREMIE_MINT_CREAM,
//                                                     SPECIES_ALCREMIE_LEMON_CREAM,
//                                                     SPECIES_ALCREMIE_SALTED_CREAM,
//                                                     SPECIES_ALCREMIE_RUBY_SWIRL,
//                                                     SPECIES_ALCREMIE_CARAMEL_SWIRL,
//                                                     SPECIES_ALCREMIE_RAINBOW_SWIRL},
//     [SPECIES_SNOM ... SPECIES_FROSMOTH]         = {SPECIES_SNOM, SPECIES_FROSMOTH},
//     [SPECIES_EISCUE]                            = {SPECIES_EISCUE, SPECIES_EISCUE_NOICE},
//     [SPECIES_EISCUE_NOICE]                 = {SPECIES_EISCUE, SPECIES_EISCUE_NOICE},
//     [SPECIES_INDEEDEE]                          = {SPECIES_INDEEDEE, SPECIES_INDEEDEE_F},
//     [SPECIES_INDEEDEE_F]                   = {SPECIES_INDEEDEE, SPECIES_INDEEDEE_F},
//     [SPECIES_MORPEKO]                           = {SPECIES_MORPEKO, SPECIES_MORPEKO_HANGRY},
//     [SPECIES_MORPEKO_HANGRY]                    = {SPECIES_MORPEKO, SPECIES_MORPEKO_HANGRY},
//     [SPECIES_CUFANT ... SPECIES_COPPERAJAH]     = {SPECIES_CUFANT, SPECIES_COPPERAJAH},
//     [SPECIES_DREEPY ... SPECIES_DRAGAPULT]      = {SPECIES_DREEPY, SPECIES_DRAKLOAK, SPECIES_DRAGAPULT},
//     [SPECIES_ZACIAN]                            = {SPECIES_ZACIAN, SPECIES_ZACIAN_CROWNED},
//     [SPECIES_ZACIAN_CROWNED]              = {SPECIES_ZACIAN, SPECIES_ZACIAN_CROWNED},
//     [SPECIES_ZAMAZENTA]                         = {SPECIES_ZAMAZENTA, SPECIES_ZAMAZENTA_CROWNED},
//     [SPECIES_ZAMAZENTA_CROWNED]          = {SPECIES_ZAMAZENTA, SPECIES_ZAMAZENTA_CROWNED},
//     [SPECIES_ETERNATUS]                         = {SPECIES_ETERNATUS, SPECIES_ETERNATUS_ETERNAMAX},
//     [SPECIES_ETERNATUS_ETERNAMAX]               = {SPECIES_ETERNATUS, SPECIES_ETERNATUS_ETERNAMAX},
//     [SPECIES_KUBFU ... SPECIES_URSHIFU]         = {SPECIES_KUBFU, SPECIES_URSHIFU, SPECIES_URSHIFU_RAPID_STRIKE},
//     [SPECIES_URSHIFU_RAPID_STRIKE]        = {SPECIES_KUBFU, SPECIES_URSHIFU, SPECIES_URSHIFU_RAPID_STRIKE},
//     [SPECIES_ZARUDE]                            = {SPECIES_ZARUDE, SPECIES_ZARUDE_DADA},
//     [SPECIES_ZARUDE_DADA]                       = {SPECIES_ZARUDE, SPECIES_ZARUDE_DADA},
//     [SPECIES_CALYREX]                           = {SPECIES_CALYREX, SPECIES_CALYREX_ICE, SPECIES_CALYREX_SHADOW},
//     [SPECIES_CALYREX_ICE ... SPECIES_CALYREX_SHADOW] = {SPECIES_CALYREX, SPECIES_CALYREX_ICE, SPECIES_CALYREX_SHADOW},
//     [SPECIES_RATTATA_ALOLA ... SPECIES_RATICATE_ALOLA] = {SPECIES_RATTATA_ALOLA, SPECIES_RATICATE_ALOLA},
//     [SPECIES_SANDSHREW_ALOLA ... SPECIES_SANDSLASH_ALOLA] = {SPECIES_SANDSHREW_ALOLA, SPECIES_SANDSLASH_ALOLA},
//     [SPECIES_VULPIX_ALOLA ... SPECIES_NINETALES_ALOLA] = {SPECIES_VULPIX_ALOLA, SPECIES_NINETALES_ALOLA},
//     [SPECIES_DIGLETT_ALOLA ... SPECIES_DUGTRIO_ALOLA] = {SPECIES_DIGLETT_ALOLA, SPECIES_DUGTRIO_ALOLA},
//     [SPECIES_MEOWTH_ALOLA ... SPECIES_PERSIAN_ALOLA] = {SPECIES_MEOWTH_ALOLA, SPECIES_PERSIAN_ALOLA},
//     [SPECIES_GEODUDE_ALOLA ... SPECIES_GOLEM_ALOLA] = {SPECIES_GEODUDE_ALOLA, SPECIES_GRAVELER_ALOLA, SPECIES_GOLEM_ALOLA},
//     [SPECIES_GRIMER_ALOLA ... SPECIES_MUK_ALOLA] = {SPECIES_GRIMER_ALOLA, SPECIES_MUK_ALOLA},
//     [SPECIES_PONYTA_GALAR ... SPECIES_RAPIDASH_GALAR] = {SPECIES_PONYTA_GALAR, SPECIES_RAPIDASH_GALAR},
//     [SPECIES_SLOWPOKE_GALAR ... SPECIES_SLOWBRO_GALAR] = {SPECIES_SLOWPOKE_GALAR, SPECIES_SLOWBRO_GALAR, SPECIES_SLOWKING_GALAR},
//     [SPECIES_SLOWKING_GALAR]                 = {SPECIES_SLOWPOKE_GALAR, SPECIES_SLOWBRO_GALAR, SPECIES_SLOWKING_GALAR},
//     [SPECIES_DARUMAKA_GALAR ... SPECIES_DARMANITAN_GALAR] = {SPECIES_DARUMAKA_GALAR, SPECIES_DARMANITAN_GALAR, SPECIES_DARMANITAN_GALAR_ZEN},
//
//     #endif
// };

#define RANDOM_TYPE_COUNT ARRAY_COUNT(sOneTypeChallengeValidTypes)
static const u8  sOneTypeChallengeValidTypes[NUMBER_OF_MON_TYPES-1] =
{
    TYPE_NORMAL   ,
    TYPE_FIGHTING ,
    TYPE_FLYING   ,
    TYPE_POISON   ,
    TYPE_GROUND   ,
    TYPE_ROCK     ,
    TYPE_BUG      ,
    TYPE_GHOST    ,
    TYPE_STEEL    ,
    TYPE_FIRE     ,
    TYPE_WATER    ,
    TYPE_GRASS    ,
    TYPE_ELECTRIC ,
    TYPE_PSYCHIC  ,
    TYPE_ICE      ,
    TYPE_DRAGON   ,
    TYPE_DARK     ,
    #ifdef POKEMON_EXPANSION
        #if P_UPDATED_TYPES >= GEN_6
            TYPE_FAIRY,
        #endif
    #endif
};

#define RANDOM_MOVES_COUNT ARRAY_COUNT(sRandomValidMoves)
static const u16 sRandomValidMoves[MOVES_COUNT-1] =
{
    MOVE_POUND,
    MOVE_KARATE_CHOP,
    MOVE_DOUBLE_SLAP,
    MOVE_COMET_PUNCH,
    MOVE_MEGA_PUNCH,
    MOVE_PAY_DAY,
    MOVE_FIRE_PUNCH,
    MOVE_ICE_PUNCH,
    MOVE_THUNDER_PUNCH,
    MOVE_SCRATCH,
    #if defined(BATTLE_ENGINE) || defined (POKEMON_EXPANSION)
    MOVE_VISE_GRIP,
    #else
    MOVE_VICE_GRIP,
    #endif
    MOVE_GUILLOTINE,
    MOVE_RAZOR_WIND,
    MOVE_SWORDS_DANCE,
    MOVE_CUT,
    MOVE_GUST,
    MOVE_WING_ATTACK,
    MOVE_WHIRLWIND,
    MOVE_FLY,
    MOVE_BIND,
    MOVE_SLAM,
    MOVE_VINE_WHIP,
    MOVE_STOMP,
    MOVE_DOUBLE_KICK,
    MOVE_MEGA_KICK,
    MOVE_JUMP_KICK,
    MOVE_ROLLING_KICK,
    MOVE_SAND_ATTACK,
    MOVE_HEADBUTT,
    MOVE_HORN_ATTACK,
    MOVE_FURY_ATTACK,
    MOVE_HORN_DRILL,
    MOVE_TACKLE,
    MOVE_BODY_SLAM,
    MOVE_WRAP,
    MOVE_TAKE_DOWN,
    MOVE_THRASH,
    MOVE_DOUBLE_EDGE,
    MOVE_TAIL_WHIP,
    MOVE_POISON_STING,
    MOVE_TWINEEDLE,
    MOVE_PIN_MISSILE,
    MOVE_LEER,
    MOVE_BITE,
    MOVE_GROWL,
    MOVE_ROAR,
    MOVE_SING,
    MOVE_SUPERSONIC,
    MOVE_SONIC_BOOM,
    MOVE_DISABLE,
    MOVE_ACID,
    MOVE_EMBER,
    MOVE_FLAMETHROWER,
    MOVE_MIST,
    MOVE_WATER_GUN,
    MOVE_HYDRO_PUMP,
    MOVE_SURF,
    MOVE_ICE_BEAM,
    MOVE_BLIZZARD,
    MOVE_PSYBEAM,
    MOVE_BUBBLE_BEAM,
    MOVE_AURORA_BEAM,
    MOVE_HYPER_BEAM,
    MOVE_PECK,
    MOVE_DRILL_PECK,
    MOVE_SUBMISSION,
    MOVE_LOW_KICK,
    MOVE_COUNTER,
    MOVE_SEISMIC_TOSS,
    MOVE_STRENGTH,
    MOVE_ABSORB,
    MOVE_MEGA_DRAIN,
    MOVE_LEECH_SEED,
    MOVE_GROWTH,
    MOVE_RAZOR_LEAF,
    MOVE_SOLAR_BEAM,
    MOVE_POISON_POWDER,
    MOVE_STUN_SPORE,
    MOVE_SLEEP_POWDER,
    MOVE_PETAL_DANCE,
    MOVE_STRING_SHOT,
    MOVE_DRAGON_RAGE,
    MOVE_FIRE_SPIN,
    MOVE_THUNDER_SHOCK,
    MOVE_THUNDERBOLT,
    MOVE_THUNDER_WAVE,
    MOVE_THUNDER,
    MOVE_ROCK_THROW,
    MOVE_EARTHQUAKE,
    MOVE_FISSURE,
    MOVE_DIG,
    MOVE_TOXIC,
    MOVE_CONFUSION,
    MOVE_PSYCHIC,
    MOVE_HYPNOSIS,
    MOVE_MEDITATE,
    MOVE_AGILITY,
    MOVE_QUICK_ATTACK,
    MOVE_RAGE,
    MOVE_TELEPORT,
    MOVE_NIGHT_SHADE,
    MOVE_MIMIC,
    MOVE_SCREECH,
    MOVE_DOUBLE_TEAM,
    MOVE_RECOVER,
    MOVE_HARDEN,
    MOVE_MINIMIZE,
    MOVE_SMOKESCREEN,
    MOVE_CONFUSE_RAY,
    MOVE_WITHDRAW,
    MOVE_DEFENSE_CURL,
    MOVE_BARRIER,
    MOVE_LIGHT_SCREEN,
    MOVE_HAZE,
    MOVE_REFLECT,
    MOVE_FOCUS_ENERGY,
    MOVE_BIDE,
    MOVE_METRONOME,
    MOVE_MIRROR_MOVE,
    MOVE_SELF_DESTRUCT,
    MOVE_EGG_BOMB,
    MOVE_LICK,
    MOVE_SMOG,
    MOVE_SLUDGE,
    MOVE_BONE_CLUB,
    MOVE_FIRE_BLAST,
    MOVE_WATERFALL,
    MOVE_CLAMP,
    MOVE_SWIFT,
    MOVE_SKULL_BASH,
    MOVE_SPIKE_CANNON,
    MOVE_CONSTRICT,
    MOVE_AMNESIA,
    MOVE_KINESIS,
    MOVE_SOFT_BOILED,
    #if defined(BATTLE_ENGINE) || defined (POKEMON_EXPANSION)
    MOVE_HIGH_JUMP_KICK,
    #else
    MOVE_HI_JUMP_KICK,
    #endif
    MOVE_GLARE,
    MOVE_DREAM_EATER,
    MOVE_POISON_GAS,
    MOVE_BARRAGE,
    MOVE_LEECH_LIFE,
    MOVE_LOVELY_KISS,
    MOVE_SKY_ATTACK,
    MOVE_TRANSFORM,
    MOVE_BUBBLE,
    MOVE_DIZZY_PUNCH,
    MOVE_SPORE,
    MOVE_FLASH,
    MOVE_PSYWAVE,
    MOVE_SPLASH,
    MOVE_ACID_ARMOR,
    MOVE_CRABHAMMER,
    MOVE_EXPLOSION,
    MOVE_FURY_SWIPES,
    MOVE_BONEMERANG,
    MOVE_REST,
    MOVE_ROCK_SLIDE,
    MOVE_HYPER_FANG,
    MOVE_SHARPEN,
    MOVE_CONVERSION,
    MOVE_TRI_ATTACK,
    MOVE_SUPER_FANG,
    MOVE_SLASH,
    MOVE_SUBSTITUTE,
    MOVE_STRUGGLE,
    MOVE_SKETCH,
    MOVE_TRIPLE_KICK,
    MOVE_THIEF,
    MOVE_SPIDER_WEB,
    MOVE_MIND_READER,
    MOVE_NIGHTMARE,
    MOVE_FLAME_WHEEL,
    MOVE_SNORE,
    MOVE_CURSE,
    MOVE_FLAIL,
    MOVE_CONVERSION_2,
    MOVE_AEROBLAST,
    MOVE_COTTON_SPORE,
    MOVE_REVERSAL,
    MOVE_SPITE,
    MOVE_POWDER_SNOW,
    MOVE_PROTECT,
    MOVE_MACH_PUNCH,
    MOVE_SCARY_FACE,
    #if defined(BATTLE_ENGINE) || defined (POKEMON_EXPANSION)
    MOVE_FEINT_ATTACK,
    #else
    MOVE_FAINT_ATTACK,
    #endif
    MOVE_SWEET_KISS,
    MOVE_BELLY_DRUM,
    MOVE_SLUDGE_BOMB,
    MOVE_MUD_SLAP,
    MOVE_OCTAZOOKA,
    MOVE_SPIKES,
    MOVE_ZAP_CANNON,
    MOVE_FORESIGHT,
    MOVE_DESTINY_BOND,
    MOVE_PERISH_SONG,
    MOVE_ICY_WIND,
    MOVE_DETECT,
    MOVE_BONE_RUSH,
    MOVE_LOCK_ON,
    MOVE_OUTRAGE,
    MOVE_SANDSTORM,
    MOVE_GIGA_DRAIN,
    MOVE_ENDURE,
    MOVE_CHARM,
    MOVE_ROLLOUT,
    MOVE_FALSE_SWIPE,
    MOVE_SWAGGER,
    MOVE_MILK_DRINK,
    MOVE_SPARK,
    MOVE_FURY_CUTTER,
    MOVE_STEEL_WING,
    MOVE_MEAN_LOOK,
    MOVE_ATTRACT,
    MOVE_SLEEP_TALK,
    MOVE_HEAL_BELL,
    MOVE_RETURN,
    MOVE_PRESENT,
    MOVE_FRUSTRATION,
    MOVE_SAFEGUARD,
    MOVE_PAIN_SPLIT,
    MOVE_SACRED_FIRE,
    MOVE_MAGNITUDE,
    MOVE_DYNAMIC_PUNCH,
    MOVE_MEGAHORN,
    MOVE_DRAGON_BREATH,
    MOVE_BATON_PASS,
    MOVE_ENCORE,
    MOVE_PURSUIT,
    MOVE_RAPID_SPIN,
    MOVE_SWEET_SCENT,
    MOVE_IRON_TAIL,
    MOVE_METAL_CLAW,
    MOVE_VITAL_THROW,
    MOVE_MORNING_SUN,
    MOVE_SYNTHESIS,
    MOVE_MOONLIGHT,
    MOVE_HIDDEN_POWER,
    MOVE_CROSS_CHOP,
    MOVE_TWISTER,
    MOVE_RAIN_DANCE,
    MOVE_SUNNY_DAY,
    MOVE_CRUNCH,
    MOVE_MIRROR_COAT,
    MOVE_PSYCH_UP,
    MOVE_EXTREME_SPEED,
    MOVE_ANCIENT_POWER,
    MOVE_SHADOW_BALL,
    MOVE_FUTURE_SIGHT,
    MOVE_ROCK_SMASH,
    MOVE_WHIRLPOOL,
    MOVE_BEAT_UP,
    MOVE_FAKE_OUT,
    MOVE_UPROAR,
    MOVE_STOCKPILE,
    MOVE_SPIT_UP,
    MOVE_SWALLOW,
    MOVE_HEAT_WAVE,
    MOVE_HAIL,
    MOVE_TORMENT,
    MOVE_FLATTER,
    MOVE_WILL_O_WISP,
    MOVE_MEMENTO,
    MOVE_FACADE,
    MOVE_FOCUS_PUNCH,
    #if defined(BATTLE_ENGINE) || defined (POKEMON_EXPANSION)
    MOVE_SMELLING_SALTS,
    #else
    MOVE_SMELLING_SALT,
    #endif
    MOVE_FOLLOW_ME,
    MOVE_NATURE_POWER,
    MOVE_CHARGE,
    MOVE_TAUNT,
    MOVE_HELPING_HAND,
    MOVE_TRICK,
    MOVE_ROLE_PLAY,
    MOVE_WISH,
    MOVE_ASSIST,
    MOVE_INGRAIN,
    MOVE_SUPERPOWER,
    MOVE_MAGIC_COAT,
    MOVE_RECYCLE,
    MOVE_REVENGE,
    MOVE_BRICK_BREAK,
    MOVE_YAWN,
    MOVE_KNOCK_OFF,
    MOVE_ENDEAVOR,
    MOVE_ERUPTION,
    MOVE_SKILL_SWAP,
    MOVE_IMPRISON,
    MOVE_REFRESH,
    MOVE_GRUDGE,
    MOVE_SNATCH,
    MOVE_SECRET_POWER,
    MOVE_DIVE,
    MOVE_ARM_THRUST,
    MOVE_CAMOUFLAGE,
    MOVE_TAIL_GLOW,
    MOVE_LUSTER_PURGE,
    MOVE_MIST_BALL,
    MOVE_FEATHER_DANCE,
    MOVE_TEETER_DANCE,
    MOVE_BLAZE_KICK,
    MOVE_MUD_SPORT,
    MOVE_ICE_BALL,
    MOVE_NEEDLE_ARM,
    MOVE_SLACK_OFF,
    MOVE_HYPER_VOICE,
    MOVE_POISON_FANG,
    MOVE_CRUSH_CLAW,
    MOVE_BLAST_BURN,
    MOVE_HYDRO_CANNON,
    MOVE_METEOR_MASH,
    MOVE_ASTONISH,
    MOVE_WEATHER_BALL,
    MOVE_AROMATHERAPY,
    MOVE_FAKE_TEARS,
    MOVE_AIR_CUTTER,
    MOVE_OVERHEAT,
    MOVE_ODOR_SLEUTH,
    MOVE_ROCK_TOMB,
    MOVE_SILVER_WIND,
    MOVE_METAL_SOUND,
    MOVE_GRASS_WHISTLE,
    MOVE_TICKLE,
    MOVE_COSMIC_POWER,
    MOVE_WATER_SPOUT,
    MOVE_SIGNAL_BEAM,
    MOVE_SHADOW_PUNCH,
    MOVE_EXTRASENSORY,
    MOVE_SKY_UPPERCUT,
    MOVE_SAND_TOMB,
    MOVE_SHEER_COLD,
    MOVE_MUDDY_WATER,
    MOVE_BULLET_SEED,
    MOVE_AERIAL_ACE,
    MOVE_ICICLE_SPEAR,
    MOVE_IRON_DEFENSE,
    MOVE_BLOCK,
    MOVE_HOWL,
    MOVE_DRAGON_CLAW,
    MOVE_FRENZY_PLANT,
    MOVE_BULK_UP,
    MOVE_BOUNCE,
    MOVE_MUD_SHOT,
    MOVE_POISON_TAIL,
    MOVE_COVET,
    MOVE_VOLT_TACKLE,
    MOVE_MAGICAL_LEAF,
    MOVE_WATER_SPORT,
    MOVE_CALM_MIND,
    MOVE_LEAF_BLADE,
    MOVE_DRAGON_DANCE,
    MOVE_ROCK_BLAST,
    MOVE_SHOCK_WAVE,
    MOVE_WATER_PULSE,
    MOVE_DOOM_DESIRE,
    MOVE_PSYCHO_BOOST,
    #ifdef BATTLE_ENGINE
    // Gen 4 moves
    MOVE_ROOST,
    MOVE_GRAVITY,
    MOVE_MIRACLE_EYE,
    MOVE_WAKE_UP_SLAP,
    MOVE_HAMMER_ARM,
    MOVE_GYRO_BALL,
    MOVE_HEALING_WISH,
    MOVE_BRINE,
    MOVE_NATURAL_GIFT,
    MOVE_FEINT,
    MOVE_PLUCK,
    MOVE_TAILWIND,
    MOVE_ACUPRESSURE,
    MOVE_METAL_BURST,
    MOVE_U_TURN,
    MOVE_CLOSE_COMBAT,
    MOVE_PAYBACK,
    MOVE_ASSURANCE,
    MOVE_EMBARGO,
    MOVE_FLING,
    MOVE_PSYCHO_SHIFT,
    MOVE_TRUMP_CARD,
    MOVE_HEAL_BLOCK,
    MOVE_WRING_OUT,
    MOVE_POWER_TRICK,
    MOVE_GASTRO_ACID,
    MOVE_LUCKY_CHANT,
    MOVE_ME_FIRST,
    MOVE_COPYCAT,
    MOVE_POWER_SWAP,
    MOVE_GUARD_SWAP,
    MOVE_PUNISHMENT,
    MOVE_LAST_RESORT,
    MOVE_WORRY_SEED,
    MOVE_SUCKER_PUNCH,
    MOVE_TOXIC_SPIKES,
    MOVE_HEART_SWAP,
    MOVE_AQUA_RING,
    MOVE_MAGNET_RISE,
    MOVE_FLARE_BLITZ,
    MOVE_FORCE_PALM,
    MOVE_AURA_SPHERE,
    MOVE_ROCK_POLISH,
    MOVE_POISON_JAB,
    MOVE_DARK_PULSE,
    MOVE_NIGHT_SLASH,
    MOVE_AQUA_TAIL,
    MOVE_SEED_BOMB,
    MOVE_AIR_SLASH,
    MOVE_X_SCISSOR,
    MOVE_BUG_BUZZ,
    MOVE_DRAGON_PULSE,
    MOVE_DRAGON_RUSH,
    MOVE_POWER_GEM,
    MOVE_DRAIN_PUNCH,
    MOVE_VACUUM_WAVE,
    MOVE_FOCUS_BLAST,
    MOVE_ENERGY_BALL,
    MOVE_BRAVE_BIRD,
    MOVE_EARTH_POWER,
    MOVE_SWITCHEROO,
    MOVE_GIGA_IMPACT,
    MOVE_NASTY_PLOT,
    MOVE_BULLET_PUNCH,
    MOVE_AVALANCHE,
    MOVE_ICE_SHARD,
    MOVE_SHADOW_CLAW,
    MOVE_THUNDER_FANG,
    MOVE_ICE_FANG,
    MOVE_FIRE_FANG,
    MOVE_SHADOW_SNEAK,
    MOVE_MUD_BOMB,
    MOVE_PSYCHO_CUT,
    MOVE_ZEN_HEADBUTT,
    MOVE_MIRROR_SHOT,
    MOVE_FLASH_CANNON,
    MOVE_ROCK_CLIMB,
    MOVE_DEFOG,
    MOVE_TRICK_ROOM,
    MOVE_DRACO_METEOR,
    MOVE_DISCHARGE,
    MOVE_LAVA_PLUME,
    MOVE_LEAF_STORM,
    MOVE_POWER_WHIP,
    MOVE_ROCK_WRECKER,
    MOVE_CROSS_POISON,
    MOVE_GUNK_SHOT,
    MOVE_IRON_HEAD,
    MOVE_MAGNET_BOMB,
    MOVE_STONE_EDGE,
    MOVE_CAPTIVATE,
    MOVE_STEALTH_ROCK,
    MOVE_GRASS_KNOT,
    MOVE_CHATTER,
    MOVE_JUDGMENT,
    MOVE_BUG_BITE,
    MOVE_CHARGE_BEAM,
    MOVE_WOOD_HAMMER,
    MOVE_AQUA_JET,
    MOVE_ATTACK_ORDER,
    MOVE_DEFEND_ORDER,
    MOVE_HEAL_ORDER,
    MOVE_HEAD_SMASH,
    MOVE_DOUBLE_HIT,
    MOVE_ROAR_OF_TIME,
    MOVE_SPACIAL_REND,
    MOVE_LUNAR_DANCE,
    MOVE_CRUSH_GRIP,
    MOVE_MAGMA_STORM,
    MOVE_DARK_VOID,
    MOVE_SEED_FLARE,
    MOVE_OMINOUS_WIND,
    MOVE_SHADOW_FORCE,
    // Gen 5 moves
    MOVE_HONE_CLAWS,
    MOVE_WIDE_GUARD,
    MOVE_GUARD_SPLIT,
    MOVE_POWER_SPLIT,
    MOVE_WONDER_ROOM,
    MOVE_PSYSHOCK,
    MOVE_VENOSHOCK,
    MOVE_AUTOTOMIZE,
    MOVE_RAGE_POWDER,
    MOVE_TELEKINESIS,
    MOVE_MAGIC_ROOM,
    MOVE_SMACK_DOWN,
    MOVE_STORM_THROW,
    MOVE_FLAME_BURST,
    MOVE_SLUDGE_WAVE,
    MOVE_QUIVER_DANCE,
    MOVE_HEAVY_SLAM,
    MOVE_SYNCHRONOISE,
    MOVE_ELECTRO_BALL,
    MOVE_SOAK,
    MOVE_FLAME_CHARGE,
    MOVE_COIL,
    MOVE_LOW_SWEEP,
    MOVE_ACID_SPRAY,
    MOVE_FOUL_PLAY,
    MOVE_SIMPLE_BEAM,
    MOVE_ENTRAINMENT,
    MOVE_AFTER_YOU,
    MOVE_ROUND,
    MOVE_ECHOED_VOICE,
    MOVE_CHIP_AWAY,
    MOVE_CLEAR_SMOG,
    MOVE_STORED_POWER,
    MOVE_QUICK_GUARD,
    MOVE_ALLY_SWITCH,
    MOVE_SCALD,
    MOVE_SHELL_SMASH,
    MOVE_HEAL_PULSE,
    MOVE_HEX,
    MOVE_SKY_DROP,
    MOVE_SHIFT_GEAR,
    MOVE_CIRCLE_THROW,
    MOVE_INCINERATE,
    MOVE_QUASH,
    MOVE_ACROBATICS,
    MOVE_REFLECT_TYPE,
    MOVE_RETALIATE,
    MOVE_FINAL_GAMBIT,
    MOVE_BESTOW,
    MOVE_INFERNO,
    MOVE_WATER_PLEDGE,
    MOVE_FIRE_PLEDGE,
    MOVE_GRASS_PLEDGE,
    MOVE_VOLT_SWITCH,
    MOVE_STRUGGLE_BUG,
    MOVE_BULLDOZE,
    MOVE_FROST_BREATH,
    MOVE_DRAGON_TAIL,
    MOVE_WORK_UP,
    MOVE_ELECTROWEB,
    MOVE_WILD_CHARGE,
    MOVE_DRILL_RUN,
    MOVE_DUAL_CHOP,
    MOVE_HEART_STAMP,
    MOVE_HORN_LEECH,
    MOVE_SACRED_SWORD,
    MOVE_RAZOR_SHELL,
    MOVE_HEAT_CRASH,
    MOVE_LEAF_TORNADO,
    MOVE_STEAMROLLER,
    MOVE_COTTON_GUARD,
    MOVE_NIGHT_DAZE,
    MOVE_PSYSTRIKE,
    MOVE_TAIL_SLAP,
    MOVE_HURRICANE,
    MOVE_HEAD_CHARGE,
    MOVE_GEAR_GRIND,
    MOVE_SEARING_SHOT,
    MOVE_TECHNO_BLAST,
    MOVE_RELIC_SONG,
    MOVE_SECRET_SWORD,
    MOVE_GLACIATE,
    MOVE_BOLT_STRIKE,
    MOVE_BLUE_FLARE,
    MOVE_FIERY_DANCE,
    MOVE_FREEZE_SHOCK,
    MOVE_ICE_BURN,
    MOVE_SNARL,
    MOVE_ICICLE_CRASH,
    MOVE_V_CREATE,
    MOVE_FUSION_FLARE,
    MOVE_FUSION_BOLT,
    // Gen 6 moves
    MOVE_FLYING_PRESS,
    MOVE_MAT_BLOCK,
    MOVE_BELCH,
    MOVE_ROTOTILLER,
    MOVE_STICKY_WEB,
    MOVE_FELL_STINGER,
    MOVE_PHANTOM_FORCE,
    MOVE_TRICK_OR_TREAT,
    MOVE_NOBLE_ROAR,
    MOVE_ION_DELUGE,
    MOVE_PARABOLIC_CHARGE,
    MOVE_FORESTS_CURSE,
    MOVE_PETAL_BLIZZARD,
    MOVE_FREEZE_DRY,
    MOVE_DISARMING_VOICE,
    MOVE_PARTING_SHOT,
    MOVE_TOPSY_TURVY,
    MOVE_DRAINING_KISS,
    MOVE_CRAFTY_SHIELD,
    MOVE_FLOWER_SHIELD,
    MOVE_GRASSY_TERRAIN,
    MOVE_MISTY_TERRAIN,
    MOVE_ELECTRIFY,
    MOVE_PLAY_ROUGH,
    MOVE_FAIRY_WIND,
    MOVE_MOONBLAST,
    MOVE_BOOMBURST,
    MOVE_FAIRY_LOCK,
    MOVE_KINGS_SHIELD,
    MOVE_PLAY_NICE,
    MOVE_CONFIDE,
    MOVE_DIAMOND_STORM,
    MOVE_STEAM_ERUPTION,
    MOVE_HYPERSPACE_HOLE,
    MOVE_WATER_SHURIKEN,
    MOVE_MYSTICAL_FIRE,
    MOVE_SPIKY_SHIELD,
    MOVE_AROMATIC_MIST,
    MOVE_EERIE_IMPULSE,
    MOVE_VENOM_DRENCH,
    MOVE_POWDER,
    MOVE_GEOMANCY,
    MOVE_MAGNETIC_FLUX,
    MOVE_HAPPY_HOUR,
    MOVE_ELECTRIC_TERRAIN,
    MOVE_DAZZLING_GLEAM,
    MOVE_CELEBRATE,
    MOVE_HOLD_HANDS,
    MOVE_BABY_DOLL_EYES,
    MOVE_NUZZLE,
    MOVE_HOLD_BACK,
    MOVE_INFESTATION,
    MOVE_POWER_UP_PUNCH,
    MOVE_OBLIVION_WING,
    MOVE_THOUSAND_ARROWS,
    MOVE_THOUSAND_WAVES,
    MOVE_LANDS_WRATH,
    MOVE_LIGHT_OF_RUIN,
    // ORAS Moves
    MOVE_ORIGIN_PULSE,
    MOVE_PRECIPICE_BLADES,
    MOVE_DRAGON_ASCENT,
    //MOVE_HYPERSPACE_FURY,
    // Gen 7 moves
    MOVE_SHORE_UP,
    MOVE_FIRST_IMPRESSION,
    MOVE_BANEFUL_BUNKER,
    MOVE_SPIRIT_SHACKLE,
    MOVE_DARKEST_LARIAT,
    MOVE_SPARKLING_ARIA,
    MOVE_ICE_HAMMER,
    MOVE_FLORAL_HEALING,
    MOVE_HIGH_HORSEPOWER,
    MOVE_STRENGTH_SAP,
    MOVE_SOLAR_BLADE,
    MOVE_LEAFAGE,
    MOVE_SPOTLIGHT,
    MOVE_TOXIC_THREAD,
    MOVE_LASER_FOCUS,
    MOVE_GEAR_UP,
    MOVE_THROAT_CHOP,
    MOVE_POLLEN_PUFF,
    MOVE_ANCHOR_SHOT,
    MOVE_PSYCHIC_TERRAIN,
    MOVE_LUNGE,
    MOVE_FIRE_LASH,
    MOVE_POWER_TRIP,
    MOVE_BURN_UP,
    MOVE_SPEED_SWAP,
    MOVE_SMART_STRIKE,
    MOVE_PURIFY,
    MOVE_REVELATION_DANCE,
    MOVE_CORE_ENFORCER,
    MOVE_TROP_KICK,
    MOVE_INSTRUCT,
    MOVE_BEAK_BLAST,
    MOVE_CLANGING_SCALES,
    MOVE_DRAGON_HAMMER,
    MOVE_BRUTAL_SWING,
    MOVE_AURORA_VEIL,
    MOVE_SHELL_TRAP,
    MOVE_FLEUR_CANNON,
    MOVE_PSYCHIC_FANGS,
    MOVE_STOMPING_TANTRUM,
    MOVE_SHADOW_BONE,
    MOVE_ACCELEROCK,
    MOVE_LIQUIDATION,
    MOVE_PRISMATIC_LASER,
    MOVE_SPECTRAL_THIEF,
    MOVE_SUNSTEEL_STRIKE,
    MOVE_MOONGEIST_BEAM,
    MOVE_TEARFUL_LOOK,
    MOVE_ZING_ZAP,
    MOVE_NATURES_MADNESS,
    MOVE_MULTI_ATTACK,
    // USUM Moves
    MOVE_MIND_BLOWN,
    MOVE_PLASMA_FISTS,
    MOVE_PHOTON_GEYSER,
    // LGPE Moves
    MOVE_ZIPPY_ZAP,
    MOVE_SPLISHY_SPLASH,
    MOVE_FLOATY_FALL,
    MOVE_PIKA_PAPOW,
    MOVE_BOUNCY_BUBBLE,
    MOVE_BUZZY_BUZZ,
    MOVE_SIZZLY_SLIDE,
    MOVE_GLITZY_GLOW,
    MOVE_BADDY_BAD,
    MOVE_SAPPY_SEED,
    MOVE_FREEZY_FROST,
    MOVE_SPARKLY_SWIRL,
    MOVE_VEEVEE_VOLLEY,
    MOVE_DOUBLE_IRON_BASH,
    // Gen 8 moves
    MOVE_DYNAMAX_CANNON,
    MOVE_SNIPE_SHOT,
    MOVE_JAW_LOCK,
    MOVE_STUFF_CHEEKS,
    MOVE_NO_RETREAT,
    MOVE_TAR_SHOT,
    MOVE_MAGIC_POWDER,
    MOVE_DRAGON_DARTS,
    MOVE_TEATIME,
    MOVE_OCTOLOCK,
    MOVE_BOLT_BEAK,
    MOVE_FISHIOUS_REND,
    MOVE_COURT_CHANGE,
    MOVE_CLANGOROUS_SOUL,
    MOVE_BODY_PRESS,
    MOVE_DECORATE,
    MOVE_DRUM_BEATING,
    MOVE_SNAP_TRAP,
    MOVE_PYRO_BALL,
    MOVE_BEHEMOTH_BLADE,
    MOVE_BEHEMOTH_BASH,
    MOVE_AURA_WHEEL,
    MOVE_BREAKING_SWIPE,
    MOVE_BRANCH_POKE,
    MOVE_OVERDRIVE,
    MOVE_APPLE_ACID,
    MOVE_GRAV_APPLE,
    MOVE_SPIRIT_BREAK,
    MOVE_STRANGE_STEAM,
    MOVE_LIFE_DEW,
    MOVE_OBSTRUCT,
    MOVE_FALSE_SURRENDER,
    MOVE_METEOR_ASSAULT,
    MOVE_ETERNABEAM,
    MOVE_STEEL_BEAM,
    // Isle of Armor Moves
    MOVE_EXPANDING_FORCE,
    MOVE_STEEL_ROLLER,
    MOVE_SCALE_SHOT,
    MOVE_METEOR_BEAM,
    MOVE_SHELL_SIDE_ARM,
    MOVE_MISTY_EXPLOSION,
    MOVE_GRASSY_GLIDE,
    MOVE_RISING_VOLTAGE,
    MOVE_TERRAIN_PULSE,
    MOVE_SKITTER_SMACK,
    MOVE_BURNING_JEALOUSY,
    MOVE_LASH_OUT,
    MOVE_POLTERGEIST,
    MOVE_CORROSIVE_GAS,
    MOVE_COACHING,
    MOVE_FLIP_TURN,
    MOVE_TRIPLE_AXEL,
    MOVE_DUAL_WINGBEAT,
    MOVE_SCORCHING_SANDS,
    MOVE_JUNGLE_HEALING,
    MOVE_WICKED_BLOW,
    MOVE_SURGING_STRIKES,
    // Crown Tundra Moves
    MOVE_THUNDER_CAGE,
    MOVE_DRAGON_ENERGY,
    MOVE_FREEZING_GLARE,
    MOVE_FIERY_WRATH,
    MOVE_THUNDEROUS_KICK,
    MOVE_GLACIAL_LANCE,
    MOVE_ASTRAL_BARRAGE,
    MOVE_EERIE_SPELL,
    // Legends: Arceus Moves
    MOVE_DIRE_CLAW,
    MOVE_PSYSHIELD_BASH,
    MOVE_POWER_SHIFT,
    MOVE_STONE_AXE,
    MOVE_SPRINGTIDE_STORM,
    MOVE_MYSTICAL_POWER,
    MOVE_RAGING_FURY,
    MOVE_WAVE_CRASH,
    MOVE_CHLOROBLAST,
    MOVE_MOUNTAIN_GALE,
    MOVE_VICTORY_DANCE,
    MOVE_HEADLONG_RUSH,
    MOVE_BARB_BARRAGE,
    MOVE_ESPER_WING,
    MOVE_BITTER_MALICE,
    MOVE_SHELTER,
    MOVE_TRIPLE_ARROWS,
    MOVE_INFERNAL_PARADE,
    MOVE_CEASELESS_EDGE,
    MOVE_BLEAKWIND_STORM,
    MOVE_WILDBOLT_STORM,
    MOVE_SANDSEAR_STORM,
    MOVE_LUNAR_BLESSING,
    MOVE_TAKE_HEART,
    // Gen 9 moves.
    MOVE_TERA_BLAST,
    MOVE_SILK_TRAP,
    MOVE_AXE_KICK,
    MOVE_LAST_RESPECTS,
    MOVE_LUMINA_CRASH,
    MOVE_ORDER_UP,
    MOVE_JET_PUNCH,
    MOVE_SPICY_EXTRACT,
    MOVE_SPIN_OUT,
    MOVE_POPULATION_BOMB,
    MOVE_ICE_SPINNER,
    MOVE_GLAIVE_RUSH,
    MOVE_REVIVAL_BLESSING,
    MOVE_SALT_CURE,
    MOVE_TRIPLE_DIVE,
    MOVE_MORTAL_SPIN,
    MOVE_DOODLE,
    MOVE_FILLET_AWAY,
    MOVE_KOWTOW_CLEAVE,
    MOVE_FLOWER_TRICK,
    MOVE_TORCH_SONG,
    MOVE_AQUA_STEP,
    MOVE_RAGING_BULL,
    MOVE_MAKE_IT_RAIN,
    MOVE_RUINATION,
    MOVE_COLLISION_COURSE,
    MOVE_ELECTRO_DRIFT,
    MOVE_SHED_TAIL,
    MOVE_CHILLY_RECEPTION,
    MOVE_TIDY_UP,
    MOVE_SNOWSCAPE,
    MOVE_POUNCE,
    MOVE_TRAILBLAZE,
    MOVE_CHILLING_WATER,
    MOVE_HYPER_DRILL,
    MOVE_TWIN_BEAM,
    MOVE_RAGE_FIST,
    MOVE_ARMOR_CANNON,
    MOVE_BITTER_BLADE,
    MOVE_DOUBLE_SHOCK,
    MOVE_GIGATON_HAMMER,
    MOVE_COMEUPPANCE,
    MOVE_AQUA_CUTTER,
    MOVE_BLAZING_TORQUE,
    MOVE_WICKED_TORQUE,
    MOVE_NOXIOUS_TORQUE,
    MOVE_COMBAT_TORQUE,
    MOVE_MAGICAL_TORQUE,
    MOVE_PSYBLADE,
    MOVE_HYDRO_STEAM,
    // The Teal Mask Moves
    MOVE_BLOOD_MOON,
    MOVE_MATCHA_GOTCHA,
    MOVE_SYRUP_BOMB,
    MOVE_IVY_CUDGEL,
    // The Indigo Disk Moves
    MOVE_ELECTRO_SHOT,
    MOVE_TERA_STARSTORM,
    MOVE_FICKLE_BEAM,
    MOVE_BURNING_BULWARK,
    MOVE_THUNDERCLAP,
    MOVE_MIGHTY_CLEAVE,
    MOVE_TACHYON_CUTTER,
    MOVE_HARD_PRESS,
    MOVE_DRAGON_CHEER,
    MOVE_ALLURING_VOICE,
    MOVE_TEMPER_FLARE,
    MOVE_SUPERCELL_SLAM,
    MOVE_PSYCHIC_NOISE,
    MOVE_UPPER_HAND,
    MOVE_MALIGNANT_CHAIN,
    #endif
};
//**********************


// NOTE: Reordering this array will break compatibility with existing
// saves.
static const u32 sCompressedStatuses[] =
{
    STATUS1_NONE,
    STATUS1_SLEEP_TURN(1),
    STATUS1_SLEEP_TURN(2),
    STATUS1_SLEEP_TURN(3),
    STATUS1_SLEEP_TURN(4),
    STATUS1_SLEEP_TURN(5),
    STATUS1_POISON,
    STATUS1_BURN,
    STATUS1_FREEZE,
    STATUS1_PARALYSIS,
    STATUS1_TOXIC_POISON,
    STATUS1_FROSTBITE,
};

// Attempt to detect situations where the BoxPokemon struct is unable to
// contain all the values.
// TODO: Is it possible to compute:
// - The maximum experience.
// - The maximum PP.
// - The maximum HP.
// - The maximum form countdown.

// The following STATIC_ASSERT will prevent developers from compiling the game if the value of the constant on the left does not fit within the number of bits defined in PokemonSubstruct0 (currently located in include/pokemon.h).

// To successfully compile, developers will need to do one of the following:
// 1) Decrease the size of the constant.
// 2) Increase the number of bits both on the struct AND in the corresponding assert. This will likely break user's saves unless there is free space after the member that is being adjsted.
// 3) Repurpose unused IDs.

// EXAMPLES
// If a developer has added enough new items so that ITEMS_COUNT now equals 1200, they could...
// 1) remove new items until ITEMS_COUNT is 1023, the max value that will fit in 10 bits.
// 2) change heldItem:10 to heldItem:11 AND change the below assert for ITEMS_COUNT to check for (1 << 11).
// 3) repurpose IDs from other items that aren't being used, like ITEM_GOLD_TEETH or ITEM_SS_TICKET until ITEMS_COUNT equals 1023, the max value that will fit in 10 bits.

STATIC_ASSERT(NUM_SPECIES < (1 << 11), PokemonSubstruct0_species_TooSmall);
STATIC_ASSERT(NUMBER_OF_MON_TYPES + 1 <= (1 << 5), PokemonSubstruct0_teraType_TooSmall);
STATIC_ASSERT(ITEMS_COUNT < (1 << 10), PokemonSubstruct0_heldItem_TooSmall);
STATIC_ASSERT(MAX_LEVEL <= 100, PokemonSubstruct0_experience_PotentiallTooSmall); // Maximum of ~2 million exp.
STATIC_ASSERT(POKEBALL_COUNT <= (1 << 6), PokemonSubstruct0_pokeball_TooSmall);
STATIC_ASSERT(MOVES_COUNT_ALL < (1 << 11), PokemonSubstruct1_moves_TooSmall);
STATIC_ASSERT(ARRAY_COUNT(sCompressedStatuses) <= (1 << 4), PokemonSubstruct3_compressedStatus_TooSmall);
STATIC_ASSERT(MAX_LEVEL < (1 << 7), PokemonSubstruct3_metLevel_TooSmall);
STATIC_ASSERT(NUM_VERSIONS < (1 << 4), PokemonSubstruct3_metGame_TooSmall);
STATIC_ASSERT(MAX_DYNAMAX_LEVEL < (1 << 4), PokemonSubstruct3_dynamaxLevel_TooSmall);
STATIC_ASSERT(MAX_PER_STAT_IVS < (1 << 5), PokemonSubstruct3_ivs_TooSmall);
STATIC_ASSERT(NUM_NATURES <= (1 << 5), BoxPokemon_hiddenNatureModifier_TooSmall);

static u32 CompressStatus(u32 status)
{
    s32 i;
    for (i = 0; i < ARRAY_COUNT(sCompressedStatuses); i++)
    {
        if (sCompressedStatuses[i] == status)
            return i;
    }
    return 0; // STATUS1_NONE
}

static u32 UncompressStatus(u32 compressedStatus)
{
    if (compressedStatus < ARRAY_COUNT(sCompressedStatuses))
        return sCompressedStatuses[compressedStatus];
    else
        return STATUS1_NONE;
}

void ZeroBoxMonData(struct BoxPokemon *boxMon)
{
    u8 *raw = (u8 *)boxMon;
    u32 i;
    for (i = 0; i < sizeof(struct BoxPokemon); i++)
        raw[i] = 0;
}

void ZeroMonData(struct Pokemon *mon)
{
    u32 arg;
    ZeroBoxMonData(&mon->box);
    arg = 0;
    SetMonData(mon, MON_DATA_STATUS, &arg);
    SetMonData(mon, MON_DATA_LEVEL, &arg);
    SetMonData(mon, MON_DATA_HP, &arg);
    SetMonData(mon, MON_DATA_MAX_HP, &arg);
    SetMonData(mon, MON_DATA_ATK, &arg);
    SetMonData(mon, MON_DATA_DEF, &arg);
    SetMonData(mon, MON_DATA_SPEED, &arg);
    SetMonData(mon, MON_DATA_SPATK, &arg);
    SetMonData(mon, MON_DATA_SPDEF, &arg);
    arg = MAIL_NONE;
    SetMonData(mon, MON_DATA_MAIL, &arg);
}

void ZeroPlayerPartyMons(void)
{
    s32 i;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gPlayerParty[i]);
}

void ZeroEnemyPartyMons(void)
{
    s32 i;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gEnemyParty[i]);
}

void CreateMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    u32 mail;
    ZeroMonData(mon);
    CreateBoxMon(&mon->box, species, level, fixedIV, hasFixedPersonality, fixedPersonality, otIdType, fixedOtId);
    SetMonData(mon, MON_DATA_LEVEL, &level);
    mail = MAIL_NONE;
    SetMonData(mon, MON_DATA_MAIL, &mail);
    CalculateMonStats(mon);
}

void CreateBoxMon(struct BoxPokemon *boxMon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    u8 speciesName[POKEMON_NAME_LENGTH + 1];
    u32 personality = Random32();
    u32 value;
    u16 checksum;
    u8 i;
    enum Stat availableIVs[NUM_STATS];
    enum Stat selectedIvs[NUM_STATS];
    bool32 isShiny;

    ZeroBoxMonData(boxMon);

    // Determine original trainer ID
    if (otIdType == OT_ID_RANDOM_NO_SHINY)
    {
        value = Random32();
        isShiny = FALSE;
    }
    else if (otIdType == OT_ID_PRESET)
    {
        value = fixedOtId;
        isShiny = GET_SHINY_VALUE(value, hasFixedPersonality ? fixedPersonality : personality) < SHINY_ODDS;
    }
    else // Player is the OT
    {
        value = gSaveBlock2Ptr->playerTrainerId[0]
              | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
              | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
              | (gSaveBlock2Ptr->playerTrainerId[3] << 24);

        if (P_FLAG_FORCE_NO_SHINY != 0 && FlagGet(P_FLAG_FORCE_NO_SHINY))
        {
            isShiny = FALSE;
        }
        else if (P_FLAG_FORCE_SHINY != 0 && FlagGet(P_FLAG_FORCE_SHINY))
        {
            isShiny = TRUE;
        }
        else if (P_ONLY_OBTAINABLE_SHINIES && (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || (B_FLAG_NO_CATCHING != 0 && FlagGet(B_FLAG_NO_CATCHING))))
        {
            isShiny = FALSE;
        }
        else if (P_NO_SHINIES_WITHOUT_POKEBALLS && !HasAtLeastOnePokeBall())
        {
            isShiny = FALSE;
        }
        else
        {
            u32 totalRerolls = 0;
            if (CheckBagHasItem(ITEM_SHINY_CHARM, 1))
                totalRerolls += I_SHINY_CHARM_ADDITIONAL_ROLLS;
            if (LURE_STEP_COUNT != 0)
                totalRerolls += 1;
            totalRerolls += CalculateChainFishingShinyRolls();
            if (gDexNavSpecies)
                totalRerolls += CalculateDexNavShinyRolls();

            while (GET_SHINY_VALUE(value, personality) >= SHINY_ODDS && totalRerolls > 0)
            {
                personality = Random32();
                totalRerolls--;
            }

            isShiny = GET_SHINY_VALUE(value, personality) < SHINY_ODDS;
        }
    }

    if (hasFixedPersonality)
        personality = fixedPersonality;

    SetBoxMonData(boxMon, MON_DATA_PERSONALITY, &personality);
    SetBoxMonData(boxMon, MON_DATA_OT_ID, &value);

    checksum = CalculateBoxMonChecksum(boxMon);
    SetBoxMonData(boxMon, MON_DATA_CHECKSUM, &checksum);
    EncryptBoxMon(boxMon);
    SetBoxMonData(boxMon, MON_DATA_IS_SHINY, &isShiny);
    StringCopy(speciesName, GetSpeciesName(species));
    SetBoxMonData(boxMon, MON_DATA_NICKNAME, speciesName);
    SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &gGameLanguage);
    SetBoxMonData(boxMon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetBoxMonData(boxMon, MON_DATA_SPECIES, &species);
    SetBoxMonData(boxMon, MON_DATA_EXP, &gExperienceTables[gSpeciesInfo[species].growthRate][level]);
    SetBoxMonData(boxMon, MON_DATA_FRIENDSHIP, &gSpeciesInfo[species].friendship);
    value = GetCurrentRegionMapSectionId();
    SetBoxMonData(boxMon, MON_DATA_MET_LOCATION, &value);
    SetBoxMonData(boxMon, MON_DATA_MET_LEVEL, &level);
    SetBoxMonData(boxMon, MON_DATA_MET_GAME, &gGameVersion);
    value = ITEM_POKE_BALL;
    SetBoxMonData(boxMon, MON_DATA_POKEBALL, &value);
    SetBoxMonData(boxMon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);

    enum Type teraType = (boxMon->personality & 0x1) == 0 ? GetSpeciesType(species, 0) : GetSpeciesType(species, 1);
    SetBoxMonData(boxMon, MON_DATA_TERA_TYPE, &teraType);

    if (fixedIV < USE_RANDOM_IVS)
    {
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &fixedIV);
    }
    else
    {
        u32 iv;
        u32 ivRandom = Random32();
        value = (u16)ivRandom;

        iv = value & MAX_IV_MASK;
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &iv);
        iv = (value & (MAX_IV_MASK << 5)) >> 5;
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &iv);
        iv = (value & (MAX_IV_MASK << 10)) >> 10;
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &iv);

        value = (u16)(ivRandom >> 16);

        iv = value & MAX_IV_MASK;
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &iv);
        iv = (value & (MAX_IV_MASK << 5)) >> 5;
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &iv);
        iv = (value & (MAX_IV_MASK << 10)) >> 10;
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &iv);

        if (gSpeciesInfo[species].perfectIVCount != 0)
        {
            iv = MAX_PER_STAT_IVS;
            // Initialize a list of IV indices.
            for (i = 0; i < NUM_STATS; i++)
            {
                availableIVs[i] = i;
            }

            // Select the IVs that will be perfected.
            for (i = 0; i < NUM_STATS && i < gSpeciesInfo[species].perfectIVCount; i++)
            {
                u8 index = Random() % (NUM_STATS - i);
                selectedIvs[i] = availableIVs[index];
                RemoveIVIndexFromList(availableIVs, index);
            }
            for (i = 0; i < NUM_STATS && i < gSpeciesInfo[species].perfectIVCount; i++)
            {
                switch (selectedIvs[i])
                {
                case STAT_HP:
                    SetBoxMonData(boxMon, MON_DATA_HP_IV, &iv);
                    break;
                case STAT_ATK:
                    SetBoxMonData(boxMon, MON_DATA_ATK_IV, &iv);
                    break;
                case STAT_DEF:
                    SetBoxMonData(boxMon, MON_DATA_DEF_IV, &iv);
                    break;
                case STAT_SPEED:
                    SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &iv);
                    break;
                case STAT_SPATK:
                    SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &iv);
                    break;
                case STAT_SPDEF:
                    SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &iv);
                    break;
                default:
                    break;
                }
            }
        }
    }

    if (GetSpeciesAbility(species, 1))
    {
        value = personality & 1;
        SetBoxMonData(boxMon, MON_DATA_ABILITY_NUM, &value);
    }

    GiveBoxMonInitialMoveset(boxMon);
}

void CreateMonWithNature(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 nature)
{
    u32 personality;

    do
    {
        personality = Random32();
    }
    while (nature != GetNatureFromPersonality(personality));

    CreateMon(mon, species, level, fixedIV, TRUE, personality, OT_ID_PLAYER_ID, 0);
}

void CreateMonWithGenderNatureLetter(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 gender, u8 nature, u8 unownLetter)
{
    u32 personality;

    if ((u8)(unownLetter - 1) < NUM_UNOWN_FORMS)
    {
        u16 actualLetter;

        do
        {
            personality = Random32();
            actualLetter = GET_UNOWN_LETTER(personality);
        }
        while (nature != GetNatureFromPersonality(personality)
            || gender != GetGenderFromSpeciesAndPersonality(species, personality)
            || actualLetter != unownLetter - 1);
    }
    else
    {
        do
        {
            personality = Random32();
        }
        while (nature != GetNatureFromPersonality(personality)
            || gender != GetGenderFromSpeciesAndPersonality(species, personality));
    }

    CreateMon(mon, species, level, fixedIV, TRUE, personality, OT_ID_PLAYER_ID, 0);
}

// This is only used to create Wally's Ralts.
void CreateMaleMon(struct Pokemon *mon, u16 species, u8 level)
{
    u32 otId;
    otId = Random32();
    CreateMon(mon, species, level, USE_RANDOM_IVS, TRUE, otId, OT_ID_PRESET, otId);
}

void CreateMonWithIVsPersonality(struct Pokemon *mon, u16 species, u8 level, u32 ivs, u32 personality)
{
    CreateMon(mon, species, level, 0, TRUE, personality, OT_ID_PLAYER_ID, 0);
    SetMonData(mon, MON_DATA_IVS, &ivs);
    CalculateMonStats(mon);
}

void CreateMonWithIVsOTID(struct Pokemon *mon, u16 species, u8 level, u8 *ivs, u32 otId)
{
    CreateMon(mon, species, level, 0, FALSE, 0, OT_ID_PRESET, otId);
    SetMonData(mon, MON_DATA_HP_IV, &ivs[STAT_HP]);
    SetMonData(mon, MON_DATA_ATK_IV, &ivs[STAT_ATK]);
    SetMonData(mon, MON_DATA_DEF_IV, &ivs[STAT_DEF]);
    SetMonData(mon, MON_DATA_SPEED_IV, &ivs[STAT_SPEED]);
    SetMonData(mon, MON_DATA_SPATK_IV, &ivs[STAT_SPATK]);
    SetMonData(mon, MON_DATA_SPDEF_IV, &ivs[STAT_SPDEF]);
    CalculateMonStats(mon);
}

void CreateMonWithEVSpread(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 evSpread)
{
    s32 i;
    s32 statCount = 0;
    u16 evAmount;
    u8 evsBits;

    CreateMon(mon, species, level, fixedIV, FALSE, 0, OT_ID_PLAYER_ID, 0);

    evsBits = evSpread;

    for (i = 0; i < NUM_STATS; i++)
    {
        if (evsBits & 1)
            statCount++;
        evsBits >>= 1;
    }

    evAmount = MAX_TOTAL_EVS / statCount;

    evsBits = 1;

    for (i = 0; i < NUM_STATS; i++)
    {
        if (evSpread & evsBits)
            SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);
        evsBits <<= 1;
    }

    CalculateMonStats(mon);
}

void CreateBattleTowerMon(struct Pokemon *mon, struct BattleTowerPokemon *src)
{
    s32 i;
    u8 nickname[max(32, POKEMON_NAME_BUFFER_SIZE)];
    u8 language;
    u8 value;

    CreateMon(mon, src->species, src->level, 0, TRUE, src->personality, OT_ID_PRESET, src->otId);

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, src->moves[i], i);

    SetMonData(mon, MON_DATA_PP_BONUSES, &src->ppBonuses);
    SetMonData(mon, MON_DATA_HELD_ITEM, &src->heldItem);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &src->friendship);

    StringCopy(nickname, src->nickname);

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN)
    {
        language = LANGUAGE_JAPANESE;
        StripExtCtrlCodes(nickname);
    }
    else
    {
        language = GAME_LANGUAGE;
    }

    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_NICKNAME, nickname);
    SetMonData(mon, MON_DATA_HP_EV, &src->hpEV);
    SetMonData(mon, MON_DATA_ATK_EV, &src->attackEV);
    SetMonData(mon, MON_DATA_DEF_EV, &src->defenseEV);
    SetMonData(mon, MON_DATA_SPEED_EV, &src->speedEV);
    SetMonData(mon, MON_DATA_SPATK_EV, &src->spAttackEV);
    SetMonData(mon, MON_DATA_SPDEF_EV, &src->spDefenseEV);
    value = src->abilityNum;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = src->hpIV;
    SetMonData(mon, MON_DATA_HP_IV, &value);
    value = src->attackIV;
    SetMonData(mon, MON_DATA_ATK_IV, &value);
    value = src->defenseIV;
    SetMonData(mon, MON_DATA_DEF_IV, &value);
    value = src->speedIV;
    SetMonData(mon, MON_DATA_SPEED_IV, &value);
    value = src->spAttackIV;
    SetMonData(mon, MON_DATA_SPATK_IV, &value);
    value = src->spDefenseIV;
    SetMonData(mon, MON_DATA_SPDEF_IV, &value);
    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void CreateBattleTowerMon_HandleLevel(struct Pokemon *mon, struct BattleTowerPokemon *src, bool8 lvl50)
{
    s32 i;
    u8 nickname[max(32, POKEMON_NAME_BUFFER_SIZE)];
    u8 level;
    u8 language;
    u8 value;

    if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_50)
        level = GetFrontierEnemyMonLevel(gSaveBlock2Ptr->frontier.lvlMode);
    else if (lvl50)
        level = FRONTIER_MAX_LEVEL_50;
    else
        level = src->level;

    CreateMon(mon, src->species, level, 0, TRUE, src->personality, OT_ID_PRESET, src->otId);

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, src->moves[i], i);

    SetMonData(mon, MON_DATA_PP_BONUSES, &src->ppBonuses);
    SetMonData(mon, MON_DATA_HELD_ITEM, &src->heldItem);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &src->friendship);

    StringCopy(nickname, src->nickname);

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN)
    {
        language = LANGUAGE_JAPANESE;
        StripExtCtrlCodes(nickname);
    }
    else
    {
        language = GAME_LANGUAGE;
    }

    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_NICKNAME, nickname);
    SetMonData(mon, MON_DATA_HP_EV, &src->hpEV);
    SetMonData(mon, MON_DATA_ATK_EV, &src->attackEV);
    SetMonData(mon, MON_DATA_DEF_EV, &src->defenseEV);
    SetMonData(mon, MON_DATA_SPEED_EV, &src->speedEV);
    SetMonData(mon, MON_DATA_SPATK_EV, &src->spAttackEV);
    SetMonData(mon, MON_DATA_SPDEF_EV, &src->spDefenseEV);
    value = src->abilityNum;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = src->hpIV;
    SetMonData(mon, MON_DATA_HP_IV, &value);
    value = src->attackIV;
    SetMonData(mon, MON_DATA_ATK_IV, &value);
    value = src->defenseIV;
    SetMonData(mon, MON_DATA_DEF_IV, &value);
    value = src->speedIV;
    SetMonData(mon, MON_DATA_SPEED_IV, &value);
    value = src->spAttackIV;
    SetMonData(mon, MON_DATA_SPATK_IV, &value);
    value = src->spDefenseIV;
    SetMonData(mon, MON_DATA_SPDEF_IV, &value);
    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void CreateApprenticeMon(struct Pokemon *mon, const struct Apprentice *src, u8 monId)
{
    s32 i;
    u16 evAmount;
    u8 language;
    u32 otId = gApprentices[src->id].otId;
    u32 personality = ((gApprentices[src->id].otId >> 8) | ((gApprentices[src->id].otId & 0xFF) << 8))
                    + src->party[monId].species + src->number;

    CreateMon(mon,
              src->party[monId].species,
              GetFrontierEnemyMonLevel(src->lvlMode - 1),
              MAX_PER_STAT_IVS,
              TRUE,
              personality,
              OT_ID_PRESET,
              otId);

    SetMonData(mon, MON_DATA_HELD_ITEM, &src->party[monId].item);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, src->party[monId].moves[i], i);

    evAmount = MAX_TOTAL_EVS / NUM_STATS;
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);

    language = src->language;
    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_OT_NAME, GetApprenticeNameInLanguage(src->id, language));
    CalculateMonStats(mon);
}

void CreateMonWithEVSpreadNatureOTID(struct Pokemon *mon, u16 species, u8 level, u8 nature, u8 fixedIV, u8 evSpread, u32 otId)
{
    s32 i;
    s32 statCount = 0;
    u8 evsBits;
    u16 evAmount;

    // i is reused as personality value
    do
    {
        i = Random32();
    } while (nature != GetNatureFromPersonality(i));

    CreateMon(mon, species, level, fixedIV, TRUE, i, OT_ID_PRESET, otId);
    evsBits = evSpread;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (evsBits & 1)
            statCount++;
        evsBits >>= 1;
    }

    evAmount = MAX_TOTAL_EVS / statCount;
    evsBits = 1;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (evSpread & evsBits)
            SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);
        evsBits <<= 1;
    }

    CalculateMonStats(mon);
}

void ConvertPokemonToBattleTowerPokemon(struct Pokemon *mon, struct BattleTowerPokemon *dest)
{
    s32 i;
    u16 heldItem;

    dest->species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);

    if (heldItem == ITEM_ENIGMA_BERRY_E_READER)
        heldItem = ITEM_NONE;

    dest->heldItem = heldItem;

    for (i = 0; i < MAX_MON_MOVES; i++)
        dest->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);

    dest->level = GetMonData(mon, MON_DATA_LEVEL, NULL);
    dest->ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    dest->otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
    dest->hpEV = GetMonData(mon, MON_DATA_HP_EV, NULL);
    dest->attackEV = GetMonData(mon, MON_DATA_ATK_EV, NULL);
    dest->defenseEV = GetMonData(mon, MON_DATA_DEF_EV, NULL);
    dest->speedEV = GetMonData(mon, MON_DATA_SPEED_EV, NULL);
    dest->spAttackEV = GetMonData(mon, MON_DATA_SPATK_EV, NULL);
    dest->spDefenseEV = GetMonData(mon, MON_DATA_SPDEF_EV, NULL);
    dest->friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
    dest->hpIV = GetMonData(mon, MON_DATA_HP_IV, NULL);
    dest->attackIV = GetMonData(mon, MON_DATA_ATK_IV, NULL);
    dest->defenseIV = GetMonData(mon, MON_DATA_DEF_IV, NULL);
    dest->speedIV  = GetMonData(mon, MON_DATA_SPEED_IV, NULL);
    dest->spAttackIV  = GetMonData(mon, MON_DATA_SPATK_IV, NULL);
    dest->spDefenseIV  = GetMonData(mon, MON_DATA_SPDEF_IV, NULL);
    dest->abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);
    dest->personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    GetMonData(mon, MON_DATA_NICKNAME10, dest->nickname);
}

static void CreateEventMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    bool32 isModernFatefulEncounter = TRUE;

    CreateMon(mon, species, level, fixedIV, hasFixedPersonality, fixedPersonality, otIdType, fixedOtId);
    SetMonData(mon, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &isModernFatefulEncounter);
}

u16 GetUnionRoomTrainerPic(void)
{
    u8 linkId;
    u32 arrId;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        linkId = gRecordedBattleMultiplayerId ^ 1;
    else
        linkId = GetMultiplayerId() ^ 1;

    arrId = gLinkPlayers[linkId].trainerId % NUM_UNION_ROOM_CLASSES;
    arrId |= gLinkPlayers[linkId].gender * NUM_UNION_ROOM_CLASSES;
    return FacilityClassToPicIndex(gUnionRoomFacilityClasses[arrId]);
}

enum TrainerClassID GetUnionRoomTrainerClass(void)
{
    u8 linkId;
    u32 arrId;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        linkId = gRecordedBattleMultiplayerId ^ 1;
    else
        linkId = GetMultiplayerId() ^ 1;

    arrId = gLinkPlayers[linkId].trainerId % NUM_UNION_ROOM_CLASSES;
    arrId |= gLinkPlayers[linkId].gender * NUM_UNION_ROOM_CLASSES;
    return gFacilityClassToTrainerClass[gUnionRoomFacilityClasses[arrId]];
}

void CreateEnemyEventMon(void)
{
    s32 species = gSpecialVar_0x8004;
    s32 level = gSpecialVar_0x8005;
    s32 itemId = gSpecialVar_0x8006;

    ZeroEnemyPartyMons();
    CreateEventMon(&gEnemyParty[0], species, level, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    if (itemId)
    {
        u8 heldItem[2];
        heldItem[0] = itemId;
        heldItem[1] = itemId >> 8;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, heldItem);
    }
}

static u16 CalculateBoxMonChecksum(struct BoxPokemon *boxMon)
{
    u32 checksum = 0;

    for (u32 i = 0; i < ARRAY_COUNT(boxMon->secure.raw); i++)
        checksum += boxMon->secure.raw[i] + (boxMon->secure.raw[i] >> 16);

    return checksum;
}

static u16 CalculateBoxMonChecksumDecrypt(struct BoxPokemon *boxMon)
{
    u32 checksum = 0;

    for (u32 i = 0; i < ARRAY_COUNT(boxMon->secure.raw); i++)
    {
        boxMon->secure.raw[i] ^= (boxMon->otId ^ boxMon->personality);
        checksum += boxMon->secure.raw[i] + (boxMon->secure.raw[i] >> 16);
    }

    return checksum;
}

static u16 CalculateBoxMonChecksumReencrypt(struct BoxPokemon *boxMon)
{
    u32 checksum = 0;

    for (u32 i = 0; i < ARRAY_COUNT(boxMon->secure.raw); i++)
    {
        checksum += boxMon->secure.raw[i] + (boxMon->secure.raw[i] >> 16);
        boxMon->secure.raw[i] ^= (boxMon->otId ^ boxMon->personality);
    }

    return checksum;
}

void CalculateMonStats(struct Pokemon *mon)
{
    s32 oldMaxHP = GetMonData(mon, MON_DATA_MAX_HP, NULL);
    s32 currentHP = GetMonData(mon, MON_DATA_HP, NULL);
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
    s32 level = GetLevelFromMonExp(mon);
    s32 newMaxHP;

    u8 nature = GetMonData(mon, MON_DATA_HIDDEN_NATURE, NULL);

    SetMonData(mon, MON_DATA_LEVEL, &level);

    bool32 hyperTrained[NUM_STATS]; //In a battle test, hyper training flag indicates a fixed stat
    s32 iv[NUM_STATS];
    s32 ev[NUM_STATS];
    for (u32 i = 0; i < NUM_STATS; i++)
    {
        hyperTrained[i] = GetMonData(mon, MON_DATA_HYPER_TRAINED_HP + i);
        iv[i] = GetMonData(mon, MON_DATA_HP_IV + i);
        ev[i] = GetMonData(mon, MON_DATA_HP_EV + i);

        if (hyperTrained[i])
        {
        #if TESTING
            if (gMain.inBattle)
                continue;
        #endif
            iv[i] = MAX_PER_STAT_IVS;
        }

        if (i == STAT_HP)
            continue;

        u8 baseStat = GetSpeciesBaseStat(species, i);
        s32 n = (((2 * baseStat + iv[i] + ev[i] / 4) * level) / 100) + 5;
        n = ModifyStatByNature(nature, n, i);
        if (B_FRIENDSHIP_BOOST == TRUE)
            n = n + ((n * 10 * friendship) / (MAX_FRIENDSHIP * 100));
        SetMonData(mon, MON_DATA_MAX_HP + i, &n);
    }

#if TESTING
    if (hyperTrained[STAT_HP] && gMain.inBattle)
        return;
#endif

    if (species == SPECIES_SHEDINJA)
    {
        newMaxHP = 1;
    }
    else
    {
        s32 n = 2 * GetSpeciesBaseHP(species) + iv[STAT_HP];
        newMaxHP = (((n + ev[STAT_HP] / 4) * level) / 100) + level + 10;
    }

    gBattleScripting.levelUpHP = newMaxHP - oldMaxHP;
    if (gBattleScripting.levelUpHP == 0)
        gBattleScripting.levelUpHP = 1;
    SetMonData(mon, MON_DATA_MAX_HP, &newMaxHP);

    // Since a pokemon's maxHP data could either not have
    // been initialized at this point or this pokemon is
    // just fainted, the check for oldMaxHP is important.
    if (currentHP == 0 && oldMaxHP != 0)
        return;

    // Only add to currentHP if newMaxHP went up.
    if (newMaxHP > oldMaxHP)
        currentHP += newMaxHP - oldMaxHP;

    // Ensure currentHP does not surpass newMaxHP.
    if (currentHP > newMaxHP)
        currentHP = newMaxHP;

    SetMonData(mon, MON_DATA_HP, &currentHP);
}

void BoxMonToMon(const struct BoxPokemon *src, struct Pokemon *dest)
{
    u32 value = 0;
    dest->box = *src;
    dest->status = GetBoxMonData(&dest->box, MON_DATA_STATUS, NULL);
    dest->hp = 0;
    dest->maxHP = 0;
    value = MAIL_NONE;
    SetMonData(dest, MON_DATA_MAIL, &value);
    value = GetBoxMonData(&dest->box, MON_DATA_HP_LOST);
    CalculateMonStats(dest);
    value = GetMonData(dest, MON_DATA_MAX_HP) - value;
    SetMonData(dest, MON_DATA_HP, &value);
}

u8 GetLevelFromMonExp(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u32 exp = GetMonData(mon, MON_DATA_EXP, NULL);
    s32 level = 1;

    while (level <= MAX_LEVEL && gExperienceTables[gSpeciesInfo[species].growthRate][level] <= exp)
        level++;

    return level - 1;
}

u8 GetLevelFromBoxMonExp(struct BoxPokemon *boxMon)
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 exp = GetBoxMonData(boxMon, MON_DATA_EXP, NULL);
    s32 level = 1;

    while (level <= MAX_LEVEL && gExperienceTables[gSpeciesInfo[species].growthRate][level] <= exp)
        level++;

    return level - 1;
}

u16 GiveMoveToMon(struct Pokemon *mon, u16 move)
{
    return GiveMoveToBoxMon(&mon->box, move);
}

u16 GiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move)
{
    s32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 existingMove = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, NULL);
        if (existingMove == MOVE_NONE)
        {
            u32 pp = GetMovePP(move);
            SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &move);
            SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp);
            return move;
        }
        if (existingMove == move)
            return MON_ALREADY_KNOWS_MOVE;
    }
    return MON_HAS_MAX_MOVES;
}

u16 GiveMoveToBattleMon(struct BattlePokemon *mon, u16 move)
{
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (mon->moves[i] == MOVE_NONE)
        {
            mon->moves[i] = move;
            mon->pp[i] = GetMovePP(move);
            return move;
        }
    }

    return MON_HAS_MAX_MOVES;
}

void SetMonMoveSlot(struct Pokemon *mon, u16 move, u8 slot)
{
    SetMonData(mon, MON_DATA_MOVE1 + slot, &move);
    u32 pp = GetMovePP(move);
    SetMonData(mon, MON_DATA_PP1 + slot, &pp);
}

static void SetMonMoveSlot_KeepPP(struct Pokemon *mon, u16 move, u8 slot)
{
    u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    u8 currPP = GetMonData(mon, MON_DATA_PP1 + slot, NULL);
    u8 newPP = CalculatePPWithBonus(move, ppBonuses, slot);
    u16 finalPP = min(currPP, newPP);

    SetMonData(mon, MON_DATA_MOVE1 + slot, &move);
    SetMonData(mon, MON_DATA_PP1 + slot, &finalPP);
}

void SetBattleMonMoveSlot(struct BattlePokemon *mon, u16 move, u8 slot)
{
    mon->moves[slot] = move;
    mon->pp[slot] = GetMovePP(move);
}

void GiveMonInitialMoveset(struct Pokemon *mon)
{
    GiveBoxMonInitialMoveset(&mon->box);
}

void GiveBoxMonInitialMoveset(struct BoxPokemon *boxMon) //Credit: AsparagusEduardo
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    s32 level = GetLevelFromBoxMonExp(boxMon);
    s32 i;
    u16 moves[MAX_MON_MOVES] = {MOVE_NONE};
    u8 addedMoves = 0;
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

    for (i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
    {
        s32 j;
        bool32 alreadyKnown = FALSE;

        if (learnset[i].level > level)
            break;
        if (learnset[i].level == 0)
            continue;

        for (j = 0; j < addedMoves; j++)
        {
            if (moves[j] == learnset[i].move)
            {
                alreadyKnown = TRUE;
                break;
            }
        }

        if (!alreadyKnown)
        {
            if (addedMoves < MAX_MON_MOVES)
            {
                moves[addedMoves] = learnset[i].move;
                addedMoves++;
            }
            else
            {
                for (j = 0; j < MAX_MON_MOVES - 1; j++)
                    moves[j] = moves[j + 1];
                moves[MAX_MON_MOVES - 1] = learnset[i].move;
            }
        }
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &moves[i]);
        u32 pp = GetMovePP(moves[i]);
        SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp);
    }
}

u16 MonTryLearningNewMoveAtLevel(struct Pokemon *mon, bool32 firstMove, u32 level)
{
    u32 retVal = MOVE_NONE;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

    // since you can learn more than one move per level
    // the game needs to know whether you decided to
    // learn it or keep the old set to avoid asking
    // you to learn the same move over and over again
    if (firstMove)
    {
        sLearningMoveTableID = 0;

        while (learnset[sLearningMoveTableID].level != level)
        {
            sLearningMoveTableID++;
            if (learnset[sLearningMoveTableID].move == LEVEL_UP_MOVE_END)
                return MOVE_NONE;
        }
    }

    //  Handler for Pokémon whose moves change upon form change.
    //  For example, if Zacian or Zamazenta should learn Iron Head,
    //  they're prevented from doing if they have Behemoth Blade/Bash,
    //  since it transforms into them while in their Crowned forms.
    const struct FormChange *formChanges = GetSpeciesFormChanges(species);

    for (u32 i = 0; formChanges != NULL && formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (formChanges[i].method == FORM_CHANGE_END_BATTLE
            && learnset[sLearningMoveTableID].move == formChanges[i].param3)
        {
            for (u32 j = 0; j < MAX_MON_MOVES; j++)
            {
                if (formChanges[i].param2 == GetMonData(mon, MON_DATA_MOVE1 + j))
                    return MOVE_NONE;
            }
        }
    }

    if (learnset[sLearningMoveTableID].level == level)
    {
        gMoveToLearn = learnset[sLearningMoveTableID].move;
        sLearningMoveTableID++;
        retVal = GiveMoveToMon(mon, gMoveToLearn);
    }

    return retVal;
}

u16 MonTryLearningNewMove(struct Pokemon *mon, bool8 firstMove)
{
    return MonTryLearningNewMoveAtLevel(mon, firstMove, GetMonData(mon, MON_DATA_LEVEL, NULL));
}

void DeleteFirstMoveAndGiveMoveToMon(struct Pokemon *mon, u16 move)
{
    s32 i;
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;

    for (i = 0; i < MAX_MON_MOVES - 1; i++)
    {
        moves[i] = GetMonData(mon, MON_DATA_MOVE2 + i, NULL);
        pp[i] = GetMonData(mon, MON_DATA_PP2 + i, NULL);
    }

    ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses >>= 2;
    moves[MAX_MON_MOVES - 1] = move;
    pp[MAX_MON_MOVES - 1] = GetMovePP(move);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &pp[i]);
    }

    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void DeleteFirstMoveAndGiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move)
{
    s32 i;
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;

    for (i = 0; i < MAX_MON_MOVES - 1; i++)
    {
        moves[i] = GetBoxMonData(boxMon, MON_DATA_MOVE2 + i, NULL);
        pp[i] = GetBoxMonData(boxMon, MON_DATA_PP2 + i, NULL);
    }

    ppBonuses = GetBoxMonData(boxMon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses >>= 2;
    moves[MAX_MON_MOVES - 1] = move;
    pp[MAX_MON_MOVES - 1] = GetMovePP(move);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &moves[i]);
        SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp[i]);
    }

    SetBoxMonData(boxMon, MON_DATA_PP_BONUSES, &ppBonuses);
}

u8 CountAliveMonsInBattle(u8 caseId, u32 battler)
{
    u32 i;
    u32 retVal = 0;

    switch (caseId)
    {
    case BATTLE_ALIVE_EXCEPT_BATTLER:
        for (i = 0; i < gBattlersCount; i++)
        {
            if (i != battler && !(gAbsentBattlerFlags & (1u << i)))
                retVal++;
        }
        break;
    case BATTLE_ALIVE_EXCEPT_BATTLER_SIDE:
        for (i = 0; i < gBattlersCount; i++)
        {
            if (i != battler && i != BATTLE_PARTNER(battler) && !(gAbsentBattlerFlags & (1u << i)))
                retVal++;
        }
        break;
    case BATTLE_ALIVE_SIDE:
        for (i = 0; i < gBattlersCount; i++)
        {
            if (IsBattlerAlly(i, battler) && !(gAbsentBattlerFlags & (1u << i)))
                retVal++;
        }
        break;
    }

    return retVal;
}

u8 GetDefaultMoveTarget(u8 battlerId)
{
    u8 opposing = BATTLE_OPPOSITE(GetBattlerSide(battlerId));

    if (!IsDoubleBattle())
        return GetBattlerAtPosition(opposing);
    if (CountAliveMonsInBattle(BATTLE_ALIVE_EXCEPT_BATTLER, battlerId) > 1)
    {
        u8 position;

        if ((Random() & 1) == 0)
            position = BATTLE_PARTNER(opposing);
        else
            position = opposing;

        return GetBattlerAtPosition(position);
    }
    else
    {
        if ((gAbsentBattlerFlags & (1u << opposing)))
            return GetBattlerAtPosition(BATTLE_PARTNER(opposing));
        else
            return GetBattlerAtPosition(opposing);
    }
}

u8 GetMonGender(struct Pokemon *mon)
{
    return GetBoxMonGender(&mon->box);
}

u8 GetBoxMonGender(struct BoxPokemon *boxMon)
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 personality = GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL);

    switch (gSpeciesInfo[species].genderRatio)
    {
    case MON_MALE:
    case MON_FEMALE:
    case MON_GENDERLESS:
        return gSpeciesInfo[species].genderRatio;
    }

    if (gSpeciesInfo[species].genderRatio > (personality & 0xFF))
        return MON_FEMALE;
    else
        return MON_MALE;
}

u8 GetGenderFromSpeciesAndPersonality(u16 species, u32 personality)
{
    switch (gSpeciesInfo[species].genderRatio)
    {
    case MON_MALE:
    case MON_FEMALE:
    case MON_GENDERLESS:
        return gSpeciesInfo[species].genderRatio;
    }

    if (gSpeciesInfo[species].genderRatio > (personality & 0xFF))
        return MON_FEMALE;
    else
        return MON_MALE;
}

bool32 IsPersonalityFemale(u16 species, u32 personality)
{
    return GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE;
}

u32 GetUnownSpeciesId(u32 personality)
{
    u16 unownLetter = GetUnownLetterByPersonality(personality);

    if (unownLetter == 0)
        return SPECIES_UNOWN;
    return unownLetter + SPECIES_UNOWN_B - 1;
}

void SetMultiuseSpriteTemplateToPokemon(u16 speciesTag, u8 battlerPosition)
{
    if (gMonSpritesGfxPtr != NULL)
        gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
    else if (sMonSpritesGfxManagers[MON_SPR_GFX_MANAGER_A])
        gMultiuseSpriteTemplate = sMonSpritesGfxManagers[MON_SPR_GFX_MANAGER_A]->templates[battlerPosition];
    else if (sMonSpritesGfxManagers[MON_SPR_GFX_MANAGER_B])
        gMultiuseSpriteTemplate = sMonSpritesGfxManagers[MON_SPR_GFX_MANAGER_B]->templates[battlerPosition];
    else
        gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];

    gMultiuseSpriteTemplate.paletteTag = speciesTag;
    if (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_PLAYER_RIGHT)
        gMultiuseSpriteTemplate.anims = gAnims_MonPic;
    else
    {
        if (speciesTag > SPECIES_SHINY_TAG)
            speciesTag = speciesTag - SPECIES_SHINY_TAG;

        speciesTag = SanitizeSpeciesId(speciesTag);
        if (gSpeciesInfo[speciesTag].frontAnimFrames != NULL)
            gMultiuseSpriteTemplate.anims = gSpeciesInfo[speciesTag].frontAnimFrames;
        else
            gMultiuseSpriteTemplate.anims = gSpeciesInfo[SPECIES_NONE].frontAnimFrames;
    }
}

void SetMultiuseSpriteTemplateToTrainerBack(u16 trainerPicId, u8 battlerPosition)
{
    gMultiuseSpriteTemplate.paletteTag = trainerPicId;
    if (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_PLAYER_RIGHT)
    {
        gMultiuseSpriteTemplate = sTrainerBackSpriteTemplate;
        gMultiuseSpriteTemplate.images = &gTrainerBacksprites[trainerPicId].backPic;
        gMultiuseSpriteTemplate.anims = gTrainerBacksprites[trainerPicId].animation;
    }
    else
    {
        if (gMonSpritesGfxPtr != NULL)
            gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
        else
            gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];
        gMultiuseSpriteTemplate.anims = gAnims_Trainer;
    }
}

void SetMultiuseSpriteTemplateToTrainerFront(u16 trainerPicId, u8 battlerPosition)
{
    if (gMonSpritesGfxPtr != NULL)
        gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
    else
        gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];

    gMultiuseSpriteTemplate.paletteTag = trainerPicId;
    gMultiuseSpriteTemplate.anims = gAnims_Trainer;
}

static void EncryptBoxMon(struct BoxPokemon *boxMon)
{
    for (u32 i = 0; i < ARRAY_COUNT(boxMon->secure.raw); i++)
    {
        boxMon->secure.raw[i] ^= boxMon->personality;
        boxMon->secure.raw[i] ^= boxMon->otId;
    }
}

static void DecryptBoxMon(struct BoxPokemon *boxMon)
{
    for (u32 i = 0; i < ARRAY_COUNT(boxMon->secure.raw); i++)
    {
        boxMon->secure.raw[i] ^= boxMon->otId;
        boxMon->secure.raw[i] ^= boxMon->personality;
    }
}

static const u8 sSubstructOffsets[4][24] =
{
    [SUBSTRUCT_TYPE_0] = {0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3},
    [SUBSTRUCT_TYPE_1] = {1, 1, 2, 3, 2, 3, 0, 0, 0, 0, 0, 0, 2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2},
    [SUBSTRUCT_TYPE_2] = {2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2, 0, 0, 0, 0, 0, 0, 3, 2, 3, 2, 1, 1},
    [SUBSTRUCT_TYPE_3] = {3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0},
};

ARM_FUNC NOINLINE static u32 ConstantMod24(u32 a) { return a % 24; }

static union PokemonSubstruct *GetSubstruct(struct BoxPokemon *boxMon, u32 personality, enum SubstructType substructType)
{
    return &boxMon->secure.substructs[sSubstructOffsets[substructType][ConstantMod24(personality)]];
}

/* GameFreak called GetMonData with either 2 or 3 arguments, for type
 * safety we have a GetMonData macro (in include/pokemon.h) which
 * dispatches to either GetMonData2 or GetMonData3 based on the number
 * of arguments. */
u32 GetMonData3(struct Pokemon *mon, s32 field, u8 *data)
{
    u32 ret;

    switch (field)
    {
    case MON_DATA_STATUS:
        ret = mon->status;
        break;
    case MON_DATA_LEVEL:
        ret = mon->level;
        break;
    case MON_DATA_HP:
        ret = mon->hp;
        break;
    case MON_DATA_MAX_HP:
        ret = mon->maxHP;
        break;
    case MON_DATA_ATK:
        ret = mon->attack;
        break;
    case MON_DATA_DEF:
        ret = mon->defense;
        break;
    case MON_DATA_SPEED:
        ret = mon->speed;
        break;
    case MON_DATA_SPATK:
        ret = mon->spAttack;
        break;
    case MON_DATA_SPDEF:
        ret = mon->spDefense;
        break;
    case MON_DATA_ATK2:
        ret = mon->attack;
        break;
    case MON_DATA_DEF2:
        ret = mon->defense;
        break;
    case MON_DATA_SPEED2:
        ret = mon->speed;
        break;
    case MON_DATA_SPATK2:
        ret = mon->spAttack;
        break;
    case MON_DATA_SPDEF2:
        ret = mon->spDefense;
        break;
    case MON_DATA_MAIL:
        ret = mon->mail;
        break;
    default:
        ret = GetBoxMonData(&mon->box, field, data);
        break;
    }
    return ret;
}

u32 GetMonData2(struct Pokemon *mon, s32 field)
{
    return GetMonData3(mon, field, NULL);
}


union EvolutionTracker
{
    u16 combinedValue:10;
    struct {
        u16 tracker1: 5;
        u16 tracker2: 5;
    };
};

static ALWAYS_INLINE struct PokemonSubstruct0 *GetSubstruct0(struct BoxPokemon *boxMon)
{
    return &(GetSubstruct(boxMon, boxMon->personality, SUBSTRUCT_TYPE_0)->type0);
}

static ALWAYS_INLINE struct PokemonSubstruct1 *GetSubstruct1(struct BoxPokemon *boxMon)
{
    return &(GetSubstruct(boxMon, boxMon->personality, SUBSTRUCT_TYPE_1)->type1);
}

static ALWAYS_INLINE struct PokemonSubstruct2 *GetSubstruct2(struct BoxPokemon *boxMon)
{
    return &(GetSubstruct(boxMon, boxMon->personality, SUBSTRUCT_TYPE_2)->type2);
}

static ALWAYS_INLINE struct PokemonSubstruct3 *GetSubstruct3(struct BoxPokemon *boxMon)
{
    return &(GetSubstruct(boxMon, boxMon->personality, SUBSTRUCT_TYPE_3)->type3);
}

static bool32 IsBadEgg(struct BoxPokemon *boxMon)
{
    if (boxMon->isBadEgg)
        return TRUE;

    if (CalculateBoxMonChecksum(boxMon) != boxMon->checksum)
    {
        boxMon->isBadEgg = TRUE;
        boxMon->isEgg = TRUE;
        GetSubstruct3(boxMon)->isEgg = TRUE;

        return TRUE;
    }

    return FALSE;
}

static ALWAYS_INLINE bool32 IsEggOrBadEgg(struct BoxPokemon *boxMon)
{
    return GetSubstruct3(boxMon)->isEgg || IsBadEgg(boxMon);
}

/* GameFreak called GetBoxMonData with either 2 or 3 arguments, for type
 * safety we have a GetBoxMonData macro (in include/pokemon.h) which
 * dispatches to either GetBoxMonData2 or GetBoxMonData3 based on the
 * number of arguments. */
u32 GetBoxMonData3(struct BoxPokemon *boxMon, s32 field, u8 *data)
{
    s32 i;
    u32 retVal = 0;

    // Any field greater than MON_DATA_ENCRYPT_SEPARATOR is encrypted and must be treated as such
    if (field > MON_DATA_ENCRYPT_SEPARATOR)
    {
        DecryptBoxMon(boxMon);

        switch (field)
        {
        case MON_DATA_NICKNAME:
        case MON_DATA_NICKNAME10:
        {
            if (IsBadEgg(boxMon))
            {
                for (retVal = 0;
                    retVal < POKEMON_NAME_LENGTH && gText_BadEgg[retVal] != EOS;
                    data[retVal] = gText_BadEgg[retVal], retVal++) {}

                data[retVal] = EOS;
            }
            else if (boxMon->isEgg)
            {
                StringCopy(data, gText_EggNickname);
                retVal = StringLength(data);
            }
            else if (boxMon->language == LANGUAGE_JAPANESE)
            {
                data[0] = EXT_CTRL_CODE_BEGIN;
                data[1] = EXT_CTRL_CODE_JPN;

                for (retVal = 2, i = 0;
                    i < 5 && boxMon->nickname[i] != EOS;
                    data[retVal] = boxMon->nickname[i], retVal++, i++) {}

                data[retVal++] = EXT_CTRL_CODE_BEGIN;
                data[retVal++] = EXT_CTRL_CODE_ENG;
                data[retVal] = EOS;
            }
            else
            {
                retVal = 0;
                while (retVal < min(sizeof(boxMon->nickname), POKEMON_NAME_LENGTH))
                {
                    data[retVal] = boxMon->nickname[retVal];
                    retVal++;
                }

                // Vanilla Pokémon have 0s in nickname11 and nickname12
                // so if both are 0 we assume that this is a vanilla
                // Pokémon and replace them with EOS. This means that
                // two CHAR_SPACE at the end of a nickname are trimmed.
                struct PokemonSubstruct0 *substruct0 = GetSubstruct0(boxMon);
                if (field != MON_DATA_NICKNAME10 && POKEMON_NAME_LENGTH >= 12)
                {
                    if (substruct0->nickname11 == 0 && substruct0->nickname12 == 0)
                    {
                        data[retVal++] = EOS;
                        data[retVal++] = EOS;
                    }
                    else
                    {
                        data[retVal++] = substruct0->nickname11;
                        data[retVal++] = substruct0->nickname12;
                    }
                }
                else if (field != MON_DATA_NICKNAME10 && POKEMON_NAME_LENGTH >= 11)
                {
                    if (substruct0->nickname11 == 0)
                    {
                        data[retVal++] = EOS;
                    }
                    else
                    {
                        data[retVal++] = substruct0->nickname11;
                    }
                }

                data[retVal] = EOS;
            }
            break;
        }
        case MON_DATA_SPECIES:
            retVal = IsBadEgg(boxMon) ? SPECIES_EGG : GetSubstruct0(boxMon)->species;
            break;
        case MON_DATA_HELD_ITEM:
            retVal = GetSubstruct0(boxMon)->heldItem;
            break;
        case MON_DATA_EXP:
            retVal = GetSubstruct0(boxMon)->experience;
            break;
        case MON_DATA_PP_BONUSES:
            retVal = GetSubstruct0(boxMon)->ppBonuses;
            break;
        case MON_DATA_FRIENDSHIP:
            retVal = GetSubstruct0(boxMon)->friendship;
            break;
        case MON_DATA_MOVE1:
            retVal = GetSubstruct1(boxMon)->move1;
            break;
        case MON_DATA_MOVE2:
            retVal = GetSubstruct1(boxMon)->move2;
            break;
        case MON_DATA_MOVE3:
            retVal = GetSubstruct1(boxMon)->move3;
            break;
        case MON_DATA_MOVE4:
            retVal = GetSubstruct1(boxMon)->move4;
            break;
        case MON_DATA_PP1:
            retVal = GetSubstruct1(boxMon)->pp1;
            break;
        case MON_DATA_PP2:
            retVal = GetSubstruct1(boxMon)->pp2;
            break;
        case MON_DATA_PP3:
            retVal = GetSubstruct1(boxMon)->pp3;
            break;
        case MON_DATA_PP4:
            retVal = GetSubstruct1(boxMon)->pp4;
            break;
        case MON_DATA_HP_EV:
            retVal = GetSubstruct2(boxMon)->hpEV;
            break;
        case MON_DATA_ATK_EV:
            retVal = GetSubstruct2(boxMon)->attackEV;
            break;
        case MON_DATA_DEF_EV:
            retVal = GetSubstruct2(boxMon)->defenseEV;
            break;
        case MON_DATA_SPEED_EV:
            retVal = GetSubstruct2(boxMon)->speedEV;
            break;
        case MON_DATA_SPATK_EV:
            retVal = GetSubstruct2(boxMon)->spAttackEV;
            break;
        case MON_DATA_SPDEF_EV:
            retVal = GetSubstruct2(boxMon)->spDefenseEV;
            break;
        case MON_DATA_COOL:
            retVal = GetSubstruct2(boxMon)->cool;
            break;
        case MON_DATA_BEAUTY:
            retVal = GetSubstruct2(boxMon)->beauty;
            break;
        case MON_DATA_CUTE:
            retVal = GetSubstruct2(boxMon)->cute;
            break;
        case MON_DATA_SMART:
            retVal = GetSubstruct2(boxMon)->smart;
            break;
        case MON_DATA_TOUGH:
            retVal = GetSubstruct2(boxMon)->tough;
            break;
        case MON_DATA_SHEEN:
            retVal = GetSubstruct2(boxMon)->sheen;
            break;
        case MON_DATA_POKERUS:
            retVal = GetSubstruct3(boxMon)->pokerus;
            break;
        case MON_DATA_MET_LOCATION:
            retVal = GetSubstruct3(boxMon)->metLocation;
            break;
        case MON_DATA_MET_LEVEL:
            retVal = GetSubstruct3(boxMon)->metLevel;
            break;
        case MON_DATA_MET_GAME:
            retVal = GetSubstruct3(boxMon)->metGame;
            break;
        case MON_DATA_POKEBALL:
            retVal = GetSubstruct0(boxMon)->pokeball;
            break;
        case MON_DATA_OT_GENDER:
            retVal = GetSubstruct3(boxMon)->otGender;
            break;
        case MON_DATA_HP_IV:
            retVal = GetSubstruct3(boxMon)->hpIV;
            break;
        case MON_DATA_ATK_IV:
            retVal = GetSubstruct3(boxMon)->attackIV;
            break;
        case MON_DATA_DEF_IV:
            retVal = GetSubstruct3(boxMon)->defenseIV;
            break;
        case MON_DATA_SPEED_IV:
            retVal = GetSubstruct3(boxMon)->speedIV;
            break;
        case MON_DATA_SPATK_IV:
            retVal = GetSubstruct3(boxMon)->spAttackIV;
            break;
        case MON_DATA_SPDEF_IV:
            retVal = GetSubstruct3(boxMon)->spDefenseIV;
            break;
        case MON_DATA_IS_EGG:
            retVal = IsEggOrBadEgg(boxMon);
            break;
        case MON_DATA_ABILITY_NUM:
            retVal = GetSubstruct3(boxMon)->abilityNum;
            break;
        case MON_DATA_COOL_RIBBON:
            retVal = GetSubstruct3(boxMon)->coolRibbon;
            break;
        case MON_DATA_BEAUTY_RIBBON:
            retVal = GetSubstruct3(boxMon)->beautyRibbon;
            break;
        case MON_DATA_CUTE_RIBBON:
            retVal = GetSubstruct3(boxMon)->cuteRibbon;
            break;
        case MON_DATA_SMART_RIBBON:
            retVal = GetSubstruct3(boxMon)->smartRibbon;
            break;
        case MON_DATA_TOUGH_RIBBON:
            retVal = GetSubstruct3(boxMon)->toughRibbon;
            break;
        case MON_DATA_CHAMPION_RIBBON:
            retVal = GetSubstruct3(boxMon)->championRibbon;
            break;
        case MON_DATA_WINNING_RIBBON:
            retVal = GetSubstruct3(boxMon)->winningRibbon;
            break;
        case MON_DATA_VICTORY_RIBBON:
            retVal = GetSubstruct3(boxMon)->victoryRibbon;
            break;
        case MON_DATA_ARTIST_RIBBON:
            retVal = GetSubstruct3(boxMon)->artistRibbon;
            break;
        case MON_DATA_EFFORT_RIBBON:
            retVal = GetSubstruct3(boxMon)->effortRibbon;
            break;
        case MON_DATA_MARINE_RIBBON:
            retVal = GetSubstruct3(boxMon)->marineRibbon;
            break;
        case MON_DATA_LAND_RIBBON:
            retVal = GetSubstruct3(boxMon)->landRibbon;
            break;
        case MON_DATA_SKY_RIBBON:
            retVal = GetSubstruct3(boxMon)->skyRibbon;
            break;
        case MON_DATA_COUNTRY_RIBBON:
            retVal = GetSubstruct3(boxMon)->countryRibbon;
            break;
        case MON_DATA_NATIONAL_RIBBON:
            retVal = GetSubstruct3(boxMon)->nationalRibbon;
            break;
        case MON_DATA_EARTH_RIBBON:
            retVal = GetSubstruct3(boxMon)->earthRibbon;
            break;
        case MON_DATA_WORLD_RIBBON:
            retVal = GetSubstruct3(boxMon)->worldRibbon;
            break;
        case MON_DATA_MODERN_FATEFUL_ENCOUNTER:
            retVal = GetSubstruct3(boxMon)->modernFatefulEncounter;
            break;
        case MON_DATA_SPECIES_OR_EGG:
            retVal = GetSubstruct0(boxMon)->species;
            if (retVal && IsEggOrBadEgg(boxMon))
                retVal = SPECIES_EGG;
            break;
        case MON_DATA_IVS:
        {
            struct PokemonSubstruct3 *substruct3 = GetSubstruct3(boxMon);
            retVal = substruct3->hpIV
                    | (substruct3->attackIV << 5)
                    | (substruct3->defenseIV << 10)
                    | (substruct3->speedIV << 15)
                    | (substruct3->spAttackIV << 20)
                    | (substruct3->spDefenseIV << 25);
            break;
        }
        case MON_DATA_KNOWN_MOVES:
            if (GetSubstruct0(boxMon)->species && !IsEggOrBadEgg(boxMon))
            {
                struct PokemonSubstruct1 *substruct1 = GetSubstruct1(boxMon);
                u16 *moves = (u16 *)data;
                s32 i = 0;

                while (moves[i] != MOVES_COUNT)
                {
                    u16 move = moves[i];
                    if (substruct1->move1 == move
                        || substruct1->move2 == move
                        || substruct1->move3 == move
                        || substruct1->move4 == move)
                        retVal |= (1u << i);
                    i++;
                }
            }
            break;
        case MON_DATA_RIBBON_COUNT:
            if (GetSubstruct0(boxMon)->species && !IsEggOrBadEgg(boxMon))
            {
                struct PokemonSubstruct3 *substruct3 = GetSubstruct3(boxMon);
                retVal = 0;
                retVal += substruct3->coolRibbon;
                retVal += substruct3->beautyRibbon;
                retVal += substruct3->cuteRibbon;
                retVal += substruct3->smartRibbon;
                retVal += substruct3->toughRibbon;
                retVal += substruct3->championRibbon;
                retVal += substruct3->winningRibbon;
                retVal += substruct3->victoryRibbon;
                retVal += substruct3->artistRibbon;
                retVal += substruct3->effortRibbon;
                retVal += substruct3->marineRibbon;
                retVal += substruct3->landRibbon;
                retVal += substruct3->skyRibbon;
                retVal += substruct3->countryRibbon;
                retVal += substruct3->nationalRibbon;
                retVal += substruct3->earthRibbon;
                retVal += substruct3->worldRibbon;
            }
            break;
        case MON_DATA_RIBBONS:
            if (GetSubstruct0(boxMon)->species && !IsEggOrBadEgg(boxMon))
            {
                struct PokemonSubstruct3 *substruct3 = GetSubstruct3(boxMon);
                retVal = substruct3->championRibbon
                       | (substruct3->coolRibbon << 1)
                       | (substruct3->beautyRibbon << 4)
                       | (substruct3->cuteRibbon << 7)
                       | (substruct3->smartRibbon << 10)
                       | (substruct3->toughRibbon << 13)
                       | (substruct3->winningRibbon << 16)
                       | (substruct3->victoryRibbon << 17)
                       | (substruct3->artistRibbon << 18)
                       | (substruct3->effortRibbon << 19)
                       | (substruct3->marineRibbon << 20)
                       | (substruct3->landRibbon << 21)
                       | (substruct3->skyRibbon << 22)
                       | (substruct3->countryRibbon << 23)
                       | (substruct3->nationalRibbon << 24)
                       | (substruct3->earthRibbon << 25)
                       | (substruct3->worldRibbon << 26);
            }
            break;
        case MON_DATA_HYPER_TRAINED_HP:
            retVal = GetSubstruct1(boxMon)->hyperTrainedHP;
            break;
        case MON_DATA_HYPER_TRAINED_ATK:
            retVal = GetSubstruct1(boxMon)->hyperTrainedAttack;
            break;
        case MON_DATA_HYPER_TRAINED_DEF:
            retVal = GetSubstruct1(boxMon)->hyperTrainedDefense;
            break;
        case MON_DATA_HYPER_TRAINED_SPEED:
            retVal = GetSubstruct1(boxMon)->hyperTrainedSpeed;
            break;
        case MON_DATA_HYPER_TRAINED_SPATK:
            retVal = GetSubstruct1(boxMon)->hyperTrainedSpAttack;
            break;
        case MON_DATA_HYPER_TRAINED_SPDEF:
            retVal = GetSubstruct1(boxMon)->hyperTrainedSpDefense;
            break;
        case MON_DATA_IS_SHADOW:
            retVal = GetSubstruct3(boxMon)->isShadow;
            break;
        case MON_DATA_DYNAMAX_LEVEL:
            retVal = GetSubstruct3(boxMon)->dynamaxLevel;
            break;
        case MON_DATA_GIGANTAMAX_FACTOR:
            retVal = GetSubstruct3(boxMon)->gigantamaxFactor;
            break;
        case MON_DATA_TERA_TYPE:
            {
                struct PokemonSubstruct0 *substruct0 = GetSubstruct0(boxMon);
                if (gSpeciesInfo[substruct0->species].forceTeraType)
                {
                    retVal = gSpeciesInfo[substruct0->species].forceTeraType;
                }
                else if (substruct0->teraType == TYPE_NONE) // Tera Type hasn't been modified so we can just use the personality
                {
                    const enum Type *types = gSpeciesInfo[substruct0->species].types;
                    retVal = (boxMon->personality & 0x1) == 0 ? types[0] : types[1];
                }
                else
                {
                    retVal = substruct0->teraType;
                }
            }
            break;
        case MON_DATA_EVOLUTION_TRACKER:
            {
                struct PokemonSubstruct1 *substruct1 = GetSubstruct1(boxMon);
                retVal = (union EvolutionTracker) {
                    .tracker1 = substruct1->evolutionTracker1,
                    .tracker2 = substruct1->evolutionTracker2,
                }.combinedValue;
            }
            break;
        default:
            break;
        }
    }
    else
    {
        switch (field)
        {
        case MON_DATA_STATUS:
            retVal = UncompressStatus(boxMon->compressedStatus);
            break;
        case MON_DATA_HP_LOST:
            retVal = boxMon->hpLost;
            break;
        case MON_DATA_PERSONALITY:
            retVal = boxMon->personality;
            break;
        case MON_DATA_OT_ID:
            retVal = boxMon->otId;
            break;
        case MON_DATA_LANGUAGE:
            retVal = boxMon->language;
            break;
        case MON_DATA_SANITY_IS_BAD_EGG:
            retVal = boxMon->isBadEgg;
            break;
        case MON_DATA_SANITY_HAS_SPECIES:
            retVal = boxMon->hasSpecies;
            break;
        case MON_DATA_SANITY_IS_EGG:
            retVal = boxMon->isEgg;
            break;
        case MON_DATA_OT_NAME:
        {
            retVal = 0;

            while (retVal < PLAYER_NAME_LENGTH)
            {
                data[retVal] = boxMon->otName[retVal];
                retVal++;
            }

            data[retVal] = EOS;
            break;
        }
        case MON_DATA_MARKINGS:
            retVal = boxMon->markings;
            break;
        case MON_DATA_CHECKSUM:
            retVal = boxMon->checksum;
            break;
        case MON_DATA_IS_SHINY:
        {
            u32 shinyValue = GET_SHINY_VALUE(boxMon->otId, boxMon->personality);
            retVal = (shinyValue < SHINY_ODDS) ^ boxMon->shinyModifier;
            break;
        }
        case MON_DATA_HIDDEN_NATURE:
        {
            u32 nature = GetNatureFromPersonality(boxMon->personality);
            retVal = nature ^ boxMon->hiddenNatureModifier;
            break;
        }
        case MON_DATA_DAYS_SINCE_FORM_CHANGE:
            retVal = boxMon->daysSinceFormChange;
            break;
        default:
            break;
        }
    }

    if (field > MON_DATA_ENCRYPT_SEPARATOR)
        EncryptBoxMon(boxMon);

    return retVal;
}

u32 GetBoxMonData2(struct BoxPokemon *boxMon, s32 field)
{
    return GetBoxMonData3(boxMon, field, NULL);
}

#define SET8(lhs) (lhs) = *data
#define SET16(lhs) (lhs) = data[0] + (data[1] << 8)
#define SET32(lhs) (lhs) = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
//
// Prefer SET_BY_WIDTH for fields whose types might be extended (e.g.
// anything whose typedef is in gametypes.h).
//
#define SET_BY_WIDTH(lhs) \
    do { \
       if (sizeof(lhs) == 1) \
          SET8(lhs); \
       else if (sizeof(lhs) == 2) \
          SET16(lhs); \
       else if (sizeof(lhs) == 4) \
          SET32(lhs); \
   } while (0)

void SetMonData(struct Pokemon *mon, s32 field, const void *dataArg)
{
    const u8 *data = dataArg;

    switch (field)
    {
    case MON_DATA_STATUS:
        SET32(mon->status);
        SetBoxMonData(&mon->box, MON_DATA_STATUS, dataArg);
        break;
    case MON_DATA_LEVEL:
        SET8(mon->level);
        break;
    case MON_DATA_HP:
    {
        u32 hpLost;
        SET16(mon->hp);
        hpLost = mon->maxHP - mon->hp;
        SetBoxMonData(&mon->box, MON_DATA_HP_LOST, &hpLost);
        break;
    }
    case MON_DATA_HP_LOST:
    {
        u32 hpLost;
        SET16(hpLost);
        mon->hp = mon->maxHP - hpLost;
        SetBoxMonData(&mon->box, MON_DATA_HP_LOST, &hpLost);
        break;
    }
    case MON_DATA_MAX_HP:
        SET16(mon->maxHP);
        break;
    case MON_DATA_ATK:
        SET16(mon->attack);
        break;
    case MON_DATA_DEF:
        SET16(mon->defense);
        break;
    case MON_DATA_SPEED:
        SET16(mon->speed);
        break;
    case MON_DATA_SPATK:
        SET16(mon->spAttack);
        break;
    case MON_DATA_SPDEF:
        SET16(mon->spDefense);
        break;
    case MON_DATA_MAIL:
        SET8(mon->mail);
        break;
    case MON_DATA_SPECIES_OR_EGG:
        break;
    default:
        SetBoxMonData(&mon->box, field, data);
        break;
    }
}

void SetBoxMonData(struct BoxPokemon *boxMon, s32 field, const void *dataArg)
{
    const u8 *data = dataArg;

    if (field > MON_DATA_ENCRYPT_SEPARATOR)
    {
        if (CalculateBoxMonChecksumDecrypt(boxMon) != boxMon->checksum)
        {
            boxMon->isBadEgg = TRUE;
            boxMon->isEgg = TRUE;
            GetSubstruct3(boxMon)->isEgg = TRUE;
            EncryptBoxMon(boxMon);
            return;
        }

        switch (field)
        {
        case MON_DATA_NICKNAME:
        case MON_DATA_NICKNAME10:
        {
            s32 i;
            struct PokemonSubstruct0 *substruct0 = GetSubstruct0(boxMon);
            for (i = 0; i < min(sizeof(boxMon->nickname), POKEMON_NAME_LENGTH); i++)
                boxMon->nickname[i] = data[i];
            if (field != MON_DATA_NICKNAME10)
            {
                if (POKEMON_NAME_LENGTH >= 11)
                    substruct0->nickname11 = data[10];
                if (POKEMON_NAME_LENGTH >= 12)
                    substruct0->nickname12 = data[11];
            }
            else
            {
                substruct0->nickname11 = EOS;
                substruct0->nickname12 = EOS;
            }
            break;
        }
        case MON_DATA_SPECIES:
        {
            struct PokemonSubstruct0 *substruct0 = GetSubstruct0(boxMon);
            SET16(substruct0->species);
            if (substruct0->species)
                boxMon->hasSpecies = TRUE;
            else
                boxMon->hasSpecies = FALSE;
            break;
        }
        case MON_DATA_HELD_ITEM:
            SET16(GetSubstruct0(boxMon)->heldItem);
            break;
        case MON_DATA_EXP:
            SET32(GetSubstruct0(boxMon)->experience);
            break;
        case MON_DATA_PP_BONUSES:
            SET8(GetSubstruct0(boxMon)->ppBonuses);
            break;
        case MON_DATA_FRIENDSHIP:
            SET8(GetSubstruct0(boxMon)->friendship);
            break;
        case MON_DATA_MOVE1:
            SET16(GetSubstruct1(boxMon)->move1);
            break;
        case MON_DATA_MOVE2:
            SET16(GetSubstruct1(boxMon)->move2);
            break;
        case MON_DATA_MOVE3:
            SET16(GetSubstruct1(boxMon)->move3);
            break;
        case MON_DATA_MOVE4:
            SET16(GetSubstruct1(boxMon)->move4);
            break;
        case MON_DATA_PP1:
            SET8(GetSubstruct1(boxMon)->pp1);
            break;
        case MON_DATA_PP2:
            SET8(GetSubstruct1(boxMon)->pp2);
            break;
        case MON_DATA_PP3:
            SET8(GetSubstruct1(boxMon)->pp3);
            break;
        case MON_DATA_PP4:
            SET8(GetSubstruct1(boxMon)->pp4);
            break;
        case MON_DATA_HP_EV:
            SET8(GetSubstruct2(boxMon)->hpEV);
            break;
        case MON_DATA_ATK_EV:
            SET8(GetSubstruct2(boxMon)->attackEV);
            break;
        case MON_DATA_DEF_EV:
            SET8(GetSubstruct2(boxMon)->defenseEV);
            break;
        case MON_DATA_SPEED_EV:
            SET8(GetSubstruct2(boxMon)->speedEV);
            break;
        case MON_DATA_SPATK_EV:
            SET8(GetSubstruct2(boxMon)->spAttackEV);
            break;
        case MON_DATA_SPDEF_EV:
            SET8(GetSubstruct2(boxMon)->spDefenseEV);
            break;
        case MON_DATA_COOL:
            SET8(GetSubstruct2(boxMon)->cool);
            break;
        case MON_DATA_BEAUTY:
            SET8(GetSubstruct2(boxMon)->beauty);
            break;
        case MON_DATA_CUTE:
            SET8(GetSubstruct2(boxMon)->cute);
            break;
        case MON_DATA_SMART:
            SET8(GetSubstruct2(boxMon)->smart);
            break;
        case MON_DATA_TOUGH:
            SET8(GetSubstruct2(boxMon)->tough);
            break;
        case MON_DATA_SHEEN:
            SET8(GetSubstruct2(boxMon)->sheen);
            break;
        case MON_DATA_POKERUS:
            SET8(GetSubstruct3(boxMon)->pokerus);
            break;
        case MON_DATA_MET_LOCATION:
            SET8(GetSubstruct3(boxMon)->metLocation);
            break;
        case MON_DATA_MET_LEVEL:
            SET8(GetSubstruct3(boxMon)->metLevel);
            break;
        case MON_DATA_MET_GAME:
            SET8(GetSubstruct3(boxMon)->metGame);
            break;
        case MON_DATA_POKEBALL:
            SET8(GetSubstruct0(boxMon)->pokeball);
            break;
        case MON_DATA_OT_GENDER:
            SET8(GetSubstruct3(boxMon)->otGender);
            break;
        case MON_DATA_HP_IV:
            SET8(GetSubstruct3(boxMon)->hpIV);
            break;
        case MON_DATA_ATK_IV:
            SET8(GetSubstruct3(boxMon)->attackIV);
            break;
        case MON_DATA_DEF_IV:
            SET8(GetSubstruct3(boxMon)->defenseIV);
            break;
        case MON_DATA_SPEED_IV:
            SET8(GetSubstruct3(boxMon)->speedIV);
            break;
        case MON_DATA_SPATK_IV:
            SET8(GetSubstruct3(boxMon)->spAttackIV);
            break;
        case MON_DATA_SPDEF_IV:
            SET8(GetSubstruct3(boxMon)->spDefenseIV);
            break;
        case MON_DATA_IS_EGG:
            SET8(GetSubstruct3(boxMon)->isEgg);
            SET8(boxMon->isEgg);
            break;
        case MON_DATA_ABILITY_NUM:
            SET8(GetSubstruct3(boxMon)->abilityNum);
            break;
        case MON_DATA_COOL_RIBBON:
            SET8(GetSubstruct3(boxMon)->coolRibbon);
            break;
        case MON_DATA_BEAUTY_RIBBON:
            SET8(GetSubstruct3(boxMon)->beautyRibbon);
            break;
        case MON_DATA_CUTE_RIBBON:
            SET8(GetSubstruct3(boxMon)->cuteRibbon);
            break;
        case MON_DATA_SMART_RIBBON:
            SET8(GetSubstruct3(boxMon)->smartRibbon);
            break;
        case MON_DATA_TOUGH_RIBBON:
            SET8(GetSubstruct3(boxMon)->toughRibbon);
            break;
        case MON_DATA_CHAMPION_RIBBON:
            SET8(GetSubstruct3(boxMon)->championRibbon);
            break;
        case MON_DATA_WINNING_RIBBON:
            SET8(GetSubstruct3(boxMon)->winningRibbon);
            break;
        case MON_DATA_VICTORY_RIBBON:
            SET8(GetSubstruct3(boxMon)->victoryRibbon);
            break;
        case MON_DATA_ARTIST_RIBBON:
            SET8(GetSubstruct3(boxMon)->artistRibbon);
            break;
        case MON_DATA_EFFORT_RIBBON:
            SET8(GetSubstruct3(boxMon)->effortRibbon);
            break;
        case MON_DATA_MARINE_RIBBON:
            SET8(GetSubstruct3(boxMon)->marineRibbon);
            break;
        case MON_DATA_LAND_RIBBON:
            SET8(GetSubstruct3(boxMon)->landRibbon);
            break;
        case MON_DATA_SKY_RIBBON:
            SET8(GetSubstruct3(boxMon)->skyRibbon);
            break;
        case MON_DATA_COUNTRY_RIBBON:
            SET8(GetSubstruct3(boxMon)->countryRibbon);
            break;
        case MON_DATA_NATIONAL_RIBBON:
            SET8(GetSubstruct3(boxMon)->nationalRibbon);
            break;
        case MON_DATA_EARTH_RIBBON:
            SET8(GetSubstruct3(boxMon)->earthRibbon);
            break;
        case MON_DATA_WORLD_RIBBON:
            SET8(GetSubstruct3(boxMon)->worldRibbon);
            break;
        case MON_DATA_MODERN_FATEFUL_ENCOUNTER:
            SET8(GetSubstruct3(boxMon)->modernFatefulEncounter);
            break;
        case MON_DATA_IVS:
        {
            u32 ivs;
            struct PokemonSubstruct3 *substruct3 = GetSubstruct3(boxMon);
            SET32(ivs);
            substruct3->hpIV = ivs & MAX_IV_MASK;
            substruct3->attackIV = (ivs >> 5) & MAX_IV_MASK;
            substruct3->defenseIV = (ivs >> 10) & MAX_IV_MASK;
            substruct3->speedIV = (ivs >> 15) & MAX_IV_MASK;
            substruct3->spAttackIV = (ivs >> 20) & MAX_IV_MASK;
            substruct3->spDefenseIV = (ivs >> 25) & MAX_IV_MASK;
            break;
        }
        case MON_DATA_HYPER_TRAINED_HP:
            SET8(GetSubstruct1(boxMon)->hyperTrainedHP);
            break;
        case MON_DATA_HYPER_TRAINED_ATK:
            SET8(GetSubstruct1(boxMon)->hyperTrainedAttack);
            break;
        case MON_DATA_HYPER_TRAINED_DEF:
            SET8(GetSubstruct1(boxMon)->hyperTrainedDefense);
            break;
        case MON_DATA_HYPER_TRAINED_SPEED:
            SET8(GetSubstruct1(boxMon)->hyperTrainedSpeed);
            break;
        case MON_DATA_HYPER_TRAINED_SPATK:
            SET8(GetSubstruct1(boxMon)->hyperTrainedSpAttack);
            break;
        case MON_DATA_HYPER_TRAINED_SPDEF:
            SET8(GetSubstruct1(boxMon)->hyperTrainedSpDefense);
            break;
        case MON_DATA_IS_SHADOW:
            SET8(GetSubstruct3(boxMon)->isShadow);
            break;
        case MON_DATA_DYNAMAX_LEVEL:
            SET8(GetSubstruct3(boxMon)->dynamaxLevel);
            break;
        case MON_DATA_GIGANTAMAX_FACTOR:
            SET8(GetSubstruct3(boxMon)->gigantamaxFactor);
            break;
        case MON_DATA_TERA_TYPE:
            SET8(GetSubstruct0(boxMon)->teraType);
            break;
        case MON_DATA_EVOLUTION_TRACKER:
        {
            union EvolutionTracker evoTracker;
            struct PokemonSubstruct1 *substruct1 = GetSubstruct1(boxMon);
            SET32(evoTracker.combinedValue);
            substruct1->evolutionTracker1 = evoTracker.tracker1;
            substruct1->evolutionTracker2 = evoTracker.tracker2;
            break;
        }
        default:
            break;
        }
    }
    else
    {
        switch (field)
        {
        case MON_DATA_STATUS:
        {
            u32 status;
            SET32(status);
            boxMon->compressedStatus = CompressStatus(status);
            break;
        }
        case MON_DATA_HP_LOST:
            SET16(boxMon->hpLost);
            break;
        case MON_DATA_PERSONALITY:
            SET32(boxMon->personality);
            break;
        case MON_DATA_OT_ID:
            SET32(boxMon->otId);
            break;
        case MON_DATA_LANGUAGE:
            SET8(boxMon->language);
            break;
        case MON_DATA_SANITY_IS_BAD_EGG:
            SET8(boxMon->isBadEgg);
            break;
        case MON_DATA_SANITY_HAS_SPECIES:
            SET8(boxMon->hasSpecies);
            break;
        case MON_DATA_SANITY_IS_EGG:
            SET8(boxMon->isEgg);
            break;
        case MON_DATA_OT_NAME:
        {
            s32 i;
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                boxMon->otName[i] = data[i];
            break;
        }
        case MON_DATA_MARKINGS:
            SET8(boxMon->markings);
            break;
        case MON_DATA_CHECKSUM:
            SET16(boxMon->checksum);
            break;
        case MON_DATA_IS_SHINY:
        {
            u32 shinyValue = GET_SHINY_VALUE(boxMon->otId, boxMon->personality);
            bool32 isShiny;
            SET8(isShiny);
            boxMon->shinyModifier = (shinyValue < SHINY_ODDS) ^ isShiny;
            break;
        }
        case MON_DATA_HIDDEN_NATURE:
        {
            u32 nature = GetNatureFromPersonality(boxMon->personality);
            u32 hiddenNature;
            SET8(hiddenNature);
            boxMon->hiddenNatureModifier = nature ^ hiddenNature;
            break;
        }
        case MON_DATA_DAYS_SINCE_FORM_CHANGE:
            SET8(boxMon->daysSinceFormChange);
            break;
        }
    }

    if (field > MON_DATA_ENCRYPT_SEPARATOR)
        boxMon->checksum = CalculateBoxMonChecksumReencrypt(boxMon);
}

void CopyMon(void *dest, void *src, size_t size)
{
    memcpy(dest, src, size);
}

u8 GiveMonToPlayer(struct Pokemon *mon)
{
    s32 i;

    SetMonData(mon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetMonData(mon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);
    SetMonData(mon, MON_DATA_OT_ID, gSaveBlock2Ptr->playerTrainerId);

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            break;
    }

    if (i >= PARTY_SIZE)
        return CopyMonToPC(mon);

    CopyMon(&gPlayerParty[i], mon, sizeof(*mon));
    gPlayerPartyCount = i + 1;
    return MON_GIVEN_TO_PARTY;
}

u8 CopyMonToPC(struct Pokemon *mon)
{
    s32 boxNo, boxPos;

    SetPCBoxToSendMon(VarGet(VAR_PC_BOX_TO_SEND_MON));

    boxNo = StorageGetCurrentBox();

    do
    {
        for (boxPos = 0; boxPos < IN_BOX_COUNT; boxPos++)
        {
            struct BoxPokemon *checkingMon = GetBoxedMonPtr(boxNo, boxPos);
            if (GetBoxMonData(checkingMon, MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            {
                MonRestorePP(mon);
                CopyMon(checkingMon, &mon->box, sizeof(mon->box));
                gSpecialVar_MonBoxId = boxNo;
                gSpecialVar_MonBoxPos = boxPos;
                if (GetPCBoxToSendMon() != boxNo)
                    FlagClear(FLAG_SHOWN_BOX_WAS_FULL_MESSAGE);
                VarSet(VAR_PC_BOX_TO_SEND_MON, boxNo);
                return MON_GIVEN_TO_PC;
            }
        }

        boxNo++;
        if (boxNo == TOTAL_BOXES_COUNT)
            boxNo = 0;
    } while (boxNo != StorageGetCurrentBox());

    return MON_CANT_GIVE;
}

u8 CalculatePartyCount(struct Pokemon *party)
{
    u32 partyCount = 0;

    while (partyCount < PARTY_SIZE
        && GetMonData(&party[partyCount], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
    {
        partyCount++;
    }

    return partyCount;
}

u8 CalculatePartyCountOfSide(u32 battler, struct Pokemon *party)
{
    s32 partyCount, partySize;
    GetAIPartyIndexes(battler, &partyCount, &partySize);

    while (partyCount < partySize
        && GetMonData(&party[partyCount], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
    {
        partyCount++;
    }

    return partyCount;
}

u8 CalculatePlayerPartyCount(void)
{
    gPlayerPartyCount = CalculatePartyCount(gPlayerParty);
    return gPlayerPartyCount;
}

u8 CalculateEnemyPartyCount(void)
{
    gEnemyPartyCount = CalculatePartyCount(gEnemyParty);
    return gEnemyPartyCount;
}

u8 CalculateEnemyPartyCountInSide(u32 battler)
{
    return CalculatePartyCountOfSide(battler, gEnemyParty);
}

u8 GetMonsStateToDoubles(void)
{
    s32 aliveCount = 0;
    s32 i;
    CalculatePlayerPartyCount();

    if (OW_DOUBLE_APPROACH_WITH_ONE_MON)
        return PLAYER_HAS_TWO_USABLE_MONS;

    if (gPlayerPartyCount == 1)
        return gPlayerPartyCount; // PLAYER_HAS_ONE_MON

    for (i = 0; i < gPlayerPartyCount; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG, NULL) != SPECIES_EGG
         && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0
         && GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG, NULL) != SPECIES_NONE)
            aliveCount++;
    }

    return (aliveCount > 1) ? PLAYER_HAS_TWO_USABLE_MONS : PLAYER_HAS_ONE_USABLE_MON;
}

u8 GetMonsStateToDoubles_2(void)
{
    s32 aliveCount = 0;
    s32 i;

    if (OW_DOUBLE_APPROACH_WITH_ONE_MON
     || FollowerNPCIsBattlePartner())
        return PLAYER_HAS_TWO_USABLE_MONS;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG, NULL);
        if (species != SPECIES_EGG && species != SPECIES_NONE
         && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0)
            aliveCount++;
    }

    if (aliveCount == 1)
        return PLAYER_HAS_ONE_MON; // may have more than one, but only one is alive

    return (aliveCount > 1) ? PLAYER_HAS_TWO_USABLE_MONS : PLAYER_HAS_ONE_USABLE_MON;
}

enum Ability GetAbilityBySpecies(u16 species, u8 abilityNum)
{
    int i;

    if (abilityNum < NUM_ABILITY_SLOTS)
        gLastUsedAbility = GetSpeciesAbility(species, abilityNum);
    else
        gLastUsedAbility = ABILITY_NONE;

    if (abilityNum >= NUM_NORMAL_ABILITY_SLOTS) // if abilityNum is empty hidden ability, look for other hidden abilities
    {
        for (i = NUM_NORMAL_ABILITY_SLOTS; i < NUM_ABILITY_SLOTS && gLastUsedAbility == ABILITY_NONE; i++)
        {
            gLastUsedAbility = GetSpeciesAbility(species, i);
        }
    }

    for (i = 0; i < NUM_ABILITY_SLOTS && gLastUsedAbility == ABILITY_NONE; i++) // look for any non-empty ability
    {
        gLastUsedAbility = GetSpeciesAbility(species, i);
    }

    return gLastUsedAbility;
}

enum Ability GetMonAbility(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);
    return GetAbilityBySpecies(species, abilityNum);
}

void CreateSecretBaseEnemyParty(struct SecretBase *secretBaseRecord)
{
    s32 i, j;

    ZeroEnemyPartyMons();
    *gBattleResources->secretBase = *secretBaseRecord;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (gBattleResources->secretBase->party.species[i])
        {
            CreateMon(&gEnemyParty[i],
                gBattleResources->secretBase->party.species[i],
                gBattleResources->secretBase->party.levels[i],
                15,
                TRUE,
                gBattleResources->secretBase->party.personality[i],
                OT_ID_RANDOM_NO_SHINY,
                0);

            SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBattleResources->secretBase->party.heldItems[i]);

            for (j = 0; j < NUM_STATS; j++)
                SetMonData(&gEnemyParty[i], MON_DATA_HP_EV + j, &gBattleResources->secretBase->party.EVs[i]);

            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                SetMonData(&gEnemyParty[i], MON_DATA_MOVE1 + j, &gBattleResources->secretBase->party.moves[i * MAX_MON_MOVES + j]);
                u32 pp = GetMovePP(gBattleResources->secretBase->party.moves[i * MAX_MON_MOVES + j]);
                SetMonData(&gEnemyParty[i], MON_DATA_PP1 + j, &pp);
            }
        }
    }
}

u8 GetSecretBaseTrainerPicIndex(void)
{
    u8 facilityClass = sSecretBaseFacilityClasses[gBattleResources->secretBase->gender][gBattleResources->secretBase->trainerId[0] % NUM_SECRET_BASE_CLASSES];
    return gFacilityClassToPicIndex[facilityClass];
}

enum TrainerClassID GetSecretBaseTrainerClass(void)
{
    u8 facilityClass = sSecretBaseFacilityClasses[gBattleResources->secretBase->gender][gBattleResources->secretBase->trainerId[0] % NUM_SECRET_BASE_CLASSES];
    return gFacilityClassToTrainerClass[facilityClass];
}

bool8 IsPlayerPartyAndPokemonStorageFull(void)
{
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            return FALSE;

    return IsPokemonStorageFull();
}

bool8 IsPokemonStorageFull(void)
{
    s32 i, j;

    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
        for (j = 0; j < IN_BOX_COUNT; j++)
            if (GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == SPECIES_NONE)
                return FALSE;

    return TRUE;
}

const u8 *GetSpeciesName(u16 species)
{
    species = SanitizeSpeciesId(species);
    if (gSpeciesInfo[species].speciesName[0] == 0)
        return gSpeciesInfo[SPECIES_NONE].speciesName;
    return gSpeciesInfo[species].speciesName;
}

const u8 *GetSpeciesCategory(u16 species)
{
    species = SanitizeSpeciesId(species);
    if (gSpeciesInfo[species].categoryName[0] == 0)
        return gSpeciesInfo[SPECIES_NONE].categoryName;
    return gSpeciesInfo[species].categoryName;
}

const u8 *GetSpeciesPokedexDescription(u16 species)
{
    species = SanitizeSpeciesId(species);
    if (gSpeciesInfo[species].description == NULL)
        return gSpeciesInfo[SPECIES_NONE].description;
    return gSpeciesInfo[species].description;
}

u32 GetSpeciesHeight(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].height;
}

u32 GetSpeciesWeight(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].weight;
}

enum Type GetSpeciesType(u16 species, u8 slot)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].types[slot];
}

enum Ability GetSpeciesAbility(u16 species, u8 slot)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].abilities[slot];
}

u32 GetSpeciesBaseHP(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].baseHP;
}

u32 GetSpeciesBaseAttack(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].baseAttack;
}

u32 GetSpeciesBaseDefense(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].baseDefense;
}

u32 GetSpeciesBaseSpAttack(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].baseSpAttack;
}

u32 GetSpeciesBaseSpDefense(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].baseSpDefense;
}

u32 GetSpeciesBaseSpeed(u16 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].baseSpeed;
}

u32 GetSpeciesBaseStat(u16 species, u32 statIndex)
{
    switch (statIndex)
    {
    case STAT_HP:
        return GetSpeciesBaseHP(species);
    case STAT_ATK:
        return GetSpeciesBaseAttack(species);
    case STAT_DEF:
        return GetSpeciesBaseDefense(species);
    case STAT_SPEED:
        return GetSpeciesBaseSpeed(species);
    case STAT_SPATK:
        return GetSpeciesBaseSpAttack(species);
    case STAT_SPDEF:
        return GetSpeciesBaseSpDefense(species);
    }
    return 0;
}

const struct LevelUpMove *GetSpeciesLevelUpLearnset(u16 species)
{
    // TODO randomize gSpeciesInfo levelUpLearnset?
    const struct LevelUpMove *learnset = gSpeciesInfo[SanitizeSpeciesId(species)].levelUpLearnset;
    if (learnset == NULL)
        return gSpeciesInfo[SPECIES_NONE].levelUpLearnset;
    return learnset;
}

const u16 *GetSpeciesTeachableLearnset(u16 species)
{
    const u16 *learnset = gSpeciesInfo[SanitizeSpeciesId(species)].teachableLearnset;
    if (learnset == NULL)
        return gSpeciesInfo[SPECIES_NONE].teachableLearnset;
    return learnset;
}

const u16 *GetSpeciesEggMoves(u16 species)
{
    const u16 *learnset = gSpeciesInfo[SanitizeSpeciesId(species)].eggMoveLearnset;
    if (learnset == NULL)
        return gSpeciesInfo[SPECIES_NONE].eggMoveLearnset;
    return learnset;
}

const struct Evolution *GetSpeciesEvolutions(u16 species)
{
    const struct Evolution *evolutions = gSpeciesInfo[SanitizeSpeciesId(species)].evolutions;
    if (evolutions == NULL)
        return gSpeciesInfo[SPECIES_NONE].evolutions;
    return evolutions;
}

const u16 *GetSpeciesFormTable(u16 species)
{
    const u16 *formTable = gSpeciesInfo[SanitizeSpeciesId(species)].formSpeciesIdTable;
    if (formTable == NULL)
        return gSpeciesInfo[SPECIES_NONE].formSpeciesIdTable;
    return formTable;
}

const struct FormChange *GetSpeciesFormChanges(u16 species)
{
    const struct FormChange *formChanges = gSpeciesInfo[SanitizeSpeciesId(species)].formChangeTable;
    if (formChanges == NULL)
        return gSpeciesInfo[SPECIES_NONE].formChangeTable;
    return formChanges;
}

u8 CalculatePPWithBonus(u16 move, u8 ppBonuses, u8 moveIndex)
{
    u8 basePP = GetMovePP(move);
    return basePP + ((basePP * 20 * ((gPPUpGetMask[moveIndex] & ppBonuses) >> (2 * moveIndex))) / 100);
}

void RemoveMonPPBonus(struct Pokemon *mon, u8 moveIndex)
{
    u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses &= gPPUpClearMask[moveIndex];
    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void RemoveBattleMonPPBonus(struct BattlePokemon *mon, u8 moveIndex)
{
    mon->ppBonuses &= gPPUpClearMask[moveIndex];
}

void PokemonToBattleMon(struct Pokemon *src, struct BattlePokemon *dst)
{
    s32 i;
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        dst->moves[i] = GetMonData(src, MON_DATA_MOVE1 + i, NULL);
        dst->pp[i] = GetMonData(src, MON_DATA_PP1 + i, NULL);
    }

    dst->species = GetMonData(src, MON_DATA_SPECIES, NULL);
    dst->item = GetMonData(src, MON_DATA_HELD_ITEM, NULL);
    dst->ppBonuses = GetMonData(src, MON_DATA_PP_BONUSES, NULL);
    dst->friendship = GetMonData(src, MON_DATA_FRIENDSHIP, NULL);
    dst->experience = GetMonData(src, MON_DATA_EXP, NULL);
    dst->hpIV = GetMonData(src, MON_DATA_HP_IV, NULL);
    dst->attackIV = GetMonData(src, MON_DATA_ATK_IV, NULL);
    dst->defenseIV = GetMonData(src, MON_DATA_DEF_IV, NULL);
    dst->speedIV = GetMonData(src, MON_DATA_SPEED_IV, NULL);
    dst->spAttackIV = GetMonData(src, MON_DATA_SPATK_IV, NULL);
    dst->spDefenseIV = GetMonData(src, MON_DATA_SPDEF_IV, NULL);
    dst->personality = GetMonData(src, MON_DATA_PERSONALITY, NULL);
    dst->status1 = GetMonData(src, MON_DATA_STATUS, NULL);
    dst->level = GetMonData(src, MON_DATA_LEVEL, NULL);
    dst->hp = GetMonData(src, MON_DATA_HP, NULL);
    dst->maxHP = GetMonData(src, MON_DATA_MAX_HP, NULL);
    dst->attack = GetMonData(src, MON_DATA_ATK, NULL);
    dst->defense = GetMonData(src, MON_DATA_DEF, NULL);
    dst->speed = GetMonData(src, MON_DATA_SPEED, NULL);
    dst->spAttack = GetMonData(src, MON_DATA_SPATK, NULL);
    dst->spDefense = GetMonData(src, MON_DATA_SPDEF, NULL);
    dst->abilityNum = GetMonData(src, MON_DATA_ABILITY_NUM, NULL);
    dst->otId = GetMonData(src, MON_DATA_OT_ID, NULL);
    dst->types[0] = GetSpeciesType(dst->species, 0);
    dst->types[1] = GetSpeciesType(dst->species, 1);
    dst->types[2] = TYPE_MYSTERY;
    dst->isShiny = IsMonShiny(src);
    dst->ability = GetAbilityBySpecies(dst->species, dst->abilityNum);
    GetMonData(src, MON_DATA_NICKNAME, nickname);
    StringCopy_Nickname(dst->nickname, nickname);
    GetMonData(src, MON_DATA_OT_NAME, dst->otName);

    for (i = 0; i < NUM_BATTLE_STATS; i++)
        dst->statStages[i] = DEFAULT_STAT_STAGE;

    memset(&dst->volatiles, 0, sizeof(struct Volatiles));
}

void CopyPartyMonToBattleData(u32 battler, u32 partyIndex)
{
    u32 side = GetBattlerSide(battler);
    struct Pokemon *party = GetSideParty(side);
    PokemonToBattleMon(&party[partyIndex], &gBattleMons[battler]);
    gBattleStruct->hpOnSwitchout[side] = gBattleMons[battler].hp;
    UpdateSentPokesToOpponentValue(battler);
    ClearTemporarySpeciesSpriteData(battler, FALSE, FALSE);
}

bool8 ExecuteTableBasedItemEffect(struct Pokemon *mon, u16 item, u8 partyIndex, u8 moveIndex)
{
    return PokemonUseItemEffects(mon, item, partyIndex, moveIndex, FALSE);
}

#define UPDATE_FRIENDSHIP_FROM_ITEM()                                                                   \
{                                                                                                       \
    if ((retVal == 0 || friendshipOnly) && !ShouldSkipFriendshipChange() && friendshipChange == 0)      \
    {                                                                                                   \
        friendshipChange = itemEffect[itemEffectParam];                                                 \
        friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);                                        \
        friendship += CalculateFriendshipBonuses(mon,friendshipChange,holdEffect);                      \
        if (friendship < 0)                                                                             \
            friendship = 0;                                                                             \
        if (friendship > MAX_FRIENDSHIP)                                                                \
            friendship = MAX_FRIENDSHIP;                                                                \
        SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);                                              \
        retVal = FALSE;                                                                                 \
    }                                                                                                   \
}

// EXP candies store an index for this table in their holdEffectParam.
const u32 sExpCandyExperienceTable[] = {
    [EXP_100 - 1] = 100,
    [EXP_800 - 1] = 800,
    [EXP_3000 - 1] = 3000,
    [EXP_10000 - 1] = 10000,
    [EXP_30000 - 1] = 30000,
};

// Returns TRUE if the item has no effect on the Pokémon, FALSE otherwise
bool8 PokemonUseItemEffects(struct Pokemon *mon, u16 item, u8 partyIndex, u8 moveIndex, bool8 usedByAI)
{
    u32 dataUnsigned;
    s32 dataSigned, evCap;
    s32 friendship;
    s32 i;
    bool8 retVal = TRUE;
    const u8 *itemEffect;
    u8 itemEffectParam = ITEM_EFFECT_ARG_START;
    u32 temp1, temp2;
    s8 friendshipChange = 0;
    enum HoldEffect holdEffect;
    u8 battler = MAX_BATTLERS_COUNT;
    bool32 friendshipOnly = FALSE;
    u16 heldItem;
    u8 effectFlags;
    s8 evChange;
    u16 evCount;

    // Determine the EV cap to use
    u32 maxAllowedEVs = !B_EV_ITEMS_CAP ? MAX_TOTAL_EVS : GetCurrentEVCap();

    // Get item hold effect
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
    if (heldItem == ITEM_ENIGMA_BERRY_E_READER)
    #if FREE_ENIGMA_BERRY == FALSE
        holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
    #else
        holdEffect = 0;
    #endif //FREE_ENIGMA_BERRY
    else
        holdEffect = GetItemHoldEffect(heldItem);

    // Skip using the item if it won't do anything
    if (GetItemEffect(item) == NULL && item != ITEM_ENIGMA_BERRY_E_READER)
        return TRUE;

    // Get item effect
    itemEffect = GetItemEffect(item);

    // Do item effect
    for (i = 0; i < ITEM_EFFECT_ARG_START; i++)
    {
        switch (i)
        {

        // Handle ITEM0 effects (infatuation, Dire Hit, X Attack). ITEM0_SACRED_ASH is handled in party_menu.c
        // Now handled in item battle scripts.
        case 0:
            break;

        // Handle ITEM1 effects (in-battle stat boosting effects)
        // Now handled in item battle scripts.
        case 1:
            break;
        // Formerly used by the item effects of the X Sp. Atk and the X Accuracy
        case 2:
            break;

        // Handle ITEM3 effects (Guard Spec, Rare Candy, cure status)
        case 3:
            // Rare Candy / EXP Candy
            if ((itemEffect[i] & ITEM3_LEVEL_UP)
             && GetMonData(mon, MON_DATA_LEVEL, NULL) != GetCurrentPartyLevelCap())
            {
                u8 param = GetItemHoldEffectParam(item);
                dataUnsigned = 0;

                if (param == 0) // Rare Candy
                {
                    dataUnsigned = gExperienceTables[gSpeciesInfo[GetMonData(mon, MON_DATA_SPECIES, NULL)].growthRate][GetMonData(mon, MON_DATA_LEVEL, NULL) + 1];
                }
                else if (param - 1 < ARRAY_COUNT(sExpCandyExperienceTable)) // EXP Candies
                {
                    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
                    dataUnsigned = sExpCandyExperienceTable[param - 1] + GetMonData(mon, MON_DATA_EXP, NULL);

                    if (B_RARE_CANDY_CAP && B_EXP_CAP_TYPE == EXP_CAP_HARD)
                    {
                        u32 currentLevelCap = GetCurrentLevelCap();
                        if (dataUnsigned > gExperienceTables[gSpeciesInfo[species].growthRate][currentLevelCap])
                            dataUnsigned = gExperienceTables[gSpeciesInfo[species].growthRate][currentLevelCap];
                    }
                    else if (dataUnsigned > gExperienceTables[gSpeciesInfo[species].growthRate][MAX_LEVEL])
                    {
                        dataUnsigned = gExperienceTables[gSpeciesInfo[species].growthRate][MAX_LEVEL];
                    }
                }

                if (dataUnsigned != 0) // Failsafe
                {
                    SetMonData(mon, MON_DATA_EXP, &dataUnsigned);
                    CalculateMonStats(mon);
                    retVal = FALSE;
                }
            }

            // Cure status
            if ((itemEffect[i] & ITEM3_SLEEP) && HealStatusConditions(mon, STATUS1_SLEEP, battler) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_POISON) && HealStatusConditions(mon, STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER, battler) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_BURN) && HealStatusConditions(mon, STATUS1_BURN, battler) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_FREEZE) && HealStatusConditions(mon, STATUS1_FREEZE | STATUS1_FROSTBITE, battler) == 0)
                retVal = FALSE;
            if ((itemEffect[i] & ITEM3_PARALYSIS) && HealStatusConditions(mon, STATUS1_PARALYSIS, battler) == 0)
                retVal = FALSE;
            break;

        // Handle ITEM4 effects (Change HP/Atk EVs, HP heal, PP heal, PP up, Revive, and evolution stones)
        case 4:
            effectFlags = itemEffect[i];

            // PP Up
            if (effectFlags & ITEM4_PP_UP)
            {
                u32 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
                effectFlags &= ~ITEM4_PP_UP;
                dataUnsigned = (ppBonuses & gPPUpGetMask[moveIndex]) >> (moveIndex * 2);
                temp1 = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), ppBonuses, moveIndex);
                if (dataUnsigned <= 2 && temp1 > 4)
                {
                    dataUnsigned = ppBonuses + gPPUpAddValues[moveIndex];
                    SetMonData(mon, MON_DATA_PP_BONUSES, &dataUnsigned);

                    dataUnsigned = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), dataUnsigned, moveIndex) - temp1;
                    dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL) + dataUnsigned;
                    SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                    retVal = FALSE;
                }
            }
            temp1 = 0;

            // Loop through and try each of the remaining ITEM4 effects
            while (effectFlags != 0)
            {
                if (effectFlags & 1)
                {
                    switch (temp1)
                    {
                    case 0: // ITEM4_EV_HP
                    case 1: // ITEM4_EV_ATK
                        evCount = GetMonEVCount(mon);
                        temp2 = itemEffect[itemEffectParam];
                        dataSigned = GetMonData(mon, sGetMonDataEVConstants[temp1], NULL);
                        evChange = temp2;

                        if (evChange > 0) // Increasing EV (HP or Atk)
                        {
                            // Check if the total EV limit is reached
                            if (evCount >= maxAllowedEVs)
                                return TRUE;

                            // Ensure the increase does not exceed the max EV per stat (252)
                            evCap = (itemEffect[10] & ITEM10_IS_VITAMIN) ? EV_ITEM_RAISE_LIMIT : MAX_PER_STAT_EVS;

                            // Check if the per-stat limit is reached
                            if (dataSigned >= evCap)
                                return TRUE;  // Prevents item use if the per-stat cap is already reached

                            if (dataSigned + evChange > evCap)
                                temp2 = evCap - dataSigned;
                            else
                                temp2 = evChange;

                            // Ensure the total EVs do not exceed the maximum allowed (510)
                            if (evCount + temp2 > maxAllowedEVs)
                                temp2 = maxAllowedEVs - evCount;

                            // Prevent item use if no EVs can be increased
                            if (temp2 == 0)
                                return TRUE;

                            // Apply the EV increase
                            dataSigned += temp2;
                        }
                        else if (evChange < 0) // Decreasing EV (HP or Atk)
                        {
                            if (dataSigned == 0)
                            {
                                // No EVs to lose, but make sure friendship updates anyway
                                friendshipOnly = TRUE;
                                itemEffectParam++;
                                break;
                            }
                            dataSigned += evChange;
                            if (I_BERRY_EV_JUMP == GEN_4 && dataSigned > 100)
                                dataSigned = 100;
                            if (dataSigned < 0)
                                dataSigned = 0;
                        }
                        else // Reset EV (HP or Atk)
                        {
                            if (dataSigned == 0)
                                break;

                            dataSigned = 0;
                        }

                        // Update EVs and stats
                        SetMonData(mon, sGetMonDataEVConstants[temp1], &dataSigned);
                        CalculateMonStats(mon);
                        itemEffectParam++;
                        retVal = FALSE;
                        break;

                    case 2: // ITEM4_HEAL_HP
                    {
                        u32 currentHP = GetMonData(mon, MON_DATA_HP, NULL);
                        u32 maxHP = GetMonData(mon, MON_DATA_MAX_HP, NULL);
                        // Check use validity.
                        if ((effectFlags & (ITEM4_REVIVE >> 2) && currentHP != 0)
                              || (!(effectFlags & (ITEM4_REVIVE >> 2)) && currentHP == 0))
                        {
                            itemEffectParam++;
                            break;
                        }

                        // Get amount of HP to restore
                        dataUnsigned = itemEffect[itemEffectParam++];
                        switch (dataUnsigned)
                        {
                        case ITEM6_HEAL_HP_FULL:
                            dataUnsigned = maxHP - currentHP;
                            break;
                        case ITEM6_HEAL_HP_HALF:
                            dataUnsigned = maxHP / 2;
                            if (dataUnsigned == 0)
                                dataUnsigned = 1;
                            break;
                        case ITEM6_HEAL_HP_LVL_UP:
                            dataUnsigned = gBattleScripting.levelUpHP;
                            break;
                        case ITEM6_HEAL_HP_QUARTER:
                            dataUnsigned = maxHP / 4;
                            if (dataUnsigned == 0)
                                dataUnsigned = 1;
                            break;
                        }

                        // Only restore HP if not at max health
                        if (maxHP != currentHP)
                        {
                            // Restore HP
                            dataUnsigned = currentHP + dataUnsigned;
                            if (dataUnsigned > maxHP)
                                dataUnsigned = maxHP;
                            SetMonData(mon, MON_DATA_HP, &dataUnsigned);
                            retVal = FALSE;
                        }
                        effectFlags &= ~(ITEM4_REVIVE >> 2);
                        break;
                    }
                    case 3: // ITEM4_HEAL_PP
                        if (!(effectFlags & (ITEM4_HEAL_PP_ONE >> 3)))
                        {
                            // Heal PP for all moves
                            for (temp2 = 0; (signed)(temp2) < (signed)(MAX_MON_MOVES); temp2++)
                            {
                                u32 move, ppBonus;
                                dataUnsigned = GetMonData(mon, MON_DATA_PP1 + temp2, NULL);
                                move = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL);
                                ppBonus = CalculatePPWithBonus(move, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2);
                                if (dataUnsigned != ppBonus)
                                {
                                    dataUnsigned += itemEffect[itemEffectParam];
                                    if (dataUnsigned > ppBonus)
                                        dataUnsigned = ppBonus;
                                    SetMonData(mon, MON_DATA_PP1 + temp2, &dataUnsigned);
                                    retVal = FALSE;
                                }
                            }
                            itemEffectParam++;
                        }
                        else
                        {
                            // Heal PP for one move
                            u16 move;
                            dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL);
                            move = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL);
                            u32 ppBonus = CalculatePPWithBonus(move, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                            if (dataUnsigned != ppBonus)
                            {
                                dataUnsigned += itemEffect[itemEffectParam++];
                                if (dataUnsigned > ppBonus)
                                    dataUnsigned = ppBonus;
                                SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                                retVal = FALSE;
                            }
                        }
                        break;

                    // cases 4-6 are ITEM4_HEAL_PP_ONE, ITEM4_PP_UP, and ITEM4_REVIVE, which
                    // are already handled above by other cases or before the loop

                    case 7: // ITEM4_EVO_STONE
                        {
                            bool32 canStopEvo = TRUE;
                            u32 targetSpecies = GetEvolutionTargetSpecies(mon, EVO_MODE_ITEM_USE, item, NULL, &canStopEvo, CHECK_EVO);

                            if (targetSpecies != SPECIES_NONE)
                            {
                                GetEvolutionTargetSpecies(mon, EVO_MODE_ITEM_USE, item, NULL, &canStopEvo, DO_EVO);
                                BeginEvolutionScene(mon, targetSpecies, canStopEvo, partyIndex);
                                return FALSE;
                            }
                        }
                        break;
                    }
                }
                temp1++;
                effectFlags >>= 1;
            }
            break;

        // Handle ITEM5 effects (Change Def/SpDef/SpAtk/Speed EVs, PP Max, and friendship changes)
        case 5:
            effectFlags = itemEffect[i];
            temp1 = 0;

            // Loop through and try each of the ITEM5 effects
            while (effectFlags != 0)
            {
                if (effectFlags & 1)
                {
                    switch (temp1)
                    {
                    case 0: // ITEM5_EV_DEF
                    case 1: // ITEM5_EV_SPEED
                    case 2: // ITEM5_EV_SPDEF
                    case 3: // ITEM5_EV_SPATK
                        evCount = GetMonEVCount(mon);
                        temp2 = itemEffect[itemEffectParam];
                        dataSigned = GetMonData(mon, sGetMonDataEVConstants[temp1 + 2], NULL);
                        evChange = temp2;
                        if (evChange > 0) // Increasing EV
                        {
                            // Check if the total EV limit is reached
                            if (evCount >= maxAllowedEVs)
                                return TRUE;

                            // Ensure the increase does not exceed the max EV per stat (252)
                            evCap = (itemEffect[10] & ITEM10_IS_VITAMIN) ? EV_ITEM_RAISE_LIMIT : MAX_PER_STAT_EVS;

                            // Check if the per-stat limit is reached
                            if (dataSigned >= evCap)
                                return TRUE;  // Prevents item use if the per-stat cap is already reached

                            if (dataSigned + evChange > evCap)
                                temp2 = evCap - dataSigned;
                            else
                                temp2 = evChange;

                            // Ensure the total EVs do not exceed the maximum allowed (510)
                            if (evCount + temp2 > maxAllowedEVs)
                                temp2 = maxAllowedEVs - evCount;

                            // Prevent item use if no EVs can be increased
                            if (temp2 == 0)
                                return TRUE;

                            // Apply the EV increase
                            dataSigned += temp2;
                        }
                        else if (evChange < 0) // Decreasing EV
                        {
                            if (dataSigned == 0)
                            {
                                // No EVs to lose, but make sure friendship updates anyway
                                friendshipOnly = TRUE;
                                itemEffectParam++;
                                break;
                            }
                            dataSigned += evChange;
                            if (I_BERRY_EV_JUMP == GEN_4 && dataSigned > 100)
                                dataSigned = 100;
                            if (dataSigned < 0)
                                dataSigned = 0;
                        }
                        else // Reset EV
                        {
                            if (dataSigned == 0)
                                break;

                            dataSigned = 0;
                        }

                        // Update EVs and stats
                        SetMonData(mon, sGetMonDataEVConstants[temp1 + 2], &dataSigned);
                        CalculateMonStats(mon);
                        retVal = FALSE;
                        itemEffectParam++;
                        break;

                    case 4: // ITEM5_PP_MAX
                    {
                        u32 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
                        dataUnsigned = (ppBonuses & gPPUpGetMask[moveIndex]) >> (moveIndex * 2);
                        temp2 = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), ppBonuses, moveIndex);

                        // Check if 3 PP Ups have been applied already, and that the move has a total PP of at least 5 (excludes Sketch)
                        if (dataUnsigned < 3 && temp2 >= 5)
                        {
                            dataUnsigned = ppBonuses;
                            dataUnsigned &= gPPUpClearMask[moveIndex];
                            dataUnsigned += gPPUpAddValues[moveIndex] * 3; // Apply 3 PP Ups (max)

                            SetMonData(mon, MON_DATA_PP_BONUSES, &dataUnsigned);
                            dataUnsigned = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), dataUnsigned, moveIndex) - temp2;
                            dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL) + dataUnsigned;
                            SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                            retVal = FALSE;
                        }
                        break;
                    }
                    case 5: // ITEM5_FRIENDSHIP_LOW
                        // Changes to friendship are given differently depending on
                        // how much friendship the Pokémon already has.
                        // In general, Pokémon with lower friendship receive more,
                        // and Pokémon with higher friendship receive less.
                        if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) < 100)
                            UPDATE_FRIENDSHIP_FROM_ITEM();
                        itemEffectParam++;
                        break;

                    case 6: // ITEM5_FRIENDSHIP_MID
                        if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) >= 100 && GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) < 200)
                            UPDATE_FRIENDSHIP_FROM_ITEM();
                        itemEffectParam++;
                        break;

                    case 7: // ITEM5_FRIENDSHIP_HIGH
                        if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) >= 200)
                            UPDATE_FRIENDSHIP_FROM_ITEM();
                        itemEffectParam++;
                        break;
                    }
                }
                temp1++;
                effectFlags >>= 1;
            }
            break;
        }
    }
    return retVal;
}

bool8 HealStatusConditions(struct Pokemon *mon, u32 healMask, u8 battler)
{
    u32 status = GetMonData(mon, MON_DATA_STATUS, 0);

    if (status & healMask)
    {
        status &= ~healMask;
        SetMonData(mon, MON_DATA_STATUS, &status);
        if (gMain.inBattle && battler != MAX_BATTLERS_COUNT)
        {
            gBattleMons[battler].status1 &= ~healMask;
            if((healMask & STATUS1_SLEEP))
            {
                u32 i = 0;
                u32 battlerSide = GetBattlerSide(battler);
                struct Pokemon *party = GetSideParty(battlerSide);

                for (i = 0; i < PARTY_SIZE; i++)
                {
                    if (&party[i] == mon)
                    {
                        TryDeactivateSleepClause(battlerSide, i);
                        break;
                    }
                }
            }
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

u8 GetItemEffectParamOffset(u32 battler, u16 itemId, u8 effectByte, u8 effectBit)
{
    const u8 *temp;
    const u8 *itemEffect;
    u8 offset;
    int i;
    u8 j;
    u8 effectFlags;

    offset = ITEM_EFFECT_ARG_START;

    temp = GetItemEffect(itemId);

    if (temp != NULL && !temp && itemId != ITEM_ENIGMA_BERRY_E_READER)
        return 0;

    if (itemId == ITEM_ENIGMA_BERRY_E_READER)
    {
        temp = gEnigmaBerries[battler].itemEffect;
    }

    itemEffect = temp;

    for (i = 0; i < ITEM_EFFECT_ARG_START; i++)
    {
        switch (i)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            if (i == effectByte)
                return 0;
            break;
        case 4:
            effectFlags = itemEffect[4];
            if (effectFlags & ITEM4_PP_UP)
                effectFlags &= ~(ITEM4_PP_UP);
            j = 0;
            while (effectFlags)
            {
                if (effectFlags & 1)
                {
                    switch (j)
                    {
                    case 2: // ITEM4_HEAL_HP
                        if (effectFlags & (ITEM4_REVIVE >> 2))
                            effectFlags &= ~(ITEM4_REVIVE >> 2);
                        // fallthrough
                    case 0: // ITEM4_EV_HP
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 1: // ITEM4_EV_ATK
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 3: // ITEM4_HEAL_PP
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 7: // ITEM4_EVO_STONE
                        if (i == effectByte)
                            return 0;
                        break;
                    }
                }
                j++;
                effectFlags >>= 1;
                if (i == effectByte)
                    effectBit >>= 1;
            }
            break;
        case 5:
            effectFlags = itemEffect[5];
            j = 0;
            while (effectFlags)
            {
                if (effectFlags & 1)
                {
                    switch (j)
                    {
                    case 0: // ITEM5_EV_DEF
                    case 1: // ITEM5_EV_SPEED
                    case 2: // ITEM5_EV_SPDEF
                    case 3: // ITEM5_EV_SPATK
                    case 4: // ITEM5_PP_MAX
                    case 5: // ITEM5_FRIENDSHIP_LOW
                    case 6: // ITEM5_FRIENDSHIP_MID
                        if (i == effectByte && (effectFlags & effectBit))
                            return offset;
                        offset++;
                        break;
                    case 7: // ITEM5_FRIENDSHIP_HIGH
                        if (i == effectByte)
                            return 0;
                        break;
                    }
                }
                j++;
                effectFlags >>= 1;
                if (i == effectByte)
                    effectBit >>= 1;
            }
            break;
        }
    }

    return offset;
}

static void BufferStatRoseMessage(enum Stat statIdx)
{
    gBattlerTarget = gBattlerInMenuId;
    StringCopy(gBattleTextBuff1, gStatNamesTable[sStatsToRaise[statIdx]]);
    if (B_X_ITEMS_BUFF >= GEN_7)
    {
        StringCopy(gBattleTextBuff2, gText_StatSharply);
        StringAppend(gBattleTextBuff2, gText_StatRose);
    }
    else
    {
        StringCopy(gBattleTextBuff2, gText_StatRose);
    }
    BattleStringExpandPlaceholdersToDisplayedString(gText_DefendersStatRose);
}

u8 *UseStatIncreaseItem(u16 itemId)
{
    const u8 *itemEffect;

    if (itemId == ITEM_ENIGMA_BERRY_E_READER)
    {
        if (gMain.inBattle)
            itemEffect = gEnigmaBerries[gBattlerInMenuId].itemEffect;
        else
        #if FREE_ENIGMA_BERRY == FALSE
            itemEffect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
        #else
            itemEffect = 0;
        #endif //FREE_ENIGMA_BERRY
    }
    else
    {
        itemEffect = GetItemEffect(itemId);
    }

    gPotentialItemEffectBattler = gBattlerInMenuId;

    if (itemEffect[0] & ITEM0_DIRE_HIT)
    {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnGettingPumped);
    }

    switch (itemEffect[1])
    {
        case ITEM1_X_ATTACK:
            BufferStatRoseMessage(STAT_ATK);
            break;
        case ITEM1_X_DEFENSE:
            BufferStatRoseMessage(STAT_DEF);
            break;
        case ITEM1_X_SPEED:
            BufferStatRoseMessage(STAT_SPEED);
            break;
        case ITEM1_X_SPATK:
            BufferStatRoseMessage(STAT_SPATK);
            break;
        case ITEM1_X_SPDEF:
            BufferStatRoseMessage(STAT_SPDEF);
            break;
        case ITEM1_X_ACCURACY:
            BufferStatRoseMessage(STAT_ACC);
            break;
    }

    if (itemEffect[3] & ITEM3_GUARD_SPEC)
    {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnShroudedInMist);
    }

    return gDisplayedStringBattle;
}

u8 GetNature(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_PERSONALITY, 0) % NUM_NATURES;
}

u8 GetNatureFromPersonality(u32 personality)
{
    return personality % NUM_NATURES;
}

u32 GetGMaxTargetSpecies(u32 species)
{
    const struct FormChange *formChanges = GetSpeciesFormChanges(species);
    u32 i;
    for (i = 0; formChanges != NULL && formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (formChanges[i].method == FORM_CHANGE_BATTLE_GIGANTAMAX)
            return formChanges[i].targetSpecies;
    }
    return species;
}

bool32 DoesMonMeetAdditionalConditions(struct Pokemon *mon, const struct EvolutionParam *params, struct Pokemon *tradePartner, u32 partyId, bool32 *canStopEvo, enum EvoState evoState)
{
    u32 i, j;
    u32 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
    u32 gender = GetMonGender(mon);
    u32 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, 0);
    u32 attack = GetMonData(mon, MON_DATA_ATK, 0);
    u32 defense = GetMonData(mon, MON_DATA_DEF, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    u16 upperPersonality = personality >> 16;
    u32 weather = GetCurrentWeather();
    u32 nature = GetNature(mon);
    bool32 removeHoldItem = FALSE;
    u32 removeBagItem = ITEM_NONE;
    u32 removeBagItemCount = 0;
    u32 evolutionTracker = GetMonData(mon, MON_DATA_EVOLUTION_TRACKER, 0);
    u32 partnerSpecies, partnerHeldItem;
    enum HoldEffect partnerHoldEffect;

    if (tradePartner != NULL)
    {
        partnerSpecies = GetMonData(tradePartner, MON_DATA_SPECIES, 0);
        partnerHeldItem = GetMonData(tradePartner, MON_DATA_HELD_ITEM, 0);

        if (partnerHeldItem == ITEM_ENIGMA_BERRY_E_READER)
        #if FREE_ENIGMA_BERRY == FALSE
            partnerHoldEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
        #else
            partnerHoldEffect = 0;
        #endif //FREE_ENIGMA_BERRY
        else
            partnerHoldEffect = GetItemHoldEffect(partnerHeldItem);
    }
    else
    {
        partnerSpecies = SPECIES_NONE;
        partnerHeldItem = ITEM_NONE;
        partnerHoldEffect = HOLD_EFFECT_NONE;
    }

    // Check for additional conditions (only if the primary method passes). Skips if there's no additional conditions.
    for (i = 0; params != NULL && params[i].condition != CONDITIONS_END; i++)
    {
        enum EvolutionConditions condition = params[i].condition;
        bool32 currentCondition = FALSE;

        switch(condition)
        {
        // Gen 2
        case IF_GENDER:
            if (gender == params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_MIN_FRIENDSHIP:
            if (friendship >= params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_ATK_GT_DEF:
            if (attack > defense)
                currentCondition = TRUE;
            break;
        case IF_ATK_EQ_DEF:
            if (attack == defense)
                currentCondition = TRUE;
            break;
        case IF_ATK_LT_DEF:
            if (attack < defense)
                currentCondition = TRUE;
            break;
        case IF_TIME:
            if (GetTimeOfDay() == params[i].arg1)
                currentCondition = TRUE;

            break;
        case IF_NOT_TIME:
            if (GetTimeOfDay() != params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_HOLD_ITEM:
            if (heldItem == params[i].arg1)
            {
                currentCondition = TRUE;
                removeHoldItem = TRUE;
            }
            break;
        // Gen 3
        case IF_PID_UPPER_MODULO_10_GT:
            if ((upperPersonality % 10) > params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_PID_UPPER_MODULO_10_EQ:
            if ((upperPersonality % 10) == params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_PID_UPPER_MODULO_10_LT:
            if ((upperPersonality % 10) < params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_MIN_BEAUTY:
        {
            u32 beauty = GetMonData(mon, MON_DATA_BEAUTY, 0);
            if (beauty >= params[i].arg1)
                currentCondition = TRUE;
            break;
        }
        case IF_MIN_COOLNESS:
        {
            u32 coolness = GetMonData(mon, MON_DATA_COOL, 0);
            if (coolness >= params[i].arg1)
                currentCondition = TRUE;
            break;
        }
        case IF_MIN_SMARTNESS:
        // remember that even though it's called "Smart/Smartness" here,
        // from gen 6 and up it's known as "Clever/Cleverness."
        {
            u32 smartness = GetMonData(mon, MON_DATA_SMART, 0);
            if (smartness >= params[i].arg1)
                currentCondition = TRUE;
            break;
        }
        case IF_MIN_TOUGHNESS:
        {
            u32 toughness = GetMonData(mon, MON_DATA_TOUGH, 0);
            if (toughness >= params[i].arg1)
                currentCondition = TRUE;
            break;
        }
        case IF_MIN_CUTENESS:
        {
            u32 cuteness = GetMonData(mon, MON_DATA_CUTE, 0);
            if (cuteness >= params[i].arg1)
                currentCondition = TRUE;
            break;
        }
        // Gen 4
        case IF_SPECIES_IN_PARTY:
            for (j = 0; j < PARTY_SIZE; j++)
            {
                if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) == params[i].arg1)
                {
                    currentCondition = TRUE;
                    break;
                }
            }
            break;
        case IF_IN_MAP:
            if (params[i].arg1 == ((gSaveBlock1Ptr->location.mapGroup) << 8 | gSaveBlock1Ptr->location.mapNum))
                currentCondition = TRUE;
            break;
        case IF_IN_MAPSEC:
            if (gMapHeader.regionMapSectionId == params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_KNOWS_MOVE:
            if (MonKnowsMove(mon, params[i].arg1))
                currentCondition = TRUE;
            break;
        // Gen 5
        case IF_TRADE_PARTNER_SPECIES:
            if (params[i].arg1 == partnerSpecies && partnerHoldEffect != HOLD_EFFECT_PREVENT_EVOLVE)
                currentCondition = TRUE;
            break;
        // Gen 6
        case IF_TYPE_IN_PARTY:
            for (j = 0; j < PARTY_SIZE; j++)
            {
                u16 currSpecies = GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL);
                if (GetSpeciesType(currSpecies, 0) == params[i].arg1
                 || GetSpeciesType(currSpecies, 1) == params[i].arg1)
                {
                    currentCondition = TRUE;
                    break;
                }
            }
            break;
        case IF_WEATHER:
            if (params[i].arg1 == WEATHER_RAIN)
            {
                if (weather == WEATHER_RAIN || weather == WEATHER_RAIN_THUNDERSTORM || weather == WEATHER_DOWNPOUR)
                    currentCondition = TRUE;
            }
            else if (params[i].arg1 == WEATHER_FOG)
            {
                if (weather == WEATHER_FOG_DIAGONAL || weather == WEATHER_FOG_HORIZONTAL)
                    currentCondition = TRUE;
            }
            else if (weather == params[i].arg1)
            {
                currentCondition = TRUE;
            }
            break;
        case IF_KNOWS_MOVE_TYPE:
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                if (GetMoveType(GetMonData(mon, MON_DATA_MOVE1 + j, NULL)) == params[i].arg1)
                {
                    currentCondition = TRUE;
                    break;
                }
            }
            break;
        // Gen 8
        case IF_NATURE:
            if (nature == params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_AMPED_NATURE:
            switch (nature)
            {
            case NATURE_HARDY:
            case NATURE_BRAVE:
            case NATURE_ADAMANT:
            case NATURE_NAUGHTY:
            case NATURE_DOCILE:
            case NATURE_IMPISH:
            case NATURE_LAX:
            case NATURE_HASTY:
            case NATURE_JOLLY:
            case NATURE_NAIVE:
            case NATURE_RASH:
            case NATURE_SASSY:
            case NATURE_QUIRKY:
                currentCondition = TRUE;
                break;
            }
            break;
        case IF_LOW_KEY_NATURE:
            switch (nature)
            {
            case NATURE_LONELY:
            case NATURE_BOLD:
            case NATURE_RELAXED:
            case NATURE_TIMID:
            case NATURE_SERIOUS:
            case NATURE_MODEST:
            case NATURE_MILD:
            case NATURE_QUIET:
            case NATURE_BASHFUL:
            case NATURE_CALM:
            case NATURE_GENTLE:
            case NATURE_CAREFUL:
                currentCondition = TRUE;
                break;
            }
            break;
        case IF_RECOIL_DAMAGE_GE:
            if (evolutionTracker >= params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_CURRENT_DAMAGE_GE:
        {
            u32 currentHp = GetMonData(mon, MON_DATA_HP, NULL);
            if (currentHp != 0 && (GetMonData(mon, MON_DATA_MAX_HP, NULL) - currentHp >= params[i].arg1))
                currentCondition = TRUE;
            break;
        }
        case IF_CRITICAL_HITS_GE:
            if (partyId != PARTY_SIZE && gPartyCriticalHits[partyId] >= params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_USED_MOVE_X_TIMES:
            if (evolutionTracker >= params[i].arg2)
                currentCondition = TRUE;
            break;
        // Gen 9
        case IF_DEFEAT_X_WITH_ITEMS:
            if (evolutionTracker >= params[i].arg3)
                currentCondition = TRUE;
            break;
        case IF_PID_MODULO_100_GT:
            if ((personality % 100) > params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_PID_MODULO_100_EQ:
            if ((personality % 100) == params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_PID_MODULO_100_LT:
            if ((personality % 100) < params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_MIN_OVERWORLD_STEPS:
            if (mon == GetFirstLiveMon() && gFollowerSteps >= params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_BAG_ITEM_COUNT:
            if (CheckBagHasItem(params[i].arg1, params[i].arg2))
            {
                currentCondition = TRUE;
                removeBagItem = params[i].arg1;
                removeBagItemCount = params[i].arg2;
                if (canStopEvo != NULL)
                    *canStopEvo = FALSE;
            }
            break;
        case IF_REGION:
            if (GetCurrentRegion() == params[i].arg1)
                currentCondition = TRUE;
            break;
        case IF_NOT_REGION:
            if (GetCurrentRegion() != params[i].arg1)
                currentCondition = TRUE;
            break;
        case CONDITIONS_END:
            break;
        }

        // check if an evolution is about to happen and items should be removed
        if (evoState == DO_EVO)
        {
            if (removeHoldItem)
            {
                u32 heldItem = ITEM_NONE;
                SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
            }

            if (removeBagItem != ITEM_NONE)
                RemoveBagItem(removeBagItem, removeBagItemCount);
        }

        if (currentCondition == FALSE)
            return FALSE;
    }

    return TRUE;
}

u32 GetEvolutionTargetSpecies(struct Pokemon *mon, enum EvolutionMode mode, u16 evolutionItem, struct Pokemon *tradePartner, bool32 *canStopEvo, enum EvoState evoState)
{
    int i;
    u32 targetSpecies = SPECIES_NONE;
    u32 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u32 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);
    u32 level = GetMonData(mon, MON_DATA_LEVEL, 0);
    enum HoldEffect holdEffect;
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);

    if (evolutions == NULL)
        return SPECIES_NONE;

    if (heldItem == ITEM_ENIGMA_BERRY_E_READER)
    #if FREE_ENIGMA_BERRY == FALSE
        holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
    #else
        holdEffect = 0;
    #endif //FREE_ENIGMA_BERRY
    else
        holdEffect = GetItemHoldEffect(heldItem);

    // Prevent evolution with Everstone, unless we're just viewing the party menu with an evolution item
    if (holdEffect == HOLD_EFFECT_PREVENT_EVOLVE
        && mode != EVO_MODE_ITEM_CHECK
        && (P_KADABRA_EVERSTONE < GEN_4 || species != SPECIES_KADABRA))
        return SPECIES_NONE;

    switch (mode)
    {
    case EVO_MODE_NORMAL:
    case EVO_MODE_BATTLE_ONLY:
        for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            bool32 conditionsMet = FALSE;
            if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
                continue;

            // Check main primary evolution method
            switch (evolutions[i].method)
            {
            case EVO_LEVEL:
                if (evolutions[i].param <= level)
                    conditionsMet = TRUE;
                break;
            case EVO_LEVEL_BATTLE_ONLY:
                if (mode == EVO_MODE_BATTLE_ONLY && evolutions[i].param <= level)
                    conditionsMet = TRUE;
                break;
            }

            if (conditionsMet && DoesMonMeetAdditionalConditions(mon, evolutions[i].params, NULL, PARTY_SIZE, canStopEvo, evoState))
            {
                // All checks passed, so stop checking the rest of the evolutions.
                // This is different from vanilla where the loop continues.
                // If you have overlapping evolutions, put the ones you want to happen first on top of the list.
                targetSpecies = evolutions[i].targetSpecies;
                break;
            }
        }
        break;
    case EVO_MODE_TRADE:
        for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            bool32 conditionsMet = FALSE;
            if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
                continue;

            switch (evolutions[i].method)
            {
            case EVO_TRADE:
                conditionsMet = TRUE;
                break;
            }

            if (conditionsMet && DoesMonMeetAdditionalConditions(mon, evolutions[i].params, tradePartner, PARTY_SIZE, canStopEvo, evoState))
            {
                // All checks passed, so stop checking the rest of the evolutions.
                // This is different from vanilla where the loop continues.
                // If you have overlapping evolutions, put the ones you want to happen first on top of the list.
                targetSpecies = evolutions[i].targetSpecies;
                break;
            }
        }
        break;
    case EVO_MODE_ITEM_USE:
    case EVO_MODE_ITEM_CHECK:
        for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            bool32 conditionsMet = FALSE;
            if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
                continue;

            switch (evolutions[i].method)
            {
            case EVO_ITEM:
                if (evolutions[i].param == evolutionItem)
                    conditionsMet = TRUE;
                break;
            }

            if (conditionsMet && DoesMonMeetAdditionalConditions(mon, evolutions[i].params, NULL, PARTY_SIZE, canStopEvo, evoState))
            {
                // All checks passed, so stop checking the rest of the evolutions.
                // This is different from vanilla where the loop continues.
                // If you have overlapping evolutions, put the ones you want to happen first on top of the list.
                targetSpecies = evolutions[i].targetSpecies;
                if (canStopEvo != NULL)
                    *canStopEvo = FALSE;
                break;
            }
        }
        break;
    // Battle evolution without leveling; party slot is being passed into the evolutionItem arg.
    case EVO_MODE_BATTLE_SPECIAL:
        for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            bool32 conditionsMet = FALSE;
            if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
                continue;

            switch (evolutions[i].method)
            {
            case EVO_BATTLE_END:
                conditionsMet = TRUE;
                break;
            }

            if (conditionsMet && DoesMonMeetAdditionalConditions(mon, evolutions[i].params, NULL, evolutionItem, canStopEvo, evoState))
            {
                // All checks passed, so stop checking the rest of the evolutions.
                // This is different from vanilla where the loop continues.
                // If you have overlapping evolutions, put the ones you want to happen first on top of the list.
                targetSpecies = evolutions[i].targetSpecies;
                break;
            }
        }
        break;
    // Overworld evolution without leveling; evolution method is being passed into the evolutionItem arg.
    case EVO_MODE_OVERWORLD_SPECIAL:
        for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            bool32 conditionsMet = FALSE;
            if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
                continue;

            switch (evolutions[i].method)
            {
            case EVO_SPIN:
                if (gSpecialVar_0x8000 == evolutions[i].param)
                    conditionsMet = TRUE;
                break;
            }

            if (conditionsMet && DoesMonMeetAdditionalConditions(mon, evolutions[i].params, NULL, PARTY_SIZE, canStopEvo, evoState))
            {
                // All checks passed, so stop checking the rest of the evolutions.
                // This is different from vanilla where the loop continues.
                // If you have overlapping evolutions, put the ones you want to happen first on top of the list.
                targetSpecies = evolutions[i].targetSpecies;
                break;
            }
        }
        break;
    case EVO_MODE_SCRIPT_TRIGGER:
        for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
                continue;
            if (evolutions[i].method != EVO_SCRIPT_TRIGGER)
                continue;
            if (DoesMonMeetAdditionalConditions(mon, evolutions[i].params, NULL, PARTY_SIZE, canStopEvo, evoState))
            {
                // All checks passed, so stop checking the rest of the evolutions.
                // This is different from vanilla where the loop continues.
                // If you have overlapping evolutions, put the ones you want to happen first on top of the list.
                targetSpecies = evolutions[i].targetSpecies;
                break;
            }
        }
        break;
    }

    // Pikachu, Meowth, Eevee and Duraludon cannot evolve if they have the
    // Gigantamax Factor. We assume that is because their evolutions
    // do not have a Gigantamax Form.
    if (GetMonData(mon, MON_DATA_GIGANTAMAX_FACTOR, NULL)
     && GetGMaxTargetSpecies(species) != species
     && GetGMaxTargetSpecies(targetSpecies) == targetSpecies)
    {
        return SPECIES_NONE;
    }

    return targetSpecies;
}

bool8 IsMonPastEvolutionLevel(struct Pokemon *mon)
{
    int i;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, 0);
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);

    if (evolutions == NULL)
        return FALSE;

    for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
    {
        if (SanitizeSpeciesId(evolutions[i].targetSpecies) == SPECIES_NONE)
            continue;

        switch (evolutions[i].method)
        {
        case EVO_LEVEL:
            if (evolutions[i].param <= level)
                return TRUE;
            break;
        }
    }

    return FALSE;
}

u16 NationalPokedexNumToSpecies(enum NationalDexOrder nationalNum)
{
    u16 species;

    if (!nationalNum)
        return 0;

    species = 1;

    while (species < (NUM_SPECIES) && gSpeciesInfo[species].natDexNum != nationalNum)
        species++;

    if (species == NUM_SPECIES)
        return NATIONAL_DEX_NONE;

    return GET_BASE_SPECIES_ID(species);
}

enum HoennDexOrder NationalToHoennOrder(enum NationalDexOrder nationalNum)
{
    u16 hoennNum;

    if (!nationalNum)
        return 0;

    hoennNum = 0;

    while (hoennNum < (HOENN_DEX_COUNT - 1) && sHoennToNationalOrder[hoennNum] != nationalNum)
        hoennNum++;

    if (hoennNum >= HOENN_DEX_COUNT - 1)
        return 0;

    return hoennNum + 1;
}

enum NationalDexOrder SpeciesToNationalPokedexNum(u16 species)
{
    species = SanitizeSpeciesId(species);
    if (!species)
        return NATIONAL_DEX_NONE;

    return gSpeciesInfo[species].natDexNum;
}

enum HoennDexOrder SpeciesToHoennPokedexNum(u16 species)
{
    if (!species)
        return 0;
    return NationalToHoennOrder(gSpeciesInfo[species].natDexNum);
}

enum NationalDexOrder HoennToNationalOrder(enum HoennDexOrder hoennNum)
{
    if (!hoennNum || hoennNum >= HOENN_DEX_COUNT)
        return 0;

    return sHoennToNationalOrder[hoennNum - 1];
}

// Spots can be drawn on Spinda's color indexes 1, 2, or 3
#define FIRST_SPOT_COLOR 1
#define LAST_SPOT_COLOR  3

// To draw a spot pixel, add 4 to the color index
#define SPOT_COLOR_ADJUSTMENT 4
/*
    The function below handles drawing the randomly-placed spots on Spinda's front sprite.
    Spinda has 4 spots, each with an entry in gSpindaSpotGraphics. Each entry contains
    a base x and y coordinate for the spot and a 16x16 binary image. Each bit in the image
    determines whether that pixel should be considered part of the spot.

    The position of each spot is randomized using the Spinda's personality. The entire 32 bit
    personality value is used, 4 bits for each coordinate of the 4 spots. If the personality
    value is 0x87654321, then 0x1 will be used for the 1st spot's x coord, 0x2 will be used for
    the 1st spot's y coord, 0x3 will be used for the 2nd spot's x coord, and so on. Each
    coordinate is calculated as (baseCoord + (given 4 bits of personality) - 8). In effect this
    means each spot can start at any position -8 to +7 off of its base coordinates (256 possibilities).

    The function then loops over the 16x16 spot image. For each bit in the spot's binary image, if
    the bit is set then it's part of the spot; try to draw it. A pixel is drawn on Spinda if the
    pixel is between FIRST_SPOT_COLOR and LAST_SPOT_COLOR (so only colors 1, 2, or 3 on Spinda will
    allow a spot to be drawn). These color indexes are Spinda's light brown body colors. To create
    the spot it adds 4 to the color index, so Spinda's spots will be colors 5, 6, and 7.

    The above is done in TRY_DRAW_SPOT_PIXEL two different ways: one with << 4, and one without.
    This is because Spinda's sprite is a 4 bits per pixel image, but the pointer to Spinda's pixels
    (destPixels) is an 8 bit pointer, so it addresses two pixels. Shifting by 4 accesses the 2nd
    of these pixels, so this is done every other time.
*/

// Draw spot pixel if this is Spinda's body color
#define TRY_DRAW_SPOT_PIXEL(pixels, shift) \
    if (((*(pixels) & (0xF << (shift))) >= (FIRST_SPOT_COLOR << (shift))) \
     && ((*(pixels) & (0xF << (shift))) <= (LAST_SPOT_COLOR << (shift)))) \
    { \
        *(pixels) += (SPOT_COLOR_ADJUSTMENT << (shift)); \
    }


void DrawSpindaSpots(u32 personality, u8 *dest, bool32 isSecondFrame)
{
    s32 i;
    for (i = 0; i < (s32)ARRAY_COUNT(gSpindaSpotGraphics); i++)
    {
        s32 row;
        u8 x = gSpindaSpotGraphics[i].x + (personality & 0x0F);
        u8 y = gSpindaSpotGraphics[i].y + ((personality & 0xF0) >> 4);

        if (isSecondFrame)
        {
            x -= 12;
            y += 56;
        }
        else
        {
            x -= 8;
            y -= 8;
        }

        for (row = 0; row < SPINDA_SPOT_HEIGHT; row++)
        {
            s32 column;
            s32 spotPixelRow = gSpindaSpotGraphics[i].image[row];

            for (column = x; column < x + SPINDA_SPOT_WIDTH; column++)
            {
                /* Get target pixels on Spinda's sprite */
                u8 *destPixels = dest + ((column / 8) * TILE_SIZE_4BPP) +
                    ((column % 8) / 2) +
                    ((y / 8) * TILE_SIZE_4BPP * 8) +
                    ((y % 8) * 4);

                /* Is this pixel in the 16x16 spot image part of the spot? */
                if (spotPixelRow & 1)
                {
                    /* destPixels addressess two pixels, alternate which */
                    /* of the two pixels is being considered for drawing */
                    if (column & 1)
                    {
                        /* Draw spot pixel if this is Spinda's body color */
                        TRY_DRAW_SPOT_PIXEL(destPixels, 4);
                    }
                    else
                    {
                        /* Draw spot pixel if this is Spinda's body color */
                        TRY_DRAW_SPOT_PIXEL(destPixels, 0);
                    }
                }

                spotPixelRow >>= 1;
            }

            y++;
        }

        personality >>= 8;
    }
}

void EvolutionRenameMon(struct Pokemon *mon, u16 oldSpecies, u16 newSpecies)
{
    u8 language;
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    language = GetMonData(mon, MON_DATA_LANGUAGE, &language);
    if (language == GAME_LANGUAGE && !StringCompare(GetSpeciesName(oldSpecies), gStringVar1))
        SetMonData(mon, MON_DATA_NICKNAME, GetSpeciesName(newSpecies));
}

// The below two functions determine which side of a multi battle the trainer battles on
// 0 is the left (top in  party menu), 1 is right (bottom in party menu)
u8 GetPlayerFlankId(void)
{
    u8 flankId = 0;
    switch (gLinkPlayers[GetMultiplayerId()].id)
    {
    case 0:
    case 3:
        flankId = 0;
        break;
    case 1:
    case 2:
        flankId = 1;
        break;
    }
    return flankId;
}

u16 GetLinkTrainerFlankId(u8 linkPlayerId)
{
    u16 flankId = 0;
    switch (gLinkPlayers[linkPlayerId].id)
    {
    case 0:
    case 3:
        flankId = 0;
        break;
    case 1:
    case 2:
        flankId = 1;
        break;
    }
    return flankId;
}

s32 GetBattlerMultiplayerId(u16 id)
{
    s32 multiplayerId;
    for (multiplayerId = 0; multiplayerId < MAX_LINK_PLAYERS; multiplayerId++)
        if (gLinkPlayers[multiplayerId].id == id)
            break;
    return multiplayerId;
}

u8 GetTrainerEncounterMusicId(u16 trainerOpponentId)
{
    u32 sanitizedTrainerId = SanitizeTrainerId(trainerOpponentId);
    enum DifficultyLevel difficulty = GetTrainerDifficultyLevel(sanitizedTrainerId);

    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return GetTrainerEncounterMusicIdInBattlePyramid(trainerOpponentId);
    else if (InTrainerHillChallenge())
        return GetTrainerEncounterMusicIdInTrainerHill(trainerOpponentId);
    else
        return gTrainers[difficulty][sanitizedTrainerId].encounterMusic_gender & (F_TRAINER_FEMALE - 1);
}

u16 ModifyStatByNature(u8 nature, u16 stat, enum Stat statIndex)
{
    // Don't modify HP, Accuracy, or Evasion by nature
    if (statIndex <= STAT_HP || statIndex > NUM_NATURE_STATS || gNaturesInfo[nature].statUp == gNaturesInfo[nature].statDown)
        return stat;
    else if (statIndex == gNaturesInfo[nature].statUp)
        return stat * 110 / 100;
    else if (statIndex == gNaturesInfo[nature].statDown)
        return stat * 90 / 100;
    else
        return stat;
}

void AdjustFriendship(struct Pokemon *mon, u8 event)
{
    u16 species, heldItem;
    enum HoldEffect holdEffect;
    s8 mod;

    if (ShouldSkipFriendshipChange())
        return;

    species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, 0);
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);

    if (heldItem == ITEM_ENIGMA_BERRY_E_READER)
    {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[0].holdEffect;
        else
        #if FREE_ENIGMA_BERRY == FALSE
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
        #else
            holdEffect = 0;
        #endif //FREE_ENIGMA_BERRY
    }
    else
    {
        holdEffect = GetItemHoldEffect(heldItem);
    }

    if (species && species != SPECIES_EGG)
    {
        u8 friendshipLevel = 0;
        s32 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, 0);
        enum TrainerClassID opponentTrainerClass = GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA);

        if (friendship > 99)
            friendshipLevel++;
        if (friendship > 199)
            friendshipLevel++;

        if (event == FRIENDSHIP_EVENT_WALKING)
        {
            // 50% chance every 128 steps
            if (Random() & 1)
                return;
        }
        if (event == FRIENDSHIP_EVENT_LEAGUE_BATTLE)
        {
            // Only if it's a trainer battle with league progression significance
            if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
                return;
            if (!(opponentTrainerClass == TRAINER_CLASS_LEADER
                || opponentTrainerClass == TRAINER_CLASS_ELITE_FOUR
                || opponentTrainerClass == TRAINER_CLASS_CHAMPION))
                return;
        }

        mod = sFriendshipEventModifiers[event][friendshipLevel];
        friendship += CalculateFriendshipBonuses(mon,mod,holdEffect);

        if (friendship < 0)
            friendship = 0;
        if (friendship > MAX_FRIENDSHIP)
            friendship = MAX_FRIENDSHIP;

        SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
    }
}

u8 CalculateFriendshipBonuses(struct Pokemon *mon, u32 modifier, enum HoldEffect itemHoldEffect)
{
    u32 bonus = 0;

    if ((modifier > 0) && (itemHoldEffect == HOLD_EFFECT_FRIENDSHIP_UP))
        bonus += 150 * modifier / 100;
    else
        bonus += modifier;

    if (modifier == 0)
        return bonus;

    if (GetMonData(mon, MON_DATA_POKEBALL, NULL) == ITEM_LUXURY_BALL)
        bonus += ITEM_FRIENDSHIP_LUXURY_BONUS;

    if (GetMonData(mon, MON_DATA_MET_LOCATION, NULL) == GetCurrentRegionMapSectionId())
        bonus += ITEM_FRIENDSHIP_MAPSEC_BONUS;

    return bonus;
}

void MonGainEVs(struct Pokemon *mon, u16 defeatedSpecies)
{
    u8 evs[NUM_STATS];
    u16 evIncrease = 0;
    u16 totalEVs = 0;
    u16 heldItem;
    enum HoldEffect holdEffect;
    enum Stat i;
    int multiplier;
    u8 stat;
    u8 bonus;
    u32 currentEVCap = GetCurrentEVCap();

    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);
    if (heldItem == ITEM_ENIGMA_BERRY_E_READER)
    {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[0].holdEffect;
        else
        #if FREE_ENIGMA_BERRY == FALSE
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
        #else
            holdEffect = 0;
        #endif //FREE_ENIGMA_BERRY
    }
    else
    {
        holdEffect = GetItemHoldEffect(heldItem);
    }

    stat = GetItemSecondaryId(heldItem);
    bonus = GetItemHoldEffectParam(heldItem);

    for (i = 0; i < NUM_STATS; i++)
    {
        evs[i] = GetMonData(mon, MON_DATA_HP_EV + i, 0);
        totalEVs += evs[i];
    }

    for (i = 0; i < NUM_STATS; i++)
    {
        if (totalEVs >= currentEVCap)
            break;

        if (CheckPartyHasHadPokerus(mon, 0))
            multiplier = 2;
        else
            multiplier = 1;

        switch (i)
        {
        case STAT_HP:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_HP)
                evIncrease = (gSpeciesInfo[defeatedSpecies].evYield_HP + bonus) * multiplier;
            else
                evIncrease = gSpeciesInfo[defeatedSpecies].evYield_HP * multiplier;
            break;
        case STAT_ATK:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_ATK)
                evIncrease = (gSpeciesInfo[defeatedSpecies].evYield_Attack + bonus) * multiplier;
            else
                evIncrease = gSpeciesInfo[defeatedSpecies].evYield_Attack * multiplier;
            break;
        case STAT_DEF:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_DEF)
                evIncrease = (gSpeciesInfo[defeatedSpecies].evYield_Defense + bonus) * multiplier;
            else
                evIncrease = gSpeciesInfo[defeatedSpecies].evYield_Defense * multiplier;
            break;
        case STAT_SPEED:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_SPEED)
                evIncrease = (gSpeciesInfo[defeatedSpecies].evYield_Speed + bonus) * multiplier;
            else
                evIncrease = gSpeciesInfo[defeatedSpecies].evYield_Speed * multiplier;
            break;
        case STAT_SPATK:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_SPATK)
                evIncrease = (gSpeciesInfo[defeatedSpecies].evYield_SpAttack + bonus) * multiplier;
            else
                evIncrease = gSpeciesInfo[defeatedSpecies].evYield_SpAttack * multiplier;
            break;
        case STAT_SPDEF:
            if (holdEffect == HOLD_EFFECT_POWER_ITEM && stat == STAT_SPDEF)
                evIncrease = (gSpeciesInfo[defeatedSpecies].evYield_SpDefense + bonus) * multiplier;
            else
                evIncrease = gSpeciesInfo[defeatedSpecies].evYield_SpDefense * multiplier;
            break;
        default:
            break;
        }

        if (holdEffect == HOLD_EFFECT_MACHO_BRACE)
            evIncrease *= 2;

        if (totalEVs + (s16)evIncrease > currentEVCap)
            evIncrease = ((s16)evIncrease + currentEVCap) - (totalEVs + evIncrease);

        if (evs[i] + (s16)evIncrease > MAX_PER_STAT_EVS)
        {
            int val1 = (s16)evIncrease + MAX_PER_STAT_EVS;
            int val2 = evs[i] + evIncrease;
            evIncrease = val1 - val2;
        }

        evs[i] += evIncrease;
        totalEVs += evIncrease;
        SetMonData(mon, MON_DATA_HP_EV + i, &evs[i]);
    }
}

u16 GetMonEVCount(struct Pokemon *mon)
{
    int i;
    u16 count = 0;

    for (i = 0; i < NUM_STATS; i++)
        count += GetMonData(mon, MON_DATA_HP_EV + i, 0);

    return count;
}

void RandomlyGivePartyPokerus(struct Pokemon *party)
{
    u16 rnd = Random();
    if (rnd == 0x4000 || rnd == 0x8000 || rnd == 0xC000)
    {
        struct Pokemon *mon;

        do
        {
            rnd = Random() % PARTY_SIZE;
            mon = &party[rnd];
        }
        while (!GetMonData(mon, MON_DATA_SPECIES, 0) || GetMonData(mon, MON_DATA_IS_EGG, 0));

        if (!(CheckPartyHasHadPokerus(party, 1u << rnd)))
        {
            u8 rnd2;

            do
            {
                rnd2 = Random();
            }
            while ((rnd2 & 0x7) == 0);

            if (rnd2 & 0xF0)
                rnd2 &= 0x7;

            rnd2 |= (rnd2 << 4);
            rnd2 &= 0xF3;
            rnd2++;

            SetMonData(&party[rnd], MON_DATA_POKERUS, &rnd2);
        }
    }
}

u8 CheckPartyPokerus(struct Pokemon *party, u8 selection)
{
    u8 retVal;

    int partyIndex = 0;
    unsigned curBit = 1;
    retVal = 0;

    if (selection)
    {
        do
        {
            if ((selection & 1) && (GetMonData(&party[partyIndex], MON_DATA_POKERUS, 0) & 0xF))
                retVal |= curBit;
            partyIndex++;
            curBit <<= 1;
            selection >>= 1;
        }
        while (selection);
    }
    else if (GetMonData(&party[0], MON_DATA_POKERUS, 0) & 0xF)
    {
        retVal = 1;
    }

    return retVal;
}

u8 CheckPartyHasHadPokerus(struct Pokemon *party, u8 selection)
{
    u8 retVal;

    int partyIndex = 0;
    unsigned curBit = 1;
    retVal = 0;

    if (selection)
    {
        do
        {
            if ((selection & 1) && GetMonData(&party[partyIndex], MON_DATA_POKERUS, 0))
                retVal |= curBit;
            partyIndex++;
            curBit <<= 1;
            selection >>= 1;
        }
        while (selection);
    }
    else if (GetMonData(&party[0], MON_DATA_POKERUS, 0))
    {
        retVal = 1;
    }

    return retVal;
}

void UpdatePartyPokerusTime(u16 days)
{
    int i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, 0))
        {
            u8 pokerus = GetMonData(&gPlayerParty[i], MON_DATA_POKERUS, 0);
            if (pokerus & 0xF)
            {
                if ((pokerus & 0xF) < days || days > 4)
                    pokerus &= 0xF0;
                else
                    pokerus -= days;

                if (pokerus == 0)
                    pokerus = 0x10;

                SetMonData(&gPlayerParty[i], MON_DATA_POKERUS, &pokerus);
            }
        }
    }
}

void PartySpreadPokerus(struct Pokemon *party)
{
    if ((Random() % 3) == 0)
    {
        int i;
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_SPECIES, 0))
            {
                u8 pokerus = GetMonData(&party[i], MON_DATA_POKERUS, 0);
                u8 curPokerus = pokerus;
                if (pokerus)
                {
                    if (pokerus & 0xF)
                    {
                        // Spread to adjacent party members.
                        if (i != 0 && !(GetMonData(&party[i - 1], MON_DATA_POKERUS, 0) & 0xF0))
                            SetMonData(&party[i - 1], MON_DATA_POKERUS, &curPokerus);
                        if (i != (PARTY_SIZE - 1) && !(GetMonData(&party[i + 1], MON_DATA_POKERUS, 0) & 0xF0))
                        {
                            SetMonData(&party[i + 1], MON_DATA_POKERUS, &curPokerus);
                            i++;
                        }
                    }
                }
            }
        }
    }
}

bool8 TryIncrementMonLevel(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u8 nextLevel = GetMonData(mon, MON_DATA_LEVEL, 0) + 1;
    u32 expPoints = GetMonData(mon, MON_DATA_EXP, 0);
    if (expPoints > gExperienceTables[gSpeciesInfo[species].growthRate][GetCurrentPartyLevelCap()])
    {
        expPoints = gExperienceTables[gSpeciesInfo[species].growthRate][GetCurrentPartyLevelCap()];
        SetMonData(mon, MON_DATA_EXP, &expPoints);
    }
    if (nextLevel > GetCurrentLevelCap() || expPoints < gExperienceTables[gSpeciesInfo[species].growthRate][nextLevel])
    {
        return FALSE;
    }
    else
    {
        SetMonData(mon, MON_DATA_LEVEL, &nextLevel);
        return TRUE;
    }
}

static const u16 sUniversalMoves[] =
{
    MOVE_BIDE,
    MOVE_FRUSTRATION,
    MOVE_HIDDEN_POWER,
    MOVE_MIMIC,
    MOVE_NATURAL_GIFT,
    MOVE_RAGE,
    MOVE_RETURN,
    MOVE_SECRET_POWER,
    MOVE_SUBSTITUTE,
    MOVE_TERA_BLAST,
};

u8 CanLearnTeachableMove(u16 species, u16 move)
{
    if (species == SPECIES_EGG)
    {
        return FALSE;
    }
    else if (species == SPECIES_MEW)
    {
        switch (move)
        {
        case MOVE_BADDY_BAD:
        case MOVE_BOUNCY_BUBBLE:
        case MOVE_BUZZY_BUZZ:
        case MOVE_DRAGON_ASCENT:
        case MOVE_FLOATY_FALL:
        case MOVE_FREEZY_FROST:
        case MOVE_GLITZY_GLOW:
        case MOVE_RELIC_SONG:
        case MOVE_SAPPY_SEED:
        case MOVE_SECRET_SWORD:
        case MOVE_SIZZLY_SLIDE:
        case MOVE_SPARKLY_SWIRL:
        case MOVE_SPLISHY_SPLASH:
        case MOVE_VOLT_TACKLE:
        case MOVE_ZIPPY_ZAP:
            return FALSE;
        default:
            return TRUE;
        }
    }
    else
    {
        u32 i, j;
        const u16 *teachableLearnset = GetSpeciesTeachableLearnset(species);
        for (i = 0; i < ARRAY_COUNT(sUniversalMoves); i++)
        {
            if (sUniversalMoves[i] == move)
            {
                if (!gSpeciesInfo[species].tmIlliterate)
                {
                    if (move == MOVE_TERA_BLAST && GET_BASE_SPECIES_ID(species) == SPECIES_TERAPAGOS)
                        return FALSE;
                    if (GET_BASE_SPECIES_ID(species) == SPECIES_PYUKUMUKU && (move == MOVE_HIDDEN_POWER || move == MOVE_RETURN || move == MOVE_FRUSTRATION))
                        return FALSE;
                    return TRUE;
                }
                else
                {
                    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

                    if (P_TM_LITERACY < GEN_6)
                        return FALSE;

                    for (j = 0; j < MAX_LEVEL_UP_MOVES && learnset[j].move != LEVEL_UP_MOVE_END; j++)
                    {
                        if (learnset[j].move == move)
                            return TRUE;
                    }
                    return FALSE;
                }
            }
        }
        for (i = 0; teachableLearnset[i] != MOVE_UNAVAILABLE; i++)
        {
            if (teachableLearnset[i] == move)
                return TRUE;
        }
        return FALSE;
    }
}

static void QuickSortMoves(u16 *moves, s32 left, s32 right)
{
    if (left >= right)
        return;

    u16 pivot = moves[(left + right) / 2];
    s32 i = left, j = right;

    while (i <= j)
    {
        while (moves[i] != MOVE_NONE && StringCompare(GetMoveName(moves[i]), GetMoveName(pivot)) < 0)
            i++;
        while (moves[j] != MOVE_NONE && StringCompare(GetMoveName(moves[j]), GetMoveName(pivot)) > 0)
            j--;

        if (i <= j)
        {
            u16 temp = moves[i];
            moves[i] = moves[j];
            moves[j] = temp;
            i++;
            j--;
        }
    }

    QuickSortMoves(moves, left, j);
    QuickSortMoves(moves, i, right);
}

static void SortMovesAlphabetically(u16 *moves, u32 numMoves)
{
    if (numMoves > 1)
        QuickSortMoves(moves, 0, numMoves - 1);
}

u32 GetRelearnerLevelUpMoves(struct Pokemon *mon, u16 *moves)
{
    u16 learnedMoves[MAX_MON_MOVES] = {0};
    u32 numMoves = 0;
    u32 species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u32 level = (P_ENABLE_ALL_LEVEL_UP_MOVES ? MAX_LEVEL : GetMonData(mon, MON_DATA_LEVEL, 0));

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    do
    {
        const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

        for (u32 i = 0; i < MAX_LEVEL_UP_MOVES && learnset[i].move != LEVEL_UP_MOVE_END; i++)
        {
            if (learnset[i].level > level)
                break;

            u32 j;
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                if (learnedMoves[j] == learnset[i].move)
                    break;
            }
            if (j < MAX_MON_MOVES)
                continue;

            for (j = 0; j < numMoves; j++)
            {
                if (moves[j] == learnset[i].move)
                    break;
            }
            if (j < numMoves)
                continue;

            moves[numMoves++] = learnset[i].move;
        }

        species = (P_PRE_EVO_MOVES ? GetSpeciesPreEvolution(species) : SPECIES_NONE);
    } while (species != SPECIES_NONE);

    if (P_SORT_MOVES)
        SortMovesAlphabetically(moves, numMoves);

    return numMoves;
}

u32 GetRelearnerEggMoves(struct Pokemon *mon, u16 *moves)
{
    if (!FlagGet(P_FLAG_EGG_MOVES) && !P_ENABLE_MOVE_RELEARNERS)
        return 0;

    u32 learnedMoves[MAX_MON_MOVES] = {0};
    u32 numMoves = 0;
    u32 species = GetMonData(mon, MON_DATA_SPECIES);

    while (GetSpeciesPreEvolution(species) != SPECIES_NONE)
        species = GetSpeciesPreEvolution(species);
    const u16 *eggMoves = GetSpeciesEggMoves(species);

    if (eggMoves == sNoneEggMoveLearnset)
        return numMoves;

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (u32 i = 0; eggMoves[i] != MOVE_UNAVAILABLE; i++)
    {
        u32 j;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            if (learnedMoves[j] == eggMoves[i])
                break;
        }
        if (j < MAX_MON_MOVES)
            continue;

        for (j = 0; j < numMoves; j++)
        {
            if (moves[j] == eggMoves[i])
                break;
        }
        if (j < numMoves)
            continue;

        moves[numMoves++] = eggMoves[i];
    }

    if (P_SORT_MOVES)
        SortMovesAlphabetically(moves, numMoves);

    return numMoves;
}

u32 GetRelearnerTMMoves(struct Pokemon *mon, u16 *moves)
{
    if (!P_TM_MOVES_RELEARNER)
        return 0;

    u32 learnedMoves[MAX_MON_MOVES] = {0};
    u32 numMoves = 0;
    u32 species = GetMonData(mon, MON_DATA_SPECIES);
    u16 allMoves[NUM_ALL_MACHINES];
    u32 totalMoveCount = 0;

    for (u32 i = 0; i < NUM_ALL_MACHINES; i++)
    {
        enum TMHMItemId item = GetTMHMItemId(i + 1);
        u32 move = GetTMHMMoveId(i + 1);

        if (move == MOVE_NONE)
            continue;

        if ((P_ENABLE_ALL_TM_MOVES || CheckBagHasItem(item, 1)) && CanLearnTeachableMove(species, move) && move != MOVE_NONE)
            allMoves[totalMoveCount++] = move;
    }

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (u32 i = 0; i < totalMoveCount; i++)
    {
        u32 j;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            if (learnedMoves[j] == allMoves[i])
                break;
        }
        if (j < MAX_MON_MOVES)
            continue;

        for (j = 0; j < numMoves; j++)
        {
            if (moves[j] == allMoves[i])
                break;
        }
        if (j < numMoves)
            continue;

        moves[numMoves++] = allMoves[i];
    }

    if (P_SORT_MOVES)
        SortMovesAlphabetically(moves, numMoves);

    return numMoves;
}

u32 GetRelearnerTutorMoves(struct Pokemon *mon, u16 *moves)
{
    if (!FlagGet(P_FLAG_TUTOR_MOVES) && !P_ENABLE_MOVE_RELEARNERS)
        return 0;

#if P_TUTOR_MOVES_ARRAY
    u16 learnedMoves[MAX_MON_MOVES] = {0};
    u32 numMoves = 0;
    u32 species = GetMonData(mon, MON_DATA_SPECIES, 0);

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (u32 i = 0; gTutorMoves[i] != MOVE_UNAVAILABLE; i++)
    {
        u32 move = gTutorMoves[i];

        if (!CanLearnTeachableMove(species, move))
            continue;

        u32 j;
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            if (learnedMoves[j] == move)
                break;
        }
        if (j < MAX_MON_MOVES)
            continue;

        for (j = 0; j < numMoves; j++)
        {
            if (moves[j] == move)
                break;
        }
        if (j < numMoves)
            continue;

        moves[numMoves++] = move;
    }

    if (P_SORT_MOVES)
        SortMovesAlphabetically(moves, numMoves);

    return numMoves;
#else
    return 0;
#endif // P_TUTOR_MOVES_ARRAY
}

static inline bool32 DoesMonHaveMove(const u16 *moves, u16 move)
{
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (moves[i] == move)
            return TRUE;
    }
    return FALSE;
}

bool32 HasRelearnerLevelUpMoves(struct Pokemon *mon)
{
    u32 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, 0);

    if (species == SPECIES_EGG)
        return FALSE;

    u16 learnedMoves[MAX_MON_MOVES];

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    u32 level = (P_ENABLE_ALL_LEVEL_UP_MOVES ? MAX_LEVEL : GetMonData(mon, MON_DATA_LEVEL, 0));

    do
    {
        const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

        for (u32 i = 0; i < MAX_LEVEL_UP_MOVES && learnset[i].move != LEVEL_UP_MOVE_END; i++)
        {
            if (learnset[i].level > level)
                break;

            if (!DoesMonHaveMove(learnedMoves, learnset[i].move))
                return TRUE;
        }

        species = (P_PRE_EVO_MOVES ? GetSpeciesPreEvolution(species) : SPECIES_NONE);

    } while (species != SPECIES_NONE);

    return FALSE;
}

bool32 HasRelearnerEggMoves(struct Pokemon *mon)
{
    if (!FlagGet(P_FLAG_EGG_MOVES) && !P_ENABLE_MOVE_RELEARNERS)
        return FALSE;

    u32 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, 0);

    if (species == SPECIES_EGG)
        return FALSE;

    u16 learnedMoves[MAX_MON_MOVES];

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    while (GetSpeciesPreEvolution(species) != SPECIES_NONE)
        species = GetSpeciesPreEvolution(species);

    const u16 *eggMoves = GetSpeciesEggMoves(species);
    if (eggMoves == sNoneEggMoveLearnset)
        return FALSE;

    for (u32 i = 0; eggMoves[i] != MOVE_UNAVAILABLE; i++)
    {
        if (!DoesMonHaveMove(learnedMoves, eggMoves[i]))
            return TRUE;
    }

    return FALSE;
}

bool32 HasRelearnerTMMoves(struct Pokemon *mon)
{
    if (!P_TM_MOVES_RELEARNER)
        return FALSE;

    u32 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, 0);

    if (species == SPECIES_EGG)
        return FALSE;

    u16 learnedMoves[MAX_MON_MOVES];

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (u32 i = 0; i < NUM_ALL_MACHINES; i++)
    {
        enum TMHMItemId item = GetTMHMItemId(i + 1);
        u32 move = GetTMHMMoveId(i + 1);

        if (move == MOVE_NONE)
            continue;

        if (!P_ENABLE_ALL_TM_MOVES && !CheckBagHasItem(item, 1))
            continue;

        if (!CanLearnTeachableMove(species, move))
            continue;

        if (!DoesMonHaveMove(learnedMoves, move))
            return TRUE;
    }

    return FALSE;
}

bool32 HasRelearnerTutorMoves(struct Pokemon *mon)
{
    if (!FlagGet(P_FLAG_TUTOR_MOVES) && !P_ENABLE_MOVE_RELEARNERS)
        return FALSE;

#if P_TUTOR_MOVES_ARRAY
    u32 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, 0);

    if (species == SPECIES_EGG)
        return FALSE;

    u16 learnedMoves[MAX_MON_MOVES];

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (u32 i = 0; gTutorMoves[i] != MOVE_UNAVAILABLE; i++)
    {
        u32 move = gTutorMoves[i];

        if (!CanLearnTeachableMove(species, move))
            continue;

        if (!DoesMonHaveMove(learnedMoves, move))
            return TRUE;
    }
#endif
    return FALSE;
}

u8 GetLevelUpMovesBySpecies(u16 species, u16 *moves)
{
    u8 numMoves = 0;
    int i;
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

    for (i = 0; i < MAX_LEVEL_UP_MOVES && learnset[i].move != LEVEL_UP_MOVE_END; i++)
         moves[numMoves++] = learnset[i].move;

     return numMoves;
}

u16 SpeciesToPokedexNum(u16 species)
{
    if (IsNationalPokedexEnabled())
    {
        return SpeciesToNationalPokedexNum(species);
    }
    else
    {
        species = SpeciesToHoennPokedexNum(species);
        if (species <= HOENN_DEX_COUNT)
            return species;
        return 0xFFFF;
    }
}

bool32 IsSpeciesInHoennDex(u16 species)
{
    if (SpeciesToHoennPokedexNum(species) > HOENN_DEX_COUNT)
        return FALSE;
    else
        return TRUE;
}

u16 GetBattleBGM(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LEGENDARY)
    {
        switch (GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL))
        {
        case SPECIES_RAYQUAZA:
            return MUS_VS_RAYQUAZA;
        case SPECIES_KYOGRE:
        case SPECIES_GROUDON:
            return MUS_VS_KYOGRE_GROUDON;
        case SPECIES_REGIROCK:
        case SPECIES_REGICE:
        case SPECIES_REGISTEEL:
        case SPECIES_REGIGIGAS:
        case SPECIES_REGIELEKI:
        case SPECIES_REGIDRAGO:
            return MUS_VS_REGI;
        default:
            return MUS_RG_VS_LEGEND;
        }
    }
    else if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
    {
        return MUS_VS_TRAINER;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        enum TrainerClassID trainerClass;

        if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
            trainerClass = GetFrontierOpponentClass(TRAINER_BATTLE_PARAM.opponentA);
        else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
            trainerClass = TRAINER_CLASS_EXPERT;
        else
            trainerClass = GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA);

        switch (trainerClass)
        {
        case TRAINER_CLASS_AQUA_LEADER:
        case TRAINER_CLASS_MAGMA_LEADER:
            return MUS_VS_AQUA_MAGMA_LEADER;
        case TRAINER_CLASS_TEAM_AQUA:
        case TRAINER_CLASS_TEAM_MAGMA:
        case TRAINER_CLASS_AQUA_ADMIN:
        case TRAINER_CLASS_MAGMA_ADMIN:
            return MUS_VS_AQUA_MAGMA;
        case TRAINER_CLASS_LEADER:
            return MUS_VS_GYM_LEADER;
        case TRAINER_CLASS_CHAMPION:
            return MUS_VS_CHAMPION;
        case TRAINER_CLASS_RIVAL:
            if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
                return MUS_VS_RIVAL;
            if (!StringCompare(GetTrainerNameFromId(TRAINER_BATTLE_PARAM.opponentA), gText_BattleWallyName))
                return MUS_VS_TRAINER;
            return MUS_VS_RIVAL;
        case TRAINER_CLASS_ELITE_FOUR:
            return MUS_VS_ELITE_FOUR;
        case TRAINER_CLASS_SALON_MAIDEN:
        case TRAINER_CLASS_DOME_ACE:
        case TRAINER_CLASS_PALACE_MAVEN:
        case TRAINER_CLASS_ARENA_TYCOON:
        case TRAINER_CLASS_FACTORY_HEAD:
        case TRAINER_CLASS_PIKE_QUEEN:
        case TRAINER_CLASS_PYRAMID_KING:
            return MUS_VS_FRONTIER_BRAIN;
        default:
            return MUS_VS_TRAINER;
        }
    }
    else
    {
        return MUS_VS_WILD;
    }
}

void PlayBattleBGM(void)
{
    ResetMapMusic();
    m4aMPlayAllStop();
    PlayBGM(GetBattleBGM());
}

void PlayMapChosenOrBattleBGM(u16 songId)
{
    ResetMapMusic();
    m4aMPlayAllStop();
    if (songId)
        PlayNewMapMusic(songId);
    else
        PlayNewMapMusic(GetBattleBGM());
}

// Identical to PlayMapChosenOrBattleBGM, but uses a task instead
// Only used by Battle Dome
#define tSongId data[0]
void CreateTask_PlayMapChosenOrBattleBGM(u16 songId)
{
    u8 taskId;

    ResetMapMusic();
    m4aMPlayAllStop();

    taskId = CreateTask(Task_PlayMapChosenOrBattleBGM, 0);
    gTasks[taskId].tSongId = songId;
}

static void Task_PlayMapChosenOrBattleBGM(u8 taskId)
{
    if (gTasks[taskId].tSongId)
        PlayNewMapMusic(gTasks[taskId].tSongId);
    else
        PlayNewMapMusic(GetBattleBGM());
    DestroyTask(taskId);
}

#undef tSongId

const u16 *GetMonFrontSpritePal(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL);
    bool32 isShiny = GetMonData(mon, MON_DATA_IS_SHINY, NULL);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    return GetMonSpritePalFromSpeciesAndPersonality(species, isShiny, personality);
}

const u16 *GetMonSpritePalFromSpeciesAndPersonality(u16 species, bool32 isShiny, u32 personality)
{
    return GetMonSpritePalFromSpecies(species, isShiny, IsPersonalityFemale(species, personality));
}

const u16 *GetMonSpritePalFromSpecies(u16 species, bool32 isShiny, bool32 isFemale)
{
    species = SanitizeSpeciesId(species);

    if (isShiny)
    {
    #if P_GENDER_DIFFERENCES
        if (gSpeciesInfo[species].shinyPaletteFemale != NULL && isFemale)
            return gSpeciesInfo[species].shinyPaletteFemale;
        else
    #endif
        if (gSpeciesInfo[species].shinyPalette != NULL)
            return gSpeciesInfo[species].shinyPalette;
        else
            return gSpeciesInfo[SPECIES_NONE].shinyPalette;
    }
    else
    {
    #if P_GENDER_DIFFERENCES
        if (gSpeciesInfo[species].paletteFemale != NULL && isFemale)
            return gSpeciesInfo[species].paletteFemale;
        else
    #endif
        if (gSpeciesInfo[species].palette != NULL)
            return gSpeciesInfo[species].palette;
        else
            return gSpeciesInfo[SPECIES_NONE].palette;
    }
}

#define OR_MOVE_IS_HM(_hm) || (move == MOVE_##_hm)

bool32 IsMoveHM(u16 move)
{
    return FALSE FOREACH_HM(OR_MOVE_IS_HM);
}

#undef OR_MOVE_IS_HM

bool32 CannotForgetMove(u16 move)
{
    if (P_CAN_FORGET_HIDDEN_MOVE)
        return FALSE;

    return IsMoveHM(move);
}

bool8 IsMonSpriteNotFlipped(u16 species)
{
    return gSpeciesInfo[species].noFlip;
}

s8 GetMonFlavorRelation(struct Pokemon *mon, u8 flavor)
{
    u8 nature = GetNature(mon);
    return gPokeblockFlavorCompatibilityTable[nature * FLAVOR_COUNT + flavor];
}

s8 GetFlavorRelationByPersonality(u32 personality, u8 flavor)
{
    u8 nature = GetNatureFromPersonality(personality);
    return gPokeblockFlavorCompatibilityTable[nature * FLAVOR_COUNT + flavor];
}

bool8 IsTradedMon(struct Pokemon *mon)
{
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u32 otId;
    GetMonData(mon, MON_DATA_OT_NAME, otName);
    otId = GetMonData(mon, MON_DATA_OT_ID, 0);
    return IsOtherTrainer(otId, otName);
}

bool8 IsOtherTrainer(u32 otId, u8 *otName)
{
    if (otId ==
        (gSaveBlock2Ptr->playerTrainerId[0]
      | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
      | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
      | (gSaveBlock2Ptr->playerTrainerId[3] << 24)))
    {
        int i;
        for (i = 0; otName[i] != EOS; i++)
            if (otName[i] != gSaveBlock2Ptr->playerName[i])
                return TRUE;
        return FALSE;
    }

    return TRUE;
}

void MonRestorePP(struct Pokemon *mon)
{
    BoxMonRestorePP(&mon->box);
}

void BoxMonRestorePP(struct BoxPokemon *boxMon)
{
    int i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, 0))
        {
            u16 move = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, 0);
            u16 bonus = GetBoxMonData(boxMon, MON_DATA_PP_BONUSES, 0);
            u8 pp = CalculatePPWithBonus(move, bonus, i);
            SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp);
        }
    }
}

void SetMonPreventsSwitchingString(void)
{
    gLastUsedAbility = gBattleStruct->abilityPreventingSwitchout;

    gBattleTextBuff1[0] = B_BUFF_PLACEHOLDER_BEGIN;
    gBattleTextBuff1[1] = B_BUFF_MON_NICK_WITH_PREFIX;
    gBattleTextBuff1[2] = gBattleStruct->battlerPreventingSwitchout;
    gBattleTextBuff1[4] = B_BUFF_EOS;

    if (IsOnPlayerSide(gBattleStruct->battlerPreventingSwitchout))
        gBattleTextBuff1[3] = GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[gBattleStruct->battlerPreventingSwitchout]);
    else
        gBattleTextBuff1[3] = gBattlerPartyIndexes[gBattleStruct->battlerPreventingSwitchout];

    PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff2, gBattlerInMenuId, GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[gBattlerInMenuId]))

    BattleStringExpandPlaceholders(gText_PkmnsXPreventsSwitching, gStringVar4, sizeof(gStringVar4));
}

static s32 GetWildMonTableIdInAlteringCave(u16 species)
{
    s32 i;
    for (i = 0; i < (s32) ARRAY_COUNT(sAlteringCaveWildMonHeldItems); i++)
        if (sAlteringCaveWildMonHeldItems[i].species == species)
            return i;
    return 0;
}

static inline bool32 CanFirstMonBoostHeldItemRarity(void)
{
    enum Ability ability;
    if (GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        return FALSE;

    ability = GetMonAbility(&gPlayerParty[0]);
    if (ability == ABILITY_COMPOUND_EYES)
        return TRUE;
    else if ((OW_SUPER_LUCK >= GEN_8) && ability == ABILITY_SUPER_LUCK)
        return TRUE;
    return FALSE;
}

void SetWildMonHeldItem(void)
{
    if (!(gBattleTypeFlags & (BATTLE_TYPE_LEGENDARY | BATTLE_TYPE_TRAINER | BATTLE_TYPE_PYRAMID | BATTLE_TYPE_PIKE)))
    {
        u16 rnd;
        u16 species;
        u16 count = (WILD_DOUBLE_BATTLE) ? 2 : 1;
        u16 i;
        bool32 itemHeldBoost = CanFirstMonBoostHeldItemRarity();
        u16 chanceNoItem = itemHeldBoost ? 20 : 45;
        u16 chanceNotRare = itemHeldBoost ? 80 : 95;

        for (i = 0; i < count; i++)
        {
            if (GetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, NULL) != ITEM_NONE)
                continue; // prevent overwriting previously set item

            rnd = Random() % 100;
            species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES, 0);
            if (gMapHeader.mapLayoutId == LAYOUT_ALTERING_CAVE)
            {
                s32 alteringCaveId = GetWildMonTableIdInAlteringCave(species);
                if (alteringCaveId != 0)
                {
                    // In active Altering Cave, use special item list
                    if (rnd < chanceNotRare)
                        continue;
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &sAlteringCaveWildMonHeldItems[alteringCaveId].item);
                }
                else
                {
                    // In inactive Altering Cave, use normal items
                    if (rnd < chanceNoItem)
                        continue;
                    if (rnd < chanceNotRare)
                        SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gSpeciesInfo[species].itemCommon);
                    else
                        SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gSpeciesInfo[species].itemRare);
                }
            }
            else
            {
                if (gSpeciesInfo[species].itemCommon == gSpeciesInfo[species].itemRare && gSpeciesInfo[species].itemCommon != ITEM_NONE)
                {
                    // Both held items are the same, 100% chance to hold item
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gSpeciesInfo[species].itemCommon);
                }
                else
                {
                    if (rnd < chanceNoItem)
                        continue;
                    if (rnd < chanceNotRare)
                        SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gSpeciesInfo[species].itemCommon);
                    else
                        SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gSpeciesInfo[species].itemRare);
                }
            }
        }
    }
}

bool8 IsMonShiny(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_IS_SHINY, NULL);
}

const u8 *GetTrainerPartnerName(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        GetFrontierTrainerName(gStringVar1, gPartnerTrainerId);
        return gStringVar1;
    }
    else
    {
        u8 id = GetMultiplayerId();
        return gLinkPlayers[GetBattlerMultiplayerId(gLinkPlayers[id].id ^ 2)].name;
    }
}

#define READ_PTR_FROM_TASK(taskId, dataId)                      \
    (void *)(                                                   \
    ((u16)(gTasks[taskId].data[dataId]) |                       \
    ((u16)(gTasks[taskId].data[dataId + 1]) << 16)))

#define STORE_PTR_IN_TASK(ptr, taskId, dataId)                 \
{                                                              \
    gTasks[taskId].data[dataId] = (u32)(ptr);                  \
    gTasks[taskId].data[dataId + 1] = (u32)(ptr) >> 16;        \
}

#define sAnimId    data[2]
#define sAnimDelay data[3]

static void Task_AnimateAfterDelay(u8 taskId)
{
    if (--gTasks[taskId].sAnimDelay == 0)
    {
        LaunchAnimationTaskForFrontSprite(READ_PTR_FROM_TASK(taskId, 0), gTasks[taskId].sAnimId);
        DestroyTask(taskId);
    }
}

static void Task_PokemonSummaryAnimateAfterDelay(u8 taskId)
{
    if (--gTasks[taskId].sAnimDelay == 0)
    {
        StartMonSummaryAnimation(READ_PTR_FROM_TASK(taskId, 0), gTasks[taskId].sAnimId);
        SummaryScreen_SetAnimDelayTaskId(TASK_NONE);
        DestroyTask(taskId);
    }
}

void BattleAnimateFrontSprite(struct Sprite *sprite, u16 species, bool8 noCry, u8 panMode)
{
    if (gHitMarker & HITMARKER_NO_ANIMATIONS && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))
        DoMonFrontSpriteAnimation(sprite, species, noCry, panMode | SKIP_FRONT_ANIM);
    else
        DoMonFrontSpriteAnimation(sprite, species, noCry, panMode);
}

void DoMonFrontSpriteAnimation(struct Sprite *sprite, u16 species, bool8 noCry, u8 panModeAnimFlag)
{
    s8 pan;
    switch (panModeAnimFlag & (u8)~SKIP_FRONT_ANIM) // Exclude anim flag to get pan mode
    {
    case 0:
        pan = -25;
        break;
    case 1:
        pan = 25;
        break;
    default:
        pan = 0;
        break;
    }
    if (panModeAnimFlag & SKIP_FRONT_ANIM)
    {
        // No animation, only check if cry needs to be played
        if (!noCry)
            PlayCry_Normal(species, pan);
        sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (!noCry)
        {
            PlayCry_Normal(species, pan);
            if (HasTwoFramesAnimation(species))
                StartSpriteAnim(sprite, 1);
        }
        if (gSpeciesInfo[species].frontAnimDelay != 0)
        {
            // Animation has delay, start delay task
            u8 taskId = CreateTask(Task_AnimateAfterDelay, 0);
            STORE_PTR_IN_TASK(sprite, taskId, 0);
            gTasks[taskId].sAnimId = gSpeciesInfo[species].frontAnimId;
            gTasks[taskId].sAnimDelay = gSpeciesInfo[species].frontAnimDelay;
        }
        else
        {
            // No delay, start animation
            LaunchAnimationTaskForFrontSprite(sprite, gSpeciesInfo[species].frontAnimId);
        }
        sprite->callback = SpriteCallbackDummy_2;
    }
}

void PokemonSummaryDoMonAnimation(struct Sprite *sprite, u16 species, bool8 oneFrame)
{
    if (!oneFrame && HasTwoFramesAnimation(species))
        StartSpriteAnim(sprite, 1);
    if (gSpeciesInfo[species].frontAnimDelay != 0)
    {
        // Animation has delay, start delay task
        u8 taskId = CreateTask(Task_PokemonSummaryAnimateAfterDelay, 0);
        STORE_PTR_IN_TASK(sprite, taskId, 0);
        gTasks[taskId].sAnimId = gSpeciesInfo[species].frontAnimId;
        gTasks[taskId].sAnimDelay = gSpeciesInfo[species].frontAnimDelay;
        SummaryScreen_SetAnimDelayTaskId(taskId);
        SetSpriteCB_MonAnimDummy(sprite);
    }
    else
    {
        // No delay, start animation
        StartMonSummaryAnimation(sprite, gSpeciesInfo[species].frontAnimId);
    }
}

void StopPokemonAnimationDelayTask(void)
{
    u8 delayTaskId = FindTaskIdByFunc(Task_PokemonSummaryAnimateAfterDelay);
    if (delayTaskId != TASK_NONE)
        DestroyTask(delayTaskId);
}

void BattleAnimateBackSprite(struct Sprite *sprite, u16 species)
{
    if (gHitMarker & HITMARKER_NO_ANIMATIONS && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))
    {
        sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        LaunchAnimationTaskForBackSprite(sprite, GetSpeciesBackAnimSet(species));
        sprite->callback = SpriteCallbackDummy_2;
    }
}

// Identical to GetOpposingLinkMultiBattlerId but for the player
// "rightSide" from that team's perspective, i.e. B_POSITION_*_RIGHT
static u8 UNUSED GetOwnOpposingLinkMultiBattlerId(bool8 rightSide)
{
    s32 i;
    s32 battler = 0;
    u8 multiplayerId = GetMultiplayerId();
    switch (gLinkPlayers[multiplayerId].id)
    {
    case 0:
    case 2:
        battler = rightSide ? 1 : 3;
        break;
    case 1:
    case 3:
        battler = rightSide ? 2 : 0;
        break;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayers[i].id == (s16)battler)
            break;
    }
    return i;
}

u8 GetOpposingLinkMultiBattlerId(bool8 rightSide, u8 multiplayerId)
{
    s32 i;
    s32 battler = 0;
    switch (gLinkPlayers[multiplayerId].id)
    {
    case 0:
    case 2:
        battler = rightSide ? 1 : 3;
        break;
    case 1:
    case 3:
        battler = rightSide ? 2 : 0;
        break;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayers[i].id == (s16)battler)
            break;
    }
    return i;
}

u16 FacilityClassToPicIndex(u16 facilityClass)
{
    return gFacilityClassToPicIndex[facilityClass];
}

u16 PlayerGenderToFrontTrainerPicId(u8 playerGender)
{
    if (playerGender != MALE)
        return FacilityClassToPicIndex(FACILITY_CLASS_MAY);
    else
        return FacilityClassToPicIndex(FACILITY_CLASS_BRENDAN);
}

void HandleSetPokedexFlag(enum NationalDexOrder nationalNum, u8 caseId, u32 personality)
{
    u8 getFlagCaseId = (caseId == FLAG_SET_SEEN) ? FLAG_GET_SEEN : FLAG_GET_CAUGHT;
    if (!GetSetPokedexFlag(nationalNum, getFlagCaseId)) // don't set if it's already set
    {
        GetSetPokedexFlag(nationalNum, caseId);
        if (NationalPokedexNumToSpecies(nationalNum) == SPECIES_UNOWN)
            gSaveBlock2Ptr->pokedex.unownPersonality = personality;
        if (NationalPokedexNumToSpecies(nationalNum) == SPECIES_SPINDA)
            gSaveBlock2Ptr->pokedex.spindaPersonality = personality;
    }
}

void HandleSetPokedexFlagFromMon(struct Pokemon *mon, u32 caseId)
{
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY);
    enum NationalDexOrder nationalNum = SpeciesToNationalPokedexNum(GetMonData(mon, MON_DATA_SPECIES));

    HandleSetPokedexFlag(nationalNum, caseId, personality);
}

bool8 HasTwoFramesAnimation(u16 species)
{
    return P_TWO_FRAME_FRONT_SPRITES
        && gSpeciesInfo[species].frontAnimFrames != sAnims_SingleFramePlaceHolder
        && species != SPECIES_UNOWN
        && !gTestRunnerHeadless;
}

bool8 ShouldSkipFriendshipChange(void)
{
    if (gMain.inBattle && gBattleTypeFlags & (BATTLE_TYPE_FRONTIER))
        return TRUE;
    if (!gMain.inBattle && (InBattlePike() || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE))
        return TRUE;
    return FALSE;
}

// The below functions are for the 'MonSpritesGfxManager', a method of allocating
// space for Pokémon sprites. These are only used for the summary screen Pokémon
// sprites (unless gMonSpritesGfxPtr is in use), but were set up for more general use.
// Only the 'default' mode (MON_SPR_GFX_MODE_NORMAL) is used, which is set
// up to allocate 4 sprites using the battler sprite templates (gBattlerSpriteTemplates).
// MON_SPR_GFX_MODE_BATTLE is identical but never used.
// MON_SPR_GFX_MODE_FULL_PARTY is set up to allocate 7 sprites (party + trainer?)
// using a generic 64x64 template, and is also never used.

// Between the unnecessarily large sizes below, a mistake allocating the spritePointers
// field, and the fact that ultimately only 1 of the 4 sprite positions is used, this
// system wastes a good deal of memory.

#define ALLOC_FAIL_BUFFER (1 << 0)
#define ALLOC_FAIL_STRUCT (1 << 1)
#define GFX_MANAGER_ACTIVE 0xA3 // Arbitrary value

static void InitMonSpritesGfx_Battle(struct MonSpritesGfxManager *gfx)
{
    u16 i, j;
    for (i = 0; i < gfx->numSprites; i++)
    {
        gfx->templates[i] = gBattlerSpriteTemplates[i];
        for (j = 0; j < gfx->numFrames; j++)
            gfx->frameImages[i * gfx->numFrames + j].data = &gfx->spritePointers[i][j * MON_PIC_SIZE];

        gfx->templates[i].images = &gfx->frameImages[i * gfx->numFrames];
    }
}

static void InitMonSpritesGfx_FullParty(struct MonSpritesGfxManager *gfx)
{
    u16 i, j;
    for (i = 0; i < gfx->numSprites; i++)
    {
        gfx->templates[i] = sSpriteTemplate_64x64;
        for (j = 0; j < gfx->numFrames; j++)
            gfx->frameImages[i * gfx->numSprites + j].data = &gfx->spritePointers[i][j * MON_PIC_SIZE];

        gfx->templates[i].images = &gfx->frameImages[i * gfx->numSprites];
        gfx->templates[i].anims = gAnims_MonPic;
        gfx->templates[i].paletteTag = i;
    }
}

struct MonSpritesGfxManager *CreateMonSpritesGfxManager(u8 managerId, u8 mode)
{
    u8 i;
    u8 failureFlags;
    struct MonSpritesGfxManager *gfx;

    failureFlags = 0;
    managerId %= MON_SPR_GFX_MANAGERS_COUNT;
    gfx = AllocZeroed(sizeof(*gfx));
    if (gfx == NULL)
        return NULL;

    switch (mode)
    {
    case MON_SPR_GFX_MODE_FULL_PARTY:
        gfx->numSprites = PARTY_SIZE + 1;
        gfx->numSprites2 = PARTY_SIZE + 1;
        gfx->numFrames = MAX_MON_PIC_FRAMES;
        gfx->dataSize = 1;
        gfx->mode = MON_SPR_GFX_MODE_FULL_PARTY;
        break;
 // case MON_SPR_GFX_MODE_BATTLE:
    case MON_SPR_GFX_MODE_NORMAL:
    default:
        gfx->numSprites = MAX_BATTLERS_COUNT;
        gfx->numSprites2 = MAX_BATTLERS_COUNT;
        gfx->numFrames = MAX_MON_PIC_FRAMES;
        gfx->dataSize = 1;
        gfx->mode = MON_SPR_GFX_MODE_NORMAL;
        break;
    }

    // Set up sprite / sprite pointer buffers
    gfx->spriteBuffer = AllocZeroed(gfx->dataSize * MON_PIC_SIZE * MAX_MON_PIC_FRAMES * gfx->numSprites);
    gfx->spritePointers = AllocZeroed(gfx->numSprites * 32); // ? Only * 4 is necessary, perhaps they were thinking bits.
    if (gfx->spriteBuffer == NULL || gfx->spritePointers == NULL)
    {
        failureFlags |= ALLOC_FAIL_BUFFER;
    }
    else
    {
        for (i = 0; i < gfx->numSprites; i++)
            gfx->spritePointers[i] = gfx->spriteBuffer + (gfx->dataSize * MON_PIC_SIZE * MAX_MON_PIC_FRAMES * i);
    }

    // Set up sprite structs
    gfx->templates = AllocZeroed(sizeof(struct SpriteTemplate) * gfx->numSprites);
    gfx->frameImages = AllocZeroed(sizeof(struct SpriteFrameImage) * gfx->numSprites * gfx->numFrames);
    if (gfx->templates == NULL || gfx->frameImages == NULL)
    {
        failureFlags |= ALLOC_FAIL_STRUCT;
    }
    else
    {
        for (i = 0; i < gfx->numFrames * gfx->numSprites; i++)
            gfx->frameImages[i].size = MON_PIC_SIZE;

        switch (gfx->mode)
        {
        case MON_SPR_GFX_MODE_FULL_PARTY:
            InitMonSpritesGfx_FullParty(gfx);
            break;
        case MON_SPR_GFX_MODE_NORMAL:
        case MON_SPR_GFX_MODE_BATTLE:
        default:
            InitMonSpritesGfx_Battle(gfx);
            break;
        }
    }

    // If either of the allocations failed free their respective members
    if (failureFlags & ALLOC_FAIL_STRUCT)
    {
        TRY_FREE_AND_SET_NULL(gfx->frameImages);
        TRY_FREE_AND_SET_NULL(gfx->templates);
    }
    if (failureFlags & ALLOC_FAIL_BUFFER)
    {
        TRY_FREE_AND_SET_NULL(gfx->spritePointers);
        TRY_FREE_AND_SET_NULL(gfx->spriteBuffer);
    }

    if (failureFlags)
    {
        // Clear, something failed to allocate
        memset(gfx, 0, sizeof(*gfx));
        Free(gfx);
    }
    else
    {
        gfx->active = GFX_MANAGER_ACTIVE;
        sMonSpritesGfxManagers[managerId] = gfx;
    }

    return sMonSpritesGfxManagers[managerId];
}

void DestroyMonSpritesGfxManager(u8 managerId)
{
    struct MonSpritesGfxManager *gfx;

    managerId %= MON_SPR_GFX_MANAGERS_COUNT;
    gfx = sMonSpritesGfxManagers[managerId];
    if (gfx == NULL)
        return;

    if (gfx->active != GFX_MANAGER_ACTIVE)
    {
        memset(gfx, 0, sizeof(*gfx));
    }
    else
    {
        TRY_FREE_AND_SET_NULL(gfx->frameImages);
        TRY_FREE_AND_SET_NULL(gfx->templates);
        TRY_FREE_AND_SET_NULL(gfx->spritePointers);
        TRY_FREE_AND_SET_NULL(gfx->spriteBuffer);
        memset(gfx, 0, sizeof(*gfx));
        Free(gfx);
    }
}

u8 *MonSpritesGfxManager_GetSpritePtr(u8 managerId, u8 spriteNum)
{
    struct MonSpritesGfxManager *gfx = sMonSpritesGfxManagers[managerId % MON_SPR_GFX_MANAGERS_COUNT];
    if (gfx->active != GFX_MANAGER_ACTIVE)
    {
        return NULL;
    }
    else
    {
        if (spriteNum >= gfx->numSprites)
            spriteNum = 0;

        return gfx->spritePointers[spriteNum];
    }
}

u16 GetFormSpeciesId(u16 speciesId, u8 formId)
{
    if (GetSpeciesFormTable(speciesId) != NULL)
        return GetSpeciesFormTable(speciesId)[formId];
    else
        return speciesId;
}

u8 GetFormIdFromFormSpeciesId(u16 formSpeciesId)
{
    u8 targetFormId = 0;

    if (GetSpeciesFormTable(formSpeciesId) != NULL)
    {
        for (targetFormId = 0; GetSpeciesFormTable(formSpeciesId)[targetFormId] != FORM_SPECIES_END; targetFormId++)
        {
            if (formSpeciesId == GetSpeciesFormTable(formSpeciesId)[targetFormId])
                break;
        }
    }
    return targetFormId;
}

// Returns the current species if no form change is possible
u32 GetFormChangeTargetSpecies(struct Pokemon *mon, enum FormChanges method, u32 arg)
{
    return GetFormChangeTargetSpeciesBoxMon(&mon->box, method, arg);
}

// Returns the current species if no form change is possible
u32 GetFormChangeTargetSpeciesBoxMon(struct BoxPokemon *boxMon, enum FormChanges method, u32 arg)
{
    u32 i;
    u32 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 targetSpecies = species;
    const struct FormChange *formChanges = GetSpeciesFormChanges(species);
    u16 heldItem;
    enum Ability ability;

    if (formChanges != NULL)
    {
        heldItem = GetBoxMonData(boxMon, MON_DATA_HELD_ITEM, NULL);
        ability = GetAbilityBySpecies(species, GetBoxMonData(boxMon, MON_DATA_ABILITY_NUM, NULL));

        for (i = 0; formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
        {
            if (method == formChanges[i].method && species != formChanges[i].targetSpecies)
            {
                switch (method)
                {
                case FORM_CHANGE_ITEM_HOLD:
                    if ((heldItem == formChanges[i].param1 || formChanges[i].param1 == ITEM_NONE)
                     && (ability == formChanges[i].param2 || formChanges[i].param2 == ABILITY_NONE))
                    {
                        // This is to prevent reverting to base form when giving the item to the corresponding form.
                        // Eg. Giving a Zap Plate to an Electric Arceus without an item (most likely to happen when using givemon)
                        bool32 currentItemForm = FALSE;
                        for (u32 j = 0; formChanges[j].method != FORM_CHANGE_TERMINATOR; j++)
                        {
                            if (species == formChanges[j].targetSpecies
                                && formChanges[j].param1 == heldItem
                                && formChanges[j].param1 != ITEM_NONE)
                            {
                                currentItemForm = TRUE;
                                break;
                            }
                        }
                        if (!currentItemForm)
                            targetSpecies = formChanges[i].targetSpecies;
                    }
                    break;
                case FORM_CHANGE_ITEM_USE:
                    if (arg == formChanges[i].param1)
                    {
                        bool32 pass = TRUE;
                        switch (formChanges[i].param2)
                        {
                        case DAY:
                            if (GetTimeOfDay() == TIME_NIGHT)
                                pass = FALSE;
                            break;
                        case NIGHT:
                            if (GetTimeOfDay() != TIME_NIGHT)
                                pass = FALSE;
                            break;
                        }

                        if (formChanges[i].param3 != STATUS1_NONE && GetBoxMonData(boxMon, MON_DATA_STATUS, NULL) & formChanges[i].param3)
                            pass = FALSE;

                        if (pass)
                            targetSpecies = formChanges[i].targetSpecies;
                    }
                    break;
                case FORM_CHANGE_ITEM_USE_MULTICHOICE:
                    if (arg == formChanges[i].param1)
                    {
                        if (formChanges[i].param2 == gSpecialVar_Result)
                            targetSpecies = formChanges[i].targetSpecies;
                    }
                    break;
                case FORM_CHANGE_MOVE:
                    if (BoxMonKnowsMove(boxMon, formChanges[i].param1) != formChanges[i].param2)
                        targetSpecies = formChanges[i].targetSpecies;
                    break;
                case FORM_CHANGE_BEGIN_BATTLE:
                case FORM_CHANGE_END_BATTLE:
                    if (heldItem == formChanges[i].param1 || formChanges[i].param1 == ITEM_NONE)
                        targetSpecies = formChanges[i].targetSpecies;
                    break;
                case FORM_CHANGE_END_BATTLE_ENVIRONMENT:
                    if (gBattleEnvironment == formChanges[i].param1)
                        targetSpecies = formChanges[i].targetSpecies;
                    break;
                case FORM_CHANGE_WITHDRAW:
                case FORM_CHANGE_DEPOSIT:
                case FORM_CHANGE_FAINT:
                case FORM_CHANGE_DAYS_PASSED:
                    targetSpecies = formChanges[i].targetSpecies;
                    break;
                case FORM_CHANGE_STATUS:
                    if (GetBoxMonData(boxMon, MON_DATA_STATUS, NULL) & formChanges[i].param1)
                        targetSpecies = formChanges[i].targetSpecies;
                    break;
                case FORM_CHANGE_TIME_OF_DAY:
                    switch (formChanges[i].param1)
                    {
                    case DAY:
                        if (GetTimeOfDay() != TIME_NIGHT)
                            targetSpecies = formChanges[i].targetSpecies;
                        break;
                    case NIGHT:
                        if (GetTimeOfDay() == TIME_NIGHT)
                            targetSpecies = formChanges[i].targetSpecies;
                        break;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    return targetSpecies;
}

void TrySetDayLimitToFormChange(struct Pokemon *mon)
{
    u32 i;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    const struct FormChange *formChanges = GetSpeciesFormChanges(species);

    for (i = 0; formChanges != NULL && formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (formChanges[i].method == FORM_CHANGE_DAYS_PASSED && species != formChanges[i].targetSpecies)
        {
            SetMonData(mon, MON_DATA_DAYS_SINCE_FORM_CHANGE, &formChanges[i].param1);
            break;
        }
    }
}

bool32 DoesSpeciesHaveFormChangeMethod(u16 species, enum FormChanges method)
{
    u32 i;
    const struct FormChange *formChanges = GetSpeciesFormChanges(species);

    for (i = 0; formChanges != NULL && formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (method == formChanges[i].method && species != formChanges[i].targetSpecies)
            return TRUE;
    }

    return FALSE;
}

u16 MonTryLearningNewMoveEvolution(struct Pokemon *mon, bool8 firstMove)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, NULL);
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);

    // Since you can learn more than one move per level,
    // the game needs to know whether you decided to
    // learn it or keep the old set to avoid asking
    // you to learn the same move over and over again.
    if (firstMove)
    {
        sLearningMoveTableID = 0;
    }
    while(learnset[sLearningMoveTableID].move != LEVEL_UP_MOVE_END)
    {
        while ((learnset[sLearningMoveTableID].level == 0 || learnset[sLearningMoveTableID].level == level)
             && !(P_EVOLUTION_LEVEL_1_LEARN >= GEN_8 && learnset[sLearningMoveTableID].level == 1))
        {
            gMoveToLearn = learnset[sLearningMoveTableID].move;
            sLearningMoveTableID++;
            return GiveMoveToMon(mon, gMoveToLearn);
        }
        sLearningMoveTableID++;
    }
    return 0;
}

// Removes the selected index from the given IV list and shifts the remaining
// elements to the left.
void RemoveIVIndexFromList(u8 *ivs, u8 selectedIv)
{
    s32 i, j;
    u8 temp[NUM_STATS];

    ivs[selectedIv] = 0xFF;
    for (i = 0; i < NUM_STATS; i++)
    {
        temp[i] = ivs[i];
    }

    j = 0;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (temp[i] != 0xFF)
            ivs[j++] = temp[i];
    }
}

// Attempts to perform non-level/item related overworld evolutions; called by tryspecialevo command.
void TryScriptEvolution(void)
{
    u8 i;
    bool32 canStopEvo = gSpecialVar_0x8001;
    u16 tryMultiple = gSpecialVar_0x8002;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 targetSpecies = GetEvolutionTargetSpecies(&gPlayerParty[i], EVO_MODE_SCRIPT_TRIGGER, 0, NULL, &canStopEvo, CHECK_EVO);

        if (targetSpecies != SPECIES_NONE && !(sTriedEvolving & (1u << i)))
        {
            GetEvolutionTargetSpecies(&gPlayerParty[i], EVO_MODE_SCRIPT_TRIGGER, 0, NULL, &canStopEvo, DO_EVO);
            sTriedEvolving |= 1u << i;
            if(gMain.callback2 == TryScriptEvolution) // This fixes small graphics glitches.
                EvolutionScene(&gPlayerParty[i], targetSpecies, canStopEvo, i);
            else
                BeginEvolutionScene(&gPlayerParty[i], targetSpecies, canStopEvo, i);

            if (tryMultiple)
                gCB2_AfterEvolution = TryScriptEvolution;
            else
                gCB2_AfterEvolution = CB2_ReturnToField;
            return;
        }
    }

    sTriedEvolving = 0;
    SetMainCallback2(CB2_ReturnToField);
}

void TrySpecialOverworldEvo(void)
{
    u8 i;
    bool32 canStopEvo = gSpecialVar_0x8001;
    u16 tryMultiple = gSpecialVar_0x8002;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 targetSpecies = GetEvolutionTargetSpecies(&gPlayerParty[i], EVO_MODE_OVERWORLD_SPECIAL, 0, NULL, &canStopEvo, CHECK_EVO);

        if (targetSpecies != SPECIES_NONE && !(sTriedEvolving & (1u << i)))
        {
            GetEvolutionTargetSpecies(&gPlayerParty[i], EVO_MODE_OVERWORLD_SPECIAL, 0, NULL, &canStopEvo, DO_EVO);
            sTriedEvolving |= 1u << i;
            if(gMain.callback2 == TrySpecialOverworldEvo) // This fixes small graphics glitches.
                EvolutionScene(&gPlayerParty[i], targetSpecies, canStopEvo, i);
            else
                BeginEvolutionScene(&gPlayerParty[i], targetSpecies, canStopEvo, i);

            if (tryMultiple)
                gCB2_AfterEvolution = TrySpecialOverworldEvo;
            else
                gCB2_AfterEvolution = CB2_ReturnToField;
            return;
        }
    }

    sTriedEvolving = 0;
    SetMainCallback2(CB2_ReturnToField);
}

bool32 SpeciesHasGenderDifferences(u16 species)
{
#if P_GENDER_DIFFERENCES
    if (gSpeciesInfo[species].frontPicFemale != NULL
     || gSpeciesInfo[species].backPicFemale != NULL
     || gSpeciesInfo[species].paletteFemale != NULL
     || gSpeciesInfo[species].shinyPaletteFemale != NULL
     || gSpeciesInfo[species].iconSpriteFemale != NULL)
        return TRUE;
#endif

    return FALSE;
}

bool32 TryFormChange(u32 monId, enum BattleSide side, enum FormChanges method)
{
    struct Pokemon *party = (side == B_SIDE_PLAYER) ? gPlayerParty : gEnemyParty;

    if (GetMonData(&party[monId], MON_DATA_SPECIES_OR_EGG, 0) == SPECIES_NONE
     || GetMonData(&party[monId], MON_DATA_SPECIES_OR_EGG, 0) == SPECIES_EGG)
        return FALSE;

    u32 currentSpecies = GetMonData(&party[monId], MON_DATA_SPECIES);
    u32 targetSpecies = GetFormChangeTargetSpecies(&party[monId], method, 0);

    if (targetSpecies == currentSpecies && gBattleStruct != NULL && gBattleStruct->partyState[side][monId].changedSpecies != SPECIES_NONE)
        targetSpecies = gBattleStruct->partyState[side][monId].changedSpecies;

    if (targetSpecies != currentSpecies)
    {
        TryToSetBattleFormChangeMoves(&party[monId], method);
        SetMonData(&party[monId], MON_DATA_SPECIES, &targetSpecies);
        CalculateMonStats(&party[monId]);
        return TRUE;
    }

    return FALSE;
}

u16 SanitizeSpeciesId(u16 species)
{
    if (species > NUM_SPECIES || !IsSpeciesEnabled(species))
        return SPECIES_NONE;
    else
        return species;
}

bool32 IsSpeciesEnabled(u16 species)
{
    // This function should not use the GetSpeciesBaseHP function, as the included sanitation will result in an infinite loop
    return gSpeciesInfo[species].baseHP > 0 || species == SPECIES_EGG;
}

void TryToSetBattleFormChangeMoves(struct Pokemon *mon, enum FormChanges method)
{
    int i, j;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    const struct FormChange *formChanges = GetSpeciesFormChanges(species);

    if (formChanges == NULL
        || (method != FORM_CHANGE_BEGIN_BATTLE && method != FORM_CHANGE_END_BATTLE))
        return;

    for (i = 0; formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (formChanges[i].method == method
            && formChanges[i].param2
            && formChanges[i].param3
            && formChanges[i].targetSpecies != species)
        {
            u16 originalMove = formChanges[i].param2;
            u16 newMove = formChanges[i].param3;

            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                u16 currMove = GetMonData(mon, MON_DATA_MOVE1 + j, NULL);
                if (currMove == originalMove)
                    SetMonMoveSlot_KeepPP(mon, newMove, j);
            }
            break;
        }
    }
}

u32 GetMonFriendshipScore(struct Pokemon *pokemon)
{
    u32 friendshipScore = GetMonData(pokemon, MON_DATA_FRIENDSHIP, NULL);

    if (friendshipScore == MAX_FRIENDSHIP)
        return FRIENDSHIP_MAX;
    if (friendshipScore >= 200)
        return FRIENDSHIP_200_TO_254;
    if (friendshipScore >= 150)
        return FRIENDSHIP_150_TO_199;
    if (friendshipScore >= 100)
        return FRIENDSHIP_100_TO_149;
    if (friendshipScore >= 50)
        return FRIENDSHIP_50_TO_99;
    if (friendshipScore >= 1)
        return FRIENDSHIP_1_TO_49;

    return FRIENDSHIP_NONE;
}

u32 GetMonAffectionHearts(struct Pokemon *pokemon)
{
    u32 friendship = GetMonData(pokemon, MON_DATA_FRIENDSHIP, NULL);

    if (friendship == MAX_FRIENDSHIP)
        return AFFECTION_FIVE_HEARTS;
    if (friendship >= 220)
        return AFFECTION_FOUR_HEARTS;
    if (friendship >= 180)
        return AFFECTION_THREE_HEARTS;
    if (friendship >= 130)
        return AFFECTION_TWO_HEARTS;
    if (friendship >= 80)
        return AFFECTION_ONE_HEART;

    return AFFECTION_NO_HEARTS;
}

void UpdateMonPersonality(struct BoxPokemon *boxMon, u32 personality)
{
    struct PokemonSubstruct0 *old0, *new0;
    struct PokemonSubstruct1 *old1, *new1;
    struct PokemonSubstruct2 *old2, *new2;
    struct PokemonSubstruct3 *old3, *new3;
    struct BoxPokemon old;

    bool32 isShiny = GetBoxMonData(boxMon, MON_DATA_IS_SHINY, NULL);
    u32 hiddenNature = GetBoxMonData(boxMon, MON_DATA_HIDDEN_NATURE, NULL);
    enum Type teraType = GetBoxMonData(boxMon, MON_DATA_TERA_TYPE, NULL);

    old = *boxMon;
    old0 = &(GetSubstruct(&old, old.personality, SUBSTRUCT_TYPE_0)->type0);
    old1 = &(GetSubstruct(&old, old.personality, SUBSTRUCT_TYPE_1)->type1);
    old2 = &(GetSubstruct(&old, old.personality, SUBSTRUCT_TYPE_2)->type2);
    old3 = &(GetSubstruct(&old, old.personality, SUBSTRUCT_TYPE_3)->type3);

    new0 = &(GetSubstruct(boxMon, personality, SUBSTRUCT_TYPE_0)->type0);
    new1 = &(GetSubstruct(boxMon, personality, SUBSTRUCT_TYPE_1)->type1);
    new2 = &(GetSubstruct(boxMon, personality, SUBSTRUCT_TYPE_2)->type2);
    new3 = &(GetSubstruct(boxMon, personality, SUBSTRUCT_TYPE_3)->type3);

    DecryptBoxMon(&old);
    boxMon->personality = personality;
    *new0 = *old0;
    *new1 = *old1;
    *new2 = *old2;
    *new3 = *old3;
    boxMon->checksum = CalculateBoxMonChecksumReencrypt(boxMon);

    SetBoxMonData(boxMon, MON_DATA_IS_SHINY, &isShiny);
    SetBoxMonData(boxMon, MON_DATA_HIDDEN_NATURE, &hiddenNature);
    SetBoxMonData(boxMon, MON_DATA_TERA_TYPE, &teraType);
}

void HealPokemon(struct Pokemon *mon)
{
    u32 data;

    data = GetMonData(mon, MON_DATA_MAX_HP);
    SetMonData(mon, MON_DATA_HP, &data);

    data = STATUS1_NONE;
    SetMonData(mon, MON_DATA_STATUS, &data);

    MonRestorePP(mon);
}

void HealBoxPokemon(struct BoxPokemon *boxMon)
{
    u32 data;

    data = 0;
    SetBoxMonData(boxMon, MON_DATA_HP_LOST, &data);

    data = STATUS1_NONE;
    SetBoxMonData(boxMon, MON_DATA_STATUS, &data);

    BoxMonRestorePP(boxMon);
}

enum PokemonCry GetCryIdBySpecies(u16 species)
{
    species = SanitizeSpeciesId(species);
    if (P_CRIES_ENABLED == FALSE || gSpeciesInfo[species].cryId >= CRY_COUNT || gTestRunnerHeadless)
        return CRY_NONE;
    return gSpeciesInfo[species].cryId;
}

u16 GetSpeciesPreEvolution(u16 species)
{
    int i, j;

    for (i = SPECIES_BULBASAUR; i < NUM_SPECIES; i++)
    {
        const struct Evolution *evolutions = GetSpeciesEvolutions(i);
        if (evolutions == NULL)
            continue;
        for (j = 0; evolutions[j].method != EVOLUTIONS_END; j++)
        {
            if (SanitizeSpeciesId(evolutions[j].targetSpecies) == species)
                return i;
        }
    }

    return SPECIES_NONE;
}

void UpdateDaysPassedSinceFormChange(u16 days)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        u32 currentSpecies = GetMonData(mon, MON_DATA_SPECIES);
        u8 daysSinceFormChange;

        if (currentSpecies == SPECIES_NONE)
            continue;

        daysSinceFormChange = GetMonData(mon, MON_DATA_DAYS_SINCE_FORM_CHANGE, 0);
        if (daysSinceFormChange == 0)
            continue;

        if (daysSinceFormChange > days)
            daysSinceFormChange -= days;
        else
            daysSinceFormChange = 0;

        SetMonData(mon, MON_DATA_DAYS_SINCE_FORM_CHANGE, &daysSinceFormChange);

        if (daysSinceFormChange == 0)
        {
            u32 targetSpecies = GetFormChangeTargetSpecies(mon, FORM_CHANGE_DAYS_PASSED, 0);

            if (targetSpecies != currentSpecies)
            {
                SetMonData(mon, MON_DATA_SPECIES, &targetSpecies);
                CalculateMonStats(mon);
            }
        }
    }
}

enum Type CheckDynamicMoveType(struct Pokemon *mon, u32 move, u32 battler, enum MonState state)
{
    enum Type moveType = GetDynamicMoveType(mon, move, battler, state);
    if (moveType != TYPE_NONE)
        return moveType;
    return GetMoveType(move);
}

uq4_12_t GetDynamaxLevelHPMultiplier(u32 dynamaxLevel, bool32 inverseMultiplier)
{
    if (inverseMultiplier)
        return UQ_4_12(1.0/(1.5 + 0.05 * dynamaxLevel));
    return UQ_4_12(1.5 + 0.05 * dynamaxLevel);
}

bool32 IsSpeciesRegionalForm(u32 species)
{
    return gSpeciesInfo[species].isAlolanForm
        || gSpeciesInfo[species].isGalarianForm
        || gSpeciesInfo[species].isHisuianForm
        || gSpeciesInfo[species].isPaldeanForm;
}

bool32 IsSpeciesRegionalFormFromRegion(u32 species, u32 region)
{
    switch (region)
    {
    case REGION_ALOLA:  return gSpeciesInfo[species].isAlolanForm;
    case REGION_GALAR:  return gSpeciesInfo[species].isGalarianForm;
    case REGION_HISUI:  return gSpeciesInfo[species].isHisuianForm;
    case REGION_PALDEA: return gSpeciesInfo[species].isPaldeanForm;
    default:            return FALSE;
    }
}

bool32 SpeciesHasRegionalForm(u32 species)
{
    u32 formId;
    const u16 *formTable = GetSpeciesFormTable(species);
    for (formId = 0; formTable != NULL && formTable[formId] != FORM_SPECIES_END; formId++)
    {
        if (IsSpeciesRegionalForm(formTable[formId]))
            return TRUE;
    }
    return FALSE;
}

u32 GetRegionalFormByRegion(u32 species, u32 region)
{
    u32 formId = 0;
    u32 firstFoundSpecies = 0;
    const u16 *formTable = GetSpeciesFormTable(species);

    if (formTable != NULL)
    {
        for (formId = 0; formTable[formId] != FORM_SPECIES_END; formId++)
        {
            if (firstFoundSpecies == 0)
                firstFoundSpecies = formTable[formId];

            if (IsSpeciesRegionalFormFromRegion(formTable[formId], region))
                return formTable[formId];
        }
        if (firstFoundSpecies != 0)
            return firstFoundSpecies;
    }
    return species;
}

bool32 IsSpeciesForeignRegionalForm(u32 species, u32 currentRegion)
{
    u32 i;
    for (i = 0; i < REGIONS_COUNT; i++)
    {
        if (currentRegion != i && IsSpeciesRegionalFormFromRegion(species, i))
            return TRUE;
        else if (currentRegion == i && SpeciesHasRegionalForm(species) && !IsSpeciesRegionalFormFromRegion(species, i))
            return TRUE;
    }
    return FALSE;
}

enum Type GetTeraTypeFromPersonality(struct Pokemon *mon)
{
    const u8 *types = gSpeciesInfo[GetMonData(mon, MON_DATA_SPECIES)].types;
    return (GetMonData(mon, MON_DATA_PERSONALITY) & 0x1) == 0 ? types[0] : types[1];
}

struct Pokemon *GetSavedPlayerPartyMon(u32 index)
{
    return &gSaveBlock1Ptr->playerParty[index];
}

u8 *GetSavedPlayerPartyCount(void)
{
    return &gSaveBlock1Ptr->playerPartyCount;
}

void SavePlayerPartyMon(u32 index, struct Pokemon *mon)
{
    gSaveBlock1Ptr->playerParty[index] = *mon;
}

bool32 IsSpeciesOfType(u32 species, enum Type type)
{
    if (gSpeciesInfo[species].types[0] == type
     || gSpeciesInfo[species].types[1] == type)
        return TRUE;
    return FALSE;
}

//******************* tx_randomizer
// static u16 PickRandomizedSpeciesFromEWRAM(u16 species) //INTERNAL use only!
// {
//     u8 i;
//     u8 depth = 2;
//     u8 startDepth = depth;
//
//     #ifdef GBA_PRINTF
//     mgba_printf(MGBA_LOG_DEBUG, "PickRandomizedSpeciesFromEWRAM(%d = %s)", species, ConvertToAscii(gSpeciesNames[species]) );
//     #endif
//
//     if (gSaveBlock1Ptr->tx_Random_MapBased)
//         depth += GetCurrentRegionMapSectionId() % 4;
//         // depth += NuzlockeGetCurrentRegionMapSectionId() % 4;
//
//     for (i = 0; i < depth; i++)
//     {
//         species = sSpeciesList[species];
//     }
//
//     #ifdef GBA_PRINTF
//         if (gSaveBlock1Ptr->tx_Random_MapBased)
//             mgba_printf(MGBA_LOG_DEBUG, "MapBased: startDepth=%d; new_depth=%d", startDepth, depth);
//         mgba_printf(MGBA_LOG_DEBUG, "depth[%d], new species = %d = %s", i, species, ConvertToAscii(gSpeciesNames[species]));
//         mgba_printf(MGBA_LOG_DEBUG, "");
//     #endif
//
//     return species;
// }
// void RandomizeTypeEffectivenessListEWRAM(u16 seed)
// {
//     u8 i;
//     u8 stemp[RANDOM_TYPE_COUNT];
//
//     memcpy(stemp, sOneTypeChallengeValidTypes, sizeof(sOneTypeChallengeValidTypes));
//     ShuffleListU8(stemp, NELEMS(sOneTypeChallengeValidTypes), seed);
//
//     sTypeEffectivenessList[TYPE_MYSTERY] = TYPE_MYSTERY;
//     for (i=0; i<NUMBER_OF_MON_TYPES; i++)
//     {
//         if (i != TYPE_MYSTERY)
//             sTypeEffectivenessList[i] = stemp[i];
//
//         #ifdef GBA_PRINTF
//             mgba_printf(MGBA_LOG_DEBUG, "sTypeEffectivenessList[%d]: %s => %s", i, ConvertToAscii(gTypeNames[i]), ConvertToAscii(gTypeNames[sTypeEffectivenessList[i]]) );
//         #endif
//     }
//     #ifdef GBA_PRINTF
//         mgba_printf(MGBA_LOG_DEBUG, "**** sTypeEffectivenessList[%d] generated ****", NELEMS(sTypeEffectivenessList));
//         mgba_printf(MGBA_LOG_DEBUG, "");
//     #endif
// }
// u8 GetTypeEffectivenessRandom(u8 type)
// {
//     if (type == TYPE_NONE)
//         return TYPE_NONE;
//
//     if (!gSaveBlock1Ptr->tx_Random_TypeEffectiveness)
//         return type;
//
//     return sTypeEffectivenessList[type];
// }
// u16 PickRandomStarterForOneTypeChallenge(u16 *speciesList, u8 starterId)
// {
//     u16 i, species;
//     u8 typeChallenge = gSaveBlock1Ptr->tx_Challenges_OneTypeChallenge;
//
//     #ifdef GBA_PRINTF
//         mgba_printf(MGBA_LOG_DEBUG, "PickRandomStarterForOneTypeChallenge(starterId=%d)", starterId);
//     #endif
//
//     if ((IsRandomizerActivated() && gSaveBlock1Ptr->tx_Random_Similar) || !IsRandomizerActivated())
//     {
//         u16 *stemp = Alloc(sizeof(sRandomSpeciesEvo0));
//         DmaCopy16(3, sRandomSpeciesEvo0, stemp, sizeof(sRandomSpeciesEvo0));
//         ShuffleListU16(stemp, RANDOM_SPECIES_EVO_0_COUNT, (starterId+13)*12289);
//         for (i=0; i<RANDOM_SPECIES_EVO_0_COUNT; i++)
//         {
//             species = stemp[i];
//             if ((GetTypeBySpecies(species, 1) == typeChallenge || GetTypeBySpecies(species, 2) == typeChallenge) 
//                 && species != speciesList[0] && species != speciesList[1] && species != speciesList[2])
//                 break;
//         }
//
//         if (i == RANDOM_SPECIES_EVO_0_COUNT)
//             species = speciesList[1];
//
//         free(stemp);
//     }
//     else if (gSaveBlock1Ptr->tx_Random_IncludeLegendaries)
//     {
//         u16 *stemp = Alloc(sizeof(sRandomSpeciesLegendary));
//         DmaCopy16(3, sRandomSpeciesLegendary, stemp, sizeof(sRandomSpeciesLegendary));
//         ShuffleListU16(stemp, RANDOM_SPECIES_COUNT_LEGENDARY, (starterId+13)*12289);
//         for (i=0; i<RANDOM_SPECIES_COUNT_LEGENDARY; i++)
//         {
//             species = stemp[i];
//             if ((GetTypeBySpecies(species, 1) == typeChallenge || GetTypeBySpecies(species, 2) == typeChallenge) 
//                 && species != speciesList[0] && species != speciesList[1] && species != speciesList[2])
//                 break;
//         }
//
//         if (i == RANDOM_SPECIES_COUNT_LEGENDARY)
//             species = speciesList[1];
//
//         free(stemp);
//     }
//     else
//     {
//         u16 *stemp = Alloc(sizeof(sRandomSpecies));
//         DmaCopy16(3, sRandomSpecies, stemp, sizeof(sRandomSpecies));
//         ShuffleListU16(stemp, RANDOM_SPECIES_COUNT, (starterId+13)*12289);
//         for (i=0; i<RANDOM_SPECIES_COUNT; i++)
//         {
//             species = stemp[i];
//             if ((GetTypeBySpecies(species, 1) == typeChallenge || GetTypeBySpecies(species, 2) == typeChallenge) 
//                 && species != speciesList[0] && species != speciesList[1] && species != speciesList[2])
//                 break;
//         }
//
//         if (i == RANDOM_SPECIES_COUNT)
//             species = speciesList[1];
//
//         free(stemp);
//     }
//
//     #ifdef GBA_PRINTF
//         mgba_printf(MGBA_LOG_DEBUG, "starterId=%d; species=%d; iterations=%d", starterId, species, i);
//     #endif
//
//     return species;
// }

//******* non EWRAM functions
// u8 GetTypeBySpecies(u16 species, u8 typeNum)
// {
//     u8 type;
//
//     if (typeNum == 1)
//         type = gBaseStats[species].type1;
//     else
//         type = gBaseStats[species].type2;
//
//     if (!gSaveBlock1Ptr->tx_Random_Type)
//         return type;
//
//     type = sOneTypeChallengeValidTypes[RandomSeededModulo(type + typeNum + species, NUMBER_OF_MON_TYPES-1)];
//
//     #ifdef GBA_PRINTF
//     if (gSaveBlock1Ptr->tx_Random_Type)
//         mgba_printf(MGBA_LOG_DEBUG, "TX RANDOM TYPE%d: species=%d; type=%d=%s", typeNum, species , type, ConvertToAscii(gTypeNames[type]));
//     #endif
//
//     return type;
// }

static u8 binarySearchForBst(const u16 *list, u8 count, u16 bst)
{
    u8 low = 0;
    u8 high = count - 1;
    u8 i;
    while(TRUE) {
        i = (high + low) / 2;
        if (i == high || i == low) return i;
        if (sSpeciesAllBst[list[i]] == bst) return i;
        if (sSpeciesAllBst[list[i]] > bst) high = i;
        else low = i;
    }
}

static u16 GetRandomSpecies(u16 species, u8 mapBased, u8 type, u16 additionalOffset) //INTERNAL use only!
{
    // u8 slot, slotNew;
    u16 mapOffset = 0; //12289, 49157
    if (mapBased)
        mapOffset = GetCurrentRegionMapSectionId();

    if (type == TX_RANDOM_T_STARTER_POKEMON) {
        u16 speciesResult = species;
        speciesResult = sRandomSpeciesStarter[RandomSeededModulo(species, RANDOM_SPECIES_STARTER_COUNT)];

        #ifndef NDEBUG
        // slotNew = gSpeciesMapping[speciesResult];
        DebugPrintf("%S: species=%d=%S; mapBased=%d; speciesResult=%d=%S", gRandomizationTypes[type], species, GetSpeciesName(species), mapBased, speciesResult, GetSpeciesName(speciesResult));
        #endif

        return speciesResult;
    }

    if (type == TX_RANDOM_T_STATIC) {
        u16 speciesResult = species;
        speciesResult = sRandomSpeciesLegendary[RandomSeededModulo(species, RANDOM_SPECIES_COUNT_LEGENDARY)];

        #ifndef NDEBUG
        // slotNew = gSpeciesMapping[speciesResult];
        DebugPrintf("%S: species=%d=%S; mapBased=%d; speciesResult=%d=%S", gRandomizationTypes[type], species, GetSpeciesName(species), mapBased, speciesResult, GetSpeciesName(speciesResult));
        #endif

        return speciesResult;
    }

    if (gSaveBlock1Ptr->tx_Random_Similar)
    {
        u16 speciesResult = species;
        u16 maxOffset = 100;
        u16 modulo = 2*maxOffset;
        u16 numMons;
        u16 speciesIndex;
        u8 gymNum = 0;
        if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_RUSTBORO_CITY_GYM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_RUSTBORO_CITY_GYM)
        )
            gymNum = 1;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_DEWFORD_TOWN_GYM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_DEWFORD_TOWN_GYM)
        )
            gymNum = 2;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_MAUVILLE_CITY_GYM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_MAUVILLE_CITY_GYM)
        )
            gymNum = 3;
        else if (
            (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_LAVARIDGE_TOWN_GYM_1F)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_LAVARIDGE_TOWN_GYM_1F))
            || (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_LAVARIDGE_TOWN_GYM_B1F)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_LAVARIDGE_TOWN_GYM_B1F))
        )
            gymNum = 4;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_PETALBURG_CITY_GYM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_PETALBURG_CITY_GYM)
        )
            gymNum = 5;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_FORTREE_CITY_GYM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_FORTREE_CITY_GYM)
        )
            gymNum = 6;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_MOSSDEEP_CITY_GYM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_MOSSDEEP_CITY_GYM)
        )
            gymNum = 7;
        else if (
            (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_SOOTOPOLIS_CITY_GYM_1F)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_SOOTOPOLIS_CITY_GYM_1F))
            || (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_SOOTOPOLIS_CITY_GYM_B1F)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_SOOTOPOLIS_CITY_GYM_B1F))
        )
            gymNum = 8;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EVER_GRANDE_CITY_SIDNEYS_ROOM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EVER_GRANDE_CITY_SIDNEYS_ROOM)
        )
            gymNum = 9;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EVER_GRANDE_CITY_PHOEBES_ROOM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EVER_GRANDE_CITY_PHOEBES_ROOM)
        )
            gymNum = 10;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EVER_GRANDE_CITY_GLACIAS_ROOM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EVER_GRANDE_CITY_GLACIAS_ROOM)
        )
            gymNum = 11;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EVER_GRANDE_CITY_DRAKES_ROOM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EVER_GRANDE_CITY_DRAKES_ROOM)
        )
            gymNum = 12;
        else if (
            gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EVER_GRANDE_CITY_CHAMPIONS_ROOM)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EVER_GRANDE_CITY_CHAMPIONS_ROOM)
        )
            gymNum = 8;

        switch (type) {
            case TX_RANDOM_T_TRAINER:
                if (gymNum > 0)
                {
                    SetUpRandomGymTypes();
                    u8 randomGymType = sRandomGymTypesShuffled[gymNum];
                    #ifndef NDEBUG
                    // DebugPrintf("%S GYM: species=%d=%S; gym=%d; gymType=%d", gRandomizationTypes[type], species, GetSpeciesName(species), gymNum, randomGymType);
                    #endif
                    numMons = sRandomTypeCounts[randomGymType];
                    if (numMons == 0) {
                        gymNum = 0;
                        #ifndef NDEBUG
                        // DebugPrintf("UNEXPECTED 0 mons for type %d", randomGymType);
                        #endif
                    } else {
                        const u16 *typedMons = sRandomTypeLists[randomGymType];
                        u16 originalBst = sSpeciesAllBst[species];
                        #ifndef NDEBUG
                        // DebugPrintf("original bst %d", originalBst);
                        #endif
                        u16 margin = max(originalBst / 10, 30);
                        #ifndef NDEBUG
                        // DebugPrintf("margin %d", margin);
                        #endif
                        u8 bstIndexLow = binarySearchForBst(typedMons, numMons, originalBst - margin);
                        #ifndef NDEBUG
                        // DebugPrintf("lower end: index %d; species: %S", bstIndexLow, GetSpeciesName(typedMons[bstIndexLow]));
                        #endif
                        u8 bstIndexHigh = binarySearchForBst(typedMons, numMons, originalBst + margin);
                        #ifndef NDEBUG
                        // DebugPrintf("higher end: index %d; species: %S", bstIndexHigh, GetSpeciesName(typedMons[bstIndexHigh]));
                        #endif
                        u16 offset = RandomSeededModulo(species + mapOffset + additionalOffset, bstIndexHigh - bstIndexLow + 1);
                        #ifndef NDEBUG
                        // DebugPrintf("randomized offset: %d", offset);
                        #endif
                        speciesResult = typedMons[bstIndexLow + offset];
                        #ifndef NDEBUG
                        // DebugPrintf("end result: index %d; species: %S", bstIndexLow + offset, GetSpeciesName(speciesResult));
                        DebugPrintf("*********** SUMMARY: %S (%d) -> %S (%d)", GetSpeciesName(species), originalBst, GetSpeciesName(speciesResult), sSpeciesAllBst[speciesResult]);
                        #endif
                        return speciesResult;
                    }
                }
                if (gymNum == 0) {
                    numMons = RANDOM_SPECIES_ALL_COUNT;
                    SetUpSpeciesAllLookup();
                    speciesIndex = sRandomSpeciesAllMap[species];
                }
                break;
            case TX_RANDOM_T_WILD_POKEMON:
                numMons = RANDOM_SPECIES_WILD_COUNT;
                SetUpSpeciesWildLookup();
                speciesIndex = sRandomSpeciesWildMap[species];
                break;
            default:
                #ifndef NDEBUG
                DebugPrintf("%S: species=%d=%S; mapBased=%d; UNEXPECTED GetRandomSpecies", gRandomizationTypes[type], species, GetSpeciesName(species), mapBased);
                #endif
                return speciesResult;
        }

        if (speciesIndex < maxOffset) {
            modulo -= (maxOffset - speciesIndex);
        } else if (speciesIndex > (numMons - maxOffset - 1)) {
            modulo -= (speciesIndex - (numMons - maxOffset - 1));
        }

        u16 offset = RandomSeededModulo(species + mapOffset + additionalOffset, modulo);
        u16 index;
        if (speciesIndex < maxOffset) {
            index = offset;
        } else if (speciesIndex > (numMons - maxOffset - 1)) {
            index = numMons - offset - 1;
        } else {
            index = speciesIndex + offset - maxOffset;
        }


        switch(type) {
            case TX_RANDOM_T_WILD_POKEMON:
                speciesResult = sRandomSpeciesWildBst[index];
                break;
            case TX_RANDOM_T_TRAINER:
                speciesResult = sRandomSpeciesAllBst[index];
                break;
        }


        // switch (slot)
        // {
        // case EVO_TYPE_0:
        //     speciesResult = sRandomSpeciesEvo0[RandomSeededModulo(species + mapOffset + additionalOffset, RANDOM_SPECIES_EVO_0_COUNT)];
        //     break;
        // case EVO_TYPE_1:
        //     speciesResult = sRandomSpeciesEvo1[RandomSeededModulo(species + mapOffset + additionalOffset, RANDOM_SPECIES_EVO_1_COUNT)];
        //     break;
        // case EVO_TYPE_2:
        //     speciesResult = sRandomSpeciesEvo2[RandomSeededModulo(species + mapOffset + additionalOffset, RANDOM_SPECIES_EVO_2_COUNT)];
        //     break;
        // case EVO_TYPE_LEGENDARY:
        //     speciesResult = sRandomSpeciesEvoLegendary[RandomSeededModulo(species + mapOffset + additionalOffset, RANDOM_SPECIES_EVO_LEGENDARY_COUNT)];
        //     break;
        // }

        #ifndef NDEBUG
        // slotNew = gSpeciesMapping[speciesResult];
        DebugPrintf("%S: species=%d=%S; mapBased=%d; speciesResult=%d=%S", gRandomizationTypes[type], species, GetSpeciesName(species), mapBased, speciesResult, GetSpeciesName(speciesResult));
        #endif

        return speciesResult;
    }

    // if (gSaveBlock1Ptr->tx_Random_IncludeLegendaries)
    //     return sRandomSpeciesLegendary[RandomSeededModulo(species + mapOffset + additionalOffset, RANDOM_SPECIES_COUNT_LEGENDARY)];

    return sRandomSpecies[RandomSeededModulo(species + mapOffset + additionalOffset, RANDOM_SPECIES_COUNT)];
}

u16 PickRandomStarter(u16 *speciesList, u8 starterId)
{
    // u16 species;
    // if (gSaveBlock1Ptr->tx_Random_Chaos)
    //     return sRandomSpeciesLegendary[RandomSeededModulo(species, RANDOM_SPECIES_COUNT_LEGENDARY)];

    #ifndef NDEBUG
        DebugPrintf("PickRandomStarterr starterId=%d;", starterId);
    #endif
    u16 species = starterId * 3 + 1;
    u8 mapBased = TRUE;
    u8 type = TX_RANDOM_T_STARTER_POKEMON;
    u16 offset = 0;
    species = GetRandomSpecies(species, mapBased, type, offset);
    return species;
    // if (gSaveBlock1Ptr->tx_Random_Similar)
    // {
    //     u16 *stemp = Alloc(sizeof(sRandomSpeciesEvo0));
    //     // TODO 3 stage
    //     DmaCopy16(3, sRandomSpeciesEvo0, stemp, sizeof(sRandomSpeciesEvo0));
    //     ShuffleListU16(stemp, RANDOM_SPECIES_EVO_0_COUNT, 12289);
    //     species = stemp[starterId*27];
    //     #ifndef NDEBUG
    //         s32 i;
    //         for (i = 0; i < 100; i++)
    //             DebugPrintf("PickRandomStarterr species=%d;", stemp[i]);
    //         DebugPrintf("PickRandomStarterr species=%d;", species);
    //     #endif
    //     Free(stemp);
    //     return species;
    // }
    // else if (gSaveBlock1Ptr->tx_Random_IncludeLegendaries)
    // {
    //     u16 *stemp = Alloc(sizeof(sRandomSpeciesLegendary));
    //     DmaCopy16(3, sRandomSpeciesLegendary, stemp, sizeof(sRandomSpeciesLegendary));
    //     ShuffleListU16(stemp, RANDOM_SPECIES_COUNT_LEGENDARY, 12289);
    //     species = stemp[starterId*27];
    //     Free(stemp);
    //     return species;
    // }
    // else
    // {
    //     u16 *stemp = Alloc(sizeof(sRandomSpecies));
    //     DmaCopy16(3, sRandomSpecies, stemp, sizeof(sRandomSpecies));
    //     ShuffleListU16(stemp, RANDOM_SPECIES_COUNT, 12289);
    //     species = stemp[starterId*27];
    //     Free(stemp);
    //     return species;  
    // } 
}

u16 GetSpeciesRandomSeeded(u16 species, u8 type, u16 additionalOffset)
{
    // u8 slot, slotNew;
    u16 speciesResult = species;
    u8 mapBased = FALSE;

    // //CHAOS
    // if (gSaveBlock1Ptr->tx_Random_Chaos)
    //     return sRandomSpeciesLegendary[RandomSeededModulo(species, RANDOM_SPECIES_COUNT_LEGENDARY)];

    //if EVO_TYPE is SELF or LEGENDARY and !tx_Random_IncludeLegendaries
    // slot = gSpeciesMapping[species];
    // if (slot == EVO_TYPE_SELF || (slot == EVO_TYPE_LEGENDARY && !gSaveBlock1Ptr->tx_Random_IncludeLegendaries))
    //     return species;

    //generate species based on the type
    //different types have different parameters, e.g. abilities are never mapBased
    switch(type)
    {
    case TX_RANDOM_T_STARTER_POKEMON:
        mapBased = gSaveBlock1Ptr->tx_Random_MapBased;
            speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
        break;
    case TX_RANDOM_T_WILD_POKEMON:
        mapBased = gSaveBlock1Ptr->tx_Random_MapBased;
        // if (gSaveBlock1Ptr->tx_Random_OneForOne)
        //     speciesResult = PickRandomizedSpeciesFromEWRAM(species);
        // else
            speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
        break;
    case TX_RANDOM_T_TRAINER:
        mapBased = gSaveBlock1Ptr->tx_Random_MapBased;
        // if (gSaveBlock1Ptr->tx_Random_OneForOne)
        //     speciesResult = PickRandomizedSpeciesFromEWRAM(species);
        // else
            speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
        break;
    case TX_RANDOM_T_MOVES:
        speciesResult = sRandomSpeciesLegendary[RandomSeededModulo(species, RANDOM_SPECIES_COUNT_LEGENDARY)];
        break;
    case TX_RANDOM_T_ABILITY:
        speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
        break;
    // case TX_RANDOM_T_EVO:
    //     speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
    //     break;
    // case TX_RANDOM_T_EVO_METH:
    //     speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
    //     break;
    case TX_RANDOM_T_STATIC:
        speciesResult = GetRandomSpecies(species, mapBased, type, additionalOffset);
        break;
    }

    return speciesResult;
}

u16 GetRandomMove(u16 move, u16 species)
{
    // TODO better move randomization
    u16 val = RandomSeededModulo(move + species, RANDOM_MOVES_COUNT);
    u16 final = sRandomValidMoves[val];

    #ifdef GBA_PRINTF
        mgba_printf(MGBA_LOG_DEBUG, "TX RANDOM MOVE     : GetRandomMove: move=%d=%s, species=%d; combined=%d; val=%d; final=%d=%s", move,  ConvertToAscii(gMoveNames[move]), species, move + species, val, final, ConvertToAscii(gMoveNames[final]));
    #endif

    return final;
}

// u8 GetRandomType(void)
// {
//     return sOneTypeChallengeValidTypes[RandomSeededModulo(12289, NUMBER_OF_MON_TYPES-1)];
// }

// Challenges
// u8 EvolutionBlockedByEvoLimit(u16 species)
// {
//     u8 slot = gSpeciesMapping[species];
//     if (slot == EVO_TYPE_1 && gSaveBlock1Ptr->tx_Challenges_EvoLimit == 1) //No Evos already previously checked
//         return TRUE;
//
//     return FALSE;
// }
//
// u8 PickRandomOneTypeChallengeType(void)
// {
//     u16 type = (RandomSeeded(1, TRUE) % (NUMBER_OF_MON_TYPES-1));
//     type = sOneTypeChallengeValidTypes[type];
// }
