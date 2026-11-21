/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   DATEEDIT.H                                                            */
/*                                                                         */
/*   defines TDate, TDateValidator, TDateInput and TDateButton             */
/*                                                                         */
/*   An input line for a date with a calendar that drops down:            */
/*                                                                         */
/*       TDateInput *d = new TDateInput( TRect( 15, 2, 27, 3 ) );          */
/*       dlg->insert( d );                                                 */
/*       dlg->insert( new TDateButton( TRect( 27, 2, 30, 3 ), d ) );       */
/*       ...                                                               */
/*       TDate when = d->date();                                           */
/*                                                                         */
/*   - the format is YYYY-MM-DD (dfISO), DD.MM.YYYY (dfDMY) or             */
/*     MM/DD/YYYY (dfMDY); typing takes only digits and the separator      */
/*     where they belong, and closing the dialog wants a real date;        */
/*   - F4 or Down in the line, or a click on the button, opens the         */
/*     calendar: the arrows move by a day or a week, PgUp/PgDn by a month, */
/*     Ctrl+PgUp/PgDn by a year, Home goes to today; Enter or a click on  */
/*     a day takes it, Esc closes; the week starts on Monday.              */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TDateInput (or Uses_TDate for    */
/*   TDate and TDateValidator alone).                                      */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TDate ) && !defined( __TDate )
#define __TDate

#include <string>

struct TDate
{
    int year, month, day;               // 1..12, 1..31; 0 0 0 = none

    TDate() : year( 0 ), month( 0 ), day( 0 ) {}
    TDate( int y, int m, int d ) : year( y ), month( m ), day( d ) {}

    Boolean isValid() const;
    // 0 = Monday ... 6 = Sunday
    int weekday() const;
    static int daysInMonth( int year, int month );
    static TDate today();

    // n days later (or earlier); months and years, keeping the day where
    // the month has it
    TDate addDays( int n ) const;
    TDate addMonths( int n ) const;

    bool operator == ( const TDate& d ) const
        { return year == d.year && month == d.month && day == d.day; }
    bool operator != ( const TDate& d ) const
        { return !( *this == d ); }
};

enum { dfISO, dfDMY, dfMDY };

// the text of a date in a format, and back (False if it is not a date)
std::string formatDate( const TDate& d, int format );
Boolean parseDate( const char *s, int format, TDate& d );

class TDateValidator : public TValidator
{

public:

    TDateValidator( int aFormat = dfISO );

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    int format;
};

#endif  // Uses_TDate


#if defined( Uses_TDateInput ) && !defined( __TDateInput )
#define __TDateInput

class TRect;
struct TEvent;

class TDateInput : public TValidatedInputLine
{

public:

    TDateInput( const TRect& bounds, int aFormat = dfISO );

    // the date typed; TDate() if it is not one
    TDate date();
    void setDate( const TDate& d );

    // opens the calendar under the line; True if a day was taken
    Boolean openCalendar();

    virtual void handleEvent( TEvent& event );

    int format;
};

// the arrow right of a TDateInput
class TDateButton : public TView
{

public:

    TDateButton( const TRect& bounds, TDateInput *aLink );

    virtual void draw();
    virtual TPalette& getPalette() const;
    virtual void handleEvent( TEvent& event );
    virtual void shutDown();

    TDateInput *link;
};

#endif  // Uses_TDateInput
