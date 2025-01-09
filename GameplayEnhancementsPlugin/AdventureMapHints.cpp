#include "pch.h"

namespace advMapHints
{
RECT AdventureMapHints::m_mapView;

AdventureMapHints *AdventureMapHints::instance = nullptr;
void AdventureMapHints::Init(PatcherInstance *pi)
{
    if (!instance)
    {

        if (instance = new AdventureMapHints(pi))
        {
            instance->_pi = pi;
            instance->CreatePatches();
        }
    }
}

AdventureMapHints::AdventureMapHints(PatcherInstance *pi) : IGamePatch(pi)
{
    m_mapView.left = 8;                                   // right panel
    m_mapView.top = 8;                                    // bottom panel
    m_mapView.right = H3GameWidth::Get() - (800 - 592);   // right panel
    m_mapView.bottom = H3GameHeight::Get() - (600 - 544); // bottom panel
    settings = new AdventureHintsSettings("Runtime/gem_AdventureMapHints.ini", "StaticHintsDrawByType");
}

DWORD lastTime = 0;
RECT rect{200, 100, 200, 24};
RECT _rect{rect};

_LHF_(WoG_AdvMapSetHint)
{
    const PCHAR text = *reinterpret_cast<PCHAR *>(c->ebp + 0x14);

    // sprintf(h3_TextBuffer, "%s = %d", "tetats" , text);
    if (text)
    {
        {
            text;
        }
    }

    return EXEC_DEFAULT;
}

void AdventureMapHints::CreatePatches() noexcept
{

    if (!m_isInited)
    {

        blockAdventureHintDraw = _pi->CreateHexPatch(0x040D0F4, const_cast<char *>("EB 40 90"));

        _pi->WriteHiHook(0x40F5D7, THISCALL_, AdvMgr_ObjectDraw);
        _pi->WriteHiHook(0x040F6C4, THISCALL_, AdvMgr_DrawCornerFrames);

        m_isInited = true;
    }
}

LPCSTR AdventureMapHints::GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX,
                                      const int mapY, const int mapZ) noexcept
{

    constexpr UINT NOT_VISITED_TEXT_ID = 354;
    constexpr UINT VISITED_TEXT_ID = 353;

    auto visitedText = H3GeneralText::Get()->GetText(VISITED_TEXT_ID + 1);
    auto notVisitedText = H3GeneralText::Get()->GetText(NOT_VISITED_TEXT_ID + 1);

    H3String changedVisitedText = H3String::Format("{~Green}\n%s}", visitedText).String();
    H3String changedNotVisitedText = H3String::Format("{~Orange}\n%s}", notVisitedText).String();

    H3Vector<LPCSTR> *generalTextPtr = reinterpret_cast<H3Vector<LPCSTR> *>(ADDRESS(H3GeneralText::Get()) + 0x1C);

    const BOOL isCreature = mapItem->objectType == eObject::MONSTER;
    DWORD oldCreatureHintFormat = 0;
    if (isCreature)
    {
        oldCreatureHintFormat = DwordAt(0x40C2E7 + 1);

        LPCSTR advMapCreatureHintFormat = "{%s}\n%s";
        DwordAt(0x40C2E7 + 1) = reinterpret_cast<DWORD>(advMapCreatureHintFormat);
    }

    auto p_visited = generalTextPtr->At(VISITED_TEXT_ID);
    auto p_notVisited = generalTextPtr->At(NOT_VISITED_TEXT_ID);

    *p_visited = changedVisitedText.String();
    *p_notVisited = changedNotVisitedText.String();
    Era::SetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint", 1);
    instance->blockAdventureHintDraw->Apply();
    THISCALL_4(void, 0x40B0B0, adv, mapItem, mapX, mapY);
    Era::SetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint", 0);

    instance->blockAdventureHintDraw->Undo();

    *p_visited = visitedText;
    *p_notVisited = notVisitedText;
    if (oldCreatureHintFormat)
    {
        DwordAt(0x40C2E7 + 1) = oldCreatureHintFormat;
    }

    return h3_TextBuffer;
}
char ermVariableNameBuffer[64];

void __stdcall AdventureMapHints::AdvMgr_ObjectDraw(HiHook *h, H3AdventureManager *adv, int mapX, int mapY, int mapZ,
                                                    int screenX, int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);

    // check is key is held
    if (STDCALL_1(SHORT, PtrAt(0x63A294), instance->settings->vKey) & 0x800 && instance->settings->isHeld)
    {
        const int currentPlayerID = P_CurrentPlayerID;
        sprintf(ermVariableNameBuffer, "gem_adventure_map_object_hints_option_%d", currentPlayerID); // ;
        const int optionIsEnabled = Era::GetAssocVarIntValue(ermVariableNameBuffer);
        if (optionIsEnabled == 0)
        {
            return;
        }
        //  isIconic -- doesn't work smh
        // if (STDCALL_1(BOOL, PtrAt(0x063A2A8), 0x699650) == false)
        //    return;

        H3Position pos(mapX, mapY, mapZ);
        if (mapX >= 0 && mapY >= 0 && mapX < *P_MapSize && mapY < *P_MapSize && H3TileVision::CanViewTile(pos))
        {
            H3MapItem *currentItem = P_Game->GetMapItem(pos.Mixed());
            if (currentItem &&
                instance->m_drawnOjects.find(currentItem->drawnObjectIndex) == instance->m_drawnOjects.cend() &&
                instance->NeedDrawMapItem(currentItem))
            {

                instance->m_drawnOjects.insert(currentItem->drawnObjectIndex);

                LPCSTR hintText = GetHintText(adv, currentItem, mapX, mapY, mapZ);

                auto &attributes = P_Game->mainSetup.objectAttributes[currentItem->drawnObjectIndex];
                auto &passability = attributes.passability;

                // passability;
                // echo(passability.m_bits[0][0]);
                // echo(P_Game->mainSetup.objectDetails.Size());

                constexpr int TILE_WIDTH = 32;

                H3FontLoader fnt(NH3Dlg::Text::TINY);

                constexpr int OBJECT_WIDTH = 1 * TILE_WIDTH;

                constexpr int minTextFieldWidth = OBJECT_WIDTH;
                const int maxHintTextWidth = fnt->GetMaxLineWidth(hintText);
                constexpr int maxTextFieldWidth = TILE_WIDTH * 3;
                const int textWidth = Clamp(minTextFieldWidth, maxHintTextWidth, maxTextFieldWidth);

                const int hintTextLines = fnt->GetLinesCountInText(hintText, textWidth);
                const int minTextFieldHeight = fnt->height + 2;
                const int textHeight = hintTextLines * (minTextFieldHeight - 1);

                // textFieldWidth =
                // if (currentItem->objectType == eObject::SPELL_SCROLL)
                //{
                //    libc::sprintf(Era::z[1], "%s", hintText);

                //    // Era::y[1] = TEMP_PCX_WIDTH;
                //    Era::y[2] = textWidth;
                //    Era::y[3] = maxHintTextWidth;
                //    Era::y[4] = HINT_MAX_WORD_WIDTH;
                //    Era::ExecErmCmd("IF:L^text = %z1, textWidth = %y2, maxHintTextWidth = %y3, HINT_MAX_WORD_WIDTH
                //    "
                //                    "width = %y4^");
                //}

                // constexpr INT START_DRAW_X = 8;

                constexpr int TEXT_MARGIN = 2;

                const int TEMP_PCX_WIDTH = textWidth + 10;
                const int TEMP_PCX_HEIGHT = textHeight + TEXT_MARGIN;

                auto tempBuffer = H3LoadedPcx16::Create(TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT);

                memset(tempBuffer->buffer, 0, tempBuffer->buffSize);

                //  create golden frame
                tempBuffer->DrawFrame(0, 0, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT, 189, 149, 57);
                // draw text to temp buffer
                fnt->TextDraw(tempBuffer, hintText, TEXT_MARGIN, 0, TEMP_PCX_WIDTH - TEXT_MARGIN, TEMP_PCX_HEIGHT);

                // resize tempBuffer to align text for screen borders

                int objectWidth = 1 * TILE_WIDTH;
                const int outOfWidthBorder = (TEMP_PCX_WIDTH - objectWidth) >> 1;

                int destPcxX = screenX * TILE_WIDTH + adv->screenDrawOffset.x - outOfWidthBorder;
                int destPcxY = screenY * TILE_WIDTH + adv->screenDrawOffset.y - TEMP_PCX_HEIGHT;

                // adjust left border draw
                UINT srcX = 0;
                if (destPcxX < m_mapView.left)
                {
                    srcX = m_mapView.left - destPcxX;
                    destPcxX = m_mapView.left;
                    tempBuffer->width -= srcX;
                }
                if (destPcxX + TEMP_PCX_WIDTH - m_mapView.left > m_mapView.right)
                    tempBuffer->width = m_mapView.right + m_mapView.left - destPcxX;

                UINT srcY = 0;
                if (destPcxY < m_mapView.top)
                {
                    srcY = m_mapView.top - destPcxY;
                    destPcxY = m_mapView.top;
                    tempBuffer->height -= srcY;
                }
                if (destPcxY + TEMP_PCX_HEIGHT - m_mapView.top > m_mapView.bottom)
                    tempBuffer->height = m_mapView.top + m_mapView.bottom - destPcxY;

                // if need to draw any hint
                if (tempBuffer->height > 0 && tempBuffer->width > 0)
                {
                    // get general Window draw buffer to draw temp pcx with x/y offsets
                    auto drawBuffer = P_WindowManager->GetDrawBuffer();
                    tempBuffer->DrawToPcx16(destPcxX, destPcxY, 1, drawBuffer, srcX, srcY);

                    constexpr UINT SHADOW_SIZE = 3;
                    int heightReserve = m_mapView.bottom - tempBuffer->height - destPcxY + m_mapView.top;
                    UINT shadowWidth = 0;

                    UINT shadowHeight = 0;

                    if (heightReserve > 0)
                        shadowHeight = heightReserve >= SHADOW_SIZE ? SHADOW_SIZE : heightReserve;

                    int widthReserve = m_mapView.right - tempBuffer->width - destPcxX + m_mapView.left;
                    if (widthReserve > 0)
                        shadowWidth = widthReserve >= SHADOW_SIZE ? SHADOW_SIZE : widthReserve;

                    if (shadowWidth)
                        drawBuffer->DrawShadow(destPcxX + tempBuffer->width, destPcxY, shadowWidth,
                                               tempBuffer->height + shadowHeight);

                    if (shadowHeight)
                        drawBuffer->DrawShadow(destPcxX, destPcxY + tempBuffer->height,
                                               tempBuffer->width + (shadowHeight ? 0 : shadowWidth), shadowHeight);
                }

                //	backPcx->Dereference();
                tempBuffer->Destroy();
            }
        }
    }
}
void __stdcall AdventureMapHints::AdvMgr_DrawCornerFrames(HiHook *h, const H3AdventureManager *adv)
{
    THISCALL_1(void, h->GetDefaultFunc(), adv);
    if (instance->m_drawnOjects.size())
    {
        instance->m_drawnOjects.clear();
    }
}

bool AdventureMapHints::NeedDrawMapItem(const H3MapItem *mIt) const noexcept
{
    if (mIt)
        return settings->m_objectsToDraw[mIt->objectType];
    return false;
}

AdventureMapHints::~AdventureMapHints()
{
}

AdventureHintsSettings::AdventureHintsSettings(const char *filePath, const char *sectionName)
    : ISettings{filePath, sectionName}
{
    reset();
    load();
    save();
    isHeld = true;
}

void AdventureHintsSettings::reset()
{
    vKey = VK_MENU;

    memset(m_objectsToDraw, false, sizeof(m_objectsToDraw));
    m_objectsToDraw[eObject::MONSTER] = true;
    m_objectsToDraw[eObject::CREATURE_BANK] = true;
    m_objectsToDraw[eObject::RESOURCE] = true;
    m_objectsToDraw[eObject::ARTIFACT] = true;
    m_objectsToDraw[eObject::DRAGON_UTOPIA] = true;
    m_objectsToDraw[eObject::CRYPT] = true;
    m_objectsToDraw[eObject::DERELICT_SHIP] = true;
    m_objectsToDraw[eObject::SHIPWRECK] = true;
    m_objectsToDraw[eObject::WITCH_HUT] = true;
    m_objectsToDraw[eObject::MARLETTO_TOWER] = true;
    m_objectsToDraw[eObject::GARDEN_OF_REVELATION] = true;
    m_objectsToDraw[eObject::MYSTICAL_GARDEN] = true;
    m_objectsToDraw[eObject::PYRAMID] = true;
    m_objectsToDraw[eObject::SHRINE_OF_MAGIC_GESTURE] = true;
    m_objectsToDraw[eObject::SHRINE_OF_MAGIC_INCANTATION] = true;
    m_objectsToDraw[eObject::SHRINE_OF_MAGIC_THOUGHT] = true;
    m_objectsToDraw[eObject::STABLES] = true;
    m_objectsToDraw[eObject::LIBRARY_OF_ENLIGHTENMENT] = true;
    m_objectsToDraw[eObject::SCHOOL_OF_MAGIC] = true;
    m_objectsToDraw[eObject::SCHOOL_OF_WAR] = true;
    m_objectsToDraw[eObject::MAGIC_SPRING] = true;
    m_objectsToDraw[eObject::STAR_AXIS] = true;
    m_objectsToDraw[eObject::TREASURE_CHEST] = true;
    m_objectsToDraw[eObject::SEA_CHEST] = true;
    m_objectsToDraw[eObject::PANDORAS_BOX] = true;
    m_objectsToDraw[eObject::TREE_OF_KNOWLEDGE] = true;
    m_objectsToDraw[eObject::SPELL_SCROLL] = true;
    m_objectsToDraw[eObject::LEARNING_STONE] = true;
    m_objectsToDraw[eObject::CORPSE] = true;
    m_objectsToDraw[eObject::WATER_WHEEL] = true;
    m_objectsToDraw[eObject::WINDMILL] = true;
    m_objectsToDraw[eObject::ARENA] = true;
    m_objectsToDraw[eObject::CAMPFIRE] = true;
    m_objectsToDraw[eObject::FLOTSAM] = true;
    m_objectsToDraw[eObject::SHIPWRECK_SURVIVOR] = true;
    m_objectsToDraw[142] = true;
    m_objectsToDraw[144] = true;
}

BOOL AdventureHintsSettings::load()
{
    for (UINT8 i = 0; i < limits::OBJECTS; ++i)
    {

        if (Era::ReadStrFromIni(Era::IntToStr(i).c_str(), sectionName, filePath, h3_TextBuffer))
            m_objectsToDraw[i] = atoi(h3_TextBuffer);
    }
    if (Era::ReadStrFromIni("KeyCode", "ControlSettings", filePath, h3_TextBuffer))
        vKey = atoi(h3_TextBuffer);
    return 0;
}

BOOL AdventureHintsSettings::save()
{
    for (UINT8 i = 0; i < limits::OBJECTS; ++i)
    {

        Era::WriteStrToIni(Era::IntToStr(i).c_str(), Era::IntToStr(m_objectsToDraw[i]).c_str(), sectionName, filePath);
    }
    Era::WriteStrToIni("KeyCode", Era::IntToStr(vKey).c_str(), "ControlSettings", filePath);

    Era::SaveIni(filePath);
    return 0;
}

LPCSTR *AdventureMapHints::AccessableH3GeneralText::GetStringAdddres(const int row)
{

    return &text[row - 1];
}
} // namespace advMapHints
