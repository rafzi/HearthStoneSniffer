// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/file.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/textdlg.h>
#include <wx/aboutdlg.h>
#include <wx/filename.h>
#include <wx/frame.h>
#include <wx/dir.h>

#include "icons/favicon-16x16-8.xpm"
#include "icons/favicon-32x32-8.xpm"
#include "icons/favicon-64x64-8.xpm"

#include "TaskBarIcon.h"
#include "Helper.h"
#include "LogWindow.h"

wxDEFINE_EVENT(HSL_LOG_AVAILABLE_EVENT, wxCommandEvent);

enum
{
	ID_Quit = 1,
	ID_About,
	ID_Log,
};

BEGIN_EVENT_TABLE(TaskBarIcon, wxTaskBarIcon)
	EVT_MENU(ID_Quit, TaskBarIcon::OnQuit)
	EVT_MENU(ID_About, TaskBarIcon::OnAbout)
	EVT_MENU(ID_Log, TaskBarIcon::OnLog)
	EVT_TASKBAR_LEFT_DOWN(TaskBarIcon::OnClick)
END_EVENT_TABLE()



TaskBarIcon::TaskBarIcon()
	: wxTaskBarIcon()
{
	SetIcon(wxIcon(favicon_16_16_8), _("HearthStoneSniffer"));

	_frame = ((LogWindow*)wxLog::GetActiveTarget())->GetFrame();
	_frame->SetIcon(wxIcon(favicon_32_32_8));
}

wxMenu *TaskBarIcon::CreatePopupMenu()
{
	auto menu = new wxMenu;

	menu->Append(ID_Log, _("Show &Log..."));
	menu->AppendSeparator();
	menu->Append(ID_About, _("&About..."));
	menu->Append(ID_Quit, _("E&xit"));

	return menu;
}

void TaskBarIcon::OnQuit(wxCommandEvent& event)
{
	Destroy();
}

void TaskBarIcon::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo about;
	about.SetName("HearthStoneSniffer");
	about.SetVersion(Helper::AppVersion());
	about.SetDescription(_("Hearthstone protocol sniffer."));
	about.SetIcon(wxIcon(favicon_32_32_8));

	wxAboutBox(about);
}

void TaskBarIcon::OnLog(wxCommandEvent& event)
{
	LogWindow *log = (LogWindow*)wxLog::GetActiveTarget();
	
	log->Show(false);
	log->Show();
}

void TaskBarIcon::OnClick(wxTaskBarIconEvent& WXUNUSED(Event))
{
	LogWindow *log = (LogWindow*)wxLog::GetActiveTarget();

	log->Show(false);
	log->Show();
}
