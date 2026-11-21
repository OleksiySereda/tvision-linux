/*------------------------------------------------------------*/
/* filename -       tmarkdown.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TMarkdownView and TMarkdownWindow member  */
/*                  functions                                 */
/*                                                            */
/*   parse() splits the text into blocks (paragraphs,         */
/*   headings, lists, ...); layout() turns them into lines    */
/*   for the width of the view, again whenever the width      */
/*   changes.  Inline marks (**, `, [..](..)) are read when   */
/*   a block is laid out.                                     */
/*------------------------------------------------------------*/

#define Uses_TMarkdownView
#define Uses_TMarkdownWindow
#define Uses_TextConv
#define Uses_TScrollBar
#define Uses_TProgram
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace
{

enum { bkParagraph, bkHeading, bkCode, bkRule, bkList, bkQuote, bkTable };

struct Run
{
    std::string text;                   // UTF-8
    uchar style;
    int link;                           // index into the targets, or -1
};

std::string trim( const std::string& s )
{
    size_t a = 0, b = s.size();
    while( a < b && isspace( uchar( s[a] ) ) )
        a++;
    while( b > a && isspace( uchar( s[b - 1] ) ) )
        b--;
    return s.substr( a, b - a );
}

int leadingSpaces( const std::string& s )
{
    int n = 0;
    for( char c : s )
        if( c == ' ' )
            n++;
        else if( c == '\t' )
            n += 4;
        else
            break;
    return n;
}

// "- x", "* x", "+ x", "12. x": the length of the marker, number -1 for
// a bullet; 0 if it is not a list item
size_t listMarker( const std::string& t, int& number )
{
    if( t.size() >= 2 && ( t[0] == '-' || t[0] == '*' || t[0] == '+' ) && t[1] == ' ' )
        {
        number = -1;
        return 2;
        }
    size_t i = 0;
    while( i < t.size() && isdigit( uchar( t[i] ) ) )
        i++;
    if( i > 0 && i < 10 && i + 1 < t.size() && ( t[i] == '.' || t[i] == ')' ) && t[i + 1] == ' ' )
        {
        number = atoi( t.c_str() );
        return i + 2;
        }
    return 0;
}

bool isRule( const std::string& t )
{
    if( t.size() < 3 || ( t[0] != '-' && t[0] != '*' && t[0] != '_' ) )
        return false;
    int n = 0;
    for( char c : t )
        if( c == t[0] )
            n++;
        else if( c != ' ' )
            return false;
    return n >= 3;
}

// **bold**, *emphasis*, `code`, [text](target), <http://...>, \x
std::vector<Run> parseInline( const std::string& s, uchar base,
                              std::vector<std::string>& targets )
{
    std::vector<Run> runs;
    bool bold = false, em = false;
    auto put = [&]( const std::string& t, uchar style, int link )
        {
        if( t.empty() )
            return;
        if( !runs.empty() && runs.back().style == style && runs.back().link == link )
            runs.back().text += t;
        else
            runs.push_back( Run{ t, style, link } );
        };
    auto now = [&]() -> uchar
        {
        return bold ? uchar( TMarkdownView::msBold ) : em ? uchar( TMarkdownView::msEmphasis ) : base;
        };
    size_t i = 0, n = s.size();
    while( i < n )
        {
        char c = s[i];
        if( c == '\\' && i + 1 < n && ispunct( uchar( s[i + 1] ) ) )
            {
            put( std::string( 1, s[i + 1] ), now(), -1 );
            i += 2;
            }
        else if( c == '`' )
            {
            size_t e = s.find( '`', i + 1 );
            if( e == std::string::npos )
                {
                put( "`", now(), -1 );
                i++;
                }
            else
                {
                put( s.substr( i + 1, e - i - 1 ), TMarkdownView::msCode, -1 );
                i = e + 1;
                }
            }
        else if( ( c == '*' || c == '_' ) && i + 1 < n && s[i + 1] == c )
            {
            bold = !bold;
            i += 2;
            }
        else if( ( c == '*' || c == '_' ) &&
                 // not a * between words or a_b in a name
                 !( c == '_' && i > 0 && isalnum( uchar( s[i - 1] ) ) &&
                    i + 1 < n && isalnum( uchar( s[i + 1] ) ) ) &&
                 ( em || ( i + 1 < n && s[i + 1] != ' ' ) ) )
            {
            em = !em;
            i++;
            }
        else if( c == '[' )
            {
            size_t close = s.find( "](", i + 1 );
            size_t end = close != std::string::npos ? s.find( ')', close + 2 ) : std::string::npos;
            if( end == std::string::npos )
                {
                put( "[", now(), -1 );
                i++;
                }
            else
                {
                targets.push_back( trim( s.substr( close + 2, end - close - 2 ) ) );
                int link = int( targets.size() ) - 1;
                std::vector<std::string> inner;
                for( const Run& r : parseInline( s.substr( i + 1, close - i - 1 ), base, inner ) )
                    put( r.text, TMarkdownView::msLink, link );
                i = end + 1;
                }
            }
        else if( c == '<' && ( s.compare( i + 1, 7, "http://" ) == 0 ||
                               s.compare( i + 1, 8, "https://" ) == 0 ) &&
                 s.find( '>', i ) != std::string::npos )
            {
            size_t e = s.find( '>', i );
            targets.push_back( s.substr( i + 1, e - i - 1 ) );
            put( targets.back(), TMarkdownView::msLink, int( targets.size() ) - 1 );
            i = e + 1;
            }
        else
            {
            put( std::string( 1, c ), now(), -1 );
            i++;
            }
        }
    return runs;
}

std::string plainText( const std::string& s )
{
    std::vector<std::string> t;
    std::string out;
    for( const Run& r : parseInline( s, TMarkdownView::msNormal, t ) )
        out += r.text;
    return out;
}

const char *bullet( int level )
{
    static const char *const b[] = { "\x07 ", "o ", "- " };
    return b[level % 3];
}

}   // namespace

// ---- TMarkdownView

TMarkdownView::TMarkdownView( const TRect& bounds, TScrollBar *aHScrollBar,
                              TScrollBar *aVScrollBar ) :
    TScroller( bounds, aHScrollBar, aVScrollBar ),
    focusedLink( -1 ),
    wheelLines( 3 ),
    laidOutWidth( -1 )
{
    options |= ofSelectable;
    eventMask |= evMouseWheel;
}

Boolean TMarkdownView::loadFile( const char *fileName )
{
    FILE *f = fopen( fileName, "rb" );
    if( f == 0 )
        {
        setText( std::string( "**Cannot read** `" ) + fileName + "`" );
        return False;
        }
    std::string s;
    char buf[8192];
    size_t n;
    while( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        s.append( buf, n );
    fclose( f );
    setText( s );
    return True;
}

void TMarkdownView::setText( const std::string& markdown )
{
    source = markdown;
    parse();
    laidOutWidth = -1;
    layout();
    scrollToPos( 0, 0 );
}

std::string TMarkdownView::anchorOf( const std::string& heading )
{
    std::string a;
    for( char c : plainText( heading ) )
        {
        uchar u = uchar( c );
        if( isalnum( u ) || u >= 0x80 || c == '-' || c == '_' )
            a += char( u < 0x80 ? tolower( u ) : u );
        else if( c == ' ' )
            a += '-';
        }
    return a;
}

void TMarkdownView::parse()
{
    blocks.clear();
    std::vector<std::string> src;
    size_t start = 0;
    while( start <= source.size() )
        {
        size_t e = source.find( '\n', start );
        std::string l = source.substr( start, e == std::string::npos ? std::string::npos : e - start );
        if( !l.empty() && l.back() == '\r' )
            l.pop_back();
        src.push_back( l );
        if( e == std::string::npos )
            break;
        start = e + 1;
        }

    Block cur;
    bool open = false;
    auto flush = [&]()
        {
        if( open )
            blocks.push_back( cur );
        open = false;
        cur = Block();
        };

    for( size_t i = 0; i < src.size(); i++ )
        {
        const std::string& l = src[i];
        std::string t = trim( l );
        int number;
        size_t m;
        if( t.compare( 0, 3, "```" ) == 0 || t.compare( 0, 3, "~~~" ) == 0 )
            {
            flush();
            Block b;
            b.kind = bkCode;
            b.level = b.number = 0;
            std::string fence = t.substr( 0, 3 );
            for( i++; i < src.size() && trim( src[i] ).compare( 0, 3, fence ) != 0; i++ )
                b.lines.push_back( src[i] );
            blocks.push_back( b );
            }
        else if( t.empty() )
            flush();
        else if( t[0] == '#' )
            {
            size_t h = 0;
            while( h < t.size() && t[h] == '#' )
                h++;
            if( h <= 6 && h < t.size() && t[h] == ' ' )
                {
                flush();
                Block b;
                b.kind = bkHeading;
                b.level = int( h );
                b.number = 0;
                std::string text = trim( t.substr( h ) );
                while( !text.empty() && text.back() == '#' )
                    text.pop_back();            // "## Title ##"
                b.lines.push_back( trim( text ) );
                blocks.push_back( b );
                }
            else
                goto paragraph;
            }
        else if( isRule( t ) && !( open && cur.kind == bkParagraph && t[0] == '-' ) )
            {
            flush();
            Block b;
            b.kind = bkRule;
            b.level = b.number = 0;
            blocks.push_back( b );
            }
        else if( t[0] == '|' )
            {
            if( !( open && cur.kind == bkTable ) )
                {
                flush();
                cur.kind = bkTable;
                cur.level = cur.number = 0;
                open = true;
                }
            cur.lines.push_back( t );
            }
        else if( ( m = listMarker( t, number ) ) > 0 )
            {
            flush();
            cur.kind = bkList;
            cur.level = leadingSpaces( l ) / 2;
            cur.number = number;
            cur.lines.push_back( t.substr( m ) );
            open = true;
            }
        else if( t[0] == '>' )
            {
            if( !( open && cur.kind == bkQuote ) )
                {
                flush();
                cur.kind = bkQuote;
                cur.level = cur.number = 0;
                open = true;
                }
            cur.lines.push_back( trim( t.substr( 1 ) ) );
            }
        else
            {
        paragraph:
            // a line under a list item, indented, goes on with it
            if( open && ( cur.kind == bkParagraph || cur.kind == bkList || cur.kind == bkQuote ) )
                cur.lines.push_back( t );
            else
                {
                flush();
                cur.kind = bkParagraph;
                cur.level = cur.number = 0;
                cur.lines.push_back( t );
                open = true;
                }
            }
        }
    flush();
}

// text wrapped to width; first: the prefix of the first line (a bullet),
// the others are indented as far; quote: lines start with a bar
void TMarkdownView::addWrapped( const std::string& text, int width, int indent,
                                const std::string& first, uchar baseStyle, int quote )
{
    std::vector<std::string> targets;
    std::vector<Run> runs = parseInline( text, baseStyle, targets );

    // words: (screen text, style, link); a space ends a word
    struct Word { std::string text; std::vector<uchar> style; std::vector<int> link; };
    std::vector<Word> words;
    Word w;
    for( const Run& r : runs )
        {
        std::string t = toScreenText( r.text );
        for( char c : t )
            if( c == ' ' || c == '\t' )
                {
                if( !w.text.empty() )
                    words.push_back( w );
                w = Word();
                }
            else
                {
                w.text += c;
                w.style.push_back( r.style );
                w.link.push_back( r.link );
                }
        }
    if( !w.text.empty() )
        words.push_back( w );

    std::string pad( size_t( indent ), ' ' );
    std::string barPrefix = quote ? "\xB3 " : "";
    int linkBase = int( links.size() );
    std::vector<int> placedLink( targets.size(), -1 );

    Line line;
    auto start = [&]( bool firstLine )
        {
        line = Line();
        line.text = " " + pad + barPrefix + ( firstLine ? first : std::string( first.size(), ' ' ) );
        line.style.assign( line.text.size(), baseStyle );
        if( quote )
            for( size_t k = 1 + pad.size(); k < 1 + pad.size() + 2; k++ )
                line.style[k] = msQuote;
        };
    auto finish = [&]()
        {
        lines.push_back( line );
        };
    start( true );
    size_t prefixLen = line.text.size();
    int room = std::max( 10, width );
    for( Word& wd : words )
        {
        bool empty = line.text.size() == prefixLen;
        if( !empty && int( line.text.size() + 1 + wd.text.size() ) > room )
            {
            finish();
            start( false );
            empty = true;
            }
        if( !empty )
            {
            line.text += ' ';
            line.style.push_back( baseStyle );
            }
        for( size_t k = 0; k < wd.text.size(); k++ )
            {
            if( int( line.text.size() ) >= room + 40 )
                {
                finish();                       // a very long word: broken
                start( false );
                }
            int l = wd.link[k];
            if( l >= 0 )
                {
                int x = int( line.text.size() );
                int y = int( lines.size() );
                Link *last = links.empty() ? 0 : &links.back();
                if( last != 0 && int( links.size() ) > linkBase && last->line == y &&
                    last->target == targets[size_t( l )] && last->x + last->length >= x - 1 &&
                    placedLink[size_t( l )] == int( links.size() ) - 1 )
                    last->length = x + 1 - last->x;
                else
                    {
                    links.push_back( Link{ y, x, 1, targets[size_t( l )] } );
                    placedLink[size_t( l )] = int( links.size() ) - 1;
                    }
                }
            line.text += wd.text[k];
            line.style.push_back( wd.style[k] );
            }
        }
    finish();
}

void TMarkdownView::addTable( const Block& b, int width )
{
    // the cells of each row; the line of dashes under the header goes
    std::vector<std::vector<std::string> > rows;
    for( size_t i = 0; i < b.lines.size(); i++ )
        {
        std::string t = b.lines[i];
        if( !t.empty() && t[0] == '|' )
            t.erase( 0, 1 );
        if( !t.empty() && t.back() == '|' )
            t.pop_back();
        std::vector<std::string> cells;
        size_t s = 0;
        for( size_t k = 0; k <= t.size(); k++ )
            if( k == t.size() || ( t[k] == '|' && ( k == 0 || t[k - 1] != '\\' ) ) )
                {
                cells.push_back( trim( t.substr( s, k - s ) ) );
                s = k + 1;
                }
        bool dashes = !cells.empty();
        for( const std::string& c : cells )
            if( c.find_first_not_of( "-: " ) != std::string::npos || c.empty() )
                dashes = false;
        if( i == 1 && dashes )
            continue;
        rows.push_back( cells );
        }
    size_t cols = 0;
    for( auto& r : rows )
        cols = std::max( cols, r.size() );
    if( cols == 0 )
        return;

    // the cells as screen text; their widths, shrunk to fit if need be
    std::vector<std::vector<std::string> > text( rows.size() );
    std::vector<int> w( cols, 1 );
    for( size_t r = 0; r < rows.size(); r++ )
        for( size_t c = 0; c < cols; c++ )
            {
            std::string t = c < rows[r].size() ? toScreenText( plainText( rows[r][c] ) ) : "";
            text[r].push_back( t );
            w[c] = std::max( w[c], int( t.size() ) );
            }
    int total = 1;
    for( int x : w )
        total += x + 3;
    while( total > width )
        {
        size_t widest = size_t( std::max_element( w.begin(), w.end() ) - w.begin() );
        if( w[widest] <= 3 )
            break;
        w[widest]--;
        total--;
        }

    for( size_t r = 0; r < rows.size(); r++ )
        {
        Line l;
        l.text = " ";
        l.style.push_back( msNormal );
        for( size_t c = 0; c < cols; c++ )
            {
            if( c > 0 )
                {
                l.text += " \xB3 ";
                l.style.insert( l.style.end(), 3, msTable );
                }
            std::string t = text[r][c].substr( 0, size_t( w[c] ) );
            t.append( size_t( w[c] ) - t.size(), ' ' );
            l.text += t;
            l.style.insert( l.style.end(), t.size(), r == 0 ? uchar( msBold ) : uchar( msNormal ) );
            }
        lines.push_back( l );
        if( r == 0 && rows.size() > 1 )
            {
            Line u;
            u.text = " ";
            for( size_t c = 0; c < cols; c++ )
                {
                if( c > 0 )
                    u.text += "\xC4\xC5\xC4";
                u.text.append( size_t( w[c] ), '\xC4' );
                }
            u.style.assign( u.text.size(), msTable );
            lines.push_back( u );
            }
        }
}

void TMarkdownView::addBlock( const Block& b, int width )
{
    std::string joined;
    for( const std::string& l : b.lines )
        joined += ( joined.empty() ? "" : " " ) + l;
    switch( b.kind )
        {
        case bkHeading:
            {
            uchar st = b.level == 1 ? msHeading1 : b.level == 2 ? msHeading2 : msHeading3;
            anchors.push_back( std::make_pair( anchorOf( joined ), int( lines.size() ) ) );
            size_t before = lines.size();
            addWrapped( joined, width, 0, "", st, 0 );
            if( b.level <= 2 )
                {
                size_t len = 0;
                for( size_t k = before; k < lines.size(); k++ )
                    len = std::max( len, lines[k].text.size() - 1 );
                Line u;
                u.text = " " + std::string( len, b.level == 1 ? '\xCD' : '\xC4' );
                u.style.assign( u.text.size(), st );
                lines.push_back( u );
                }
            break;
            }
        case bkCode:
            for( const std::string& raw : b.lines )
                {
                std::string t;
                for( char c : raw )
                    if( c == '\t' )
                        t.append( 4 - t.size() % 4, ' ' );
                    else
                        t += c;
                Line l;
                l.text = "   " + toScreenText( t );
                l.style.assign( l.text.size(), msCodeBlock );
                lines.push_back( l );
                }
            break;
        case bkRule:
            {
            Line l;
            l.text = " " + std::string( size_t( std::max( 1, width - 1 ) ), '\xC4' );
            l.style.assign( l.text.size(), msRule );
            lines.push_back( l );
            break;
            }
        case bkList:
            {
            std::string mark = b.number >= 0 ? std::to_string( b.number ) + ". " : bullet( b.level );
            addWrapped( joined, width, 2 * b.level + 1, mark, msNormal, 0 );
            break;
            }
        case bkQuote:
            addWrapped( joined, width, 1, "", msQuote, 1 );
            break;
        case bkTable:
            addTable( b, width );
            break;
        default:
            addWrapped( joined, width, 0, "", msNormal, 0 );
            break;
        }
}

void TMarkdownView::layout()
{
    int width = std::max( 20, size.x - 2 );
    if( width == laidOutWidth )
        return;
    laidOutWidth = width;
    lines.clear();
    links.clear();
    anchors.clear();
    for( size_t i = 0; i < blocks.size(); i++ )
        {
        // a blank line between blocks, not between the items of a list
        if( i > 0 && !( blocks[i].kind == bkList && blocks[i - 1].kind == bkList ) )
            lines.push_back( Line() );
        addBlock( blocks[i], width );
        }
    if( focusedLink >= int( links.size() ) )
        focusedLink = -1;
    int w = 1;
    for( const Line& l : lines )
        w = std::max( w, int( l.text.size() ) + 1 );
    setLimit( short( std::min( w, 32000 ) ), short( std::min( int( lines.size() ), 32000 ) ) );
}

void TMarkdownView::changeBounds( const TRect& bounds )
{
    // the line at the top stays roughly where it was
    double at = lines.empty() ? 0 : double( delta.y ) / lines.size();
    TScroller::changeBounds( bounds );
    int old = laidOutWidth;
    layout();
    if( laidOutWidth != old )
        scrollToPos( 0, int( at * lines.size() ) );
}

void TMarkdownView::scrollToPos( int x, int y )
{
    x = std::max( 0, std::min( x, limit.x - size.x ) );
    y = std::max( 0, std::min( y, limit.y - size.y ) );
    drawLock++;
    if( hScrollBar != 0 )
        hScrollBar->setValue( short( x ) );
    if( vScrollBar != 0 )
        vScrollBar->setValue( short( y ) );
    drawLock--;
    delta.x = short( x );
    delta.y = short( y );
    drawFlag = False;
    drawView();
}

void TMarkdownView::scrollDraw()
{
    TPoint d = delta;
    if( hScrollBar != 0 )
        d.x = hScrollBar->value;
    if( vScrollBar != 0 )
        d.y = vScrollBar->value;
    if( d.x != delta.x || d.y != delta.y )
        {
        delta = d;
        if( drawLock != 0 )
            drawFlag = True;
        else
            drawView();
        }
}

Boolean TMarkdownView::gotoAnchor( const char *anchor )
{
    std::string a = anchor != 0 ? anchor : "";
    if( !a.empty() && a[0] == '#' )
        a.erase( 0, 1 );
    for( auto& p : anchors )
        if( p.first == a )
            {
            scrollToPos( 0, p.second );
            return True;
            }
    return False;
}

uchar TMarkdownView::styleColor( int style, uchar normal )
{
    static const uchar fg[] =
        { 0, 0x0F, 0x0E, 0x0D, 0x0F, 0x0E, 0x0A, 0x0A, 0x07, 0x0B, 0x03, 0x03 };
    if( style <= msNormal || style > msTable )
        return normal;
    uchar bg = uchar( normal & 0xF0 );
    uchar f = fg[style];
    int back = ( normal >> 4 ) & 0x07;
    if( back == 7 || back == 3 || back == 2 || back == 6 )
        f &= 0x07;                      // a light background: dark letters
    if( ( f & 0x07 ) == back && ( f & 0x08 ) == 0 )
        f = uchar( normal & 0x0F );
    return uchar( bg | f );
}

void TMarkdownView::draw()
{
    uchar normal = uchar( getColor( 1 ) );
    uchar inverse = uchar( ( ( normal & 0x0F ) << 4 ) | ( ( normal >> 4 ) & 0x0F ) );
    uchar colors[msTable + 1];
    for( int s = 0; s <= msTable; s++ )
        colors[s] = styleColor( s, normal );
    for( int y = 0; y < size.y; y++ )
        {
        TDrawBuffer b;
        b.moveChar( 0, ' ', normal, size.x );
        int i = delta.y + y;
        if( i < int( lines.size() ) )
            {
            const Line& l = lines[size_t( i )];
            for( int x = 0; x < size.x; x++ )
                {
                int k = delta.x + x;
                if( k >= int( l.text.size() ) )
                    break;
                uchar st = k < int( l.style.size() ) ? l.style[size_t( k )] : uchar( msNormal );
                uchar a = colors[st <= msTable ? st : 0];
                if( focusedLink >= 0 )
                    {
                    const Link& f = links[size_t( focusedLink )];
                    if( f.line == i && k >= f.x && k < f.x + f.length )
                        a = inverse;
                    }
                b.moveChar( ushort( x ), l.text[size_t( k )], a, 1 );
                }
            }
        writeLine( 0, short( y ), size.x, 1, b );
        }
}

int TMarkdownView::linkAt( int line, int x ) const
{
    for( size_t i = 0; i < links.size(); i++ )
        if( links[i].line == line && x >= links[i].x && x < links[i].x + links[i].length )
            return int( i );
    return -1;
}

void TMarkdownView::nextLink( Boolean forwards )
{
    if( links.empty() )
        return;
    int n = int( links.size() );
    if( focusedLink < 0 )
        {
        // the first one on the screen, or after it
        int i = 0;
        while( i < n && links[size_t( i )].line < delta.y )
            i++;
        focusedLink = forwards == True ? ( i < n ? i : 0 ) : ( i > 0 ? i - 1 : n - 1 );
        }
    else
        focusedLink = ( focusedLink + ( forwards == True ? 1 : n - 1 ) ) % n;
    int y = links[size_t( focusedLink )].line;
    if( y < delta.y || y >= delta.y + size.y )
        scrollToPos( delta.x, y - size.y / 2 );
    else
        drawView();
}

void TMarkdownView::followLink( int i )
{
    if( i < 0 || i >= int( links.size() ) )
        return;
    const std::string& t = links[size_t( i )].target;
    if( !t.empty() && t[0] == '#' )
        {
        focusedLink = -1;
        gotoAnchor( t.c_str() );
        }
    else
        message( TProgram::application, evBroadcast, cmMarkdownLink, (void *) t.c_str() );
}

void TMarkdownView::handleEvent( TEvent& event )
{
    TScroller::handleEvent( event );
    if( event.what == evMouseWheel )
        {
        switch( event.mouse.buttons )
            {
            case mwUp:    scrollToPos( delta.x, delta.y - wheelLines ); break;
            case mwDown:  scrollToPos( delta.x, delta.y + wheelLines ); break;
            case mwLeft:  scrollToPos( delta.x - wheelLines, delta.y ); break;
            case mwRight: scrollToPos( delta.x + wheelLines, delta.y ); break;
            }
        clearEvent( event );
        }
    else if( event.what == evMouseDown )
        {
        TPoint p = makeLocal( event.mouse.where );
        int i = linkAt( delta.y + p.y, delta.x + p.x );
        if( i >= 0 )
            {
            focusedLink = i;
            drawView();
            followLink( i );
            clearEvent( event );
            }
        }
    else if( event.what == evKeyDown )
        {
        int page = std::max( 1, size.y - 1 );
        switch( ctrlToArrow( event.keyDown.keyCode ) )
            {
            case kbTab:      nextLink( True ); break;
            case kbShiftTab: nextLink( False ); break;
            case kbEnter:
                if( focusedLink < 0 )
                    return;
                followLink( focusedLink );
                break;
            case kbUp:       scrollToPos( delta.x, delta.y - 1 ); break;
            case kbDown:     scrollToPos( delta.x, delta.y + 1 ); break;
            case kbLeft:     scrollToPos( delta.x - 1, delta.y ); break;
            case kbRight:    scrollToPos( delta.x + 1, delta.y ); break;
            case kbPgUp:     scrollToPos( delta.x, delta.y - page ); break;
            case kbPgDn:     scrollToPos( delta.x, delta.y + page ); break;
            case kbHome:
            case kbCtrlPgUp: scrollToPos( 0, 0 ); break;
            case kbEnd:
            case kbCtrlPgDn: scrollToPos( 0, limit.y ); break;
            default:
                return;
            }
        clearEvent( event );
        }
}

// ---- TMarkdownWindow

TMarkdownWindow::TMarkdownWindow( const TRect& bounds, const char *fileName ) :
    TWindowInit( &TMarkdownWindow::initFrame ),
    TWindow( bounds, fileName, wnNoNumber )
{
    TRect r = getExtent();
    r.grow( -1, -1 );
    view = new TMarkdownView( r, standardScrollBar( sbHorizontal | sbHandleKeyboard ),
                              standardScrollBar( sbVertical | sbHandleKeyboard ) );
    view->growMode = gfGrowHiX | gfGrowHiY;
    insert( view );
    view->loadFile( fileName );
}
