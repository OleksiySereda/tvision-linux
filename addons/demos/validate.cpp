/*------------------------------------------------------------*/
/*   VALIDATE: a dialog with an input line for each           */
/*   validator of the add-ons.  F2 opens it again, Alt-X      */
/*   quits.  OK checks every line; what was entered is shown  */
/*   afterwards.                                              */
/*------------------------------------------------------------*/

#define Uses_TApplication
#define Uses_TDeskTop
#define Uses_TDialog
#define Uses_TButton
#define Uses_TLabel
#define Uses_TRect
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_MsgBox
#define Uses_TValidatedInputLine
#include <tvaddons.h>

#include <stdio.h>
#include <string.h>
#include <string>

const ushort cmShowDialog = 100;

struct Field
{
    const char *label;
    int length;
    TValidator *(*make)();
    Boolean password;
};

static const Field fields[] =
{
    { "~A~ddress",  15, [] { return (TValidator *) new TIPAddressValidator; }, False },
    { "~N~etwork",  18, [] { return (TValidator *) new TNetworkValidator( True ); }, False },
    { "~M~ask",     15, [] { return (TValidator *) new TNetMaskValidator; }, False },
    { "~H~ost",     40, [] { return (TValidator *) new THostValidator; }, False },
    { "~E~-mail",   40, [] { return (TValidator *) new TEmailValidator; }, False },
    { "~P~assword", 30, [] { return (TValidator *) new TPasswordValidator; }, True },
    { "P~o~rt",      5, [] { return (TValidator *) new TRangeValidator( 1, 65535 ); }, False },
};
const int fieldCount = int( sizeof( fields ) / sizeof( fields[0] ) );

// the data of the dialog: one char[length + 1] after the other (the
// second argument of TInputLine is the size with the 0)
static char data[512];

class TValidateApp : public TApplication
{

public:

    TValidateApp() :
        TProgInit( &TValidateApp::initStatusLine, &TValidateApp::initMenuBar,
                   &TValidateApp::initDeskTop )
    {
        TEvent e;
        e.what = evCommand;
        e.message.command = cmShowDialog;
        putEvent( e );
    }

    static TStatusLine *initStatusLine( TRect r )
    {
        r.a.y = r.b.y - 1;
        return new TStatusLine( r,
            *new TStatusDef( 0, 0xFFFF ) +
                *new TStatusItem( "~F2~ Dialog", kbF2, cmShowDialog ) +
                *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit ) );
    }

    virtual void handleEvent( TEvent& event )
    {
        TApplication::handleEvent( event );
        if( event.what == evCommand && event.message.command == cmShowDialog )
            {
            showDialog();
            clearEvent( event );
            }
    }

    void showDialog()
    {
        TDialog *d = new TDialog( TRect( 0, 0, 62, fieldCount * 2 + 6 ), "Validators" );
        d->options |= ofCentered;
        for( int i = 0; i < fieldCount; i++ )
            {
            int y = 2 + i * 2;
            int w = fields[i].length < 40 ? fields[i].length + 3 : 43;   // + the cursor
            TValidatedInputLine *line = new TValidatedInputLine(
                TRect( 15, y, 15 + w, y + 1 ), fields[i].length + 1, fields[i].make() );
            line->password = fields[i].password;
            d->insert( line );
            d->insert( new TLabel( TRect( 2, y, 14, y + 1 ), fields[i].label, line ) );
            }
        int by = fieldCount * 2 + 3;
        d->insert( new TButton( TRect( 36, by, 46, by + 2 ), "O~K~", cmOK, bfDefault ) );
        d->insert( new TButton( TRect( 48, by, 58, by + 2 ), "Cancel", cmCancel, bfNormal ) );
        d->selectNext( False );

        d->setData( data );
        ushort result = deskTop->execView( d );
        if( result != cmCancel )
            d->getData( data );
        destroy( d );

        if( result != cmCancel )
            {
            std::string t = "Accepted:\n";
            const char *p = data;
            for( int i = 0; i < fieldCount; i++ )
                {
                if( i != 0 )
                    t += ", ";
                t += fields[i].password ? std::string( strlen( p ), '*' ) : std::string( p );
                p += fields[i].length + 1;
                }
            messageBox( t.c_str(), mfInformation | mfOKButton );
            }
    }
};

int main()
{
    memset( data, 0, sizeof( data ) );
    TValidateApp app;
    app.run();
    return 0;
}
