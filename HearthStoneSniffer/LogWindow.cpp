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
#include <wx/textfile.h>
#include <wx/filedlg.h>

#include "LogWindow.h"

static int OpenLogFile(wxFile& file, wxString *filename = NULL, wxWindow *parent = NULL);

class LogFrame : public wxFrame
{
public:
	LogFrame(wxWindow *pParent, LogWindow *log, const wxString& szTitle);
	virtual ~LogFrame();

	virtual bool ShouldPreventAppExit() const { return false; }

	void OnClose(wxCommandEvent& event);
	void OnCloseWindow(wxCloseEvent& event);

	void OnSave(wxCommandEvent& event);

	void OnClear(wxCommandEvent& event);

	void ShowLogMessage(const wxString& message)
	{
		m_pTextCtrl->AppendText(message + wxS('\n'));
	}

private:
	// use standard ids for our commands!
	enum
	{
		Menu_Close = wxID_CLOSE,
		Menu_Save = wxID_SAVE,
		Menu_Clear = wxID_CLEAR
	};

	// common part of OnClose() and OnCloseWindow()
	void DoClose();

	wxTextCtrl  *m_pTextCtrl;
	LogWindow *m_log;

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LogFrame, wxFrame)
// wxLogWindow menu events
EVT_MENU(Menu_Close, LogFrame::OnClose)
EVT_MENU(Menu_Save, LogFrame::OnSave)
EVT_MENU(Menu_Clear, LogFrame::OnClear)

EVT_CLOSE(LogFrame::OnCloseWindow)
END_EVENT_TABLE()

LogFrame::LogFrame(wxWindow *pParent, LogWindow *log, const wxString& szTitle)
		:wxFrame(pParent, wxID_ANY, szTitle)
{
	m_log = log;
	SetWindowStyleFlag(wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN | wxSTAY_ON_TOP | wxRESIZE_BORDER);

	m_pTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxDefaultSize,
		wxTE_MULTILINE | wxHSCROLL | wxTE_RICH | wxTE_READONLY);

	m_pTextCtrl->SetFont(wxFont(11, wxMODERN, wxNORMAL, wxNORMAL, false, wxT("Terminal")));

	// create menu
	wxMenuBar *pMenuBar = new wxMenuBar;
	wxMenu *pMenu = new wxMenu;
	pMenu->Append(Menu_Save, _("Save &As..."), _("Save log contents to file"));
	pMenu->Append(Menu_Clear, _("C&lear"), _("Clear the log contents"));
	pMenu->AppendSeparator();
	pMenu->Append(Menu_Close, _("&Close"), _("Close this window"));
	pMenuBar->Append(pMenu, _("&Log"));
	SetMenuBar(pMenuBar);
}

void LogFrame::DoClose()
{
	if (m_log->onFrameClose(this))
	{
		Show(false);
	}
}

void LogFrame::OnClose(wxCommandEvent& WXUNUSED(event))
{
	DoClose();
}

void LogFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
	DoClose();
}

void LogFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
	wxString filename;
	wxFile file;
	int rc = OpenLogFile(file, &filename, this);
	if (rc == -1)
	{
		// cancelled
		return;
	}

	bool bOk = rc != 0;

	// retrieve text and save it
	// -------------------------
	int nLines = m_pTextCtrl->GetNumberOfLines();
	for (int nLine = 0; bOk && nLine < nLines; nLine++) {
		bOk = file.Write(m_pTextCtrl->GetLineText(nLine) +
			wxTextFile::GetEOL());
	}

	if (bOk)
		bOk = file.Close();

	if (!bOk) {
		wxLogError(_("Can't save log contents to file."));
	}
	else {
		wxLogStatus((wxFrame*)this, _("Log saved to the file '%s'."), filename.c_str());
	}
}

void LogFrame::OnClear(wxCommandEvent& WXUNUSED(event))
{
	m_pTextCtrl->Clear();
}

LogFrame::~LogFrame()
{
	m_log->OnFrameDelete(this);
}

LogWindow::LogWindow(wxWindow *pParent,
	                 const wxString& szTitle,
					 bool bShow,
					 bool bDoPass)
{
	m_pLogFrame = NULL;

	PassMessages(bDoPass);

	m_pLogFrame = new LogFrame(pParent, this, szTitle);

	if (bShow)
		m_pLogFrame->Show();
}

void LogWindow::Show(bool bShow)
{
	m_pLogFrame->Show(bShow);
}

void LogWindow::DoLogTextAtLevel(wxLogLevel level, const wxString& msg)
{
	if (!m_pLogFrame)
		return;

	if (level == wxLOG_Trace)
		return;

	m_pLogFrame->ShowLogMessage(msg);
}

wxFrame *LogWindow::GetFrame() const
{
	return m_pLogFrame;
}

bool LogWindow::onFrameClose(wxFrame * WXUNUSED(frame))
{
	// allow to close
	return true;
}

void LogWindow::OnFrameDelete(wxFrame * WXUNUSED(frame))
{
	m_pLogFrame = NULL;
}

LogWindow::~LogWindow()
{
	delete m_pLogFrame;
}


static int OpenLogFile(wxFile& file, wxString *pFilename, wxWindow *parent)
{
	// get the file name
	// -----------------
	wxString filename = wxSaveFileSelector(wxT("log"), wxT("txt"), wxT("log.txt"), parent);
	if (!filename) {
		// cancelled
		return -1;
	}

	// open file
	// ---------
	bool bOk = true; // suppress warning about it being possible uninitialized
	if (wxFile::Exists(filename)) {
		bool bAppend = false;
		wxString strMsg;
		strMsg.Printf(_("Append log to file '%s' (choosing [No] will overwrite it)?"),
			filename.c_str());
		switch (wxMessageBox(strMsg, _("Question"),
			wxICON_QUESTION | wxYES_NO | wxCANCEL)) {
		case wxYES:
			bAppend = true;
			break;

		case wxNO:
			bAppend = false;
			break;

		case wxCANCEL:
			return -1;

		default:
			wxFAIL_MSG(_("invalid message box return value"));
		}

		if (bAppend) {
			bOk = file.Open(filename, wxFile::write_append);
		}
		else {
			bOk = file.Create(filename, true /* overwrite */);
		}
	}
	else {
		bOk = file.Create(filename);
	}

	if (pFilename)
		*pFilename = filename;

	return bOk;
}
