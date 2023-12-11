#ifndef GUARD_CRAFT_MENU_H
#define GUARD_CRAFT_MENU_H

extern bool8 (*gMenuCallback)(void);

void ShowReturnToFieldCraftMenu(void);
void Task_ShowCraftMenu(u8 taskId);
void ShowCraftMenu(void);
void HideCraftMenu(void);

#endif // GUARD_CRAFT_MENU_H
