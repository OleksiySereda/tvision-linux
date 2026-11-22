/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TVPROC.H                                                              */
/*                                                                         */
/*   defines the classes TProcess, TProcessView and TProcessDialog         */
/*                                                                         */
/*   TProcess runs a command through the shell (/bin/sh -c on Linux,       */
/*   cmd.exe /d /s /c on Windows), or a program and its arguments without  */
/*   a shell (startArgv), in the background: the output comes through     */
/*   pipes read without waiting, so the user interface keeps running.     */
/*   TProcessOptions say where it runs, what is added to its environment,  */
/*   what goes to its stdin (and whether stdin stays open for send()),     */
/*   whether stdout is kept apart (readCaptured(), for a value the command */
/*   prints) and, on Linux, whether it gets a session of its own without   */
/*   a terminal (ssh then asks through SSH_ASKPASS).  stop() ends it with  */
/*   everything it started: SIGTERM to its process group and SIGKILL if    */
/*   it is still there after killMs; on Windows the job object ends.       */
/*   runAttached() runs a command on the terminal itself and waits.        */
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
#include <utility>
#include <vector>

// variables for a command: name, value
typedef std::vector< std::pair<std::string, std::string> > TProcessEnv;

struct TProcessOptions
{
    std::string dir;                    // where it runs; "" = here
    TProcessEnv env;                    // added to the environment
    std::string input;                  // written to stdin, which then closes
    bool keepInput;                     // stdin stays open for send()
    bool captureStdout;                 // stdout to readCaptured(), stderr to read()
    bool newSession;                    // Linux: setsid() (no terminal); else
                                        // a process group of its own

    TProcessOptions() : keepInput( false ), captureStdout( false ), newSession( false ) {}
};

class TProcess
{

public:

    TProcess();
    ~TProcess();                        // kills it if it still runs

    // the command line goes to the shell as it is.  False if it could not
    // be started (error() says why).
    bool start( const std::string& command, const TProcessOptions& options = TProcessOptions() );
    // the same with a directory and "NAME=value" variables
    bool start( const std::string& command, const std::string& dir,
                const std::vector<std::string>& env = std::vector<std::string>() );
    // argv[0] searched in PATH, no shell
    bool startArgv( const std::vector<std::string>& argv,
                    const TProcessOptions& options = TProcessOptions() );

    // what came out since the last call, without waiting (raw bytes):
    // stdout and stderr, or stderr alone with captureStdout
    std::string read();
    // stdout with captureStdout
    std::string readCaptured();
    // appends read() (and readCaptured()); returns running()
    bool poll( std::string& out );
    bool poll( std::string& out, std::string& captured );

    // more for stdin (keepInput); False once it is closed
    bool send( const std::string& data );
    void closeInput();

    // True while it runs or output may still come: after it has ended,
    // what it started in the background may keep the pipes open, for
    // drainMs at most
    bool running();
    // it has ended (its output may still be in the pipes)
    bool exited();
    // the exit code once it has ended: 128 + signal if it was killed
    // (143 after stop()), -1 while it runs
    int exitCode() const
        { return code; }
    bool stopped() const
        { return stopRequested; }
    const std::string& error() const
        { return failure; }

    // SIGTERM to its process group, SIGKILL killMs later if it is still
    // there (or at a second stop()); on Windows the job ends at once
    void stop();
    void terminate()
        { stop(); }

    // runs command on the terminal and waits for it (Turbo Vision has to
    // be suspended); Ctrl+C goes to the command.  The exit code, 127 if it
    // could not be started.
    static int runAttached( const std::string& command,
                            const TProcessOptions& options = TProcessOptions() );

    // output as UTF-8: on Linux as it is; on Windows the OEM code page
    // of cmd.exe is converted unless it already is UTF-8
    static std::string toUtf8( const std::string& raw );

    // a clock in milliseconds that only goes forward
    static long clockMs();

    static const long drainMs = 2000;
    static const long killMs = 3000;

private:

    bool launch( const std::vector<std::string>& argv, bool shell, const TProcessOptions& o );
    void check();

    std::string failure;
    int code;
    bool started, stopRequested, done;
    int stops;
    long exitTime, termTime;

#if defined( _WIN32 )
    void *process, *job, *outPipe, *capPipe, *inPipe;
#else
    int pid, outFd, capFd, inFd;
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
