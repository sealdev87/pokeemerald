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
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "strings.h"
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
    [C_WIN_CRAFT_TABLE] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 18,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 313,
    },
    [C_WIN_DUMMY] = {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 57,
        .width = 0,
        .height = 0,
        .paletteNum = 5,
        .baseBlock = 0x0298,
    },
    [C_WIN_CRAFT_PROMPT] = {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 55,
        .width = 8,
        .height = 4,
        .paletteNum = 5,
        .baseBlock = 0x02b0,
    },
    [C_WIN_YESNO] = {
        .bg = 0,
        .tilemapLeft = 26,
        .tilemapTop = 9,
        .width = 3,
        .height = 4,
        .paletteNum = 5,
        .baseBlock = 0x0100,
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

static const struct MenuAction MultichoiceList_CraftEmpty[] = {
    {gText_Dash},
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
    LockPlayerFieldControls();


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
    
    struct WindowTemplate CraftTableWindow = sCraftWindowTemplates[C_WIN_CRAFT_TABLE];
    u8 left, top, width, columnCount, taskId, rowCount;
    u8 multichoiceId, windowId, inputTaskId;

    //Grid parameters
    width = 9;
    columnCount = 2;
    rowCount = 2; 

    //Option List Id
    multichoiceId = 0;

    if (FuncIsActiveTask(Task_HandleCraftMenuInput) == TRUE){
        return FALSE;
    }
    else{
        
        gSpecialVar_Result = 0xFF;

        //InitWindows(sCraftWindowTemplates);

        windowId = AddWindow(&CraftTableWindow);
        LoadMessageBoxAndBorderGfx(); //gets red border if not & cuts off regardless    
        DrawStdWindowFrame(windowId, FALSE);

        PrintMenuGridTable(windowId, width * 8, columnCount, rowCount, sCountCraftItemConvert[multichoiceId].list);
        InitMenuActionGrid(windowId, width * 8, columnCount, rowCount, 0);
        
        CopyWindowToVram(windowId, 3);

        inputTaskId = CreateTask(Task_HandleCraftMenuInput, 80);
        gTasks[inputTaskId].data[1] = windowId;

        //return TRUE;
    }
}

static void Task_HandleCraftMenuInput(u8 taskId){
    s16 *data = gTasks[taskId].data;
    s8 cursorPos = Menu_GetCursorPos();
    s8 selection = Menu_ProcessGridInput();

    switch (selection){
    case MENU_NOTHING_CHOSEN:
        return;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        Craft_DestroyMainMenu(taskId);
        gSpecialVar_Result = MULTI_B_PRESSED;
        break;
    default:
        PlaySE(SE_LEDGE); 
        gSpecialVar_Result = selection;
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

    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    
    DestroyTask(taskId);
    ScriptContext_Enable();

    ClearScheduledBgCopiesToVram();
    ResetTasks();
}

