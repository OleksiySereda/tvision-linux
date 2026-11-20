/* Borland <iostream.h> compatibility shim for the Linux port */
#if !defined( __TV_IOSTREAM_H )
#define __TV_IOSTREAM_H
#include <iostream>
#include <iomanip>
using std::ios;
using std::istream;
using std::ostream;
using std::iostream;
using std::streambuf;
using std::streampos;
using std::streamoff;
using std::streamsize;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::ends;
using std::flush;
using std::dec;
using std::hex;
using std::setw;
using std::setfill;
using std::setiosflags;
using std::resetiosflags;
using std::setprecision;
#endif
