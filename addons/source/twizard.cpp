/*------------------------------------------------------------*/
/* filename -       twizard.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  TWizard member functions                  */
/*                                                            */
/*   The pages are TPages inside the dialog, one shown at a   */
/*   time; Next and Finish are two buttons of which one is    */
/*   shown, and it is the default one.                        */
/*------------------------------------------------------------*/

#define Uses_TWizard
#define Uses_TPage
#define Uses_TDialog
#define Uses_TButton
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <stdio.h>
#include <string.h>

// "Step 2 of 4: Options" and a line under it
class TWizardHeader : public TView
{

public:

    TWizardHeader( const TRect& r, TWizard *aWizard ) : TView( r ), wizard( aWizard )
    {
        growMode = gfGrowHiX;
    }

    virtual void draw()
    {
        TDrawBuffer b;
        ushort text = getColor( 0x0908 ), line = getColor( 0x0707 );
        b.moveChar( 0, ' ', text, size.x );
        if( wizard->currentPage() >= 0 )
            {
            std::string t = wizard->pageTitle( wizard->currentPage() );
            std::string plain;
            for( char c : t )
                if( c != '~' )
                    plain += c;
            char s[200];
            snprintf( s, sizeof( s ), "Step %d of %d: %s", wizard->currentPage() + 1,
                      wizard->pageCount(), plain.c_str() );
            b.moveStr( 0, s, text );
            }
        writeLine( 0, 0, size.x, 1, b );
        b.moveChar( 0, '\xC4', line, size.x );
        writeLine( 0, 1, size.x, size.y - 1, b );
    }

    TWizard *wizard;
};

TWizard::TWizard( const TRect& bounds, const char *aTitle ) :
    TWindowInit( &TWizard::initFrame ),
    TDialog( bounds, aTitle ),
    shown( -1 )
{
    options |= ofCentered;
    header = new TWizardHeader( TRect( 2, 1, size.x - 2, 3 ), this );
    insert( header );
    int y = size.y - 3;
    backButton = new TButton( TRect( size.x - 37, y, size.x - 26, y + 2 ),
                              "< ~B~ack", cmWizardBack, bfNormal );
    nextButton = new TButton( TRect( size.x - 25, y, size.x - 14, y + 2 ),
                              "~N~ext >", cmWizardNext, bfDefault );
    finishButton = new TButton( TRect( size.x - 25, y, size.x - 14, y + 2 ),
                                "~F~inish", cmWizardFinish, bfNormal );
    insert( backButton );
    insert( nextButton );
    insert( finishButton );
    insert( new TButton( TRect( size.x - 13, y, size.x - 2, y + 2 ),
                         "Cancel", cmCancel, bfNormal ) );
    backButton->hide();
    finishButton->hide();
}

TPage *TWizard::addPage( const char *title )
{
    TPage *p = new TPage( TRect( 1, 3, size.x - 1, size.y - 3 ) );
    if( !pages.empty() )
        p->hide();
    insert( p );
    pages.push_back( p );
    titles.push_back( title != 0 ? title : "" );
    started.push_back( false );
    if( shown < 0 )
        {
        shown = 0;
        p->select();
        }
    updateButtons();
    header->drawView();
    return p;
}

const char *TWizard::pageTitle( int i ) const
{
    return i >= 0 && i < pageCount() ? titles[size_t( i )].c_str() : "";
}

void TWizard::updateButtons()
{
    if( shown > 0 )
        backButton->show();
    else
        backButton->hide();
    Boolean last = Boolean( shown == pageCount() - 1 );
    if( last == True )
        {
        nextButton->hide();
        finishButton->show();
        nextButton->makeDefault( False );
        finishButton->makeDefault( True );
        }
    else
        {
        finishButton->hide();
        nextButton->show();
        finishButton->makeDefault( False );
        nextButton->makeDefault( True );
        }
}

void TWizard::showPage( int i )
{
    if( i < 0 || i >= pageCount() )
        return;
    if( i != shown )
        {
        if( shown >= 0 )
            pages[size_t( shown )]->hide();
        shown = i;
        pages[size_t( i )]->show();
        }
    pages[size_t( i )]->select();
    if( !started[size_t( i )] )
        {
        started[size_t( i )] = true;
        pages[size_t( i )]->focusFirst();
        }
    updateButtons();
    header->drawView();
    pageShown( i );
}

Boolean TWizard::canLeave( int i, Boolean forwards )
{
    if( forwards == False || i < 0 || i >= pageCount() )
        return True;
    return pages[size_t( i )]->validPage( cmOK );
}

void TWizard::pageShown( int )
{
}

Boolean TWizard::next()
{
    if( shown < 0 || shown >= pageCount() - 1 || !canLeave( shown, True ) )
        return False;
    showPage( shown + 1 );
    return True;
}

void TWizard::back()
{
    if( shown > 0 && canLeave( shown, False ) )
        showPage( shown - 1 );
}

ushort TWizard::execute()
{
    if( shown >= 0 && !started[size_t( shown )] )
        showPage( shown );
    return TDialog::execute();
}

void TWizard::handleEvent( TEvent& event )
{
    if( event.what == evKeyDown && shown >= 0 &&
        ( event.keyDown.keyCode == kbTab || event.keyDown.keyCode == kbShiftTab ) &&
        current == pages[size_t( shown )] )
        {
        if( pages[size_t( shown )]->focusNext( Boolean( event.keyDown.keyCode == kbTab ) ) )
            {
            clearEvent( event );
            return;
            }
        }
    TDialog::handleEvent( event );
    if( event.what == evCommand )
        switch( event.message.command )
            {
            case cmWizardNext:
                next();
                clearEvent( event );
                break;
            case cmWizardBack:
                back();
                clearEvent( event );
                break;
            case cmWizardFinish:
                if( canLeave( shown, True ) )
                    endModal( cmOK );
                clearEvent( event );
                break;
            }
}
