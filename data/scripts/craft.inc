Craft_EventScript_CraftTable::
    msgbox Text_CraftTable_WantToCraft, MSGBOX_YESNO
    delay 4
    goto_if_eq VAR_RESULT, YES, Craft_UseCraftTable
    goto Craft_Cancel
    end

Text_CraftTable_WantToCraft:
    .string "There's a WORK STATION...\n"
    .string "Want to CRAFT something?$"

Craft_UseCraftTable::
    special ShowCraftMenu
    waitstate
    releaseall
    end

Craft_Cancel:
    releaseall
    end


