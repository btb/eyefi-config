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
#include <wx/treelist.h>


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
	wxTreeListCtrl *treeList;
	
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
	
	treeList = new wxTreeListCtrl(frame, wxID_ANY);
	treeList->AppendColumn("Item");
	treeList->AppendColumn("Value", 240);

	frame->Show(true);
	SetTopWindow(frame);

	char *mountPoint = locate_eyefi_mount();
	
	if (mountPoint) {
		char tempStr[33];

		wxTreeListItem cardBranch = treeList->AppendItem(treeList->GetRootItem(), _("Eye-Fi Card Mounted at"));
		treeList->SetItemText(cardBranch, 1, mountPoint);

		wxTreeListItem leaf = treeList->AppendItem(cardBranch, _("Firmware Version"));
		struct card_firmware_info *info = fetch_card_firmware_info();
		pstrcpy(tempStr, &info->info);
		treeList->SetItemText(leaf, 1, tempStr);
		
		leaf = treeList->AppendItem(cardBranch, _("MAC Address"));
		struct mac_address *mac = fetch_mac_address();
		snprintf(tempStr, sizeof(tempStr), "%02x:%02x:%02x:%02x:%02x:%02x",
				 mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
		treeList->SetItemText(leaf, 1, tempStr);
		
		leaf = treeList->AppendItem(cardBranch, _("Upload Key"));
		struct card_info_rsp_key *key = fetch_card_upload_key();
		pstrcpy(tempStr, &key->key);
		treeList->SetItemText(leaf, 1, tempStr);
		
		leaf = treeList->AppendItem(cardBranch, _("Transfer Mode"));
		enum transfer_mode mode = fetch_transfer_mode();
		treeList->SetItemText(leaf, 1, transfer_mode_names[mode]);
		
		leaf = treeList->AppendItem(cardBranch, _("WiFi Radio"));
		treeList->SetItemText(leaf, 1, wlan_enabled() ? "enabled" : "disabled");
		
		wxTreeListItem endlessBranch = treeList->AppendItem(cardBranch, _("Endless Storage"));
		int endless = fetch_endless();
		treeList->SetItemText(endlessBranch, 1, endless >> 7 ? "enabled" : "disabled");
		if (endless >> 7) {
			leaf = treeList->AppendItem(endlessBranch, _("Percentage"));
			sprintf(tempStr, "%d%%", endless % 128);
			treeList->SetItemText(leaf, 1, tempStr);
		}
		
		leaf = treeList->AppendItem(cardBranch, _("Direct Mode SSID"));
		card_info_cmd(DIRECT_MODE_SSID);
		struct pascal_string *pStr = (struct pascal_string *)eyefi_response();
		pstrcpy(tempStr, pStr);
		treeList->SetItemText(leaf, 1, tempStr);

		leaf = treeList->AppendItem(cardBranch, _("Direct Mode Password"));
		card_info_cmd(DIRECT_MODE_PASS);
		pStr = (struct pascal_string *)eyefi_response();
		pstrcpy(tempStr, pStr);
		treeList->SetItemText(leaf, 1, tempStr);
		
		wxTreeListItem networksBranch = treeList->AppendItem(cardBranch, _("Configured Wireless Networks"));
		struct configured_net_list *configured = fetch_configured_nets();
		sprintf(tempStr, "%d", configured->nr);
		treeList->SetItemText(networksBranch, 1, tempStr);
		for (int i = 0; i < configured->nr; i++) {
			struct configured_net *net = &configured->nets[i];
			wxTreeListItem netLeaf = treeList->AppendItem(networksBranch, "ESSID");
			treeList->SetItemText(netLeaf, 1, net->essid);
		}

		leaf = treeList->AppendItem(cardBranch, _("Card Key"));
		key = fetch_card_key();
		pstrcpy(tempStr, &key->key);
		treeList->SetItemText(leaf, 1, tempStr);

		treeList->Expand(cardBranch);
		treeList->Expand(endlessBranch);
		treeList->Expand(networksBranch);

	}

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
