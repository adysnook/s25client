// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "iwLobbyConnect.h"

#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "desktops/dskLobby.h"
#include "iwMsgbox.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/src/LobbyClient.h"

iwLobbyConnect::iwLobbyConnect()
    : IngameWindow(CGI_LOBBYCONNECT, IngameWindow::posLastOrCenter, Extent(500, 260), _("Connecting to Lobby"),
                   LOADER.GetImageN("resource", 41))
{
    // Benutzername
    AddText(0, DrawPoint(20, 40), _("Username:"), COLOR_YELLOW, 0, NormalFont);
    ctrlEdit* user = AddEdit(1, DrawPoint(260, 40), Extent(220, 22), TC_GREEN2, NormalFont, 15);
    user->SetFocus();
    user->SetText(SETTINGS.lobby.name); //-V807

    // Passwort
    AddText(2, DrawPoint(20, 70), _("Password:"), COLOR_YELLOW, 0, NormalFont);
    ctrlEdit* pass = AddEdit(3, DrawPoint(260, 70), Extent(220, 22), TC_GREEN2, NormalFont, 0, true);
    pass->SetText(SETTINGS.lobby.password);

    // Emailadresse
    AddText(4, DrawPoint(20, 100), _("Email Address:"), COLOR_YELLOW, 0, NormalFont);
    ctrlEdit* email = AddEdit(5, DrawPoint(260, 100), Extent(220, 22), TC_GREEN2, NormalFont);
    email->SetText(SETTINGS.lobby.email);

    // Passwort speichern ja/nein
    AddText(6, DrawPoint(20, 130), _("Save Password?"), COLOR_YELLOW, 0, NormalFont);

    Extent btSize = Extent(105, 22);
    ctrlOptionGroup* savepassword = AddOptionGroup(10, ctrlOptionGroup::CHECK);
    savepassword->AddTextButton(0, DrawPoint(260, 130), btSize, TC_GREEN2, _("No"), NormalFont);  // nein
    savepassword->AddTextButton(1, DrawPoint(375, 130), btSize, TC_GREEN2, _("Yes"), NormalFont); // ja
    savepassword->SetSelection((SETTINGS.lobby.save_password ? 1 : 0));

    // ipv6 oder ipv4 benutzen
    AddText(11, DrawPoint(20, 160), _("Use IPv6:"), COLOR_YELLOW, 0, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(12, ctrlOptionGroup::CHECK);
    ipv6->AddTextButton(0, DrawPoint(260, 160), btSize, TC_GREEN2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(375, 160), btSize, TC_GREEN2, _("IPv6"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 1 : 0));

    btSize = Extent(220, 22);
    AddTextButton(7, DrawPoint(20, 220), btSize, TC_RED1, _("Connect"), NormalFont);
    // AddTextButton(8, DrawPoint(260, 220), btSize, TC_GREEN2, _("Register"), NormalFont);

    // Status
    AddText(9, DrawPoint(250, 195), "", COLOR_RED, glArchivItem_Font::DF_CENTER, NormalFont);

    // Lobby-Interface setzen
    LOBBYCLIENT.SetInterface(this);
}

iwLobbyConnect::~iwLobbyConnect()
{
    // Form abrufen und ggf in settings speichern
    std::string user, pass, email;
    LobbyForm(user, pass, email);

    if(!LOBBYCLIENT.IsLoggedIn())
    {
        LOBBYCLIENT.Stop();
        LOBBYCLIENT.SetInterface(NULL);
    }
}

/**
 *  speichert die eingegebenen Benutzerdaten in die Settings
 */
void iwLobbyConnect::LobbyForm(std::string& user, std::string& pass, std::string& email)
{
    // Dann Form abrufen
    user = GetCtrl<ctrlEdit>(1)->GetText();
    pass = GetCtrl<ctrlEdit>(3)->GetText();
    email = GetCtrl<ctrlEdit>(5)->GetText();

    // Name speichern
    SETTINGS.lobby.name = user; //-V807

    // Ist Passwort speichern an?
    if(SETTINGS.lobby.save_password)
    {
        // ja, Passwort speichern
        SETTINGS.lobby.password = pass;

        // Email speichern
        SETTINGS.lobby.email = email;
    } else
    {
        SETTINGS.lobby.password = "";
        SETTINGS.lobby.email = "";
    }
}

void iwLobbyConnect::Msg_EditChange(const unsigned /*ctrl_id*/)
{
    // Statustext resetten
    SetText(0, COLOR_RED, true);
}

void iwLobbyConnect::Msg_EditEnter(const unsigned ctrl_id)
{
    ctrlEdit* user = GetCtrl<ctrlEdit>(1);
    ctrlEdit* pass = GetCtrl<ctrlEdit>(3);
    ctrlEdit* email = GetCtrl<ctrlEdit>(5);

    switch(ctrl_id)
    {
        case 1:
        {
            user->SetFocus(false);
            pass->SetFocus(true);
            email->SetFocus(false);
        }
        break;
        case 3:
        {
            user->SetFocus(false);
            pass->SetFocus(false);
            email->SetFocus(true);
        }
        break;
        case 5: { Msg_ButtonClick(7);
        }
        break;
    }
}

void iwLobbyConnect::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 7: // Verbinden
        {
            // Text auf "Verbinde mit Host..." setzen und Button deaktivieren
            SetText(_("Connecting with Host..."), COLOR_RED, false);

            // Form abrufen und ggf in settings speichern
            std::string user, pass, email;
            LobbyForm(user, pass, email);

            // Einloggen
            if(!LOBBYCLIENT.Login(LOADER.GetTextN("client", 0), atoi(LOADER.GetTextN("client", 1).c_str()), user, pass,
                                  SETTINGS.server.ipv6))
            {
                SetText(_("Connection failed!"), COLOR_RED, true);
                break;
            }
        }
        break;
        case 8: // Registrieren
        {
            WINDOWMANAGER.Show(new iwMsgbox(
              _("Error"), _("To register, you have to create a valid board account at http://forum.siedler25.org at the moment.\n"), this,
              MSB_OK, MSB_EXCLAMATIONRED, 0));

            /*
            // Text auf "Verbinde mit Host..." setzen und Button deaktivieren
            SetText( _("Connecting with Host..."), COLOR_RED, false);

            // Form abrufen und ggf in settings speichern
            std::string user, pass, email;
            LobbyForm(user, pass, email);

            if( user == "" || pass == "" || email == "")
            {
                // Einige Felder nicht ausgefüllt
                SetText(_("Please fill out all fields!"), COLOR_RED, true);
                break; // raus
            }

            if(!LOBBYCLIENT.Register(LOADER.GetTextN("client", 0), atoi(LOADER.GetTextN("client", 1)), user, pass, email))
            {
                SetText(_("Connection failed!"), COLOR_RED, true);
                break;
            }*/
        }
        break;
    }
}

void iwLobbyConnect::Msg_OptionGroupChange(const unsigned ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 10: // Passwort speichern Ja/Nein
        {
            SETTINGS.lobby.save_password = (selection == 1);
        }
        break;
        case 12: // IPv6 Ja/Nein
        {
            SETTINGS.server.ipv6 = (selection == 1);
        }
        break;
    }
}

/**
 *  Setzt den Text und Schriftfarbe vom Textfeld und den Status des
 *  Buttons.
 */
void iwLobbyConnect::SetText(const std::string& text, unsigned color, bool button)
{
    ctrlText* t = GetCtrl<ctrlText>(9);
    ctrlButton* b = GetCtrl<ctrlButton>(7);
    //ctrlButton* b2 = GetCtrl<ctrlButton>(8);

    // Text setzen
    t->SetTextColor(color);
    t->SetText(text);

    // Button (de)aktivieren
    b->SetEnabled(button);
    //b2->SetEnabled(button);
}

/**
 *  Wir wurden eingeloggt
 */
void iwLobbyConnect::LC_LoggedIn(const std::string& email)
{
    // geänderte Daten speichern (also die erhaltene Emailadresse)
    std::string user, pass, email2 = email;

    GetCtrl<ctrlEdit>(5)->SetText(email);
    LobbyForm(user, pass, email2);

    //GetCtrl<ctrlButton>(8)->SetEnabled(false);

    WINDOWMANAGER.Switch(new dskLobby);
}

/**
 *  Wir wurden registriert.
 */
void iwLobbyConnect::LC_Registered()
{
    // Registrierung erfolgreich
    SetText(_("Registration successful!"), COLOR_YELLOW, true);

    //GetCtrl<ctrlButton>(8)->SetEnabled(false);
}

/**
 *  Status: Warten auf Antwort.
 */
void iwLobbyConnect::LC_Status_Waiting()
{
    SetText(_("Waiting for Reply..."), COLOR_YELLOW, false);
}

/**
 *  Status: Benutzerdefinierter Fehler (inkl Conn-Reset u.ä)
 */
void iwLobbyConnect::LC_Status_Error(const std::string& error)
{
    SetText(error, COLOR_RED, true);
}
