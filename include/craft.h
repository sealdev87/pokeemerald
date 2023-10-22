#ifndef GUARD_CRAFT_H
#define GUARD_CRAFT_H

//extern const u8 *const gStdStrings[];
/*extern u8 CraftItem1[0x100];
extern u8 CraftItem2[0x100];
extern u8 CraftItem3[0x100];
extern u8 CraftItem4[0x100];*/

extern u8 gStringVar1[0x100];
extern u8 gStringVar2[0x100];
extern u8 gStringVar3[0x100];
extern u8 gStringVar4[0x3E8];



// Window IDs
/*
#define C_WIN_CRAFT_TABLE         1 // Crafting Table
#define C_WIN_ITEM_NAME_1         1 // Top left
#define C_WIN_ITEM_NAME_2         2 // Top right
#define C_WIN_ITEM_NAME_3         3 // Bottom left
#define C_WIN_ITEM_NAME_4         4 // Bottom right
#define C_WIN_DUMMY               2
#define C_WIN_CRAFT_PROMPT        3 // Ready to craft? Press start!
#define C_WIN_YESNO               4
*/

// This is from ketsuban's debug_menu_tutorial (https://gist.github.com/ketsuban/1751fd9cdca331831fe4da9283675465)
#define DEBUG_MAIN_MENU_HEIGHT 6
#define DEBUG_MAIN_MENU_WIDTH 11


void Debug_ShowMainMenu(void);
static void Debug_DestroyMainMenu(u8);
static void DebugAction_Cancel(u8);
static void DebugTask_HandleMainMenuInput(u8);

// On to the crafting stuff!
void CraftMenu_Init(void);
bool32 Craft_ShowMainMenu(void);
static void Task_HandleCraftMenuInput(u8 taskId);
static void Craft_DestroyMainMenu(u8);


#endif // GUARD_CRAFT_H