#include "global.h"
#include "craft.h"
#include "bg.h"
#include "battle_bg.h"
#include "battle_main.h"
#include "event_data.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "graphics.h"
#include "item.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "constants/field_specials.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "event_object_movement.h"
#include "event_object_lock.h"

#include "script_menu.h"
#include "constants/script_menu.h"
//#include "data/script_menu.h"

//extern const struct MultichoiceListStruct gMultichoiceLists[];

enum {
    DEBUG_MENU_ITEM_CANCEL,
};

static const u8 gDebugText_Cancel[] = _("Cancel");

static const struct ListMenuItem sDebugMenuItems[] = {
    [DEBUG_MENU_ITEM_CANCEL] = {gDebugText_Cancel, DEBUG_MENU_ITEM_CANCEL}
};

static void (*const sDebugMenuActions[])(u8) = {
    [DEBUG_MENU_ITEM_CANCEL] = DebugAction_Cancel
};

static const struct WindowTemplate sDebugMenuWindowTemplate = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_MAIN_MENU_WIDTH,
    .height = 2 * DEBUG_MAIN_MENU_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct ListMenuTemplate sDebugMenuListTemplate = {
    .items = sDebugMenuItems,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .totalItems = ARRAY_COUNT(sDebugMenuItems),
    .maxShowed = DEBUG_MAIN_MENU_HEIGHT,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = 1,
    .cursorKind = 0
};

void Debug_ShowMainMenu(void) {
    struct ListMenuTemplate menuTemplate;
    u8 windowId;
    u8 menuTaskId;
    u8 inputTaskId;

    // create window
    windowId = AddWindow(&sDebugMenuWindowTemplate);
    LoadMessageBoxAndBorderGfx();
    DrawStdWindowFrame(windowId, FALSE);

    // create list menu
    menuTemplate = sDebugMenuListTemplate;
    menuTemplate.windowId = windowId;
    menuTaskId = ListMenuInit(&menuTemplate, 0, 0);

    // draw everything
    CopyWindowToVram(windowId, 3);

    // create input handler task
    inputTaskId = CreateTask(DebugTask_HandleMainMenuInput, 3);
    gTasks[inputTaskId].data[0] = menuTaskId;
    gTasks[inputTaskId].data[1] = windowId;
}

static void Debug_DestroyMainMenu(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].data[0], NULL, NULL);
    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static void DebugTask_HandleMainMenuInput(u8 taskId)
{
    void (*func)(u8);
    u32 input = ListMenu_ProcessInput(gTasks[taskId].data[0]);

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        if ((func = sDebugMenuActions[input]) != NULL)
            func(taskId);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyMainMenu(taskId);
    }
}

static void DebugAction_Cancel(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);
}










static const u8 sText_SlotTest[] = _("Option {STR_VAR_1}!");

enum CraftWindows {
    WINDOW_CRAFT_MSG,
    WINDOW_CRAFT_TABLE,
    WINDOW_CRAFT_INFO,
    WINDOW_CRAFT_YESNO,
    WINDOW_CRAFT_READYUP,
    WINDOW_CRAFT_SOUSCHEFS,
};

#define WINDOW_TILE 313
static const struct WindowTemplate sDummyWindowTemplates[] = {DUMMY_WIN_TEMPLATE};

static const struct WindowTemplate sCraftWindowTemplates[] = {
/*    [C_WIN_ITEM_NAME_1] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 55,
        .width = 8,
        .height = 2,
        .paletteNum = 5,
        .baseBlock = 0x0300,
    },
    [C_WIN_ITEM_NAME_2] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 55,
        .width = 8,
        .height = 2,
        .paletteNum = 5,
        .baseBlock = 0x0310,
    },
    [C_WIN_ITEM_NAME_3] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 57,
        .width = 8,
        .height = 2,
        .paletteNum = 5,
        .baseBlock = 0x0320,
    },
    [C_WIN_ITEM_NAME_4] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 57,
        .width = 8,
        .height = 2,
        .paletteNum = 5,
        .baseBlock = 0x0330,
    },
*/
    [WINDOW_CRAFT_MSG] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1
    },    
    [WINDOW_CRAFT_TABLE] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 18,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1 + 27*4,
    },
    [WINDOW_CRAFT_INFO] = {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 55,
        .width = 8,
        .height = 4,
        .paletteNum = 5,
        .baseBlock = 1 + 18*4 + 27*4,
    },
    [WINDOW_CRAFT_YESNO] = {
        .bg = 0,
        .tilemapLeft = 26,
        .tilemapTop = 9,
        .width = 3,
        .height = 4,
        .paletteNum = 5,
        .baseBlock = 1 + 18*4 + 8*4 + 27*4,
    },
    DUMMY_WIN_TEMPLATE
};

//May just need loadbattlemenuwindowgfx, or maybe not at all (loadmessageboxandbordergfx instead)
/*void LoadCraftTextbox(void) //from src/battle_bg.c LoadBattleTextboxAndBackground
{
    LZDecompressVram(gBattleTextboxTiles, (void *)(BG_CHAR_ADDR(0)));
    CopyToBgTilemapBuffer(0, gBattleTextboxTilemap, 0, 0);
    CopyBgTilemapBufferToVram(0);
    LoadCompressedPalette(gBattleTextboxPalette, BG_PLTT_ID(0), 2 * PLTT_SIZE_4BPP);
    LoadBattleMenuWindowGfx();
    //DrawMainBattleBackground();
}
*/

//Useful stuff
/*
battle_controller_player.c
static void MoveSelectionDisplayMoveNames(void) - after selecting Fight, add names
static void HandleInputChooseMove(void)
static void HandleMoveSwitching(void) - after choosing to start switching moves
static void PlayerHandleYesNoBox(void)
SaveCallback - hopping back and forth between menus


width = w=18, left = 2, top=55
cook = w=8, left=21, top=55
height = 4

*/

static const u16 Craft_ItemList[4];

static const u8 Craft_GetItemName(u16 itemId){
    if(itemId == ITEM_NONE || itemId > ITEM_OLD_SEA_MAP){
        StringCopy(gStringVar2, gText_Dash);
    }
    else{
        StringExpandPlaceholders(gStringVar2,sText_SlotTest);
        //StringCopy(gStringVar1, ItemId_GetName(itemId));
    }
}

//Have to use gStringVars??
static const struct MenuAction MultichoiceList_CraftEmpty[] = {
    {gStringVar4},
    {gText_Dash},
    {gText_Dash},
    {gText_Dash},
};
struct CountCraftItemStruct {
    const struct MenuAction *list;
    u8 count;
};
static const struct CountCraftItemStruct sCountCraftItemConvert[] = {
    [0] = MULTICHOICE(MultichoiceList_CraftEmpty),
};

//#define tLeft           data[0]
//#define tTop            data[1]
//#define tRight          data[2]
//#define tBottom         data[3]
//#define tIgnoreBPress   data[4]
//#define tDoWrap         data[5]
//#define tWindowId       data[6]
//#define tMultichoiceId  data[7]

void CraftMenu_Init(void){
    
    //FreezeObjectEvents();
    //PlayerFreeze();
    //StopPlayerAvatar();
    
    //NOTE: freeze if used by Mess Kit item in the overworld, otherwise all cratt situations are controlled and
    //       miss accidental event interaction



    LockPlayerFieldControls();

    //InitWindows(sDummyWindowTemplates);
    /*{
        int i, baseBlock;
        struct WindowTemplate template;
        baseBlock = WINDOW_TILE;
        for (i = 0; i < ARRAY_COUNT(sCraftWindowTemplates); ++i)
        {
            template = sCraftWindowTemplates[i];
            template.baseBlock = baseBlock;
            //AddWindow(&template);
            baseBlock += template.width * template.height;
        }
    }*/



    if (Craft_ShowMainMenu() == TRUE){
        ScriptContext_Stop();
        //return TRUE;
    }
    //else {
    //    return FALSE;
    //}
}

/* //taken from txregistereditemsmenu
static u32 Craft_InitWindows(void)
{
    u32 *windowId = &(gTxRegItemsMenu->windowIds[0]);
    if (*windowId == WINDOW_NONE)
    {
        *windowId = AddWindow(&TxRegItemsMenu_WindowTemplates[0]);
        DrawStdFrameWithCustomTileAndPalette(*windowId, FALSE, 0x214, 0xE);
        ScheduleBgCopyTilemapToVram(0);
    }
    return *windowId;
}
*/



//modified from src/script_menu.c ScriptMenu_MultichoiceGrid
bool32 Craft_ShowMainMenu(void){
    
    struct WindowTemplate CraftTableWindow = sCraftWindowTemplates[WINDOW_CRAFT_TABLE];
    //u8 left, top, width, columnCount, taskId, rowCount;
    u8 multichoiceId, windowId, inputTaskId;

    //Option List Id
    multichoiceId = 0;

    if (FuncIsActiveTask(Task_HandleCraftMenuInput) == TRUE){
        return FALSE;
    }
    else{
        
        if(gSpecialVar_Result < 1 || gSpecialVar_Result > 4){
            gSpecialVar_Result = 1;
        }

        StringCopy(gStringVar4, gText_Dash);
        ConvertIntToDecimalStringN(gStringVar1, gSpecialVar_Result,STR_CONV_MODE_LEFT_ALIGN,1);

        InitWindows(sCraftWindowTemplates); //FreeAllWindowBuffers messes up other windows after?

        windowId = AddWindow(&CraftTableWindow);
        LoadMessageBoxAndBorderGfx(); //gets red border if not & cuts off regardless    
        DrawStdWindowFrame(windowId, FALSE);

        PrintMenuGridTable(windowId, 9 * 8, 2, 2, sCountCraftItemConvert[multichoiceId].list);
        InitMenuActionGrid(windowId, 9 * 8, 2, 2, gSpecialVar_Result - 1);
        
        CopyWindowToVram(windowId, COPYWIN_FULL);

        inputTaskId = CreateTask(Task_HandleCraftMenuInput, 80);
        gTasks[inputTaskId].data[1] = windowId;

        return TRUE;
    }
}

static void RefreshCraftTable(void){
    
    FillWindowPixelBuffer(WINDOW_CRAFT_TABLE, PIXEL_FILL(TEXT_COLOR_WHITE));
    //FillWindowPixelRect(WINDOW_CRAFT_TABLE,PIXEL_FILL(TEXT_COLOR_WHITE), 9*8 + 8, 16 + 1, 8, 4);
}

static const u8 sWindowColors[3] = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

static void UpdateCraftSlot(void){
    /*If item = ITEM_NONE then display craft instructions "A to Add Item, Start to Craft"
     *Go to Bag, choose item for gSpecialVar_0x8000-3, update with name (refresh)
     *If item = anything else, ask SWAP/BAG/READY/CANCEL*/
        u32 i, j;


    RefreshCraftTable();
    ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_NONE);
    StringExpandPlaceholders(gStringVar4, sText_SlotTest);

    //PrintMenuGridTable(WINDOW_CRAFT_TABLE, 9 * 8, 2, 2, sCountCraftItemConvert[0].list);    

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
            AddTextPrinterParameterized(WINDOW_CRAFT_TABLE, 1, MultichoiceList_CraftEmpty[(i * 2) + j].text,
                (9*8 * j) + 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
            //AddTextPrinterParameterized(WINDOW_CRAFT_TABLE, 1, gText_SelectorArrow3,
            //     (9*8 * j), (i * 16) + 1, TEXT_SKIP_DRAW, NULL);

    }


    PutWindowTilemap(WINDOW_CRAFT_TABLE);
    CopyWindowToVram(WINDOW_CRAFT_TABLE, COPYWIN_GFX);

    ScheduleBgCopyTilemapToVram(0);
}

//Detect cursor pos change for refresh info window, maybe delete
/*static EWRAM_DATA struct Menu sMenu = {0};
static s8 Craft_ProcessGridChange(void){

    u8 oldPos = sMenu.cursorPos;

    
    if (JOY_NEW(DPAD_UP))
    {
        if (oldPos != ChangeGridMenuCursorPosition(0, -1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (oldPos != ChangeGridMenuCursorPosition(0, 1))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
    {
        if (oldPos != ChangeGridMenuCursorPosition(-1, 0))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }
    else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
    {
        if (oldPos != ChangeGridMenuCursorPosition(1, 0))
            PlaySE(SE_SELECT);
        return MENU_NOTHING_CHOSEN;
    }

    return MENU_NOTHING_CHOSEN;
}
*/

static void Task_HandleCraftMenuInput(u8 taskId){
    s16 *data = gTasks[taskId].data;
    s8 selection;

    selection = Menu_ProcessGridInput();


    //if they pressed anything at all ("newkeys = true" or something [something = if(JOY_NEW(DPAD_ANY))] ) refresh little info window
    if(JOY_NEW(DPAD_ANY)){
        gSpecialVar_Result += 1;
        if (gSpecialVar_Result > 4){
            gSpecialVar_Result = 1;
        }
        ConvertIntToDecimalStringN(gStringVar1, gSpecialVar_Result,STR_CONV_MODE_LEFT_ALIGN,1);


        UpdateCraftSlot();
        //ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_NONE);
    }


    switch (selection){
    case MENU_NOTHING_CHOSEN:
        return;
    case 0:
    case 1:
    case 2:
    case 3:
        
        //gSpecialVar_Result = selection;
    case MENU_B_PRESSED:
    default:
        gSpecialVar_Result = selection;
        Craft_DestroyMainMenu(taskId);
    //    gSpecialVar_Result = MULTI_B_PRESSED;
        break;
    }

}

static void Craft_DestroyMainMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 windowId = gTasks[taskId].data[1];
    
    ClearStdWindowAndFrame(windowId, TRUE);
    
    if (windowId != WINDOW_NONE){
        RemoveWindow(windowId);
        windowId = WINDOW_NONE;
    }

    //ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    
    //FreeAllWindowBuffers();    
    DestroyTask(taskId);
    ScriptContext_Enable();

    ClearScheduledBgCopiesToVram();
    ResetTasks();
}

