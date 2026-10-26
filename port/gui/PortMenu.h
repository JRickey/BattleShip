/*
 * SPDX-License-Identifier: MIT
 *
 * Portions of this file are derived from the Ship of Harkinian (SoH) project
 *   Copyright (c) The Harbour Masters
 *   https://github.com/HarbourMasters/Shipwright
 * Licensed under the MIT License; see LICENSE at repository root.
 */

#pragma once

#include "Menu.h"
#include "MenuTypes.h"

#include <map>

namespace ssb64 {

class PortMenu : public Ship::Menu {
  public:
    PortMenu();
    ~PortMenu() override = default;

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
    void Draw() override;

  protected:
    void AddSidebarEntry(std::string sectionName, std::string sidbarName, uint32_t columnCount);
    WidgetInfo& AddWidget(WidgetPath& pathInfo, std::string widgetName, WidgetType widgetType);
    void AddMenuElements();
    void AddMenuSettings();
    void AddMenuAssets();
    void AddMenuNetplay();
    void AddMenuAbout();
#ifndef DISABLE_SCRIPTING
    void AddMenuMods();
#endif

  private:
    bool mShowReextractMessage = false;
    bool mMenuElementsInitialized = false;
};

} // namespace ssb64
