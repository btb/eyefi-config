//
//  eyefi-wxgui.cpp
//  eyefi-config
//
//  Created by Bradley Bell on 2/25/18.
//

#include <cstring>

// For compilers that don't support precompilation, include "wx/wx.h";
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/propgrid/propgrid.h>


extern "C" {
#include "eyefi-config.h"

	void open_error(char *file, int ret)
	{
		fprintf(stderr, "unable to open '%s' (%d)\n", file, ret);
		fprintf(stderr, "Is the Eye-Fi card inserted and mounted at: %s ?\n", locate_eyefi_mount());
		fprintf(stderr, "Do you have write permissions to it?\n");
		fprintf(stderr, "debug information:\n");
		if (eyefi_debug_level > 0)
			system("cat /proc/mounts >&2");
		if (eyefi_debug_level > 1)
			perror("bad open");
		exit(1);
	}
	
}

const char *transfer_mode_names[] = {
	"AUTO",
	"SELSHARE",
	"SELUPLOAD",
};

int pstrcpy(char *dest, pascal_string *src)
{
	return snprintf(dest, src->length, "%s", src->value);
}

class EyeFiGui : public wxApp
{
	virtual bool OnInit();

protected:
	wxToolBar *toolBar;
	
private:
	void OnAbout(wxCommandEvent& evt);
	void OnPrefs(wxCommandEvent& evt);
	void Eject(wxCommandEvent& evt);

	DECLARE_EVENT_TABLE()
};

IMPLEMENT_APP(EyeFiGui)

BEGIN_EVENT_TABLE(EyeFiGui, wxApp)
EVT_MENU(wxID_ABOUT, EyeFiGui::OnAbout)
EVT_MENU(wxID_CLOSE, EyeFiGui::Eject)
EVT_MENU(wxID_PREFERENCES, EyeFiGui::OnPrefs)
END_EVENT_TABLE()

bool EyeFiGui::OnInit()
{
	wxFrame *frame = new wxFrame((wxFrame*) NULL, -1, _T("EyeFi-Config GUI"));

	wxMenuBar* menubar = new wxMenuBar();
	wxMenu* filemenu = new  wxMenu();
	wxMenu* helpmenu = new  wxMenu();

//	filemenu->Append(wxID_PREFERENCES, _("Preferences"));
	filemenu->Append(wxID_CLOSE, _("Eject"), _("Unmount the Eye-Fi card"));
	filemenu->Append(wxID_EXIT, _("Exit"));
	
	helpmenu->Append(wxID_ABOUT, _("About"));
	
	menubar->Append(filemenu, _("File"));
	menubar->Append(helpmenu, _("Help"));

	frame->SetMenuBar(menubar);
	
	
	toolBar = frame->CreateToolBar();
	toolBar->AddTool(wxID_CLOSE, _("Eject"), wxArtProvider::GetBitmap(wxART_CLOSE));
	toolBar->Realize();
	
	frame->CreateStatusBar();
//	frame->SetStatusText(_T("Card not found"));
	
	// Construct wxPropertyGrid control
	wxPropertyGrid* pg = new wxPropertyGrid(
											frame, // parent
											wxID_ANY, // id
											wxDefaultPosition, // position
											wxDefaultSize, // size
											// Here are just some of the supported window styles
//											wxPG_AUTO_SORT | // Automatic sorting after items added
											wxPG_SPLITTER_AUTO_CENTER | // Automatically center splitter until user manually adjusts it
											// Default style
											wxPG_DEFAULT_STYLE );
	// Window style flags are at premium, so some less often needed ones are
	// available as extra window styles (wxPG_EX_xxx) which must be set using
	// SetExtraStyle member function. wxPG_EX_HELP_AS_TOOLTIPS, for instance,
	// allows displaying help strings as tool tips.
	pg->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );

	char *mountPoint = locate_eyefi_mount();
	
	if (mountPoint) {
		char tempStr[33];

		pg->Append( new wxStringProperty(_("Eye-Fi Card Mounted at"), wxPG_LABEL, mountPoint) );

		struct card_firmware_info *info = fetch_card_firmware_info();
		pstrcpy(tempStr, &info->info);
		pg->Append( new wxStringProperty(_("Firmware Version"), wxPG_LABEL, tempStr) );

		struct mac_address *mac = fetch_mac_address();
		snprintf(tempStr, sizeof(tempStr), "%02x:%02x:%02x:%02x:%02x:%02x",
				 mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
		pg->Append( new wxStringProperty(_("MAC Address"), wxPG_LABEL, tempStr) );

		struct card_info_rsp_key *key = fetch_card_upload_key();
		pstrcpy(tempStr, &key->key);
		pg->Append( new wxStringProperty(_("Upload Key"), wxPG_LABEL, tempStr) );

		enum transfer_mode mode = fetch_transfer_mode();
		pg->Append( new wxStringProperty(_("Transfer Mode"), wxPG_LABEL, transfer_mode_names[mode]) );

		pg->Append( new wxStringProperty(_("WiFi Radio"), wxPG_LABEL, wlan_enabled() ? "enabled" : "disabled") );

		int endless = fetch_endless();
		pg->Append( new wxStringProperty(_("Endless Storage"), wxPG_LABEL, endless >> 7 ? "enabled" : "disabled") );
		if (endless >> 7) {
			sprintf(tempStr, "%d%%", endless % 128);
			pg->Append( new wxStringProperty(_("Percentage"), wxPG_LABEL, tempStr) );
		}

		card_info_cmd(DIRECT_MODE_SSID);
		struct pascal_string *pStr = (struct pascal_string *)eyefi_response();
		pstrcpy(tempStr, pStr);
		pg->Append( new wxStringProperty(_("Direct Mode SSID"), wxPG_LABEL, tempStr) );

		card_info_cmd(DIRECT_MODE_PASS);
		pStr = (struct pascal_string *)eyefi_response();
		pstrcpy(tempStr, pStr);
		pg->Append( new wxStringProperty(_("Direct Mode Password"), wxPG_LABEL, tempStr) );

		struct configured_net_list *configured = fetch_configured_nets();
		sprintf(tempStr, "%d", configured->nr);
		pg->Append( new wxStringProperty(_("Configured Wireless Networks"), wxPG_LABEL, tempStr) );
		for (int i = 0; i < configured->nr; i++) {
			struct configured_net *net = &configured->nets[i];
			sprintf(tempStr, "ESSID %d", i);
			pg->Append( new wxStringProperty(_(tempStr), wxPG_LABEL, net->essid) );
		}

		key = fetch_card_key();
		pstrcpy(tempStr, &key->key);
		pg->Append( new wxStringProperty(_("Card Key"), wxPG_LABEL, tempStr) );

	}

	frame->Show(true);
	SetTopWindow(frame);

	return true;
}

void EyeFiGui::OnAbout(wxCommandEvent& evt)
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName("EyeFi-Config GUI");
	aboutInfo.SetVersion("0.0.1");
	aboutInfo.SetDescription(_("An Eye-Fi Configuration Tool"));
	aboutInfo.SetCopyright("(C) 2007-2018");
//	aboutInfo.SetWebSite("https://github.com/hansendc/eyefi-config");
	aboutInfo.AddDeveloper("Dave Hansen");
	aboutInfo.AddDeveloper("Bradley Bell (GUI)");
	wxAboutBox(aboutInfo);
}

void EyeFiGui::OnPrefs(wxCommandEvent& evt)
{
	wxMessageBox(_("Here are preferences. Not much to configure yet"));
}

void EyeFiGui::Eject(wxCommandEvent& evt)
{
	eject_card();
}
