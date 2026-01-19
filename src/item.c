#include "global.h"
#include "item.h"
#include "berry.h"
#include "pokeball.h"
#include "string_util.h"
#include "text.h"
#include "event_data.h"
#include "malloc.h"
#include "secret_base.h"
#include "item_menu.h"
#include "party_menu.h"
#include "strings.h"
#include "load_save.h"
#include "item_use.h"
#include "random.h" // tx_randomizer
#include "overworld.h" // tx_randomizer
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "graphics.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/item_effects.h"
#include "constants/hold_effects.h"

#define DUMMY_PC_BAG_POCKET                 \
{                                           \
    .id = POCKET_DUMMY,                     \
    .capacity = PC_ITEMS_COUNT,             \
    .itemSlots = gSaveBlock1Ptr->pcItems,   \
}

static bool32 CheckPyramidBagHasItem(u16 itemId, u16 count);
static bool32 CheckPyramidBagHasSpace(u16 itemId, u16 count);
static const u8 *GetItemPluralName(u16);
static bool32 DoesItemHavePluralName(u16);
static void NONNULL BagPocket_CompactItems(struct BagPocket *pocket);

EWRAM_DATA struct BagPocket gBagPockets[POCKETS_COUNT] = {0};

#include "data/pokemon/item_effects.h"
#include "data/items.h"

#define UNPACK_TM_ITEM_ID(_tm) [CAT(ENUM_TM_HM_, _tm) + 1] = { CAT(ITEM_TM_, _tm), CAT(MOVE_, _tm) },
#define UNPACK_HM_ITEM_ID(_hm) [CAT(ENUM_TM_HM_, _hm) + 1] = { CAT(ITEM_HM_, _hm), CAT(MOVE_, _hm) },

const struct TmHmIndexKey gTMHMItemMoveIds[NUM_ALL_MACHINES + 1] =
{
    [0] = { ITEM_NONE, MOVE_NONE }, // Failsafe
    FOREACH_TM(UNPACK_TM_ITEM_ID)
    FOREACH_HM(UNPACK_HM_ITEM_ID)
    /*
     * Expands to the following:
     *
     * [1] = { ITEM_TM_FOCUS_PUNCH, MOVE_FOCUS_PUNCH },
     * [2] = { ITEM_TM_DRAGON_CLAW, MOVE_DRAGON_CLAW },
     * [3] = { ITEM_TM_WATER_PULSE, MOVE_WATER_PULSE },
     * etc etc
    */
};

#undef UNPACK_TM_ITEM_ID
#undef UNPACK_HM_ITEM_ID

static inline struct ItemSlot NONNULL BagPocket_GetSlotDataGeneric(struct BagPocket *pocket, u32 pocketPos)
{
    return (struct ItemSlot) {
        .itemId = pocket->itemSlots[pocketPos].itemId,
        .quantity = pocket->itemSlots[pocketPos].quantity ^ gSaveBlock2Ptr->encryptionKey,
    };
}

static inline struct ItemSlot NONNULL BagPocket_GetSlotDataPC(struct BagPocket *pocket, u32 pocketPos)
{
    return (struct ItemSlot) {
        .itemId = pocket->itemSlots[pocketPos].itemId,
        .quantity = pocket->itemSlots[pocketPos].quantity,
    };
}

static inline void NONNULL BagPocket_SetSlotDataGeneric(struct BagPocket *pocket, u32 pocketPos, struct ItemSlot newSlot)
{
    pocket->itemSlots[pocketPos].itemId = newSlot.itemId;
    pocket->itemSlots[pocketPos].quantity = newSlot.quantity ^ gSaveBlock2Ptr->encryptionKey;
}

static inline void NONNULL BagPocket_SetSlotDataPC(struct BagPocket *pocket, u32 pocketPos, struct ItemSlot newSlot)
{
    pocket->itemSlots[pocketPos].itemId = newSlot.itemId;
    pocket->itemSlots[pocketPos].quantity = newSlot.quantity;
}

struct ItemSlot NONNULL BagPocket_GetSlotData(struct BagPocket *pocket, u32 pocketPos)
{
    switch (pocket->id)
    {
    case POCKET_ITEMS:
    case POCKET_KEY_ITEMS:
    case POCKET_POKE_BALLS:
    case POCKET_TM_HM:
    case POCKET_BERRIES:
        return BagPocket_GetSlotDataGeneric(pocket, pocketPos);
    case POCKET_DUMMY:
        return BagPocket_GetSlotDataPC(pocket, pocketPos);
    }

    return (struct ItemSlot) {0}; // Because compiler complains
}

void NONNULL BagPocket_SetSlotData(struct BagPocket *pocket, u32 pocketPos, struct ItemSlot newSlot)
{
    if (newSlot.itemId == ITEM_NONE || newSlot.quantity == 0) // Sets to zero if quantity or itemId is zero
    {
        newSlot.itemId = ITEM_NONE;
        newSlot.quantity = 0;
    }

    switch (pocket->id)
    {
    case POCKET_ITEMS:
    case POCKET_KEY_ITEMS:
    case POCKET_POKE_BALLS:
    case POCKET_TM_HM:
    case POCKET_BERRIES:
        BagPocket_SetSlotDataGeneric(pocket, pocketPos, newSlot);
        break;
    case POCKET_DUMMY:
        BagPocket_SetSlotDataPC(pocket, pocketPos, newSlot);
        break;
    }
}

void ApplyNewEncryptionKeyToBagItems(u32 newKey)
{
    enum Pocket pocketId;
    u32 item;
    for (pocketId = 0; pocketId < POCKETS_COUNT; pocketId++)
    {
        for (item = 0; item < gBagPockets[pocketId].capacity; item++)
            ApplyNewEncryptionKeyToHword(&(gBagPockets[pocketId].itemSlots[item].quantity), newKey);
    }
}

void SetBagItemsPointers(void)
{
    gBagPockets[POCKET_ITEMS].itemSlots = gSaveBlock1Ptr->bag.items;
    gBagPockets[POCKET_ITEMS].capacity = BAG_ITEMS_COUNT;
    gBagPockets[POCKET_ITEMS].id = POCKET_ITEMS;

    gBagPockets[POCKET_KEY_ITEMS].itemSlots = gSaveBlock1Ptr->bag.keyItems;
    gBagPockets[POCKET_KEY_ITEMS].capacity = BAG_KEYITEMS_COUNT;
    gBagPockets[POCKET_KEY_ITEMS].id = POCKET_KEY_ITEMS;

    gBagPockets[POCKET_POKE_BALLS].itemSlots = gSaveBlock1Ptr->bag.pokeBalls;
    gBagPockets[POCKET_POKE_BALLS].capacity = BAG_POKEBALLS_COUNT;
    gBagPockets[POCKET_POKE_BALLS].id = POCKET_POKE_BALLS;

    gBagPockets[POCKET_TM_HM].itemSlots = gSaveBlock1Ptr->bag.TMsHMs;
    gBagPockets[POCKET_TM_HM].capacity = BAG_TMHM_COUNT;
    gBagPockets[POCKET_TM_HM].id = POCKET_TM_HM;

    gBagPockets[POCKET_BERRIES].itemSlots = gSaveBlock1Ptr->bag.berries;
    gBagPockets[POCKET_BERRIES].capacity = BAG_BERRIES_COUNT;
    gBagPockets[POCKET_BERRIES].id = POCKET_BERRIES;
}

u8 *CopyItemName(u16 itemId, u8 *dst)
{
    return StringCopy(dst, GetItemName(itemId));
}

const u8 sText_s[] =_("s");

u8 *CopyItemNameHandlePlural(u16 itemId, u8 *dst, u32 quantity)
{
    if (quantity == 1)
    {
        return StringCopy(dst, GetItemName(itemId));
    }
    else if (DoesItemHavePluralName(itemId))
    {
        return StringCopy(dst, GetItemPluralName(itemId));
    }
    else
    {
        u8 *end = StringCopy(dst, GetItemName(itemId));
        return StringCopy(end, sText_s);
    }
}

bool32 IsBagPocketNonEmpty(enum Pocket pocketId)
{
    u8 i;

    for (i = 0; i < gBagPockets[pocketId].capacity; i++)
    {
        if (GetBagItemId(pocketId, i) != ITEM_NONE)
            return TRUE;
    }
    return FALSE;
}

static bool32 NONNULL BagPocket_CheckHasItem(struct BagPocket *pocket, u16 itemId, u16 count)
{
    struct ItemSlot tempItem;

    // Check for item slots that contain the item
    for (u32 i = 0; i < pocket->capacity && count > 0; i++)
    {
        tempItem = BagPocket_GetSlotData(pocket, i);
        if (tempItem.itemId == itemId)
            count -= min(count, tempItem.quantity);
    }

    return count == 0;
}

bool32 CheckBagHasItem(u16 itemId, u16 count)
{
    if (GetItemPocket(itemId) >= POCKETS_COUNT)
        return FALSE;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG) == TRUE)
        return CheckPyramidBagHasItem(itemId, count);

    return BagPocket_CheckHasItem(&gBagPockets[GetItemPocket(itemId)], itemId, count);
}

bool32 HasAtLeastOneBerry(void)
{
    gSpecialVar_Result = FALSE;

    for (u32 i = FIRST_BERRY_INDEX; i <= LAST_BERRY_INDEX && gSpecialVar_Result == FALSE; i++)
        gSpecialVar_Result = CheckBagHasItem(i, 1);

    return gSpecialVar_Result;
}

bool32 HasAtLeastOnePokeBall(void)
{
    for (enum PokeBall ballId = BALL_STRANGE; ballId < POKEBALL_COUNT; ballId++)
    {
        if (CheckBagHasItem(gBallItemIds[ballId], 1) == TRUE)
            return TRUE;
    }
    return FALSE;
}

bool32 CheckBagHasSpace(u16 itemId, u16 count)
{
    if (GetItemPocket(itemId) >= POCKETS_COUNT)
        return FALSE;

    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG) == TRUE)
        return CheckPyramidBagHasSpace(itemId, count);

    return GetFreeSpaceForItemInBag(itemId) >= count;
}

static u32 NONNULL BagPocket_GetFreeSpaceForItem(struct BagPocket *pocket, u16 itemId)
{
    u32 spaceForItem = 0;
    struct ItemSlot tempItem;

    // Check space in any existing item slots that already contain this item
    for (u32 i = 0; i < pocket->capacity; i++)
    {
        tempItem = BagPocket_GetSlotData(pocket, i);
        if (tempItem.itemId == ITEM_NONE || tempItem.itemId == itemId)
            spaceForItem += (tempItem.itemId ? (MAX_BAG_ITEM_CAPACITY - tempItem.quantity) : MAX_BAG_ITEM_CAPACITY);
    }

    return spaceForItem;
}

u32 GetFreeSpaceForItemInBag(u16 itemId)
{
    if (GetItemPocket(itemId) >= POCKETS_COUNT)
        return 0;

    return BagPocket_GetFreeSpaceForItem(&gBagPockets[GetItemPocket(itemId)], itemId);
}

static inline bool32 NONNULL CheckSlotAndUpdateCount(struct BagPocket *pocket, u16 itemId, u32 pocketPos, u32 *nextPocketPos, u16 *count, u16 *tempPocketSlotQuantities)
{
    struct ItemSlot tempItem = BagPocket_GetSlotData(pocket, pocketPos);
    if (tempItem.itemId == ITEM_NONE || tempItem.itemId == itemId)
    {
        // The quantity already at the slot - zero if an empty slot
        if (tempItem.itemId == ITEM_NONE)
            tempItem.quantity = 0;

        // Record slot quantity in tempPocketSlotQuantities, adjust count
        tempPocketSlotQuantities[pocketPos] = min(MAX_BAG_ITEM_CAPACITY, *count + tempItem.quantity);
        *count -= min(*count, MAX_BAG_ITEM_CAPACITY - tempItem.quantity);

        // Set the starting index for the next loop to set items (shifted by one)
        if (!*nextPocketPos)
            *nextPocketPos = pocketPos + 1;

        return TRUE;
    }

    return FALSE;
}

static bool32 NONNULL BagPocket_AddItem(struct BagPocket *pocket, u16 itemId, u16 count)
{
    u32 itemLookupIndex, itemAddIndex = 0;

    // First, check that there is a free slot for this item
    u16 *tempPocketSlotQuantities = AllocZeroed(sizeof(u16) * pocket->capacity);

    switch (pocket->id)
    {
        case POCKET_TM_HM:
        case POCKET_BERRIES:
            for (itemLookupIndex = 0; itemLookupIndex < pocket->capacity && count > 0; itemLookupIndex++)
            {
                // Check if we found a slot to store the item but weren't able to reduce count to 0
                // This means that we have more than one stack's worth, which isn't allowed in these pockets
                if (CheckSlotAndUpdateCount(pocket, itemId, itemLookupIndex, &itemAddIndex, &count, tempPocketSlotQuantities) && count > 0)
                {
                    Free(tempPocketSlotQuantities);
                    return FALSE;
                }
            }
            break;
        default:
            for (itemLookupIndex = 0; itemLookupIndex < pocket->capacity && count > 0; itemLookupIndex++)
                CheckSlotAndUpdateCount(pocket, itemId, itemLookupIndex, &itemAddIndex, &count, tempPocketSlotQuantities);
    }

    // If the count is still greater than zero, clearly we have not found enough slots for this...
    // Otherwise, we have found slots - update the actual pockets with the updated quantities
    if (count == 0)
    {
        for (--itemAddIndex; itemAddIndex < itemLookupIndex; itemAddIndex++)
        {
            if (tempPocketSlotQuantities[itemAddIndex] > 0)
                BagPocket_SetSlotItemIdAndCount(pocket, itemAddIndex, itemId, tempPocketSlotQuantities[itemAddIndex]);
        }
    }

    Free(tempPocketSlotQuantities);
    return count == 0;
}

bool32 AddBagItem(u16 itemId, u16 count)
{
    if (GetItemPocket(itemId) >= POCKETS_COUNT)
        return FALSE;

    // check Battle Pyramid Bag
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG) == TRUE)
        return AddPyramidBagItem(itemId, count);

    return BagPocket_AddItem(&gBagPockets[GetItemPocket(itemId)], itemId, count);
}

static bool32 NONNULL BagPocket_RemoveItem(struct BagPocket *pocket, u16 itemId, u16 count)
{
    u32 itemLookupIndex, itemRemoveIndex = 0, totalQuantity = 0;
    struct ItemSlot tempItem;
    u16 *tempPocketSlotQuantities = AllocZeroed(sizeof(u16) * pocket->capacity);

    for (itemLookupIndex = 0; itemLookupIndex < pocket->capacity && totalQuantity < count; itemLookupIndex++)
    {
        tempItem = BagPocket_GetSlotData(pocket, itemLookupIndex);
        if (tempItem.itemId == itemId)
        {
            // Index for the next loop - where we should start removing items
            if (!itemRemoveIndex)
                itemRemoveIndex = itemLookupIndex + 1;

            // Gather quantities (+ 1 to tempPocketSlotQuantities so that even if setting to 0 we know which indices to target)
            totalQuantity += tempItem.quantity;
            tempPocketSlotQuantities[itemLookupIndex] = (tempItem.quantity <= count ? 0 : tempItem.quantity - count) + 1;
        }
    }

    if (totalQuantity >= count) // We have enough of the item
    {
        if (CurMapIsSecretBase() == TRUE)
        {
            VarSet(VAR_SECRET_BASE_LOW_TV_FLAGS, VarGet(VAR_SECRET_BASE_LOW_TV_FLAGS) | SECRET_BASE_USED_BAG);
            VarSet(VAR_SECRET_BASE_LAST_ITEM_USED, itemId);
        }

        // Update the quantities correctly with the items removed
        for (--itemRemoveIndex; itemRemoveIndex < itemLookupIndex; itemRemoveIndex++)
        {
            if (tempPocketSlotQuantities[itemRemoveIndex] > 0)
                BagPocket_SetSlotItemIdAndCount(pocket, itemRemoveIndex, itemId, tempPocketSlotQuantities[itemRemoveIndex] - 1);
        }
    }

    if (totalQuantity == count)
        BagPocket_CompactItems(pocket);

    Free(tempPocketSlotQuantities);
    return totalQuantity >= count;
}

bool32 RemoveBagItem(u16 itemId, u16 count)
{
    if (GetItemPocket(itemId) >= POCKETS_COUNT || itemId == ITEM_NONE)
        return FALSE;

    // check Battle Pyramid Bag
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG) == TRUE)
        return RemovePyramidBagItem(itemId, count);

    return BagPocket_RemoveItem(&gBagPockets[GetItemPocket(itemId)], itemId, count);
}

// Unsafe function: Only use with functions that already check the slot and count are valid
void RemoveBagItemFromSlot(struct BagPocket *pocket, u16 slotId, u16 count)
{
    struct ItemSlot itemSlot = BagPocket_GetSlotData(pocket, slotId);
    BagPocket_SetSlotItemIdAndCount(pocket, slotId, itemSlot.itemId, itemSlot.quantity - count);
}

static u8 NONNULL BagPocket_CountUsedItemSlots(struct BagPocket *pocket)
{
    u8 usedSlots = 0;

    for (u32 i = 0; i < pocket->capacity; i++)
    {
        if (BagPocket_GetSlotData(pocket, i).itemId != ITEM_NONE)
            usedSlots++;
    }
    return usedSlots;
}

u8 CountUsedPCItemSlots(void)
{
    struct BagPocket dummyPocket = DUMMY_PC_BAG_POCKET;
    return BagPocket_CountUsedItemSlots(&dummyPocket);
}

static bool32 NONNULL BagPocket_CheckPocketForItemCount(struct BagPocket *pocket, u16 itemId, u16 count)
{
    struct ItemSlot tempItem;

    for (u32 i = 0; i < pocket->capacity; i++)
    {
        tempItem = BagPocket_GetSlotData(pocket, i);
        if (tempItem.itemId == itemId && tempItem.quantity >= count)
            return TRUE;
    }
    return FALSE;
}

bool32 CheckPCHasItem(u16 itemId, u16 count)
{
    struct BagPocket dummyPocket = DUMMY_PC_BAG_POCKET;
    return BagPocket_CheckPocketForItemCount(&dummyPocket, itemId, count);
}

bool32 AddPCItem(u16 itemId, u16 count)
{
    struct BagPocket dummyPocket = DUMMY_PC_BAG_POCKET;
    return BagPocket_AddItem(&dummyPocket, itemId, count);
}

static void NONNULL BagPocket_CompactItems(struct BagPocket *pocket)
{
    struct ItemSlot tempItem;
    u32 slotCursor = 0;
    for (u32 i = 0; i < pocket->capacity; i++)
    {
        tempItem = BagPocket_GetSlotData(pocket, i);
        if (tempItem.itemId == ITEM_NONE)
        {
            if (!slotCursor)
                slotCursor = i + 1;
        }
        else if (slotCursor > 0)
        {
            BagPocket_SetSlotData(pocket, slotCursor++ - 1, tempItem);
            BagPocket_SetSlotItemIdAndCount(pocket, i, ITEM_NONE, 0);
        }
    }
}

void RemovePCItem(u8 index, u16 count)
{
    struct BagPocket dummyPocket = DUMMY_PC_BAG_POCKET;

    // Get id, quantity at slot
    struct ItemSlot tempItem = BagPocket_GetSlotData(&dummyPocket, index);

    // Remove quantity
    BagPocket_SetSlotItemIdAndCount(&dummyPocket, index, tempItem.itemId, tempItem.quantity - count);

    // Compact if necessary
    if (tempItem.quantity == 0)
        BagPocket_CompactItems(&dummyPocket);
}

void CompactPCItems(void)
{
    struct BagPocket dummyPocket = DUMMY_PC_BAG_POCKET;
    BagPocket_CompactItems(&dummyPocket);
}

void SwapRegisteredBike(void)
{
    switch (gSaveBlock1Ptr->registeredItem)
    {
    case ITEM_MACH_BIKE:
        gSaveBlock1Ptr->registeredItem = ITEM_ACRO_BIKE;
        break;
    case ITEM_ACRO_BIKE:
        gSaveBlock1Ptr->registeredItem = ITEM_MACH_BIKE;
        break;
    }
}

void CompactItemsInBagPocket(enum Pocket pocketId)
{
    BagPocket_CompactItems(&gBagPockets[pocketId]);
}

static inline void NONNULL BagPocket_MoveItemSlot(struct BagPocket *pocket, u32 from, u32 to)
{
    if (from != to)
    {
        s8 shift = (to > from) ? 1 : -1;
        if (to > from)
            to--;

        // Record the values at "from"
        struct ItemSlot fromSlot = BagPocket_GetSlotData(pocket, from);

        // Shuffle items between "to" and "from"
        for (u32 i = from; i != to; i += shift)
            BagPocket_SetSlotData(pocket, i, BagPocket_GetSlotData(pocket, i + shift));

        // Move the saved "from" to "to"
        BagPocket_SetSlotData(pocket, to, fromSlot);
    }
}

void MoveItemSlotInPocket(enum Pocket pocketId, u32 from, u32 to)
{
    BagPocket_MoveItemSlot(&gBagPockets[pocketId], from, to);
}

void MoveItemSlotInPC(struct ItemSlot *itemSlots, u32 from, u32 to)
{
    struct BagPocket dummyPocket = DUMMY_PC_BAG_POCKET;
    return BagPocket_MoveItemSlot(&dummyPocket, from, to);
}

void ClearBag(void)
{
    CpuFastFill(0, &gSaveBlock1Ptr->bag, sizeof(struct Bag));
}

static inline u16 NONNULL BagPocket_CountTotalItemQuantity(struct BagPocket *pocket, u16 itemId)
{
    u32 ownedCount = 0;
    struct ItemSlot tempItem;

    for (u32 i = 0; i < pocket->capacity; i++)
    {
        tempItem = BagPocket_GetSlotData(pocket, i);
        if (tempItem.itemId == itemId)
            ownedCount += tempItem.quantity;
    }

    return ownedCount;
}

u16 CountTotalItemQuantityInBag(u16 itemId)
{
    return BagPocket_CountTotalItemQuantity(&gBagPockets[GetItemPocket(itemId)], itemId);
}

static bool32 CheckPyramidBagHasItem(u16 itemId, u16 count)
{
    u8 i;
    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
#if MAX_PYRAMID_BAG_ITEM_CAPACITY > 255
    u16 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
#else
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
#endif

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        if (items[i] == itemId)
        {
            if (quantities[i] >= count)
                return TRUE;

            count -= quantities[i];
            if (count == 0)
                return TRUE;
        }
    }

    return FALSE;
}

static bool32 CheckPyramidBagHasSpace(u16 itemId, u16 count)
{
    u8 i;
    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
#if MAX_PYRAMID_BAG_ITEM_CAPACITY > 255
    u16 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
#else
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
#endif

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        if (items[i] == itemId || items[i] == ITEM_NONE)
        {
            if (quantities[i] + count <= MAX_PYRAMID_BAG_ITEM_CAPACITY)
                return TRUE;

            count = (quantities[i] + count) - MAX_PYRAMID_BAG_ITEM_CAPACITY;
            if (count == 0)
                return TRUE;
        }
    }

    return FALSE;
}

bool32 AddPyramidBagItem(u16 itemId, u16 count)
{
    u16 i;

    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u16 *newItems = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));

#if MAX_PYRAMID_BAG_ITEM_CAPACITY > 255
    u16 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
    u16 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
#else
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
#endif

    memcpy(newItems, items, PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));
    memcpy(newQuantities, quantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        if (newItems[i] == itemId && newQuantities[i] < MAX_PYRAMID_BAG_ITEM_CAPACITY)
        {
            newQuantities[i] += count;
            if (newQuantities[i] > MAX_PYRAMID_BAG_ITEM_CAPACITY)
            {
                count = newQuantities[i] - MAX_PYRAMID_BAG_ITEM_CAPACITY;
                newQuantities[i] = MAX_PYRAMID_BAG_ITEM_CAPACITY;
            }
            else
            {
                count = 0;
            }

            if (count == 0)
                break;
        }
    }

    if (count > 0)
    {
        for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
        {
            if (newItems[i] == ITEM_NONE)
            {
                newItems[i] = itemId;
                newQuantities[i] = count;
                if (newQuantities[i] > MAX_PYRAMID_BAG_ITEM_CAPACITY)
                {
                    count = newQuantities[i] - MAX_PYRAMID_BAG_ITEM_CAPACITY;
                    newQuantities[i] = MAX_PYRAMID_BAG_ITEM_CAPACITY;
                }
                else
                {
                    count = 0;
                }

                if (count == 0)
                    break;
            }
        }
    }

    if (count == 0)
    {
        memcpy(items, newItems, PYRAMID_BAG_ITEMS_COUNT * sizeof(*items));
        memcpy(quantities, newQuantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(*quantities));
        Free(newItems);
        Free(newQuantities);
        return TRUE;
    }
    else
    {
        Free(newItems);
        Free(newQuantities);
        return FALSE;
    }
}

bool32 RemovePyramidBagItem(u16 itemId, u16 count)
{
    u16 i;

    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
#if MAX_PYRAMID_BAG_ITEM_CAPACITY > 255
    u16 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
#else
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
#endif

    i = gPyramidBagMenuState.cursorPosition + gPyramidBagMenuState.scrollPosition;
    if (items[i] == itemId && quantities[i] >= count)
    {
        quantities[i] -= count;
        if (quantities[i] == 0)
            items[i] = ITEM_NONE;
        return TRUE;
    }
    else
    {
        u16 *newItems = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));
    #if MAX_PYRAMID_BAG_ITEM_CAPACITY > 255
        u16 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
    #else
        u8 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));
    #endif

        memcpy(newItems, items, PYRAMID_BAG_ITEMS_COUNT * sizeof(*newItems));
        memcpy(newQuantities, quantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(*newQuantities));

        for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
        {
            if (newItems[i] == itemId)
            {
                if (newQuantities[i] >= count)
                {
                    newQuantities[i] -= count;
                    count = 0;
                    if (newQuantities[i] == 0)
                        newItems[i] = ITEM_NONE;
                }
                else
                {
                    count -= newQuantities[i];
                    newQuantities[i] = 0;
                    newItems[i] = ITEM_NONE;
                }

                if (count == 0)
                    break;
            }
        }

        if (count == 0)
        {
            memcpy(items, newItems, PYRAMID_BAG_ITEMS_COUNT * sizeof(*items));
            memcpy(quantities, newQuantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(*quantities));
            Free(newItems);
            Free(newQuantities);
            return TRUE;
        }
        else
        {
            Free(newItems);
            Free(newQuantities);
            return FALSE;
        }
    }
}

static u16 SanitizeItemId(u16 itemId)
{
    if (itemId >= ITEMS_COUNT)
        return ITEM_NONE;
    else
        return itemId;
}

const u8 *GetItemName(u16 itemId)
{
    const u8 *name = gItemsInfo[SanitizeItemId(itemId)].name;

    return name == NULL ? gQuestionMarksItemName : name;
}

u32 GetItemPrice(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].price;
}

static bool32 DoesItemHavePluralName(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].pluralName != NULL;
}

static const u8 *GetItemPluralName(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].pluralName;
}

const u8 *GetItemEffect(u32 itemId)
{
    if (itemId == ITEM_ENIGMA_BERRY_E_READER)
    #if FREE_ENIGMA_BERRY == FALSE
        return gSaveBlock1Ptr->enigmaBerry.itemEffect;
    #else
        return 0;
    #endif //FREE_ENIGMA_BERRY
    else
        return gItemsInfo[SanitizeItemId(itemId)].effect;
}

enum HoldEffect GetItemHoldEffect(u32 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].holdEffect;
}

u32 GetItemHoldEffectParam(u32 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].holdEffectParam;
}

const u8 *GetItemDescription(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].description;
}

u8 GetItemImportance(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].importance;
}

u8 GetItemConsumability(u16 itemId)
{
    return !gItemsInfo[SanitizeItemId(itemId)].notConsumed;
}

enum Pocket GetItemPocket(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].pocket;
}

u8 GetItemType(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].type;
}

ItemUseFunc GetItemFieldFunc(u16 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].fieldUseFunc;
}

// Returns an item's battle effect script ID.
u8 GetItemBattleUsage(u16 itemId)
{
    u16 item = SanitizeItemId(itemId);
    // Handle E-Reader berries.
    if (item == ITEM_ENIGMA_BERRY_E_READER)
    {
        switch (GetItemEffectType(gSpecialVar_ItemId))
        {
            case ITEM_EFFECT_X_ITEM:
                return EFFECT_ITEM_INCREASE_STAT;
            case ITEM_EFFECT_HEAL_HP:
                return EFFECT_ITEM_RESTORE_HP;
            case ITEM_EFFECT_CURE_POISON:
            case ITEM_EFFECT_CURE_SLEEP:
            case ITEM_EFFECT_CURE_BURN:
            case ITEM_EFFECT_CURE_FREEZE_FROSTBITE:
            case ITEM_EFFECT_CURE_PARALYSIS:
            case ITEM_EFFECT_CURE_ALL_STATUS:
            case ITEM_EFFECT_CURE_CONFUSION:
            case ITEM_EFFECT_CURE_INFATUATION:
                return EFFECT_ITEM_CURE_STATUS;
            case ITEM_EFFECT_HEAL_PP:
                return EFFECT_ITEM_RESTORE_PP;
            default:
                return 0;
        }
    }
    else
        return gItemsInfo[item].battleUsage;
}

u32 GetItemSecondaryId(u32 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].secondaryId;
}

// tx_randomizer
#define RANDOM_ITEM_COUNT ARRAY_COUNT(sRandomValidItems)
static const u16 sRandomValidItems[] =
{
#ifndef ITEM_EXPANSION
    // Balls,
    ITEM_MASTER_BALL,
    ITEM_ULTRA_BALL,
    ITEM_GREAT_BALL,
    ITEM_POKE_BALL,
    ITEM_SAFARI_BALL,
    ITEM_NET_BALL,
    ITEM_DIVE_BALL,
    ITEM_NEST_BALL,
    ITEM_REPEAT_BALL,
    ITEM_TIMER_BALL,
    ITEM_LUXURY_BALL,
    ITEM_PREMIER_BALL,
    // Pokemon Items,
    ITEM_POTION,
    ITEM_ANTIDOTE,
    ITEM_BURN_HEAL,
    ITEM_ICE_HEAL,
    ITEM_AWAKENING,
    ITEM_PARALYZE_HEAL,
    ITEM_FULL_RESTORE,
    ITEM_MAX_POTION,
    ITEM_HYPER_POTION,
    ITEM_SUPER_POTION,
    ITEM_FULL_HEAL,
    ITEM_REVIVE,
    ITEM_MAX_REVIVE,
    ITEM_FRESH_WATER,
    ITEM_SODA_POP,
    ITEM_LEMONADE,
    ITEM_MOOMOO_MILK,
    ITEM_ENERGY_POWDER,
    ITEM_ENERGY_ROOT,
    ITEM_HEAL_POWDER,
    ITEM_REVIVAL_HERB,
    ITEM_ETHER,
    ITEM_MAX_ETHER,
    ITEM_ELIXIR,
    ITEM_MAX_ELIXIR,
    ITEM_LAVA_COOKIE,
    ITEM_BLUE_FLUTE,
    ITEM_YELLOW_FLUTE,
    ITEM_RED_FLUTE,
    ITEM_BLACK_FLUTE,
    ITEM_WHITE_FLUTE,
    ITEM_BERRY_JUICE,
    ITEM_SACRED_ASH,
    ITEM_SHOAL_SALT,
    ITEM_SHOAL_SHELL,
    ITEM_RED_SHARD,
    ITEM_BLUE_SHARD,
    ITEM_YELLOW_SHARD,
    ITEM_GREEN_SHARD,
    ITEM_HP_UP,
    ITEM_PROTEIN,
    ITEM_IRON,
    ITEM_CARBOS,
    ITEM_CALCIUM,
    ITEM_RARE_CANDY,
    ITEM_PP_UP,
    ITEM_ZINC,
    ITEM_PP_MAX,
    ITEM_GUARD_SPEC,
    ITEM_DIRE_HIT,
    ITEM_X_ATTACK,
    ITEM_X_DEFEND,
    ITEM_X_SPEED,
    ITEM_X_ACCURACY,
    ITEM_X_SPECIAL,
    ITEM_POKE_DOLL,
    ITEM_FLUFFY_TAIL,
    ITEM_SUPER_REPEL,
    ITEM_MAX_REPEL,
    ITEM_ESCAPE_ROPE,
    ITEM_REPEL,
    ITEM_SUN_STONE,
    ITEM_MOON_STONE,
    ITEM_FIRE_STONE,
    ITEM_THUNDER_STONE,
    ITEM_WATER_STONE,
    ITEM_LEAF_STONE,
    // Mails,
    ITEM_ORANGE_MAIL,
    ITEM_HARBOR_MAIL,
    ITEM_GLITTER_MAIL,
    ITEM_MECH_MAIL,
    ITEM_WOOD_MAIL,
    ITEM_WAVE_MAIL,
    ITEM_BEAD_MAIL,
    ITEM_SHADOW_MAIL,
    ITEM_TROPIC_MAIL,
    ITEM_DREAM_MAIL,
    ITEM_FAB_MAIL,
    ITEM_RETRO_MAIL,
    // Berries,
    ITEM_CHERI_BERRY,
    ITEM_CHESTO_BERRY,
    ITEM_PECHA_BERRY,
    ITEM_RAWST_BERRY,
    ITEM_ASPEAR_BERRY,
    ITEM_LEPPA_BERRY,
    ITEM_ORAN_BERRY,
    ITEM_PERSIM_BERRY,
    ITEM_LUM_BERRY,
    ITEM_SITRUS_BERRY,
    ITEM_FIGY_BERRY,
    ITEM_WIKI_BERRY,
    ITEM_MAGO_BERRY,
    ITEM_AGUAV_BERRY,
    ITEM_IAPAPA_BERRY,
    // ITEM_RAZZ_BERRY,
    // ITEM_BLUK_BERRY,
    // ITEM_NANAB_BERRY,
    // ITEM_WEPEAR_BERRY,
    // ITEM_PINAP_BERRY,
    ITEM_POMEG_BERRY,
    ITEM_KELPSY_BERRY,
    ITEM_QUALOT_BERRY,
    ITEM_HONDEW_BERRY,
    ITEM_GREPA_BERRY,
    ITEM_TAMATO_BERRY,
    // ITEM_CORNN_BERRY,
    // ITEM_MAGOST_BERRY,
    // ITEM_RABUTA_BERRY,
    // ITEM_NOMEL_BERRY,
    // ITEM_SPELON_BERRY,
    // ITEM_PAMTRE_BERRY,
    // ITEM_WATMEL_BERRY,
    // ITEM_DURIN_BERRY,
    // ITEM_BELUE_BERRY,
    ITEM_LIECHI_BERRY,
    ITEM_GANLON_BERRY,
    ITEM_SALAC_BERRY,
    ITEM_PETAYA_BERRY,
    ITEM_APICOT_BERRY,
    ITEM_LANSAT_BERRY,
    ITEM_STARF_BERRY,
    ITEM_ENIGMA_BERRY,
    // Battle Held items,
    ITEM_BRIGHT_POWDER,
    ITEM_WHITE_HERB,
    ITEM_MACHO_BRACE,
    ITEM_EXP_SHARE,
    ITEM_QUICK_CLAW,
    ITEM_SOOTHE_BELL,
    ITEM_MENTAL_HERB,
    ITEM_CHOICE_BAND,
    ITEM_KINGS_ROCK,
    ITEM_SILVER_POWDER,
    ITEM_AMULET_COIN,
    ITEM_CLEANSE_TAG,
    ITEM_SOUL_DEW,
    ITEM_DEEP_SEA_TOOTH,
    ITEM_DEEP_SEA_SCALE,
    ITEM_SMOKE_BALL,
    ITEM_EVERSTONE,
    ITEM_FOCUS_BAND,
    ITEM_LUCKY_EGG,
    ITEM_SCOPE_LENS,
    ITEM_METAL_COAT,
    ITEM_LEFTOVERS,
    ITEM_DRAGON_SCALE,
    ITEM_LIGHT_BALL,
    ITEM_SOFT_SAND,
    ITEM_HARD_STONE,
    ITEM_MIRACLE_SEED,
    ITEM_BLACK_GLASSES,
    ITEM_BLACK_BELT,
    ITEM_MAGNET,
    ITEM_MYSTIC_WATER,
    ITEM_SHARP_BEAK,
    ITEM_POISON_BARB,
    ITEM_NEVER_MELT_ICE,
    ITEM_SPELL_TAG,
    ITEM_TWISTED_SPOON,
    ITEM_CHARCOAL,
    ITEM_DRAGON_FANG,
    ITEM_SILK_SCARF,
    ITEM_UP_GRADE,
    ITEM_SHELL_BELL,
    ITEM_SEA_INCENSE,
    ITEM_LAX_INCENSE,
    ITEM_LUCKY_PUNCH,
    ITEM_METAL_POWDER,
    ITEM_THICK_CLUB,
    ITEM_STICK,
    // Contest held items,
    // ITEM_RED_SCARF,
    // ITEM_BLUE_SCARF,
    // ITEM_PINK_SCARF,
    // ITEM_GREEN_SCARF,
    // ITEM_YELLOW_SCARF,
#else
    // Poké Balls
    ITEM_POKE_BALL,
    ITEM_GREAT_BALL,
    ITEM_ULTRA_BALL,
    ITEM_MASTER_BALL,
    ITEM_PREMIER_BALL,
    ITEM_HEAL_BALL,
    ITEM_NET_BALL,
    ITEM_NEST_BALL,
    ITEM_DIVE_BALL,
    ITEM_DUSK_BALL,
    ITEM_TIMER_BALL,
    ITEM_QUICK_BALL,
    ITEM_REPEAT_BALL,
    ITEM_LUXURY_BALL,
    ITEM_LEVEL_BALL,
    ITEM_LURE_BALL,
    ITEM_MOON_BALL,
    ITEM_FRIEND_BALL,
    ITEM_LOVE_BALL,
    ITEM_FAST_BALL,
    ITEM_HEAVY_BALL,
    ITEM_DREAM_BALL,
    ITEM_SAFARI_BALL,
    //ITEM_SPORT_BALL,
    //ITEM_PARK_BALL,
    ITEM_BEAST_BALL,
    ITEM_CHERISH_BALL,
    // Medicine
    ITEM_POTION,
    ITEM_SUPER_POTION,
    ITEM_HYPER_POTION,
    ITEM_MAX_POTION,
    ITEM_FULL_RESTORE,
    ITEM_REVIVE,
    ITEM_MAX_REVIVE,
    ITEM_FRESH_WATER,
    ITEM_SODA_POP,
    ITEM_LEMONADE,
    ITEM_MOOMOO_MILK,
    ITEM_ENERGY_POWDER,
    ITEM_ENERGY_ROOT,
    ITEM_HEAL_POWDER,
    ITEM_REVIVAL_HERB,
    ITEM_ANTIDOTE,
    ITEM_PARALYZE_HEAL,
    ITEM_BURN_HEAL,
    ITEM_ICE_HEAL,
    ITEM_AWAKENING,
    ITEM_FULL_HEAL,
    ITEM_ETHER,
    ITEM_MAX_ETHER,
    ITEM_ELIXIR,
    ITEM_MAX_ELIXIR,
    ITEM_BERRY_JUICE,
    ITEM_SACRED_ASH,
    // ITEM_SWEET_HEART,
    // ITEM_MAX_HONEY,
    // Regional Specialties
    ITEM_PEWTER_CRUNCHIES,
    ITEM_RAGE_CANDY_BAR,
    ITEM_LAVA_COOKIE,
    ITEM_OLD_GATEAU,
    ITEM_CASTELIACONE,
    ITEM_LUMIOSE_GALETTE,
    ITEM_SHALOUR_SABLE,
    ITEM_BIG_MALASADA,
    // Vitamins
    ITEM_HP_UP,
    ITEM_PROTEIN,
    ITEM_IRON,
    ITEM_CALCIUM,
    ITEM_ZINC,
    ITEM_CARBOS,
    ITEM_PP_UP,
    ITEM_PP_MAX,
    // EV Feathers
    // ITEM_HEALTH_FEATHER,
    // ITEM_MUSCLE_FEATHER,
    // ITEM_RESIST_FEATHER,
    // ITEM_GENIUS_FEATHER,
    // ITEM_CLEVER_FEATHER,
    // ITEM_SWIFT_FEATHER,
    // Ability Modifiers
    ITEM_ABILITY_CAPSULE,
    ITEM_ABILITY_PATCH,
    // Mints
    ITEM_LONELY_MINT,
    ITEM_ADAMANT_MINT,
    ITEM_NAUGHTY_MINT,
    ITEM_BRAVE_MINT,
    ITEM_BOLD_MINT,
    ITEM_IMPISH_MINT,
    ITEM_LAX_MINT,
    ITEM_RELAXED_MINT,
    ITEM_MODEST_MINT,
    ITEM_MILD_MINT,
    ITEM_RASH_MINT,
    ITEM_QUIET_MINT,
    ITEM_CALM_MINT,
    ITEM_GENTLE_MINT,
    ITEM_CAREFUL_MINT,
    ITEM_SASSY_MINT,
    ITEM_TIMID_MINT,
    ITEM_HASTY_MINT,
    ITEM_JOLLY_MINT,
    ITEM_NAIVE_MINT,
    ITEM_SERIOUS_MINT,
    // Candy
    ITEM_RARE_CANDY,
    //ITEM_EXP_CANDY_XS,
    //ITEM_EXP_CANDY_S,
    //ITEM_EXP_CANDY_M,
    //ITEM_EXP_CANDY_L,
    //ITEM_EXP_CANDY_XL,
    //ITEM_DYNAMAX_CANDY,
    // // Medicinal Flutes
    // ITEM_BLUE_FLUTE,
    // ITEM_YELLOW_FLUTE,
    // ITEM_RED_FLUTE,
    // // Encounter-modifying Flutes
    // ITEM_BLACK_FLUTE,
    // ITEM_WHITE_FLUTE,
    // Encounter Modifiers
    ITEM_REPEL,
    ITEM_SUPER_REPEL,
    ITEM_MAX_REPEL,
    //ITEM_LURE,
    //ITEM_SUPER_LURE,
    //ITEM_MAX_LURE,
    ITEM_ESCAPE_ROPE,
    // X Items
    ITEM_X_ATTACK,
    ITEM_X_DEFENSE,
    ITEM_X_SP_ATK,
    ITEM_X_SP_DEF,
    ITEM_X_SPEED,
    ITEM_X_ACCURACY,
    ITEM_DIRE_HIT,
    ITEM_GUARD_SPEC,
    // Escape Items
    ITEM_POKE_DOLL,
    ITEM_FLUFFY_TAIL,
    ITEM_POKE_TOY,
    //ITEM_MAX_MUSHROOMS,
    // Treasures
    //ITEM_BOTTLE_CAP,
    //ITEM_GOLD_BOTTLE_CAP,
    ITEM_NUGGET,
    ITEM_BIG_NUGGET,
    ITEM_TINY_MUSHROOM,
    ITEM_BIG_MUSHROOM,
    ITEM_BALM_MUSHROOM,
    ITEM_PEARL,
    ITEM_BIG_PEARL,
    ITEM_PEARL_STRING,
    ITEM_STARDUST,
    ITEM_STAR_PIECE,
    ITEM_COMET_SHARD,
    ITEM_SHOAL_SALT,
    ITEM_SHOAL_SHELL,
    ITEM_RED_SHARD,
    ITEM_BLUE_SHARD,
    ITEM_YELLOW_SHARD,
    ITEM_GREEN_SHARD,
    ITEM_HEART_SCALE,
    ITEM_HONEY,
    ITEM_RARE_BONE,
    ITEM_ODD_KEYSTONE,
    ITEM_PRETTY_FEATHER,
    //ITEM_RELIC_COPPER,
    //ITEM_RELIC_SILVER,
    //ITEM_RELIC_GOLD,
    //ITEM_RELIC_VASE,
    //ITEM_RELIC_BAND,
    //ITEM_RELIC_STATUE,
    //ITEM_RELIC_CROWN,
    ITEM_STRANGE_SOUVENIR,
    // Fossils
    //ITEM_HELIX_FOSSIL,
    //ITEM_DOME_FOSSIL,
    //ITEM_OLD_AMBER,
    ITEM_ROOT_FOSSIL,
    ITEM_CLAW_FOSSIL,
    //ITEM_ARMOR_FOSSIL,
    //ITEM_SKULL_FOSSIL,
    //ITEM_COVER_FOSSIL,
    //ITEM_PLUME_FOSSIL,
    //ITEM_JAW_FOSSIL,
    //ITEM_SAIL_FOSSIL,
    //ITEM_FOSSILIZED_BIRD,
    //ITEM_FOSSILIZED_FISH,
    //ITEM_FOSSILIZED_DRAKE,
    //ITEM_FOSSILIZED_DINO,
    // Mulch
    //ITEM_GROWTH_MULCH,
    //ITEM_DAMP_MULCH,
    //ITEM_STABLE_MULCH,
    //ITEM_GOOEY_MULCH,
    //ITEM_RICH_MULCH,
    //ITEM_SURPRISE_MULCH,
    //ITEM_BOOST_MULCH,
    //ITEM_AMAZE_MULCH,
    // Apricorns
    //ITEM_RED_APRICORN,
    //ITEM_BLUE_APRICORN,
    //ITEM_YELLOW_APRICORN,
    //ITEM_GREEN_APRICORN,
    //ITEM_PINK_APRICORN,
    //ITEM_WHITE_APRICORN,
    //ITEM_BLACK_APRICORN,
    //ITEM_WISHING_PIECE,
    //ITEM_GALARICA_TWIG,
    //ITEM_ARMORITE_ORE,
    //ITEM_DYNITE_ORE,
    // Mail
    // ITEM_ORANGE_MAIL,
    // ITEM_HARBOR_MAIL,
    // ITEM_GLITTER_MAIL,
    // ITEM_MECH_MAIL,
    // ITEM_WOOD_MAIL,
    // ITEM_WAVE_MAIL,
    // ITEM_BEAD_MAIL,
    // ITEM_SHADOW_MAIL,
    // ITEM_TROPIC_MAIL,
    // ITEM_DREAM_MAIL,
    // ITEM_FAB_MAIL,
    // ITEM_RETRO_MAIL,
    // Evolution Items
    ITEM_FIRE_STONE,
    ITEM_WATER_STONE,
    ITEM_THUNDER_STONE,
    ITEM_LEAF_STONE,
    ITEM_ICE_STONE,
    ITEM_SUN_STONE,
    ITEM_MOON_STONE,
    #ifdef POKEMON_EXPANSION
    ITEM_SHINY_STONE,
    ITEM_DUSK_STONE,
    ITEM_DAWN_STONE,
    ITEM_SWEET_APPLE,
    ITEM_TART_APPLE,
    ITEM_CRACKED_POT,
    ITEM_CHIPPED_POT,
    ITEM_GALARICA_CUFF,
    ITEM_GALARICA_WREATH,
    #endif
    ITEM_DRAGON_SCALE,
    ITEM_UPGRADE,
    #ifdef POKEMON_EXPANSION
    ITEM_PROTECTOR,
    ITEM_ELECTIRIZER,
    ITEM_MAGMARIZER,
    ITEM_DUBIOUS_DISC,
    ITEM_REAPER_CLOTH,
    ITEM_PRISM_SCALE,
    ITEM_WHIPPED_DREAM,
    ITEM_SACHET,
    ITEM_OVAL_STONE,
    ITEM_STRAWBERRY_SWEET,
    ITEM_LOVE_SWEET,
    ITEM_BERRY_SWEET,
    ITEM_CLOVER_SWEET,
    ITEM_FLOWER_SWEET,
    ITEM_STAR_SWEET,
    ITEM_RIBBON_SWEET,
    #endif
    ITEM_EVERSTONE,
    // Nectars
    #ifdef POKEMON_EXPANSION
    ITEM_RED_NECTAR,
    ITEM_YELLOW_NECTAR,
    ITEM_PINK_NECTAR,
    ITEM_PURPLE_NECTAR,
    // Plates
    ITEM_FLAME_PLATE,
    ITEM_SPLASH_PLATE,
    ITEM_ZAP_PLATE,
    ITEM_MEADOW_PLATE,
    ITEM_ICICLE_PLATE,
    ITEM_FIST_PLATE,
    ITEM_TOXIC_PLATE,
    ITEM_EARTH_PLATE,
    ITEM_SKY_PLATE,
    ITEM_MIND_PLATE,
    ITEM_INSECT_PLATE,
    ITEM_STONE_PLATE,
    ITEM_SPOOKY_PLATE,
    ITEM_DRACO_PLATE,
    ITEM_DREAD_PLATE,
    ITEM_IRON_PLATE,
    ITEM_PIXIE_PLATE,
    // Drives
    ITEM_DOUSE_DRIVE,
    ITEM_SHOCK_DRIVE,
    ITEM_BURN_DRIVE,
    ITEM_CHILL_DRIVE,
    // Memories
    ITEM_FIRE_MEMORY,
    ITEM_WATER_MEMORY,
    ITEM_ELECTRIC_MEMORY,
    ITEM_GRASS_MEMORY,
    ITEM_ICE_MEMORY,
    ITEM_FIGHTING_MEMORY,
    ITEM_POISON_MEMORY,
    ITEM_GROUND_MEMORY,
    ITEM_FLYING_MEMORY,
    ITEM_PSYCHIC_MEMORY,
    ITEM_BUG_MEMORY,
    ITEM_ROCK_MEMORY,
    ITEM_GHOST_MEMORY,
    ITEM_DRAGON_MEMORY,
    ITEM_DARK_MEMORY,
    ITEM_STEEL_MEMORY,
    ITEM_FAIRY_MEMORY,
    ITEM_RUSTED_SWORD,
    ITEM_RUSTED_SHIELD,
    #endif
    // Colored Orbs
    ITEM_RED_ORB,
    ITEM_BLUE_ORB,
    #if defined(BATTLE_ENGINE) && defined(POKEMON_EXPANSION)
    // Mega Stones
    ITEM_VENUSAURITE,
    ITEM_CHARIZARDITE_X,
    ITEM_CHARIZARDITE_Y,
    ITEM_BLASTOISINITE,
    ITEM_BEEDRILLITE,
    ITEM_PIDGEOTITE,
    ITEM_ALAKAZITE,
    ITEM_SLOWBRONITE,
    ITEM_GENGARITE,
    ITEM_KANGASKHANITE,
    ITEM_PINSIRITE,
    ITEM_GYARADOSITE,
    ITEM_AERODACTYLITE,
    ITEM_MEWTWONITE_X,
    ITEM_MEWTWONITE_Y,
    ITEM_AMPHAROSITE,
    ITEM_STEELIXITE,
    ITEM_SCIZORITE,
    ITEM_HERACRONITE,
    ITEM_HOUNDOOMINITE,
    ITEM_TYRANITARITE,
    ITEM_SCEPTILITE,
    ITEM_BLAZIKENITE,
    ITEM_SWAMPERTITE,
    ITEM_GARDEVOIRITE,
    ITEM_SABLENITE,
    ITEM_MAWILITE,
    ITEM_AGGRONITE,
    ITEM_MEDICHAMITE,
    ITEM_MANECTITE,
    ITEM_SHARPEDONITE,
    ITEM_CAMERUPTITE,
    ITEM_ALTARIANITE,
    ITEM_BANETTITE,
    ITEM_ABSOLITE,
    ITEM_GLALITITE,
    ITEM_SALAMENCITE,
    ITEM_METAGROSSITE,
    ITEM_LATIASITE,
    ITEM_LATIOSITE,
    ITEM_LOPUNNITE,
    ITEM_GARCHOMPITE,
    ITEM_LUCARIONITE,
    ITEM_ABOMASITE,
    ITEM_GALLADITE,
    ITEM_AUDINITE,
    ITEM_DIANCITE,
    #endif
    // Gems
    ITEM_NORMAL_GEM,
    ITEM_FIRE_GEM,
    ITEM_WATER_GEM,
    ITEM_ELECTRIC_GEM,
    ITEM_GRASS_GEM,
    ITEM_ICE_GEM,
    ITEM_FIGHTING_GEM,
    ITEM_POISON_GEM,
    ITEM_GROUND_GEM,
    ITEM_FLYING_GEM,
    ITEM_PSYCHIC_GEM,
    ITEM_BUG_GEM,
    ITEM_ROCK_GEM,
    ITEM_GHOST_GEM,
    ITEM_DRAGON_GEM,
    ITEM_DARK_GEM,
    ITEM_STEEL_GEM,
    ITEM_FAIRY_GEM,
    #ifdef BATTLE_ENGINE
    // Z-Crystals
    //ITEM_NORMALIUM_Z,
    //ITEM_FIRIUM_Z,
    //ITEM_WATERIUM_Z,
    //ITEM_ELECTRIUM_Z,
    //ITEM_GRASSIUM_Z,
    //ITEM_ICIUM_Z,
    //ITEM_FIGHTINIUM_Z,
    //ITEM_POISONIUM_Z,
    //ITEM_GROUNDIUM_Z,
    //ITEM_FLYINIUM_Z,
    //ITEM_PSYCHIUM_Z,
    //ITEM_BUGINIUM_Z,
    //ITEM_ROCKIUM_Z,
    //ITEM_GHOSTIUM_Z,
    //ITEM_DRAGONIUM_Z,
    //ITEM_DARKINIUM_Z,
    //ITEM_STEELIUM_Z,
    //ITEM_FAIRIUM_Z,
    //ITEM_PIKANIUM_Z,
    //ITEM_EEVIUM_Z,
    //ITEM_SNORLIUM_Z,
    //ITEM_MEWNIUM_Z,
    //ITEM_DECIDIUM_Z,
    //ITEM_INCINIUM_Z,
    //ITEM_PRIMARIUM_Z,
    //ITEM_LYCANIUM_Z,
    //ITEM_MIMIKIUM_Z,
    //ITEM_KOMMONIUM_Z,
    //ITEM_TAPUNIUM_Z,
    //ITEM_SOLGANIUM_Z,
    //ITEM_LUNALIUM_Z,
    //ITEM_MARSHADIUM_Z,
    //ITEM_ALORAICHIUM_Z,
    //ITEM_PIKASHUNIUM_Z,
    //ITEM_ULTRANECROZIUM_Z,
    #endif
    // Species-specific Held Items
    ITEM_LIGHT_BALL,
    ITEM_LEEK,
    ITEM_THICK_CLUB,
    ITEM_LUCKY_PUNCH,
    ITEM_METAL_POWDER,
    ITEM_QUICK_POWDER,
    ITEM_DEEP_SEA_SCALE,
    ITEM_DEEP_SEA_TOOTH,
    ITEM_SOUL_DEW,
    #if defined(BATTLE_ENGINE) && defined(POKEMON_EXPANSION)
    ITEM_ADAMANT_ORB,
    ITEM_LUSTROUS_ORB,
    ITEM_GRISEOUS_ORB,
    #endif
    // Incenses
    ITEM_SEA_INCENSE,
    ITEM_LAX_INCENSE,
    #ifdef POKEMON_EXPANSION
    ITEM_ODD_INCENSE,
    ITEM_ROCK_INCENSE,
    ITEM_FULL_INCENSE,
    ITEM_WAVE_INCENSE,
    ITEM_ROSE_INCENSE,
    ITEM_LUCK_INCENSE,
    ITEM_PURE_INCENSE,
    #endif
    // Contest Scarves
    // ITEM_RED_SCARF,
    // ITEM_BLUE_SCARF,
    // ITEM_PINK_SCARF,
    // ITEM_GREEN_SCARF,
    // ITEM_YELLOW_SCARF,
    // EV Gain Modifiers
    ITEM_MACHO_BRACE,
    ITEM_POWER_WEIGHT,
    ITEM_POWER_BRACER,
    ITEM_POWER_BELT,
    ITEM_POWER_LENS,
    ITEM_POWER_BAND,
    ITEM_POWER_ANKLET,
    // Type-boosting Held Items
    ITEM_SILK_SCARF,
    ITEM_CHARCOAL,
    ITEM_MYSTIC_WATER,
    ITEM_MAGNET,
    ITEM_MIRACLE_SEED,
    ITEM_NEVER_MELT_ICE,
    ITEM_BLACK_BELT,
    ITEM_POISON_BARB,
    ITEM_SOFT_SAND,
    ITEM_SHARP_BEAK,
    ITEM_TWISTED_SPOON,
    ITEM_SILVER_POWDER,
    ITEM_HARD_STONE,
    ITEM_SPELL_TAG,
    ITEM_DRAGON_FANG,
    ITEM_BLACK_GLASSES,
    ITEM_METAL_COAT,
    #ifdef BATTLE_ENGINE
    // Choice Items
    ITEM_CHOICE_BAND,
    ITEM_CHOICE_SPECS,
    ITEM_CHOICE_SCARF,
    // Status Orbs
    ITEM_FLAME_ORB,
    ITEM_TOXIC_ORB,
    // Weather Rocks
    ITEM_DAMP_ROCK,
    ITEM_HEAT_ROCK,
    ITEM_SMOOTH_ROCK,
    ITEM_ICY_ROCK,
    // Terrain Seeds
    ITEM_ELECTRIC_SEED,
    ITEM_PSYCHIC_SEED,
    ITEM_MISTY_SEED,
    ITEM_GRASSY_SEED,
    // Type-activated Stat Modifiers
    ITEM_ABSORB_BULB,
    ITEM_CELL_BATTERY,
    ITEM_LUMINOUS_MOSS,
    ITEM_SNOWBALL,
    #endif
    // Misc. Held Items
    ITEM_BRIGHT_POWDER,
    ITEM_WHITE_HERB,
    ITEM_EXP_SHARE,
    ITEM_QUICK_CLAW,
    ITEM_SOOTHE_BELL,
    ITEM_MENTAL_HERB,
    ITEM_KINGS_ROCK,
    ITEM_AMULET_COIN,
    ITEM_CLEANSE_TAG,
    ITEM_SMOKE_BALL,
    ITEM_FOCUS_BAND,
    ITEM_LUCKY_EGG,
    ITEM_SCOPE_LENS,
    ITEM_LEFTOVERS,
    ITEM_SHELL_BELL,
    #ifdef BATTLE_ENGINE
    ITEM_WIDE_LENS,
    ITEM_MUSCLE_BAND,
    ITEM_WISE_GLASSES,
    ITEM_EXPERT_BELT,
    ITEM_LIGHT_CLAY,
    ITEM_LIFE_ORB,
    ITEM_POWER_HERB,
    ITEM_FOCUS_SASH,
    ITEM_ZOOM_LENS,
    ITEM_METRONOME,
    ITEM_IRON_BALL,
    ITEM_LAGGING_TAIL,
    ITEM_DESTINY_KNOT,
    ITEM_BLACK_SLUDGE,
    ITEM_GRIP_CLAW,
    ITEM_STICKY_BARB,
    ITEM_SHED_SHELL,
    ITEM_BIG_ROOT,
    ITEM_RAZOR_CLAW,
    ITEM_RAZOR_FANG,
    ITEM_EVIOLITE,
    ITEM_FLOAT_STONE,
    ITEM_ROCKY_HELMET,
    ITEM_AIR_BALLOON,
    ITEM_RED_CARD,
    ITEM_RING_TARGET,
    ITEM_BINDING_BAND,
    ITEM_EJECT_BUTTON,
    ITEM_WEAKNESS_POLICY,
    ITEM_ASSAULT_VEST,
    ITEM_SAFETY_GOGGLES,
    ITEM_ADRENALINE_ORB,
    ITEM_TERRAIN_EXTENDER,
    ITEM_PROTECTIVE_PADS,
    ITEM_THROAT_SPRAY,
    ITEM_EJECT_PACK,
    ITEM_HEAVY_DUTY_BOOTS,
    ITEM_BLUNDER_POLICY,
    ITEM_ROOM_SERVICE,
    ITEM_UTILITY_UMBRELLA,
    #endif
    // Berries
    ITEM_CHERI_BERRY,
    ITEM_CHESTO_BERRY,
    ITEM_PECHA_BERRY,
    ITEM_RAWST_BERRY,
    ITEM_ASPEAR_BERRY,
    ITEM_LEPPA_BERRY,
    ITEM_ORAN_BERRY,
    ITEM_PERSIM_BERRY,
    ITEM_LUM_BERRY,
    ITEM_SITRUS_BERRY,
    ITEM_FIGY_BERRY,
    ITEM_WIKI_BERRY,
    ITEM_MAGO_BERRY,
    ITEM_AGUAV_BERRY,
    ITEM_IAPAPA_BERRY,
    // ITEM_RAZZ_BERRY,
    // ITEM_BLUK_BERRY,
    // ITEM_NANAB_BERRY,
    // ITEM_WEPEAR_BERRY,
    // ITEM_PINAP_BERRY,
    ITEM_POMEG_BERRY,
    ITEM_KELPSY_BERRY,
    ITEM_QUALOT_BERRY,
    ITEM_HONDEW_BERRY,
    ITEM_GREPA_BERRY,
    ITEM_TAMATO_BERRY,
    // ITEM_CORNN_BERRY,
    // ITEM_MAGOST_BERRY,
    // ITEM_RABUTA_BERRY,
    // ITEM_NOMEL_BERRY,
    // ITEM_SPELON_BERRY,
    // ITEM_PAMTRE_BERRY,
    // ITEM_WATMEL_BERRY,
    // ITEM_DURIN_BERRY,
    // ITEM_BELUE_BERRY,
    ITEM_CHILAN_BERRY,
    ITEM_OCCA_BERRY,
    ITEM_PASSHO_BERRY,
    ITEM_WACAN_BERRY,
    ITEM_RINDO_BERRY,
    ITEM_YACHE_BERRY,
    ITEM_CHOPLE_BERRY,
    ITEM_KEBIA_BERRY,
    ITEM_SHUCA_BERRY,
    ITEM_COBA_BERRY,
    ITEM_PAYAPA_BERRY,
    ITEM_TANGA_BERRY,
    ITEM_CHARTI_BERRY,
    ITEM_KASIB_BERRY,
    ITEM_HABAN_BERRY,
    ITEM_COLBUR_BERRY,
    ITEM_BABIRI_BERRY,
    ITEM_ROSELI_BERRY,
    ITEM_LIECHI_BERRY,
    ITEM_GANLON_BERRY,
    ITEM_SALAC_BERRY,
    ITEM_PETAYA_BERRY,
    ITEM_APICOT_BERRY,
    ITEM_LANSAT_BERRY,
    ITEM_STARF_BERRY,
    ITEM_ENIGMA_BERRY,
    ITEM_MICLE_BERRY,
    ITEM_CUSTAP_BERRY,
    ITEM_JABOCA_BERRY,
    ITEM_ROWAP_BERRY,
    ITEM_KEE_BERRY,
    ITEM_MARANGA_BERRY,
#endif
};

u16 RandomItemId(u16 itemId)
{
    u8 mapId = GetCurrentRegionMapSectionId();
    if (gItemsInfo[itemId].pocket == POCKET_TM_HM)
    {
        if (itemId != ITEM_HM01
            && itemId != ITEM_HM02
            && itemId != ITEM_HM03
            && itemId != ITEM_HM04
            && itemId != ITEM_HM05
            && itemId != ITEM_HM06
            && itemId != ITEM_HM07
            && itemId != ITEM_HM08)
        {
            u8 i;
            itemId = ITEM_TM01 + RandomSeededModulo(itemId, 50);
            for (i = 0; i < 255; i++)
            {
                if (!CheckBagHasItem(itemId, 1))
                    break;
                itemId = ITEM_TM01 + RandomSeededModulo(itemId, 50);
            }
        }
    }
    else if (gItemsInfo[itemId].pocket != POCKET_KEY_ITEMS)
        itemId = sRandomValidItems[RandomSeededModulo(itemId + mapId, RANDOM_ITEM_COUNT)];

    return itemId;
}

u16 RandomItem(void)
{
    u16 itemId = RandomItemId(gSpecialVar_0x8000);

    gSpecialVar_0x8000 = itemId;
    return itemId;
}

u16 RandomItemHidden(void) //same as normal hidden item, but differen special var
{
    u16 itemId = RandomItemId(gSpecialVar_0x8005);

    gSpecialVar_0x8005 = itemId;
    return itemId;
}

u32 GetItemFlingPower(u32 itemId)
{
    return gItemsInfo[SanitizeItemId(itemId)].flingPower;
}


u32 GetItemStatus1Mask(u16 itemId)
{
    const u8 *effect = GetItemEffect(itemId);
    switch (effect[3])
    {
        case ITEM3_PARALYSIS:
            return STATUS1_PARALYSIS;
        case ITEM3_FREEZE:
            return STATUS1_FREEZE | STATUS1_FROSTBITE;
        case ITEM3_BURN:
            return STATUS1_BURN;
        case ITEM3_POISON:
            return STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER;
        case ITEM3_SLEEP:
            return STATUS1_SLEEP;
        case ITEM3_STATUS_ALL:
            return STATUS1_ANY | STATUS1_TOXIC_COUNTER;
    }
    return 0;
}

bool32 ItemHasVolatileFlag(u16 itemId, enum Volatile _volatile)
{
    const u8 *effect = GetItemEffect(itemId);
    switch (_volatile)
    {
    case VOLATILE_CONFUSION:
        return (effect[3] & ITEM3_STATUS_ALL) || (effect[3] & ITEM3_CONFUSION);
    case VOLATILE_INFATUATION:
        return (effect[3] & ITEM3_STATUS_ALL) || (effect[0] & ITEM0_INFATUATION);
    default:
        return FALSE;
    }
}

u32 GetItemSellPrice(u32 itemId)
{
    return GetItemPrice(itemId) / ITEM_SELL_FACTOR;
}

bool32 IsHoldEffectChoice(enum HoldEffect holdEffect)
{
    return holdEffect == HOLD_EFFECT_CHOICE_BAND
        || holdEffect == HOLD_EFFECT_CHOICE_SCARF
        || holdEffect == HOLD_EFFECT_CHOICE_SPECS;
}
