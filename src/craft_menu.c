#include "global.h"
#include "bg.h"
#include "craft_menu.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_object_lock.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "field_weather.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item_menu.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "start_menu.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Menu actions
enum CraftMenuActions {
    MENU_ACTION_SWAP,
    MENU_ACTION_BAG,
    MENU_ACTION_READY,
    MENU_ACTION_CANCEL,
};

// Table actions 
enum TableActions {
    TABLE_ACTION_BLANK,
    TABLE_ACTION_ITEM,
};

// sCurrentCraftTableItems[][] slot names
enum CraftTableSlots {
    CRAFT_TABLE_ITEM,
    CRAFT_TABLE_ACTION,
};

// Input handling
enum CraftState {
    STATE_TABLE_INPUT,
    STATE_OPTIONS_INPUT,
    STATE_CONFIRM_READY,
    STATE_CONFIRM_PACKUP,
};

// IWRAM common
bool8 (*gMenuCallback)(void);

// EWRAM
EWRAM_DATA static u8 sCraftTableWindowId = 0;
EWRAM_DATA static u8 sCraftInfoWindowId = 0;
EWRAM_DATA static u8 sCraftReadyUpWindowId = 0;
EWRAM_DATA static u8 sCraftSousChefsWindowId = 0;

EWRAM_DATA static u8 sCraftMenuCursorPos = 0;
EWRAM_DATA static u8 sInitCraftMenuData[1] = {0};
EWRAM_DATA static u16 sCurrentCraftTableItems[4][2] = {0}; //craft table items, actions
EWRAM_DATA static u8 sCraftState = 0;

// Menu action callbacks - SWAP/BAG/READY/CANCEL
static bool8 CraftMenuItemOptionsCallback(void);
static bool8 CraftMenuPackUpCallback(void);
static bool8 CraftMenuAddSwapCallback(void);
static bool8 CraftMenuRemoveBagCallback(void);
static bool8 CraftMenuReadyCallback(void);
static bool8 CraftMenuCancelCallback(void);
static void HideOptionsMenu(void);

// Menu callbacks
static bool8 HandleCraftMenuInput(void);

// Task callbacks
static void CraftMenuTask(u8 taskId);
static bool8 FieldCB_ReturnToFieldCraftMenu(void);

static const u8 sText_Ready[] = _("READY");

static const struct MenuAction sCraftTableActions[] = {
    [TABLE_ACTION_BLANK]            = {gText_Dash,        {.u8_void = CraftMenuAddSwapCallback}},
    [TABLE_ACTION_ITEM]             = {gText_Dash,        {.u8_void = CraftMenuItemOptionsCallback}} //check out startmenusavecallback
};

static const struct MenuAction sCraftMenuItems[] = { //craft actions, SWAP/READY/BAG/CANCEL
    [MENU_ACTION_SWAP]            = {gText_Swap,        {.u8_void = CraftMenuAddSwapCallback}},
    [MENU_ACTION_BAG]             = {gText_MenuBag,     {.u8_void = CraftMenuRemoveBagCallback}},
    [MENU_ACTION_READY]           = {sText_Ready,       {.u8_void = CraftMenuReadyCallback}},
    [MENU_ACTION_CANCEL]          = {gText_Cancel,      {.u8_void = CraftMenuCancelCallback}}
};

enum CraftWindows {
    WINDOW_CRAFT_MSG,
    WINDOW_CRAFT_TABLE,
    WINDOW_CRAFT_INFO,
    WINDOW_CRAFT_YESNO,
    WINDOW_CRAFT_READYUP,
    WINDOW_CRAFT_SOUSCHEFS,
};

static const struct WindowTemplate sCraftWindowTemplates[] = {
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
        .tilemapTop = 15,
        .width = 8,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1 + 27*4 + 18*4,
    },
    [WINDOW_CRAFT_YESNO] = {
        .bg = 0,
        .tilemapLeft = 26,
        .tilemapTop = 9,
        .width = 3,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1 + 27*4 + 18*4 + 8*4,
    },
    [WINDOW_CRAFT_READYUP] = {
        .bg = 0,
        .tilemapLeft = 15,
        .tilemapTop = 9,
        .width = 14,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1 + 27*4 + 18*4 + 8*4 + 3*4,
    },
    DUMMY_WIN_TEMPLATE
};

//Local functions
static void BuildCraftTableActions(void);
static void ShowCraftTableWindow(void);
static void ShowCraftInfoWindow(void);
static void ShowCraftReadyUpWindow(void);
static void ShowCraftTableAndInfoWindows(void);
static void LoadCraftWindows(void);

static const u8 *GetCraftTableItemName(u16 itemId);
static void RemoveExtraCraftMenuWindows(void);
static void PrintCraftTableItems(void);
static bool32 InitCraftMenuStep(void);
static void CraftMenuTask(u8 taskId);
static void ClearCraftTable(void);
static void Task_CreateCraftMenu(TaskFunc followupFunc);


static void BuildCraftTableActions(void){
    u8 i;

    //Stores action per item. 
    for (i = 0; i < 4; i++){
        if(sCurrentCraftTableItems[i][CRAFT_TABLE_ITEM] == ITEM_NONE ||
            sCurrentCraftTableItems[i][CRAFT_TABLE_ITEM] > ITEM_OLD_SEA_MAP){
            
            sCurrentCraftTableItems[i][CRAFT_TABLE_ACTION] = TABLE_ACTION_BLANK;
        }
        else sCurrentCraftTableItems[i][CRAFT_TABLE_ACTION] = TABLE_ACTION_ITEM;
    } 

}

void ShowReturnToFieldCraftMenu(void){

    HideCraftMenu();
    
}

static void ShowCraftTableWindow(void)
{
    //sCraftTableWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_TABLE]);
    PutWindowTilemap(sCraftTableWindowId);
    DrawStdWindowFrame(sCraftTableWindowId, FALSE);
    CopyWindowToVram(sCraftTableWindowId, COPYWIN_GFX);
}

static void ShowCraftInfoWindow(void)
{
    //sCraftInfoWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
    PutWindowTilemap(sCraftInfoWindowId);
    DrawStdWindowFrame(sCraftInfoWindowId, FALSE);
    //ConvertIntToDecimalStringN(gStringVar1, gNumSafariBalls, STR_CONV_MODE_RIGHT_ALIGN, 2);
    //StringExpandPlaceholders(gStringVar4, gText_SafariBallStock);
    //AddTextPrinterParameterized(sCraftInfoWindowId, FONT_NORMAL, gStringVar4, 0, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sCraftInfoWindowId, COPYWIN_GFX);
}

static void ShowCraftReadyUpWindow(void)
{
    //sCraftReadyUpWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_READYUP]);
    PutWindowTilemap(sCraftReadyUpWindowId);
    DrawStdWindowFrame(sCraftReadyUpWindowId, FALSE);
    //CopyWindowToVram(sCraftReadyUpWindowId, COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(0);
}

static void ShowCraftTableAndInfoWindows(void){
    ShowCraftTableWindow();
    ShowCraftInfoWindow();
}

static void LoadCraftWindows(void){
    //Craft Table
    sCraftTableWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_TABLE]);
    //PutWindowTilemap(sCraftTableWindowId);
    //DrawStdWindowFrame(sCraftTableWindowId, FALSE);

    //Info
    sCraftInfoWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
    //PutWindowTilemap(sCraftInfoWindowId);
    //DrawStdWindowFrame(sCraftInfoWindowId, FALSE);

    //Ready/Options
    sCraftReadyUpWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_READYUP]);
    //PutWindowTilemap(sCraftReadyUpWindowId);
    //DrawStdWindowFrame(sCraftReadyUpWindowId, FALSE);

}

static const u8 *GetCraftTableItemName(u16 itemId){
    //Print filters for item names. GetFacilityCancelString makes the function a pointer so ¯\_(ツ)_/¯

    if(itemId == ITEM_NONE || itemId >= ITEMS_COUNT)
    {
        return gText_Dash;
    }
    else
    {
        return ItemId_GetName(itemId);
    }
}

static void RemoveExtraCraftMenuWindows(void)
{

}

static void PrintCraftTableItems(void){
    //Modified from PrintMenuGridTable
    u32 i, j;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
            AddTextPrinterParameterized(sCraftTableWindowId, 7,
                GetCraftTableItemName(sCurrentCraftTableItems[(i * 2) + j][CRAFT_TABLE_ITEM]),
                (9*8 * j) + 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
    }

}

static bool32 InitCraftMenuStep(void)
{
    //list items, draw window, check for extra windows, print menu actions, place cursor, draw everything
    s8 state = sInitCraftMenuData[0];

    switch (state)
    {
    case 0:
        sInitCraftMenuData[0]++;
        break;
    case 1:
        BuildCraftTableActions(); //update to list blank/item. Old function checks what kind of menu,
                                 //adds items based on flags, then adds each one w/ appendtolist
                                 //uses sCurrentStartMenuActions, sNumStartMenuActions
        sInitCraftMenuData[0]++;
        break;
    case 2:
        LoadMessageBoxAndBorderGfx();
        LoadCraftWindows();
        sInitCraftMenuData[0]++;
        break;
    case 3:
        // if (GetSafariZoneFlag()) //update to sous chefs??
        //     ShowSafariBallsWindow();
        // if (InBattlePyramid())
        //     ShowPyramidFloorWindow();
        ShowCraftTableAndInfoWindows();
        sInitCraftMenuData[0]++;
        break;
    case 4:
        PrintCraftTableItems();
        sInitCraftMenuData[0]++;
        break;
    case 5:
        sCraftMenuCursorPos = InitMenuGrid(sCraftTableWindowId, FONT_NARROW, 0, 1, 9 * 8, 15, 2, //9*8 from ScriptMenu_MultichoiceGrid, 15 from FONT_NARROW
                                                2, 4, sCraftMenuCursorPos);
        CopyWindowToVram(sCraftTableWindowId, COPYWIN_MAP);
        return TRUE;
    }

    return FALSE;
}

static void CraftMenuTask(u8 taskId){
    
    if (InitCraftMenuStep() == TRUE)
        SwitchTaskToFollowupFunc(taskId);
}

static void ClearCraftTable(void){
    //ClearRoamerLocationData wasn't cool with just = {0} for a 2D array so guess we gotta do it here
    u32 i;

    //reset cursor position
    sCraftMenuCursorPos = 0;

    //clear table
    for (i = 0; i < 4; i++){
        sCurrentCraftTableItems[i][CRAFT_TABLE_ITEM] = ITEM_NONE;
        sCurrentCraftTableItems[i][CRAFT_TABLE_ACTION] = TABLE_ACTION_BLANK;
    }
}

static void Task_CreateCraftMenu(TaskFunc followupFunc){ //change to Task_CreateCraftMenu
    
    u8 taskId;

    sInitCraftMenuData[0] = 0;

    //everything else goes straight to Task_ShowStartMenu, so this Create function must be like a startup init    
    ClearCraftTable();

    taskId = CreateTask(CraftMenuTask, 0x50);
    SetTaskFuncWithFollowupFunc(taskId, CraftMenuTask, followupFunc);
}

void Task_ShowCraftMenu(u8 taskId){
              
                //pointer for global task location, acts as state for create switch
    struct Task *task = &gTasks[taskId];

    switch(task->data[0])
    {
    case 0:
        gMenuCallback = HandleCraftMenuInput;
        task->data[0]++;
        break;
    case 1:
        if (gMenuCallback() == TRUE)
            DestroyTask(taskId);
        break;
    }
}

void ShowCraftMenu(void){
    PlayerFreeze();
    StopPlayerAvatar();

    Task_CreateCraftMenu(Task_ShowCraftMenu);
    LockPlayerFieldControls();
}

static bool8 HandleCraftMenuInput(void)
{
    //update info window with every cursor move (item: xQty in Bag & str to craft, blank: A to add item & "items ? str to craft : B to leave")
    //if clicking on something, either add or show options
    //options = SWAP/READY/BAG/CANCEL
    //swap - get different item, ready - check craft recipe, bag - remove item, cancel - close options (update info window!)
    //ready: Want to craft this item? YES/NO
    //Crafting... mixing... combining... item obtained!
    //packup: Pack up items? YES/NO
    //Item4 -> blank, item3 -> blank, item2 -> blank, item1 -> blank, exit
    //Select for secret recipe book? Or add to options once flagged
    //soux chefs: charmander, squirtle, bulbasaur, aron, snorunt (heat, water, spices, salt/tools, ice)

    switch(sCraftState)
    {
    
    case STATE_TABLE_INPUT:

        if (JOY_NEW(DPAD_UP))
        {
            PlaySE(SE_SELECT);
            sCraftMenuCursorPos = ChangeGridMenuCursorPosition(0,-1);
        }
        if (JOY_NEW(DPAD_LEFT))
        {
            PlaySE(SE_SELECT);
            sCraftMenuCursorPos = ChangeGridMenuCursorPosition(-1,0);
        }
        if (JOY_NEW(DPAD_RIGHT))
        {
            PlaySE(SE_SELECT);
            sCraftMenuCursorPos = ChangeGridMenuCursorPosition(1,0);
        }
        if (JOY_NEW(DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            sCraftMenuCursorPos = ChangeGridMenuCursorPosition(0,1);
        }

        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);

            gMenuCallback = sCraftTableActions[
                sCurrentCraftTableItems[sCraftMenuCursorPos][CRAFT_TABLE_ACTION]].func.u8_void;

            if (gMenuCallback == CraftMenuAddSwapCallback){ //if it's blank
                //FadeScreen(FADE_TO_BLACK, 0);
            }
            else //if it's an item
            {
                //turn arrow gray
                ShowCraftReadyUpWindow();
                sCraftState = STATE_OPTIONS_INPUT;
            }

            return FALSE;
        }

        if (JOY_NEW(START_BUTTON))
        {
            PlaySE(SE_SELECT);
            gMenuCallback = CraftMenuReadyCallback; //set up yes/no window
        }

        //select button, instant addswap item (or recipe book??)
        if (JOY_NEW(SELECT_BUTTON))
        {
            PlaySE(SE_SELECT);
            ShowCraftReadyUpWindow();
            sCraftState = STATE_OPTIONS_INPUT;
        }

        if (JOY_NEW(B_BUTTON)) //If !IsCraftTableEmpty then gmenucallback = CraftMenuPackUpCallback, otherwise just quit
        {
            //RemoveExtraCraftMenuWindows(); //if safarizone/battlepyramid flags, remove those windows. In this case,
                                             //it'd be sous chefs 
            HideCraftMenu();
            return TRUE;
        }

        return FALSE;

    case STATE_OPTIONS_INPUT:
        /*How the ITEM MENU works:
          gray cursor in item list
          gspecialvar_itemId
          open context menu
                ask what you wanna do with var_itemid
                print options grid
          taskid = which grid format handling?
          ACTION_CANCEL
                remove window
                reprint desc
                black cursor
                return to item list handling
                        refresh graphics
                        taskid = bag handling
        */
    
       if (JOY_NEW(B_BUTTON)){
            PlaySE(SE_SELECT);
            HideOptionsMenu();
            sCraftState = STATE_TABLE_INPUT;
       }



        return FALSE;
    }
}

static bool8 CraftMenuItemOptionsCallback(void){
    HideCraftMenu();

    return TRUE;
}

static bool8 CraftMenuPackUpCallback(void){
    HideCraftMenu();

    return TRUE;
}

static bool8 CraftMenuAddSwapCallback(void){
    HideCraftMenu();

    return TRUE;
}

static bool8 CraftMenuRemoveBagCallback(void){
    HideCraftMenu();

    return TRUE;    
}

static bool8 CraftMenuReadyCallback(void){
    HideCraftMenu();

    return TRUE;    
}

static bool8 CraftMenuCancelCallback(void){
    HideCraftMenu();

    return TRUE;
}

static void HideOptionsMenu(void){

    ClearStdWindowAndFrame(sCraftReadyUpWindowId, TRUE);

    if (sCraftReadyUpWindowId != WINDOW_NONE){
        RemoveWindow(sCraftReadyUpWindowId);
        sCraftReadyUpWindowId = WINDOW_NONE;
    }
}

void HideCraftMenu(void){

    ClearStdWindowAndFrame(sCraftTableWindowId, TRUE);
    if (sCraftTableWindowId != WINDOW_NONE){
        RemoveWindow(sCraftTableWindowId);
        sCraftTableWindowId = WINDOW_NONE;
    }

    ClearStdWindowAndFrame(sCraftInfoWindowId, TRUE);
    if (sCraftInfoWindowId != WINDOW_NONE){
        RemoveWindow(sCraftInfoWindowId);
        sCraftInfoWindowId = WINDOW_NONE;
    }

    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();

}
