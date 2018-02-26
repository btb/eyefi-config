//
//  eyefi-wxgui.cpp
//  eyefi-config
//
//  Created by Bradley Bell on 2/25/18.
//

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
	frame->SetStatusText(_T("Card not found"));
	
	treeList = new wxTreeListCtrl(frame, wxID_ANY);
	treeList->AppendColumn("Item");
	treeList->AppendColumn("Value");

	
	
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
