#pragma once

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

class LogFrame;

class LogWindow :
	public wxLogPassThrough
{
public:
	LogWindow(wxWindow *pParent,
		              const wxString& szTitle,
					  bool bShow = true,
					  bool bPassToOld = true);
	virtual ~LogWindow();

	void Show(bool bShow = true);

	wxFrame *GetFrame() const;

	virtual bool onFrameClose(wxFrame *frame);

	virtual void OnFrameDelete(wxFrame *frame);

protected:
	virtual void DoLogTextAtLevel(wxLogLevel Level, const wxString& msg);

private:
	LogFrame *m_pLogFrame;
};
