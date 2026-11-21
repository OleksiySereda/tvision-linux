/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TVPROC.H                                                              */
/*                                                                         */
/*   defines the classes TProcess, TProcessView and TProcessDialog         */
/*                                                                         */
/*   TProcess runs a command through the shell (/bin/sh -c on Linux,       */
/*   cmd.exe /c on Windows) with its output (stdout and stderr together)   */
/*   in a pipe that is read without waiting; stdin is empty.  stop()       */
/*   ends it with everything it started.                                   */
/*                                                                         */
/*   TProcessView is a TLogView that shows the output of a TProcess as     */
/*   it comes; poll() reads it, and is called from the program's idle()    */
/*   (or by TProcessDialog).                                               */
/*                                                                         */
/*   TProcessDialog runs a command in a dialog: the output, a status       */
/*   line and Stop / Close:                                                */
/*                                                                         */
/*       int code = TProcessDialog::run( "Build", "make -j4" );            */
/*                                                                         */
/*   The output is taken as UTF-8 (on Windows: UTF-8 if it is valid, or   */
/*   else the OEM code page) and shown in the code page of the screen.    */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TProcess, Uses_TProcessView or   */
/*   Uses_TProcessDialog.                                                  */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TProcess ) && !defined( __TProcess )
#define __TProcess

#include <string>
#include <vector>

class TProcess
{

public:

    TProcess();
    ~TProcess();                        // stops it if it runs

    // the command line goes to the shell as it is; dir: where it runs
    // ("" = here); env: "NAME=value" added to the environment.  False if
    // it could not be started (error() says why).
    bool start( const std::string& command, const std::string& dir = "",
                const std::vector<std::string>& env = std::vector<std::string>() );

    // what came out since the last call, without waiting (raw bytes)
    std::string read();
    // True while it runs or output is still in the pipe
    bool running();
    // the exit code once it has ended: 128 + signal if it was killed
    // (143 after stop()), -1 while it runs
    int exitCode() const
        { return code; }
    bool stopped() const
        { return stopRequested; }
    const std::string& error() const
        { return failure; }

    // asks it to end (SIGTERM to its process group; on Windows the job
    // ends); a second stop() kills it
    void stop();

    // output as UTF-8: on Linux as it is; on Windows the OEM code page
    // of cmd.exe is converted unless it already is UTF-8
    static std::string toUtf8( const std::string& raw );

private:

    bool finished();

    std::string failure;
    int code;
    bool started, stopRequested, eof;
    int stops;

#if defined( _WIN32 )
    void *process, *job, *pipe;
#else
    int pid, fd;
#endif

    TProcess( const TProcess& );
    TProcess& operator = ( const TProcess& );
};

#endif  // Uses_TProcess


#if defined( Uses_TProcessView ) && !defined( __TProcessView )
#define __TProcessView

class TRect;
class TScrollBar;

class TProcessView : public TLogView
{

public:

    TProcessView( const TRect& bounds, TScrollBar *aHScrollBar, TScrollBar *aVScrollBar );

    // clears the view, shows "$ command" and starts it
    bool start( const char *command, const char *dir = 0 );
    // reads the new output into the view; True while it runs
    Boolean poll();
    void stop();
    Boolean running();
    int exitCode() const
        { return process != 0 ? process->exitCode() : -1; }

    // shows the exit line at the end ("exit code 0", in goodColor or
    // errorColor); True by default
    Boolean showExit;

    // the process of the last start(), or 0
    TProcess *process;

    ~TProcessView();

private:

    void addOutput( const std::string& text, Boolean flush );

    std::string partial;                // a line without its end yet
    Boolean wasRunning;
};

#endif  // Uses_TProcessView


#if defined( Uses_TProcessDialog ) && !defined( __TProcessDialog )
#define __TProcessDialog

class TProcessView;
class TButton;
class TProcessStatus;
struct TEvent;

const ushort cmProcessStop = 544;

class TProcessDialog : public TDialog
{

public:

    // most of the desktop
    TProcessDialog( const char *title, const char *command, const char *dir = 0 );

    // starts the command when the dialog is run
    virtual ushort execute();
    // the dialog reads the output between its events
    virtual void getEvent( TEvent& event );
    virtual void handleEvent( TEvent& event );

    // runs command in a dialog on the desktop; returns its exit code
    // (-1 if it could not be started)
    static int run( const char *title, const char *command, const char *dir = 0 );

    TProcessView *view;
    TButton *stopButton;

private:

    void showStatus();

    std::string command, dir;
    TProcessStatus *status;
    Boolean ended;
};

#endif  // Uses_TProcessDialog
