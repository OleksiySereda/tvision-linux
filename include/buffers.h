/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   BUFFERS.H                                                             */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   defines the functions getBufMem() and freeBufMem() for use            */
/*   in allocating and freeing viedo buffers                               */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( __BCOPT__ )

#endif

#if defined( Uses_TVMemMgr ) && !defined( __TVMemMgr )
#define __TVMemMgr

const int DEFAULT_SAFETY_POOL_SIZE = 4096;

class TBufListEntry
{

private:

    TBufListEntry( void*& );
    ~TBufListEntry();

    void *operator new( size_t, size_t ) throw();
    void *operator new( size_t ) throw();
    void operator delete( void * );

    TBufListEntry *next;
    TBufListEntry *prev;
    void*& owner;

    static TBufListEntry *bufList;
    static Boolean freeHead();

    friend class TVMemMgr;
    friend void *operator new( size_t );

};

class TVMemMgr
{

public:

    TVMemMgr();

    static void resizeSafetyPool( size_t = DEFAULT_SAFETY_POOL_SIZE );
    static int safetyPoolExhausted();

    static void allocateDiscardable( void *&, size_t );
    static void freeDiscardable( void * );

private:

    static void * safetyPool;
    static size_t safetyPoolSize;
    static int inited;
    static int initMemMgr();

};

#endif  // Uses_TVMemMgr

#if defined( __BCOPT__ )

#endif


