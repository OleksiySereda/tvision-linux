/*------------------------------------------------------------*/
/* filename -       new.cpp                                   */
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

#include <assert.h>

#if !defined( __MEM_H )
#include <mem.h>
#endif  // __MEM_H

#if !defined( __ALLOC_H )
#include <alloc.h>
#endif  // __ALLOC_H

#if !defined( __STDLIB_H )
#include <stdlib.h>
#endif  // __STDLIB_H

#define Uses_TVMemMgr
#include <tv.h>

#include <new>

TBufListEntry * TBufListEntry::bufList = 0;

TBufListEntry::TBufListEntry( void*& o ) : owner( o )
{
    next = bufList;
    prev = 0;
    bufList = this;
    if( next != 0 )
        next->prev = this;
}

TBufListEntry::~TBufListEntry()
{
    owner = 0;
    if( prev == 0 )
        bufList = next;
    else
        prev->next = next;
    if( next != 0 )
        next->prev = prev;
}

void *TBufListEntry::operator new( size_t sz, size_t extra ) throw()
{
    return malloc( sz + extra*sizeof( unsigned ) );
}

void *TBufListEntry::operator new( size_t ) throw()
{
    return 0;
}

void TBufListEntry::operator delete( void *b )
{
    free( b );
}

Boolean TBufListEntry::freeHead()
{
    if( bufList == 0 )
        return False;
    else
        {
        delete bufList;
        return True;
        }
}

void * TVMemMgr::safetyPool = 0;
size_t TVMemMgr::safetyPoolSize = 0;
int TVMemMgr::inited = 0;

TVMemMgr memMgr;

TVMemMgr::TVMemMgr()
{
    if( !inited )
        resizeSafetyPool();
};

void TVMemMgr::resizeSafetyPool( size_t sz )
{
    inited = 1;
    free( safetyPool );
    if( sz == 0 )
        safetyPool = 0;
    else
        safetyPool = malloc( sz );
    safetyPoolSize = sz;
}

int TVMemMgr::safetyPoolExhausted()
{
    return inited && (safetyPool == 0);
}

void TVMemMgr::allocateDiscardable( void *&adr, size_t sz )
{
    if( safetyPoolExhausted() )
        adr = 0;
    else
        {
        TBufListEntry *newEntry = new( sz ) TBufListEntry( adr );
        if( newEntry == 0 )
            adr = 0;
        else
            adr = (char *)newEntry + sizeof(TBufListEntry);
        }
}

void TVMemMgr::freeDiscardable( void *block )
{
    delete (TBufListEntry *)((char *)block - sizeof(TBufListEntry));
}



#if defined( TV_DEBUG_HEAP )
const int BLK_SIZE = 16;
const int BLK_DATA = 0xA6;
#else
const int BLK_SIZE = 0;
#endif

void *operator new( size_t sz )
{

    sz += BLK_SIZE;
    if( sz == 0 )
        sz = 1;

    void *temp = malloc( sz );
    while( temp == 0 && TBufListEntry::freeHead() == True )
        temp = malloc( sz );
    if( temp == 0 )
        if( TVMemMgr::safetyPoolExhausted() )
            abort();
        else
            {
            TVMemMgr::resizeSafetyPool( 0 );
            temp = malloc( sz );
            if( temp == 0 )
                abort();
            }
#if defined( TV_DEBUG_HEAP )
    memset( temp, BLK_DATA, BLK_SIZE );
#endif
    return (char *)temp + BLK_SIZE;
}

// Linux port: the nothrow form, for the standard library (the buffer of
// std::stable_sort, for one), which frees it with the plain delete.  It
// takes the memory the same way but returns 0 when there is none: its
// callers do with less, so neither the discardable buffers nor the
// safety pool are given up for it.
void *operator new( size_t sz, const std::nothrow_t& ) noexcept
{
    sz += BLK_SIZE;
    if( sz == 0 )
        sz = 1;
    void *temp = malloc( sz );
    if( temp == 0 )
        return 0;
#if defined( TV_DEBUG_HEAP )
    memset( temp, BLK_DATA, BLK_SIZE );
#endif
    return (char *)temp + BLK_SIZE;
}

#if defined( TV_DEBUG_HEAP )
static void check( void *blk )
{
    for( int i = 0; i < BLK_SIZE; i++ )
        assert( *((unsigned char *)blk + i) == BLK_DATA );
}
#endif

// Linux port: the array and nothrow forms must use the same allocator.
// The standard library allocates through the nothrow forms itself (the
// buffer of std::stable_sort, for one) and frees with the plain delete;
// AddressSanitizer replaces every form it is not given, which then
// reported a mismatch.
void *operator new[]( size_t sz )
{
    return operator new( sz );
}

void operator delete[]( void *blk ) noexcept
{
    operator delete( blk );
}

void *operator new[]( size_t sz, const std::nothrow_t& ) noexcept
{
    return operator new( sz, std::nothrow );
}

void operator delete( void *blk, const std::nothrow_t& ) noexcept
{
    operator delete( blk );
}

void operator delete[]( void *blk, const std::nothrow_t& ) noexcept
{
    operator delete( blk );
}

void operator delete( void *blk ) noexcept
{
    if( blk == 0 )
        return;
    void *tmp = (char *)blk - BLK_SIZE;
#if defined( TV_DEBUG_HEAP )
    check( tmp );
#endif
    free( tmp );
    if( TVMemMgr::safetyPoolExhausted() )
        TVMemMgr::resizeSafetyPool();
}

