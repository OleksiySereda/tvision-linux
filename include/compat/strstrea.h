/* Borland <strstrea.h> compatibility shim for the Linux port */
#if !defined( __TV_STRSTREA_H )
#define __TV_STRSTREA_H

#include <iostream.h>
#include <sstream>

using std::ostringstream;
using std::istringstream;

// ostrstream writing into a caller supplied fixed-size buffer, as the
// demos use it:  ostrstream os( buf, sizeof buf ); os << ... << ends;
class ostrstream : public std::ostream
{

    class FixedBuf : public std::streambuf
    {
    public:
        FixedBuf( char *s, int n ) { setp( s, s + ( n > 0 ? n : 0 ) ); }
        char *base() const { return pbase(); }
    };

    FixedBuf buf;

public:

    ostrstream( char *s, int n, int = ios::out ) :
        std::ostream( 0 ), buf( s, n )
    {
        rdbuf( &buf );
    }

    char *str() { return buf.base(); }

};

#endif
