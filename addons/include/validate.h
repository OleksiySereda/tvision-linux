/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   VALIDATE.H                                                            */
/*                                                                         */
/*   defines TValidatedInputLine and its validators                        */
/*                                                                         */
/*   Turbo Vision 1.0 has no validators (they came with 2.0).  A           */
/*   TValidatedInputLine is a TInputLine with a TValidator that checks     */
/*   the text twice:                                                       */
/*                                                                         */
/*     - while it is typed: a key that would make the text impossible      */
/*       (a letter in a number) is ignored;                                */
/*     - when the dialog is closed with any command but cmCancel (OK,      */
/*       Yes, ...): if the text is wrong, a message box says why, the      */
/*       line is selected and the dialog stays open.                       */
/*                                                                         */
/*       d->insert( new TValidatedInputLine( TRect( 3, 2, 12, 3 ), 6,     */
/*                      new TRangeValidator( 1, 65535 ) ) );               */
/*                                                                         */
/*   The validators:                                                       */
/*                                                                         */
/*     TFilterValidator   only the given characters                        */
/*     TRangeValidator    a whole number from min to max                   */
/*     TLookupValidator   one of a list ("yes|no|maybe")                   */
/*     TRegexValidator    the whole text matches a regular expression      */
/*                        (std::regex, ECMAScript)                         */
/*     TFuncValidator     a function of the program says yes or no         */
/*                                                                         */
/*   and for network settings and accounts:                                */
/*                                                                         */
/*     TIPAddressValidator  an IPv4 address: 192.168.0.1                   */
/*     TNetworkValidator    address/prefix: 10.0.0.0/8                     */
/*     TNetMaskValidator    a mask: 255.255.255.0, /24 or 24               */
/*     THostValidator       a host name (RFC 1123) or an IPv4 address      */
/*     TEmailValidator      name@domain.tld (a practical check, not all    */
/*                          of RFC 5322)                                   */
/*     TPasswordValidator   a minimum length and the kinds of characters   */
/*                          it must have                                   */
/*                                                                         */
/*   A TValidatedInputLine with password = True shows '*' for every        */
/*   character; its data are the real text.                                */
/*                                                                         */
/*   The data of the line stay text (getData / setData as TInputLine);     */
/*   TRangeValidator::value() turns them into a number.  The line owns     */
/*   its validator and deletes it.  It is written to a stream as a plain   */
/*   TInputLine, without the validator.                                    */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TValidatedInputLine, or          */
/*   Uses_TValidator for the validators alone.                             */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TValidator ) && !defined( __TValidator )
#define __TValidator

#include <string>

class TValidator
{

public:

    TValidator();
    virtual ~TValidator();

    // while typing: False if s cannot become valid by typing more
    virtual Boolean isValidInput( const char *s );
    // the final check
    virtual Boolean isValid( const char *s );
    // why s is not valid, for the message box
    virtual std::string errorText( const char *s );

    // isValid(), and if not, the message box with errorText()
    Boolean validate( const char *s );

    // True: an empty text is valid whatever the validator says
    Boolean allowEmpty;
};

class TFilterValidator : public TValidator
{

public:

    TFilterValidator( const char *validChars );

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    std::string chars;
};

class TRangeValidator : public TValidator
{

public:

    TRangeValidator( long aMin, long aMax );

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    // the number s holds (0 if none)
    static long value( const char *s );

    long minValue, maxValue;
};

class TLookupValidator : public TValidator
{

public:

    // items separated by separator: "red|green|blue"
    TLookupValidator( const char *items, char separator = '|',
                      Boolean aCaseSensitive = False );

    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    std::string items;
    char separator;
    Boolean caseSensitive;
};

class TRegexValidator : public TValidator
{

public:

    // message: the text of the message box; 0 gives a general one.
    // An expression that does not compile accepts nothing.
    TRegexValidator( const char *expression, const char *message = 0,
                     Boolean caseSensitive = True );
    ~TRegexValidator();

    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    std::string expression, message;

private:

    void *re;                           // a std::regex, or 0
};

class TFuncValidator : public TValidator
{

public:

    // check( s, data ) says whether s is valid
    TFuncValidator( Boolean (*aCheck)( const char *s, void *data ),
                    const char *aMessage, void *aData = 0 );

    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    Boolean (*check)( const char *, void * );
    std::string message;
    void *data;
};

// ---- network settings and accounts

class TIPAddressValidator : public TValidator
{

public:

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    // 4 numbers 0-255 without leading zeros; addr is in host order
    // (192.168.0.1 = 0xC0A80001)
    static Boolean parse( const char *s, unsigned long& addr );
    // the start of an address, as it is while it is typed
    static Boolean isPartial( const char *s );
};

class TNetworkValidator : public TValidator
{

public:

    // strict: the bits after the prefix must be 0 (10.0.0.0/8, not
    // 10.1.2.3/8)
    TNetworkValidator( Boolean aStrict = False );

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    static Boolean parse( const char *s, unsigned long& addr, int& prefix );

    Boolean strict;
};

class TNetMaskValidator : public TValidator
{

public:

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    // the prefix length of 255.255.255.0, /24 or 24; -1 if s is not a
    // mask (255.0.255.0 is not: the ones must come first)
    static int prefix( const char *s );
};

class THostValidator : public TValidator
{

public:

    THostValidator( Boolean anAllowAddress = True );

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );

    // labels of letters, digits and '-' (not first or last), 1 to 63
    // long, separated by dots, at most 253 in all; not only digits and
    // dots (that would be an address)
    static Boolean isHostName( const char *s, size_t n );

    Boolean allowAddress;       // True: an IPv4 address is valid too
};

class TEmailValidator : public TValidator
{

public:

    virtual Boolean isValidInput( const char *s );
    virtual Boolean isValid( const char *s );
    virtual std::string errorText( const char *s );
};

const ushort
    pwDigit  = 0x0001,          // a digit
    pwUpper  = 0x0002,          // an upper case letter
    pwLower  = 0x0004,          // a lower case letter
    pwSymbol = 0x0008;          // anything else: !, #, a blank, ...

class TPasswordValidator : public TValidator
{

public:

    TPasswordValidator( int aMinLength = 8,
                        ushort aRequire = pwDigit | pwUpper | pwLower );

    virtual Boolean isValid( const char *s );
    // says what is missing
    virtual std::string errorText( const char *s );

    // the pw... kinds s has
    static ushort kinds( const char *s );

    int minLength;
    ushort require;
};

#endif  // Uses_TValidator


#if defined( Uses_TValidatedInputLine ) && !defined( __TValidatedInputLine )
#define __TValidatedInputLine

class TRect;
struct TEvent;
class TValidator;

class TValidatedInputLine : public TInputLine
{

public:

    TValidatedInputLine( const TRect& bounds, int aMaxLen, TValidator *aValidator );
    ~TValidatedInputLine();

    virtual void handleEvent( TEvent& event );
    virtual Boolean valid( ushort command );
    // with password, '*' for every character
    virtual void draw();

    Boolean password;

    // deletes the validator it had; 0: no checks
    void setValidator( TValidator *aValidator );
    TValidator *validator;

};

#endif  // Uses_TValidatedInputLine
