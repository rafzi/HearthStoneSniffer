#include <wx/frame.h>
#include <wx/config.h>
#include <wx/fileconf.h>

#include <fstream>
#include <memory>

#include "HSSnifferApp.h"

#include "Helper.h"
#include "LogWindow.h"
#include "TaskBarIcon.h"
#include "PacketCapture.h"
#include "tcp/Parser.h"
#include "GameDecoder.h"

IMPLEMENT_APP(HSSnifferApp);

TaskBarIcon *icon;

std::ofstream fout;

bool HSSnifferApp::OnInit()
{

	// Built the path for a log file
	auto file = Helper::GetUserDataDir();
	file.SetFullName("log.txt");

	// Create the containing directory if needed
	if (!file.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		wxLogError("error creating save directory: %s", file.GetPath());
		return false;
	}

	// Open a file for logging
	fout.open(file.GetFullPath().c_str().AsChar(), std::ofstream::out);

	// Setup the log file
	auto logFile = new wxLogStream(&fout);
	wxLog::SetActiveTarget(logFile);

	// Wrap the log file with a GUI window
	auto logWindow = new LogWindow(NULL, _("Log"), false, true);
	wxLog::SetActiveTarget(logWindow);

	// Start logging
	wxLog::SetVerbose();
	wxLogMessage(_("Hearth Log %s"), Helper::AppVersion());

	// Create the GUI bits
	icon = new TaskBarIcon();

	// Setup a packet parsing stack
	PacketCapture::Start("tcp port 3724 or tcp port 1119",
		[]() -> PacketCapture::Callback::Ptr {
		return std::make_unique<tcp::Parser>(
			[](int64_t nanotime, tcp::Stream *stream) ->tcp::Parser::Callback::Ptr {
			return std::make_unique<GameDecoder>(nanotime, stream);
		});
	});

	return true;
}