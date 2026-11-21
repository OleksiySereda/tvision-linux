/*------------------------------------------------------------*/
/* filename -       tpage.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*                  TPage member functions                    */
/*------------------------------------------------------------*/

#define Uses_TPage
#define Uses_TValidatedInputLine
#define Uses_TDrawBuffer
#define Uses_TRect
#include <tvaddons.h>

// the dialog's interior under the controls of a page
class TPageBackground : public TView
{

public:

    TPageBackground( const TRect& r ) : TView( r )
    {
        growMode = gfGrowHiX | gfGrowHiY;
    }

    virtual void draw()
    {
        TDrawBuffer b;
        b.moveChar( 0, ' ', getColor( 1 ), size.x );    // as TFrame fills it
        writeLine( 0, 0, size.x, size.y, b );
    }
};

TPage::TPage( const TRect& bounds ) :
    TGroup( bounds )
{
    options |= ofSelectable;
    growMode = gfGrowHiX | gfGrowHiY;
    insert( new TPageBackground( getExtent() ) );       // under everything
}

// TGroup::selectNext( False ) goes from last through prev()
std::vector<TView *> TPage::tabOrder()
{
    std::vector<TView *> v;
    if( last == 0 )
        return v;
    TView *p = last;
    do  {
        if( ( p->state & ( sfVisible | sfDisabled ) ) == sfVisible &&
            ( p->options & ofSelectable ) != 0 )
            v.push_back( p );
        p = p->prev();
        } while( p != last );
    return v;
}

TValidatedInputLine *TPage::firstInvalid()
{
    if( last == 0 )
        return 0;
    TView *p = last;
    do  {
        TValidatedInputLine *l = dynamic_cast<TValidatedInputLine *>( p );
        if( l != 0 && l->validator != 0 && ( l->state & sfDisabled ) == 0 &&
            !( l->validator->allowEmpty == True && l->data[0] == 0 ) &&
            !l->validator->isValid( l->data ) )
            return l;
        p = p->prev();
        } while( p != last );
    return 0;
}

Boolean TPage::validPage( ushort command )
{
    if( command == cmValid || command == cmCancel || command == cmReleasedFocus )
        return TGroup::valid( command );
    TValidatedInputLine *l = firstInvalid();
    if( l != 0 )
        return l->valid( command );
    return TGroup::valid( command );
}

void TPage::focusFirst()
{
    std::vector<TView *> v = tabOrder();
    if( !v.empty() )
        v.front()->select();
}

Boolean TPage::focusNext( Boolean forwards )
{
    std::vector<TView *> v = tabOrder();
    if( v.empty() )
        return False;
    int at = -1;
    for( size_t k = 0; k < v.size(); k++ )
        if( v[k] == current )
            at = int( k );
    int next = forwards == True ? at + 1 : ( at < 0 ? int( v.size() ) - 1 : at - 1 );
    if( next >= 0 && next < int( v.size() ) )
        {
        v[size_t( next )]->select();
        return True;
        }
    v[forwards == True ? 0 : v.size() - 1]->select();
    return False;
}
