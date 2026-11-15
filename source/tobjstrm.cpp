/*------------------------------------------------------------*/
/* filename -       tobjstrm.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  Member function(s) of following classes:  */
/*                     TParamText                             */
/*                     TStreamable                            */
/*                     TStreamableClass                       */
/*                     TStreamableTypes                       */
/*                     TPWrittenObjects                       */
/*                     TPReadObjects                          */
/*                     pstream                                */
/*                     ipstream                               */
/*                     opstream                               */
/*                     iopstream                              */
/*                     fpbase                                 */
/*                     ifpstream                              */
/*                     ofpstream                              */
/*                     fpstream                               */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TStreamable
#define Uses_TStreamableClass
#define Uses_TStreamableTypes
#define Uses_TPWrittenObjects
#define Uses_TPReadObjects
#define Uses_pstream
#define Uses_ipstream
#define Uses_opstream
#define Uses_iopstream
#define Uses_fpbase
#define Uses_ifpstream
#define Uses_ofpstream
#define Uses_fpstream
#include <tv.h>

#if !defined( __LIMITS_H )
#include <limits.h>
#endif  // __LIMITS_H

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

#if !defined( __FSTREAM_H )
#include <fstream.h>
#endif  // __FSTREAM_H

#if !defined( __IO_H )
#include <io.h>
#endif  // __IO_H

#if !defined( __STAT_H )
#include <sys/stat.h>
#endif  // __STAT_H

#if !defined( __FCNTL_H )
#include <fcntl.h>
#endif  // __FCNTL_H

#if !defined( __STDLIB_H )
#include <stdlib.h>
#endif  // __STDLIB_H

#if !defined( __ASSERT_H )
#include <assert.h>
#endif  // __ASSERT_H

const uchar nullStringLen = UCHAR_MAX;

TStreamableClass::TStreamableClass( const char *n, BUILDER b, int d ) :
    name( n ),
    build( b ),
    delta( d )
{
    pstream::initTypes();
    pstream::registerType( this );
}

TStreamableTypes::TStreamableTypes() : TNSSortedCollection( 5, 5 )
{
}

void *TStreamableTypes::operator new( size_t, void * arena )
{
    return arena;
}

TStreamableTypes::~TStreamableTypes()
{
}

void TStreamableTypes::registerType( const TStreamableClass *d )
{
    insert( (void *)d );
}

const TStreamableClass *TStreamableTypes::lookup( const char *name )
{
    ccIndex loc;
    if( search( (void *)name, loc ) )
        return (TStreamableClass *)at( loc );
    else
        return 0;
}

void *TStreamableTypes::keyOf( void *d )
{
    return (void *)((TStreamableClass *)d)->name;
}

int TStreamableTypes::compare( void *d1, void *d2 )
{
    return strcmp( (char *)d1, (char *)d2 );
}

TPWrittenObjects::TPWrittenObjects() : TNSSortedCollection( 5, 5 ), curId( 0 )
{
}

TPWrittenObjects::~TPWrittenObjects()
{
}

void TPWrittenObjects::registerObject( const void *adr )
{
    TPWObj *o = new TPWObj( adr, curId++ );
    insert( o );
}

P_id_type TPWrittenObjects::find( const void *d )
{
    ccIndex loc;
    if( search( (void *)d, loc ) )
        return ((TPWObj *)at( loc ))->ident;
    else
        return P_id_notFound;
}

void *TPWrittenObjects::keyOf( void *d )
{
    return (void *)((TPWObj *)d)->address;
}

int TPWrittenObjects::compare( void *o1, void *o2 )
{
    if( o1 == o2 )
        return 0;
    else if( uintptr_t( o1 ) < uintptr_t( o2 ) )
        return -1;
    else
        return 1;
}

TPWObj::TPWObj( const void *adr, P_id_type id ) : address( adr ), ident( id )
{
}

TPReadObjects::TPReadObjects() : TNSCollection( 5, 5 ), curId( 0 )
{
}

TPReadObjects::~TPReadObjects()
{
}


void TPReadObjects::registerObject( const void *adr )
{
    ccIndex loc = insert( (void *)adr );
    assert( loc == curId++ );   // to be sure that TNSCollection
                                // continues to work the way
                                // it does now...
}


const void *TPReadObjects::find( P_id_type id )
{
    return at( id );
}

pstream::pstream( streambuf *sb )
{
    init( sb );
}

pstream::~pstream()
{
}

void pstream::initTypes()
{
    if( types == 0 )
        types = new TStreamableTypes;
}

// the old iostream library had a "hard failure" state bit
static const int hardfail = 0x80;

int pstream::rdstate() const
{
    return state;
}

int pstream::eof() const
{
    return state & ios::eofbit;
}

int pstream::fail() const
{
    return state & (ios::failbit | ios::badbit | hardfail);
}

int pstream::bad() const
{
    return state & (ios::badbit | hardfail);
}

int pstream::good() const
{
    return state == 0;
}

void pstream::clear( int i )
{
    state = (i & 0xFF) | (state & hardfail);
}

void pstream::registerType( TStreamableClass *ts )
{ 
    types->registerType( ts ); 
}

pstream::operator void *() const
{
    return fail() ? 0 : (void *)this;
}

int pstream::operator! () const
{
    return fail();
}

streambuf * pstream::rdbuf() const
{
    return bp;
}

pstream::pstream()
{
}

void pstream::error( StreamableError )
{
    abort();
}

void pstream::error( StreamableError, const TStreamable& )
{
    abort();
}

void pstream::init( streambuf *sbp )
{
    state = 0;
    bp = sbp;
}

void pstream::setstate( int b )
{
    state |= (b&0xFF);
}

ipstream::ipstream( streambuf *sb )
{
    pstream::init( sb );
}

ipstream::~ipstream()
{
    objs.shouldDelete = False;
    objs.shutDown();
}

streampos ipstream::tellg()
{
    return bp->pubseekoff( 0, ios::cur, ios::in );
}

ipstream& ipstream::seekg( streampos pos )
{
    objs.removeAll();
    bp->pubseekoff( pos, ios::beg );
    return *this;
}

ipstream& ipstream::seekg( streamoff off, ios::seekdir dir )
{
    objs.removeAll();
    bp->pubseekoff( off, dir );
    return *this;
}

uchar ipstream::readByte()
{
    return bp->sbumpc();
}

ushort ipstream::readWord()
{
    ushort temp;
    bp->sgetn( (char *)&temp, sizeof( ushort ) );
    return temp;
}

void ipstream::readBytes( void *data, size_t sz )
{
    bp->sgetn( (char *)data, sz );
}

char *ipstream::readString()
{
    uchar len = readByte();
    if( len == nullStringLen )
        return 0;
    char *buf = new char[len+1];
    if( buf == 0 )
        return 0;
    readBytes( buf, len );
    buf[len] = EOS;
    return buf;
}

char *ipstream::readString( char *buf, unsigned maxLen )
{
    assert( buf != 0 );

    uchar len = readByte();
    if( len > maxLen-1 )
        return 0;
    readBytes( buf, len );
    buf[len] = EOS;
    return buf;
}

ipstream& operator >> ( ipstream& ps, signed char &ch )
{
    ch = ps.readByte();
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned char &ch )
{
    ch = ps.readByte();
    return ps;
}

ipstream& operator >> ( ipstream& ps, signed short &sh )
{
    sh = ps.readWord();
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned short &sh )
{
    sh = ps.readWord();
    return ps;
}

ipstream& operator >> ( ipstream& ps, signed int &i )
{
    i = short( ps.readWord() );
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned int &i )
{
    i = ps.readWord();
    return ps;
}

// Linux port: the stream format stays the DOS one, so long is stored in
// 32 bits (and int in 16, see readWord)
ipstream& operator >> ( ipstream& ps, signed long &l )
{
    int32_t t;
    ps.readBytes( &t, sizeof(t) );
    l = t;
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned long &l )
{
    uint32_t t;
    ps.readBytes( &t, sizeof(t) );
    l = t;
    return ps;
}

ipstream& operator >> ( ipstream& ps, char &ch )
{
    ch = ps.readByte();
    return ps;
}

ipstream& operator >> ( ipstream& ps, float &f )
{
    ps.readBytes( &f, sizeof(f) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, double &d )
{
    ps.readBytes( &d, sizeof(d) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, TStreamable& t )
{
    const TStreamableClass *pc = ps.readPrefix();
    ps.readData( pc, &t );
    ps.readSuffix();
    return ps;
}

ipstream& operator >> ( ipstream& ps, void *&t )
{
    char ch = ps.readByte();
    switch( ch )
        {
        case pstream::ptNull:
            t = 0;
            break;
        case pstream::ptIndexed:
            {
            P_id_type index = ps.readWord();
            t = (void *)ps.find( index );
            assert( t != 0 );
            break;
            }
        case pstream::ptObject:
            {
            const TStreamableClass *pc = ps.readPrefix();
            t = ps.readData( pc, 0 );
            ps.readSuffix();
            break;
            }
        default:
            ps.error( pstream::peInvalidType );
            break;
        }
    return ps;
}

ipstream::ipstream()
{
}


const TStreamableClass *ipstream::readPrefix()
{
    char ch = readByte();
    assert( ch == '[' );    // don't combine this with the previous line!
                            // We must always do the read, even if we're
                            // not checking assertions

    char name[128];
    readString( name, sizeof name );
    return types->lookup( name );
}


void *ipstream::readData( const TStreamableClass *c, TStreamable *mem )
{
    if( mem == 0 )
        mem = c->build();

    registerObject( (char *)mem - c->delta );   // register the actual address
                                        // of the object, not the address
                                        // of the TStreamable sub-object
    return mem->read( *this );
}


void ipstream::readSuffix()
{
    char ch = readByte();
    assert( ch == ']' );    // don't combine this with the previous line!
                            // We must always do the write, even if we're
                            // not checking assertions

}


const void *ipstream::find( P_id_type id )
{
    return objs.find( id );
}

void ipstream::registerObject( const void *adr )
{
    objs.registerObject( adr );
}

opstream::opstream()
{
    objs = new TPWrittenObjects;
}

opstream::opstream( streambuf * sb )
{
    objs = new TPWrittenObjects;
    pstream::init( sb );
}

opstream::~opstream()
{
    objs->shutDown();
    delete objs;
}

opstream& opstream::seekp( streampos pos )
{
    objs->removeAll();
    bp->pubseekoff( pos, ios::beg );
    return *this;
}

opstream& opstream::seekp( streamoff pos, ios::seekdir dir )
{
    objs->removeAll();
    bp->pubseekoff( pos, dir );
    return *this;
}

streampos opstream::tellp()
{
    return bp->pubseekoff( 0, ios::cur, ios::out );
}

opstream& opstream::flush()
{
    bp->pubsync();
    return *this;
}

void opstream::writeByte( uchar ch )
{
    bp->sputc( ch );
}

void opstream::writeBytes( const void *data, size_t sz )
{
    bp->sputn( (char *)data, sz );
}

void opstream::writeWord( ushort sh )
{
    bp->sputn( (char *)&sh, sizeof( ushort ) );
}

void opstream::writeString( const char *str )
{
    if( str == 0 )
        {
        writeByte( nullStringLen );
        return;
        }
    int len = strlen( str );
    writeByte( (uchar)len );
    writeBytes( str, len );
}

opstream& operator << ( opstream& ps, signed char ch )
{
    ps.writeByte( ch );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned char ch )
{
    ps.writeByte( ch );
    return ps;
}

opstream& operator << ( opstream& ps, signed short sh )
{
    ps.writeWord( sh );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned short sh )
{
    ps.writeWord( sh );
    return ps;
}

opstream& operator << ( opstream& ps, signed int i )
{
    ps.writeWord( i );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned int i )
{
    ps.writeWord( i );
    return ps;
}
opstream& operator << ( opstream& ps, signed long l )
{
    int32_t t = int32_t( l );
    ps.writeBytes( &t, sizeof(t) );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned long l )
{
    uint32_t t = uint32_t( l );
    ps.writeBytes( &t, sizeof(t) );
    return ps;
}

opstream& operator << ( opstream& ps, char ch )
{
    ps.writeByte( ch );
    return ps;
}

opstream& operator << ( opstream& ps, float f )
{
    ps.writeBytes( &f, sizeof(f) );
    return ps;
}

opstream& operator << ( opstream& ps, double d )
{
    ps.writeBytes( &d, sizeof(d) );
    return ps;
}

opstream& operator << ( opstream& ps, TStreamable& t )
{
    ps.writePrefix( t );
    ps.writeData( t );
    ps.writeSuffix( t );
    return ps;
}

opstream& operator << ( opstream& ps, TStreamable *t )
{
    P_id_type index;
    if( t == 0 )
        ps.writeByte( pstream::ptNull );
    else if( (index = ps.find( t )) != P_id_notFound )
        {
        ps.writeByte( pstream::ptIndexed );
        ps.writeWord( index );
        }
    else
        {
        ps.writeByte( pstream::ptObject );
        ps << *t;
        }
    return ps;
}

void opstream::writePrefix( const TStreamable& t )
{
    writeByte( '[' );
    writeString( t.streamableName() );
}

void opstream::writeData( TStreamable& t )
{
    if( types->lookup( t.streamableName() ) == 0 )
        error( peNotRegistered, t );
    else
        {
        registerObject( &t );
        t.write( *this );
        }
}

void opstream::writeSuffix( const TStreamable& )
{
    writeByte( ']' );
}

P_id_type opstream::find( const void *adr )
{
    return objs->find( adr );
}

void opstream::registerObject( const void *adr )
{
    objs->registerObject( adr );
}

iopstream::iopstream( streambuf * sb )
{
    pstream::init( sb );
}

iopstream::~iopstream()
{
}

iopstream::iopstream()
{
}

fpbase::fpbase()
{
    pstream::init( &buf );
}

fpbase::fpbase( const char *name, int omode, int prot )
{
    pstream::init( &buf );
    open( name, omode, prot );
}

fpbase::~fpbase()
{
}

void fpbase::open( const char *b, int m, int )
{
    std::ios::openmode mode = std::ios::openmode( m );
    if( buf.is_open() )
        clear(ios::failbit);        // fail - already open
    else if( buf.open(b, mode) )
        clear(ios::goodbit);        // successful open
    else if( (mode & ios::in) && (mode & ios::out) &&
             buf.open(b, mode | ios::trunc) )
        clear(ios::goodbit);        // read/write of a new file
    else
        clear(ios::badbit);     // open failed
}

void fpbase::close()
{
    if( buf.close() )
        clear(ios::goodbit);
    else
        setstate(ios::failbit);
}

void fpbase::setbuf(char* b, int len)
{
    if( buf.pubsetbuf(b, len) )
        clear(ios::goodbit);
    else
        setstate(ios::failbit);
}

filebuf *fpbase::rdbuf()
{
    return &buf;
}

ifpstream::ifpstream()
{
}

ifpstream::ifpstream( const char* name, int omode, int prot ) :
        fpbase( name, omode | ios::in | ios::binary, prot )
{
}



ifpstream::~ifpstream()
{
}

filebuf *ifpstream::rdbuf()
{
    return fpbase::rdbuf();
}

void ifpstream::open( const char *name, int omode, int prot )
{
    fpbase::open( name, omode | ios::in | ios::binary, prot );
}

ofpstream::ofpstream()
{
}

ofpstream::ofpstream( const char* name, int omode, int prot ) :
        fpbase( name, omode | ios::out | ios::binary, prot )
{
}



ofpstream::~ofpstream()
{
}

filebuf *ofpstream::rdbuf()
{
    return fpbase::rdbuf();
}

void ofpstream::open( const char *name, int omode, int prot )
{
    fpbase::open( name, omode | ios::out | ios::binary, prot );
}

fpstream::fpstream()
{
}

fpstream::fpstream( const char* name, int omode, int prot ) :
        fpbase( name, omode | ios::out | ios::binary, prot )
{
}



fpstream::~fpstream()
{
}

filebuf *fpstream::rdbuf()
{
    return fpbase::rdbuf();
}

void fpstream::open( const char *name, int omode, int prot )
{
    fpbase::open( name, omode | ios::in | ios::out | ios::binary, prot );
}


