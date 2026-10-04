#include "pch.h"

namespace
{
#pragma pack(push, 1)
struct LegacyDlg8Item
{
    INT32 pictureType;
    INT32 pictureSubtype;
    H3String text;
    H3String message;
    INT32 defIndex;
    INT32 field2C;
    INT32 field30;
    INT32 pictureFrameHeight;
    INT32 pictureFrameWidth;
    INT32 field3C;
    INT32 field40;
    INT32 field44;
    INT32 fullWidth;
};
#pragma pack(pop)
static_assert(sizeof(LegacyDlg8Item) == 0x4C, "Legacy dialog item layout mismatch");

_LHF_(HandleLegacyPictureCategories)
{
    const INT32 pictureType = c->edi;
    if (pictureType < 37 || pictureType > 43)
        return EXEC_DEFAULT;

    auto *item = c->Ebx<LegacyDlg8Item *>();
    const INT32 amount = c->eax;

    if (pictureType == 42)
    {
        libc::sprintf(h3_TextBuffer, "jsExtraHints.%d", pictureType);
        if (const LPCSTR extraHint = EraJS::read(h3_TextBuffer))
            item->message.Append(H3String(extraHint));
        constexpr LPCSTR DEF_NAME = "PTRAIL.def";
        if (THISCALL_3(char, 0x404A40, &item->text, static_cast<UINT>(strlen(DEF_NAME)), 1))
            item->text = DEF_NAME;
        item->defIndex = 0;
    }
    else if (pictureType == 37 || pictureType == 38)
    {
        if (amount != 0)
        {
            libc::sprintf(h3_TextBuffer, amount > 0 ? "+%d" : "%d", amount);
            item->message.Append(H3String(h3_TextBuffer));
        }
        constexpr LPCSTR DEF_NAME = "PMOVE.def";
        if (THISCALL_3(char, 0x404A40, &item->text, static_cast<UINT>(strlen(DEF_NAME)), 1))
            item->text = DEF_NAME;
        item->defIndex = pictureType == 37 ? 0 : 1;
    }
    else if (pictureType == 39 || pictureType == 40)
    {
        if (amount != 0)
        {
            libc::sprintf(h3_TextBuffer, amount > 0 ? "+%d" : "%d", amount);
            item->message.Append(H3String(h3_TextBuffer));
        }
        constexpr LPCSTR DEF_NAME = "PSCOUT.def";
        if (THISCALL_3(char, 0x404A40, &item->text, static_cast<UINT>(strlen(DEF_NAME)), 1))
            item->text = DEF_NAME;
        item->defIndex = pictureType == 39 ? 0 : 1;
    }
    else if (pictureType == 41)
    {
        if (amount != 0)
        {
            libc::sprintf(h3_TextBuffer, amount > 0 ? "+%d" : "%d", amount);
            item->message.Append(H3String(h3_TextBuffer));
        }
        constexpr LPCSTR DEF_NAME = "PMANDL.def";
        if (THISCALL_3(char, 0x404A40, &item->text, static_cast<UINT>(strlen(DEF_NAME)), 1))
            item->text = DEF_NAME;
        item->defIndex = 0;
    }
    else // category 43: navigation / sea movement
    {
        if (amount != 0)
        {
            libc::sprintf(h3_TextBuffer, amount > 0 ? "+%d" : "%d", amount);
            item->message.Append(H3String(h3_TextBuffer));
        }
        constexpr LPCSTR DEF_NAME = "PNAVIG.def";
        if (THISCALL_3(char, 0x404A40, &item->text, static_cast<UINT>(strlen(DEF_NAME)), 1))
            item->text = DEF_NAME;
        item->defIndex = 0;
    }

    c->return_address = 0x4F6229;
    return NO_EXEC_DEFAULT;
}
} // namespace

void InstallLegacyDialogCategories(PatcherInstance *patcher) noexcept
{
    if (patcher)
        patcher->WriteLoHook(0x4F61DD, HandleLegacyPictureCategories);
}
