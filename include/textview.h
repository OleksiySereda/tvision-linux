/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TEXTVIEW.H                                                            */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   defines the classes TTextDevice and TTerminal                         */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( __BCOPT__ )

#endif

#if defined( Uses_TTextDevice ) && !defined( __TTextDevice )
#define __TTextDevice

#include <iostream.h>

#if defined( __BCOPT__ )

#endif

class TRect;
class TScrollBar;

class TTextDevice : public TScroller, public streambuf
{

public:

    TTextDevice( const TRect& bounds,
                 TScrollBar *aHScrollBar,
                 TScrollBar *aVScrollBar
               );

    virtual int do_sputn( const char *s, int count ) = 0;
    virtual int overflow( int = EOF );

protected:

    // std::streambuf bulk output goes through do_sputn()
    virtual std::streamsize xsputn( const char *s, std::streamsize n )
        { return do_sputn( s, int( n ) ); }

};

#endif  // Uses_TTextDevice

#if defined( Uses_TTerminal ) && !defined( __TTerminal )
#define __TTerminal

class TRect;
class TScrollBar;

class TTerminal: public TTextDevice
{

public:

    friend void genRefs();

    TTerminal( const TRect& bounds,
               TScrollBar *aHScrollBar,
               TScrollBar *aVScrollBar,
               ushort aBufSize
             );
    ~TTerminal();

    virtual int do_sputn( const char *s, int count );

    void bufInc( ushort& val );
    Boolean canInsert( ushort amount );
    short calcWidth();
    virtual void draw();
    ushort nextLine( ushort pos );
    ushort prevLines( ushort pos, ushort lines );
    Boolean queEmpty();

protected:

    ushort bufSize;
    char *buffer;
    ushort queFront, queBack;
    void bufDec(ushort& val);
};

#endif  // Uses_TTerminal

#if defined( Uses_otstream ) && !defined( __otstream )
#define __otstream

#include <iostream.h>

#if defined( __BCOPT__ )

#endif


class otstream : public ostream
{

public:

    otstream( TTerminal * );

};


#endif

#if defined( __BCOPT__ )

#endif
