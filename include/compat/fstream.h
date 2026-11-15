/* Borland <fstream.h> compatibility shim for the Linux port */
#if !defined( __TV_FSTREAM_H )
#define __TV_FSTREAM_H
#include <iostream.h>
#include <fstream>
using std::filebuf;
using std::ifstream;
using std::ofstream;
using std::fstream;
#endif
