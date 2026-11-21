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
#include <vector>

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

// ---- TIPAddressValidator

// the dotted address in s[0..n): with full, exactly 4 numbers; without,
// the start of one (an empty last group is fine: "192.168.")
static Boolean scanAddress( const char *s, size_t n, Boolean full, unsigned long& addr )
{
    int groups = 0;
    unsigned long a = 0;
    size_t i = 0;
    for( ;; )
        {
        size_t start = i;
        unsigned v = 0;
        while( i < n && isdigit( uchar( s[i] ) ) )
            {
            if( i - start == 3 )
                return False;
            v = v * 10 + unsigned( s[i] - '0' );
            i++;
            }
        size_t len = i - start;
        if( len == 0 )
            return Boolean( full == False && i == n );
        if( ( len > 1 && s[start] == '0' ) || v > 255 )
            return False;
        groups++;
        a = a * 256 + v;
        if( i == n )
            {
            addr = a;
            return Boolean( full == False || groups == 4 );
            }
        if( s[i] != '.' || groups == 4 )
            return False;
        i++;
        }
}

Boolean TIPAddressValidator::parse( const char *s, unsigned long& addr )
{
    return scanAddress( s, strlen( s ), True, addr );
}

Boolean TIPAddressValidator::isPartial( const char *s )
{
    unsigned long a;
    return scanAddress( s, strlen( s ), False, a );
}

Boolean TIPAddressValidator::isValidInput( const char *s )
{
    return isPartial( s );
}

Boolean TIPAddressValidator::isValid( const char *s )
{
    unsigned long a;
    return parse( s, a );
}

std::string TIPAddressValidator::errorText( const char * )
{
    return "Not a valid IPv4 address (four numbers from 0 to 255, e.g. 192.168.0.1).";
}

// ---- TNetworkValidator

// up to 2 digits, no leading zero, at most 32; empty only when partial
static Boolean scanPrefix( const char *s, Boolean full, int& prefix )
{
    size_t n = strlen( s );
    if( n == 0 )
        return Boolean( full == False );
    if( n > 2 || ( n == 2 && s[0] == '0' ) )
        return False;
    for( size_t i = 0; i < n; i++ )
        if( !isdigit( uchar( s[i] ) ) )
            return False;
    prefix = atoi( s );
    return Boolean( prefix <= 32 );
}

static unsigned long prefixMask( int prefix )
{
    return prefix == 0 ? 0 : ( 0xFFFFFFFFUL << ( 32 - prefix ) ) & 0xFFFFFFFFUL;
}

TNetworkValidator::TNetworkValidator( Boolean aStrict ) :
    strict( aStrict )
{
}

Boolean TNetworkValidator::parse( const char *s, unsigned long& addr, int& prefix )
{
    const char *slash = strchr( s, '/' );
    return Boolean( slash != 0 && scanAddress( s, size_t( slash - s ), True, addr ) &&
                    scanPrefix( slash + 1, True, prefix ) );
}

Boolean TNetworkValidator::isValidInput( const char *s )
{
    unsigned long a;
    int p;
    const char *slash = strchr( s, '/' );
    if( slash == 0 )
        return scanAddress( s, strlen( s ), False, a );
    return Boolean( scanAddress( s, size_t( slash - s ), False, a ) &&
                    scanPrefix( slash + 1, False, p ) );
}

Boolean TNetworkValidator::isValid( const char *s )
{
    unsigned long a;
    int p;
    if( !parse( s, a, p ) )
        return False;
    return Boolean( strict == False || ( a & ~prefixMask( p ) & 0xFFFFFFFFUL ) == 0 );
}

std::string TNetworkValidator::errorText( const char *s )
{
    unsigned long a;
    int p;
    if( strict == True && parse( s, a, p ) )
        return "The address has bits set after the prefix (e.g. 10.0.0.0/8, not 10.1.2.3/8).";
    return "Not a valid network: an IPv4 address, '/' and a prefix from 0 to 32 "
           "(e.g. 10.0.0.0/8).";
}

// ---- TNetMaskValidator

int TNetMaskValidator::prefix( const char *s )
{
    int p;
    if( *s == '/' )
        return scanPrefix( s + 1, True, p ) ? p : -1;
    if( strchr( s, '.' ) == 0 )
        return scanPrefix( s, True, p ) ? p : -1;
    unsigned long m;
    if( !scanAddress( s, strlen( s ), True, m ) )
        return -1;
    unsigned long inv = ~m & 0xFFFFFFFFUL;
    if( ( inv & ( inv + 1 ) ) != 0 )
        return -1;                      // the ones are not all in front
    int bits = 0;
    for( ; m != 0; m = ( m << 1 ) & 0xFFFFFFFFUL )
        bits++;
    return bits;
}

Boolean TNetMaskValidator::isValidInput( const char *s )
{
    int p;
    unsigned long a;
    if( *s == '/' )
        return scanPrefix( s + 1, False, p );
    // a dotted mask, or a prefix without '/' (which looks like its start)
    return scanAddress( s, strlen( s ), False, a );
}

Boolean TNetMaskValidator::isValid( const char *s )
{
    return Boolean( prefix( s ) >= 0 );
}

std::string TNetMaskValidator::errorText( const char * )
{
    return "Not a valid network mask: e.g. 255.255.255.0, /24 or 24 "
           "(the ones of a mask come first).";
}

// ---- THostValidator

THostValidator::THostValidator( Boolean anAllowAddress ) :
    allowAddress( anAllowAddress )
{
}

static Boolean isHostChar( char c )
{
    return Boolean( ( isalnum( uchar( c ) ) && uchar( c ) < 0x80 ) || c == '-' || c == '.' );
}

Boolean THostValidator::isHostName( const char *s, size_t n )
{
    if( n == 0 || n > 253 )
        return False;
    Boolean onlyDigits = True;
    size_t start = 0;
    for( size_t i = 0; i <= n; i++ )
        {
        if( i < n && s[i] != '.' )
            {
            if( !isHostChar( s[i] ) )
                return False;
            if( !isdigit( uchar( s[i] ) ) )
                onlyDigits = False;
            continue;
            }
        size_t len = i - start;
        if( len == 0 || len > 63 || s[start] == '-' || s[i - 1] == '-' )
            return False;
        start = i + 1;
        }
    return Boolean( onlyDigits == False );
}

Boolean THostValidator::isValidInput( const char *s )
{
    size_t n = strlen( s );
    if( n > 253 )
        return False;
    for( size_t i = 0; i < n; i++ )
        if( !isHostChar( s[i] ) )
            return False;
    return True;
}

Boolean THostValidator::isValid( const char *s )
{
    unsigned long a;
    if( allowAddress == True && TIPAddressValidator::parse( s, a ) )
        return True;
    return isHostName( s, strlen( s ) );
}

std::string THostValidator::errorText( const char * )
{
    if( allowAddress == True )
        return "Not a valid host name or IPv4 address (e.g. db1.example.com, 10.0.0.5).";
    return "Not a valid host name: letters, digits and '-' between dots "
           "(e.g. db1.example.com).";
}

// ---- TEmailValidator

static Boolean isLocalChar( char c )
{
    return Boolean( ( isalnum( uchar( c ) ) && uchar( c ) < 0x80 ) ||
                    ( c != 0 && strchr( "!#$%&'*+/=?^_`{|}~.-", c ) != 0 ) );
}

Boolean TEmailValidator::isValidInput( const char *s )
{
    size_t n = strlen( s );
    if( n > 254 )
        return False;
    int ats = 0;
    for( size_t i = 0; i < n; i++ )
        if( s[i] == '@' )
            ats++;
        else if( !isLocalChar( s[i] ) )
            return False;
    return Boolean( ats <= 1 );
}

Boolean TEmailValidator::isValid( const char *s )
{
    if( !isValidInput( s ) )
        return False;
    const char *at = strchr( s, '@' );
    if( at == 0 )
        return False;
    size_t local = size_t( at - s );
    if( local == 0 || local > 64 || s[0] == '.' || at[-1] == '.' )
        return False;
    for( size_t i = 1; i < local; i++ )
        if( s[i] == '.' && s[i - 1] == '.' )
            return False;
    const char *domain = at + 1;
    return Boolean( strchr( domain, '.' ) != 0 &&
                    THostValidator::isHostName( domain, strlen( domain ) ) );
}

std::string TEmailValidator::errorText( const char * )
{
    return "Not a valid e-mail address (e.g. name@example.com).";
}

// ---- TPasswordValidator

TPasswordValidator::TPasswordValidator( int aMinLength, ushort aRequire ) :
    minLength( aMinLength ),
    require( aRequire )
{
}

ushort TPasswordValidator::kinds( const char *s )
{
    ushort k = 0;
    for( ; *s != 0; s++ )
        {
        uchar c = uchar( *s );
        if( c < 0x80 && isdigit( c ) )
            k |= pwDigit;
        else if( c < 0x80 && isupper( c ) )
            k |= pwUpper;
        else if( c < 0x80 && islower( c ) )
            k |= pwLower;
        else
            k |= pwSymbol;
        }
    return k;
}

Boolean TPasswordValidator::isValid( const char *s )
{
    return Boolean( int( strlen( s ) ) >= minLength && ( kinds( s ) & require ) == require );
}

std::string TPasswordValidator::errorText( const char *s )
{
    std::vector<std::string> missing;
    if( int( strlen( s ) ) < minLength )
        missing.push_back( "at least " + std::to_string( minLength ) + " characters" );
    ushort lacks = ushort( require & ~kinds( s ) );
    if( lacks & pwDigit )
        missing.push_back( "a digit" );
    if( lacks & pwUpper )
        missing.push_back( "an upper case letter" );
    if( lacks & pwLower )
        missing.push_back( "a lower case letter" );
    if( lacks & pwSymbol )
        missing.push_back( "a symbol such as ! or #" );
    std::string t = "The password needs ";
    for( size_t i = 0; i < missing.size(); i++ )
        {
        if( i > 0 )
            t += i + 1 == missing.size() ? " and " : ", ";
        t += missing[i];
        }
    return t + ".";
}

// ---- TValidatedInputLine

TValidatedInputLine::TValidatedInputLine( const TRect& bounds, int aMaxLen,
                                          TValidator *aValidator ) :
    TInputLine( bounds, aMaxLen ),
    validator( aValidator ),
    password( False )
{
}

// TInputLine::draw() with the text replaced by stars for the moment
void TValidatedInputLine::draw()
{
    if( password == False )
        {
        TInputLine::draw();
        return;
        }
    std::string real( data );
    memset( data, '*', real.size() );
    TInputLine::draw();
    memcpy( data, real.data(), real.size() );
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
