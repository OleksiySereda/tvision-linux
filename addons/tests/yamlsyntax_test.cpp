/*------------------------------------------------------------*/
/*   Tests of yamlScanLine(), yamlOpensBlock() and            */
/*   TYamlEditor::inBlockScalar().  make check-addons         */
/*------------------------------------------------------------*/

#define Uses_TYamlEditor
#define Uses_TRect
#include <tvaddons.h>

#include <stdio.h>
#include <string.h>
#include <string>

static int failures = 0, checks = 0;

// one letter per character: . plain  K key  S string  C constant
// # comment  P punctuation  A anchor/tag  D document  B block text
static std::string classes( const char *line, Boolean inBlock = False )
{
    static const char letters[] = ".KSC#PADB";
    int n = int( strlen( line ) );
    uchar cls[256];
    yamlScanLine( line, n, inBlock, cls );
    std::string s;
    for( int i = 0; i < n; i++ )
        s += letters[cls[i]];
    return s;
}

static void expect( const char *line, const char *want, Boolean inBlock = False )
{
    checks++;
    std::string got = classes( line, inBlock );
    if( got != want )
        {
        failures++;
        printf( "line  [%s]\n  got [%s]\n want [%s]\n", line, got.c_str(), want );
        }
}

static void expectBlock( const char *line, Boolean want )
{
    checks++;
    if( yamlOpensBlock( line, int( strlen( line ) ) ) != want )
        {
        failures++;
        printf( "yamlOpensBlock( \"%s\" ) is not %s\n", line, want ? "True" : "False" );
        }
}

int main()
{
    expect( "key: value",             "KKKP......" );
    expect( "port: 8080 # x",         "KKKKP.CCCC.###" );
    expect( "- name: 'a b'",          "P.KKKKP.SSSSS" );
    expect( "  - [a, 1, \"x\"]",      "..P.P.P.CP.SSSP" );
    expect( "run: |-",                "KKKP.PP" );
    expect( "---",                    "DDD" );
    expect( "--- # start",            "DDD.#######" );
    expect( "base: &b {x: true}",     "KKKKP.AA.PKP.CCCCP" );
    expect( "url: http://a:8/x",      "KKKP............." );
    expect( "# full comment",         "##############" );
    expect( "text: it's # c",         "KKKKP......###" );
    expect( "\"quoted key\": ~",      "KKKKKKKKKKKKP.C" );
    expect( "ref: *b",                "KKKP.AA" );
    expect( "v: !!str 12",            "KP.AAAAA.CC" );
    expect( "n: [-1.5e3, 0x1F, .inf, no]", "KP.PCCCCCCP.CCCCP.CCCCP.CCP" );
    expect( "msg: \"say \\\"hi\\\"\"", "KKKP.SSSSSSSSSSSS" );
    expect( "  echo a: b # x",        "BBBBBBBBBBBBBBB", True );
    expect( "- - x",                  "P.P.." );

    expectBlock( "run: |", True );
    expectBlock( "- >-", True );
    expectBlock( "  run: |2  # comment", True );
    expectBlock( "a: b", False );
    expectBlock( "a: 'x |'", False );
    expectBlock( "x: a|", False );
    expectBlock( "", False );

    // inBlockScalar() on a real editor buffer
    const char *text =
        "a: 1\r\n"
        "run: |\r\n"
        "  echo x\r\n"
        "    deeper\r\n"
        "list:\r\n"
        "  - cmd: |\r\n"
        "      inside\r\n"
        "    name: x\r\n"
        "b: >-\r\n"
        "  folded\r\n"
        "c: 2\r\n";
    const Boolean want[] = { False, False, True, True, False, False, True, False,
                             False, True, False };
    TYamlEditor *ed = new TYamlEditor( TRect( 0, 0, 80, 25 ), 0, 0, 0, 0 );
    ed->insertText( text, ushort( strlen( text ) ), False );
    ushort p = 0;
    for( int line = 0; line < 11; line++ )
        {
        checks++;
        if( ed->inBlockScalar( p ) != want[line] )
            {
            failures++;
            printf( "inBlockScalar( line %d ) is not %s\n", line + 1,
                    want[line] ? "True" : "False" );
            }
        p = ed->nextLine( p );
        }
    TObject::destroy( ed );

    printf( "%d checks, %d failed\n", checks, failures );
    return failures == 0 ? 0 : 1;
}
