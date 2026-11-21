/*------------------------------------------------------------*/
/* filename -       tvalid.cpp                                */
/*                                                            */
/* function(s)                                                */
/*                  TValidator and the validators,            */
/*                  TValidatedInputLine member functions      */
/*------------------------------------------------------------*/

#define Uses_TValidatedInputLine
#define Uses_TValidator
#define Uses_TEvent
#define Uses_MsgBox
#include <tvaddons.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <regex>
#include <string>

// ---- TValidator

TValidator::TValidator() :
    allowEmpty( False )
{
}

TValidator::~TValidator()
{
}

Boolean TValidator::isValidInput( const char * )
{
    return True;
}

Boolean TValidator::isValid( const char * )
{
    return True;
}

std::string TValidator::errorText( const char * )
{
    return "The value is not valid.";
}

Boolean TValidator::validate( const char *s )
{
    if( allowEmpty == True && *s == 0 )
        return True;
    if( isValid( s ) )
        return True;
    messageBox( errorText( s ).c_str(), mfError | mfOKButton );
    return False;
}

// ---- TFilterValidator

TFilterValidator::TFilterValidator( const char *validChars ) :
    chars( validChars )
{
}

Boolean TFilterValidator::isValidInput( const char *s )
{
    for( ; *s != 0; s++ )
        if( chars.find( *s ) == std::string::npos )
            return False;
    return True;
}

Boolean TFilterValidator::isValid( const char *s )
{
    return isValidInput( s );
}

std::string TFilterValidator::errorText( const char * )
{
    return "Only these characters are allowed: " + chars;
}

// ---- TRangeValidator

TRangeValidator::TRangeValidator( long aMin, long aMax ) :
    minValue( aMin ),
    maxValue( aMax )
{
}

long TRangeValidator::value( const char *s )
{
    return strtol( s, 0, 10 );
}

// a sign (only when negative numbers are allowed), then digits; more
// digits only take the number away from 0, so one beyond the range
// stays beyond it
Boolean TRangeValidator::isValidInput( const char *s )
{
    const char *p = s;
    if( *p == '-' )
        {
        if( minValue >= 0 )
            return False;
        p++;
        }
    for( const char *q = p; *q != 0; q++ )
        if( !isdigit( uchar( *q ) ) )
            return False;
    if( *p == 0 )
        return True;
    errno = 0;
    long v = strtol( s, 0, 10 );
    if( errno == ERANGE )
        return False;
    if( v > 0 && v > maxValue )
        return False;
    if( v < 0 && v < minValue )
        return False;
    return True;
}

Boolean TRangeValidator::isValid( const char *s )
{
    while( *s == ' ' )
        s++;
    char *end;
    errno = 0;
    long v = strtol( s, &end, 10 );
    if( end == s || errno == ERANGE )
        return False;
    while( *end == ' ' )
        end++;
    return Boolean( *end == 0 && v >= minValue && v <= maxValue );
}

std::string TRangeValidator::errorText( const char * )
{
    return "The value must be a number from " + std::to_string( minValue ) +
           " to " + std::to_string( maxValue ) + ".";
}

// ---- TLookupValidator

TLookupValidator::TLookupValidator( const char *anItems, char aSeparator,
                                    Boolean aCaseSensitive ) :
    items( anItems ),
    separator( aSeparator ),
    caseSensitive( aCaseSensitive )
{
}

static Boolean sameText( const std::string& a, const char *b, Boolean caseSensitive )
{
    if( a.size() != strlen( b ) )
        return False;
    for( size_t i = 0; i < a.size(); i++ )
        {
        uchar x = uchar( a[i] ), y = uchar( b[i] );
        if( caseSensitive == False )
            {
            x = uchar( tolower( x ) );
            y = uchar( tolower( y ) );
            }
        if( x != y )
            return False;
        }
    return True;
}

Boolean TLookupValidator::isValid( const char *s )
{
    size_t start = 0;
    for( ;; )
        {
        size_t end = items.find( separator, start );
        std::string item = items.substr( start, end == std::string::npos ? end : end - start );
        if( sameText( item, s, caseSensitive ) )
            return True;
        if( end == std::string::npos )
            return False;
        start = end + 1;
        }
}

std::string TLookupValidator::errorText( const char * )
{
    std::string t;
    for( char c : items )
        if( c == separator )
            t += ", ";
        else
            t += c;
    return "The value must be one of: " + t;
}

// ---- TRegexValidator

TRegexValidator::TRegexValidator( const char *anExpression, const char *aMessage,
                                  Boolean caseSensitive ) :
    expression( anExpression ),
    message( aMessage != 0 ? aMessage : "" ),
    re( 0 )
{
    try
        {
        std::regex::flag_type f = std::regex::ECMAScript;
        if( caseSensitive == False )
            f |= std::regex::icase;
        re = new std::regex( expression, f );
        }
    catch( const std::regex_error& )
        {
        re = 0;
        }
}

TRegexValidator::~TRegexValidator()
{
    delete (std::regex *) re;
}

Boolean TRegexValidator::isValid( const char *s )
{
    if( re == 0 )
        return False;
    return Boolean( std::regex_match( s, *(std::regex *) re ) );
}

std::string TRegexValidator::errorText( const char * )
{
    if( !message.empty() )
        return message;
    return "The value does not have the expected form.";
}

// ---- TFuncValidator

TFuncValidator::TFuncValidator( Boolean (*aCheck)( const char *, void * ),
                                const char *aMessage, void *aData ) :
    check( aCheck ),
    message( aMessage != 0 ? aMessage : "The value is not valid." ),
    data( aData )
{
}

Boolean TFuncValidator::isValid( const char *s )
{
    return check != 0 ? check( s, data ) : True;
}

std::string TFuncValidator::errorText( const char * )
{
    return message;
}

// ---- TValidatedInputLine

TValidatedInputLine::TValidatedInputLine( const TRect& bounds, int aMaxLen,
                                          TValidator *aValidator ) :
    TInputLine( bounds, aMaxLen ),
    validator( aValidator )
{
}

TValidatedInputLine::~TValidatedInputLine()
{
    delete validator;
}

void TValidatedInputLine::setValidator( TValidator *aValidator )
{
    if( aValidator != validator )
        delete validator;
    validator = aValidator;
}

// a key that makes the text impossible is taken back
void TValidatedInputLine::handleEvent( TEvent& event )
{
    if( event.what != evKeyDown || validator == 0 )
        {
        TInputLine::handleEvent( event );
        return;
        }
    std::string old( data );
    int oldCur = curPos, oldFirst = firstPos, oldStart = selStart, oldEnd = selEnd;
    TInputLine::handleEvent( event );
    if( old != data && !validator->isValidInput( data ) )
        {
        strcpy( data, old.c_str() );
        curPos = oldCur;
        firstPos = oldFirst;
        selStart = oldStart;
        selEnd = oldEnd;
        drawView();
        }
}

// every command that closes the dialog but Cancel checks the text;
// moving to another control does not
Boolean TValidatedInputLine::valid( ushort command )
{
    if( command == cmValid || command == cmCancel || command == cmReleasedFocus ||
        validator == 0 || ( state & sfDisabled ) != 0 )
        return TInputLine::valid( command );
    if( validator->validate( data ) )
        return True;
    select();
    return False;
}
