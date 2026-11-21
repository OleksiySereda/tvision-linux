/*------------------------------------------------------------*/
/* filename -       tdateedit.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TDate, TDateValidator, TDateInput,        */
/*                  TDateButton and the calendar              */
/*                                                            */
/*   Days are counted as in the proleptic Gregorian calendar  */
/*   (days_from_civil); the calendar is a window run in the   */
/*   owner of the line, as the list of THistory is.           */
/*------------------------------------------------------------*/

#define Uses_TDate
#define Uses_TDateInput
#define Uses_TValidatedInputLine
#define Uses_TWindow
#define Uses_TDrawBuffer
#define Uses_TPalette
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// ---- TDate

static long daysFromCivil( int y, int m, int d )
{
    y -= m <= 2;
    long era = ( y >= 0 ? y : y - 399 ) / 400;
    long yoe = y - era * 400;
    long doy = ( 153 * ( m + ( m > 2 ? -3 : 9 ) ) + 2 ) / 5 + d - 1;
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static TDate civilFromDays( long z )
{
    z += 719468;
    long era = ( z >= 0 ? z : z - 146096 ) / 146097;
    long doe = z - era * 146097;
    long yoe = ( doe - doe / 1460 + doe / 36524 - doe / 146096 ) / 365;
    long y = yoe + era * 400;
    long doy = doe - ( 365 * yoe + yoe / 4 - yoe / 100 );
    long mp = ( 5 * doy + 2 ) / 153;
    int d = int( doy - ( 153 * mp + 2 ) / 5 + 1 );
    int m = int( mp < 10 ? mp + 3 : mp - 9 );
    return TDate( int( y + ( m <= 2 ) ), m, d );
}

int TDate::daysInMonth( int year, int month )
{
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if( month < 1 || month > 12 )
        return 0;
    if( month == 2 && ( ( year % 4 == 0 && year % 100 != 0 ) || year % 400 == 0 ) )
        return 29;
    return days[month - 1];
}

Boolean TDate::isValid() const
{
    return Boolean( year >= 1 && year <= 9999 && month >= 1 && month <= 12 &&
                    day >= 1 && day <= daysInMonth( year, month ) );
}

int TDate::weekday() const
{
    // 1970-01-01 was a Thursday (3, counting from Monday)
    long z = daysFromCivil( year, month, day );
    return int( ( ( z % 7 ) + 7 + 3 ) % 7 );
}

TDate TDate::today()
{
    time_t t = time( 0 );
    struct tm *lt = localtime( &t );
    return lt != 0 ? TDate( lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday ) : TDate( 2000, 1, 1 );
}

TDate TDate::addDays( int n ) const
{
    return civilFromDays( daysFromCivil( year, month, day ) + n );
}

TDate TDate::addMonths( int n ) const
{
    int m = year * 12 + ( month - 1 ) + n;
    TDate r( m / 12, m % 12 + 1, day );
    int last = daysInMonth( r.year, r.month );
    if( r.day > last )
        r.day = last;
    return r;
}

// ---- the formats

static const char *pattern( int format )
{
    return format == dfDMY ? "99.99.9999" : format == dfMDY ? "99/99/9999" : "9999-99-99";
}

std::string formatDate( const TDate& d, int format )
{
    if( !d.isValid() )
        return std::string();
    char s[48];
    if( format == dfDMY )
        snprintf( s, sizeof( s ), "%02d.%02d.%04d", d.day, d.month, d.year );
    else if( format == dfMDY )
        snprintf( s, sizeof( s ), "%02d/%02d/%04d", d.month, d.day, d.year );
    else
        snprintf( s, sizeof( s ), "%04d-%02d-%02d", d.year, d.month, d.day );
    return s;
}

Boolean parseDate( const char *s, int format, TDate& d )
{
    const char *p = pattern( format );
    if( strlen( s ) != strlen( p ) )
        return False;
    for( size_t i = 0; p[i] != 0; i++ )
        if( p[i] == '9' ? !isdigit( uchar( s[i] ) ) : s[i] != p[i] )
            return False;
    TDate r;
    if( format == dfDMY )
        r = TDate( atoi( s + 6 ), atoi( s + 3 ), atoi( s ) );
    else if( format == dfMDY )
        r = TDate( atoi( s + 6 ), atoi( s ), atoi( s + 3 ) );
    else
        r = TDate( atoi( s ), atoi( s + 5 ), atoi( s + 8 ) );
    if( !r.isValid() )
        return False;
    d = r;
    return True;
}

// ---- TDateValidator

TDateValidator::TDateValidator( int aFormat ) :
    format( aFormat )
{
}

Boolean TDateValidator::isValidInput( const char *s )
{
    const char *p = pattern( format );
    size_t n = strlen( s );
    if( n > strlen( p ) )
        return False;
    for( size_t i = 0; i < n; i++ )
        if( p[i] == '9' ? !isdigit( uchar( s[i] ) ) : s[i] != p[i] )
            return False;
    return True;
}

Boolean TDateValidator::isValid( const char *s )
{
    TDate d;
    return parseDate( s, format, d );
}

std::string TDateValidator::errorText( const char * )
{
    std::string f = format == dfDMY ? "DD.MM.YYYY" : format == dfMDY ? "MM/DD/YYYY" : "YYYY-MM-DD";
    return "Not a valid date (" + f + ", e.g. " + formatDate( TDate::today(), format ) + ").";
}

// ---- the calendar

#define cpCalendarWindow "\x13\x13\x15\x18\x17\x13\x14"  // as THistoryWindow
#define cpCalendarView   "\x06\x06\x07\x06\x06"          // as THistoryViewer
#define cpDateButton     "\x16\x17"                      // as THistory

namespace
{

const char *const monthNames[] =
    { "January", "February", "March", "April", "May", "June", "July", "August",
      "September", "October", "November", "December" };

// the month of the day chosen: its name with arrows, the days of the
// week, then up to six weeks
class TCalendarView : public TView
{

public:

    TCalendarView( const TRect& r, const TDate& d ) : TView( r ), chosen( d )
    {
        options |= ofSelectable;
        eventMask |= evMouseWheel;
    }

    virtual TPalette& getPalette() const
    {
        static TPalette palette( cpCalendarView, sizeof( cpCalendarView ) - 1 );
        return palette;
    }

    // the screen cell of day: row 2 + week, column 3 * weekday
    TPoint cellOf( int day ) const
    {
        int first = TDate( chosen.year, chosen.month, 1 ).weekday();
        int k = first + day - 1;
        return TPoint{ short( 3 * ( k % 7 ) ), short( 2 + k / 7 ) };
    }

    virtual void draw()
    {
        ushort normal = getColor( 1 ), selected = getColor( 3 );
        uchar todayColor = uchar( ( normal & 0xF0 ) | 0x0E );
        TDrawBuffer b;
        char s[40];
        snprintf( s, sizeof( s ), "%s %d", monthNames[chosen.month - 1], chosen.year );
        b.moveChar( 0, ' ', normal, size.x );
        b.moveStr( ushort( ( size.x - int( strlen( s ) ) ) / 2 ), s, normal );
        b.moveChar( 0, '\x11', normal, 1 );
        b.moveChar( ushort( size.x - 1 ), '\x10', normal, 1 );
        writeLine( 0, 0, size.x, 1, b );
        b.moveChar( 0, ' ', normal, size.x );
        b.moveStr( 0, " Mo Tu We Th Fr Sa Su", normal );
        writeLine( 0, 1, size.x, 1, b );
        TDate today = TDate::today();
        int days = TDate::daysInMonth( chosen.year, chosen.month );
        for( int row = 2; row < size.y; row++ )
            {
            b.moveChar( 0, ' ', normal, size.x );
            for( int d = 1; d <= days; d++ )
                {
                TPoint c = cellOf( d );
                if( c.y != row )
                    continue;
                snprintf( s, sizeof( s ), "%2d", d );
                ushort color = normal;
                if( d == chosen.day )
                    color = selected;
                else if( TDate( chosen.year, chosen.month, d ) == today )
                    color = todayColor;
                b.moveChar( ushort( c.x ), ' ', color, 3 );
                b.moveStr( ushort( c.x + 1 ), s, color );
                }
            writeLine( 0, short( row ), size.x, 1, b );
            }
    }

    void move( const TDate& d )
    {
        if( d.isValid() )
            {
            chosen = d;
            drawView();
            }
    }

    virtual void handleEvent( TEvent& event )
    {
        TView::handleEvent( event );
        if( event.what == evKeyDown )
            {
            switch( ctrlToArrow( event.keyDown.keyCode ) )
                {
                case kbLeft:     move( chosen.addDays( -1 ) ); break;
                case kbRight:    move( chosen.addDays( 1 ) ); break;
                case kbUp:       move( chosen.addDays( -7 ) ); break;
                case kbDown:     move( chosen.addDays( 7 ) ); break;
                case kbPgUp:     move( chosen.addMonths( -1 ) ); break;
                case kbPgDn:     move( chosen.addMonths( 1 ) ); break;
                case kbCtrlPgUp: move( chosen.addMonths( -12 ) ); break;
                case kbCtrlPgDn: move( chosen.addMonths( 12 ) ); break;
                case kbHome:     move( TDate::today() ); break;
                case kbEnter:    endModal( cmOK ); break;
                case kbEsc:      endModal( cmCancel ); break;
                default:
                    return;
                }
            clearEvent( event );
            }
        else if( event.what == evMouseWheel )
            {
            move( chosen.addMonths( event.mouse.buttons == mwUp ? -1 : 1 ) );
            clearEvent( event );
            }
        else if( event.what == evMouseDown )
            {
            TPoint p = makeLocal( event.mouse.where );
            if( p.y == 0 && p.x <= 1 )
                move( chosen.addMonths( -1 ) );
            else if( p.y == 0 && p.x >= size.x - 2 )
                move( chosen.addMonths( 1 ) );
            else if( p.y >= 2 )
                for( int d = 1; d <= TDate::daysInMonth( chosen.year, chosen.month ); d++ )
                    {
                    TPoint c = cellOf( d );
                    if( c.y == p.y && p.x >= c.x && p.x < c.x + 3 )
                        {
                        move( TDate( chosen.year, chosen.month, d ) );
                        endModal( cmOK );
                        }
                    }
            clearEvent( event );
            }
        else if( event.what == evCommand && event.message.command == cmCancel )
            {
            endModal( cmCancel );
            clearEvent( event );
            }
    }

    TDate chosen;
};

class TCalendarWindow : public TWindow
{

public:

    TCalendarWindow( const TRect& r, const TDate& d ) :
        TWindowInit( &TCalendarWindow::initFrame ),
        TWindow( r, 0, wnNoNumber )
    {
        flags = wfClose;
        TRect v = getExtent();
        v.grow( -1, -1 );
        view = new TCalendarView( v, d );
        insert( view );
    }

    virtual TPalette& getPalette() const
    {
        static TPalette palette( cpCalendarWindow, sizeof( cpCalendarWindow ) - 1 );
        return palette;
    }

    TCalendarView *view;
};

}   // namespace

// ---- TDateInput

TDateInput::TDateInput( const TRect& bounds, int aFormat ) :
    TValidatedInputLine( bounds, 11, new TDateValidator( aFormat ) ),
    format( aFormat )
{
}

TDate TDateInput::date()
{
    TDate d;
    parseDate( data, format, d );
    return d;
}

void TDateInput::setDate( const TDate& d )
{
    std::string s = formatDate( d, format );
    strncpy( data, s.c_str(), size_t( maxLen ) );
    data[maxLen] = 0;
    selectAll( True );
    drawView();
}

Boolean TDateInput::openCalendar()
{
    if( owner == 0 )
        return False;
    select();
    TDate start = date();
    if( !start.isValid() )
        start = TDate::today();
    TRect r = getBounds();
    r.a.x--;
    r.b.x = r.a.x + 24;
    r.a.y = r.b.y - 1;
    r.b.y = r.a.y + 10;
    TRect ext = owner->getExtent();
    if( r.b.y > ext.b.y )
        r.move( 0, ext.b.y - r.b.y );
    if( r.b.x > ext.b.x )
        r.move( ext.b.x - r.b.x, 0 );
    if( r.a.x < 0 )
        r.move( -r.a.x, 0 );
    r.intersect( ext );
    TCalendarWindow *w = new TCalendarWindow( r, start );
    ushort c = owner->execView( w );
    TDate chosen = w->view->chosen;
    destroy( w );
    if( c != cmOK )
        return False;
    setDate( chosen );
    return True;
}

void TDateInput::handleEvent( TEvent& event )
{
    if( event.what == evKeyDown && ( state & sfSelected ) != 0 )
        {
        ushort code = event.keyDown.keyCode;
        if( code == kbF4 || ctrlToArrow( code ) == kbDown )
            {
            openCalendar();
            clearEvent( event );
            return;
            }
        // a digit where the separator goes: the separator comes first
        char c = event.keyDown.charScan.charCode;
        const char *p = pattern( format );
        size_t n = strlen( data );
        if( isdigit( uchar( c ) ) && curPos == int( n ) && selStart == selEnd &&
            n < strlen( p ) && p[n] != '9' )
            {
            data[n] = p[n];
            data[n + 1] = 0;
            curPos++;
            }
        }
    TValidatedInputLine::handleEvent( event );
}

// ---- TDateButton

TDateButton::TDateButton( const TRect& bounds, TDateInput *aLink ) :
    TView( bounds ),
    link( aLink )
{
}

void TDateButton::shutDown()
{
    link = 0;
    TView::shutDown();
}

TPalette& TDateButton::getPalette() const
{
    static TPalette palette( cpDateButton, sizeof( cpDateButton ) - 1 );
    return palette;
}

void TDateButton::draw()
{
    TDrawBuffer b;
    b.moveCStr( 0, "\xDE~\x19~\xDD", getColor( 0x0102 ) );
    writeLine( 0, 0, size.x, size.y, b );
}

void TDateButton::handleEvent( TEvent& event )
{
    TView::handleEvent( event );
    if( event.what == evMouseDown && link != 0 )
        {
        link->openCalendar();
        clearEvent( event );
        }
}
