/*------------------------------------------------------------*/
/* filename -       tvtext2.cpp                               */
/*                                                            */
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

#define Uses_TEditWindow
#define Uses_TFileList
#define Uses_TProgram
#define Uses_MsgBox
#define Uses_TChDirDialog
#define Uses_TFileDialog
#define Uses_TFileInfoPane
#define Uses_TSystemError
#define Uses_TDeskTop
#include <tv.h>

#if !defined( __CTYPE_H )
#include <ctype.h>
#endif  // __CTYPE_H

static const char altCodes1[] =
    "QWERTYUIOP\0\0\0\0ASDFGHJKL\0\0\0\0\0ZXCVBNM";
static const char altCodes2[] = "1234567890-=";


char getAltChar(ushort keyCode)
{
    if ((keyCode & 0xff) == 0)
        {
        ushort tmp = (keyCode >> 8);

        if( tmp == 2 )
            return '\xF0';      // special case to handle alt-Space

        else if( tmp >= 0x10 && tmp <= 0x32 )
            return altCodes1[tmp-0x10];     // alt-letter

        else if( tmp >= 0x78 && tmp <= 0x83 )
            return altCodes2[tmp - 0x78];   // alt-number

        }
    return 0;
}

ushort getAltCode(char c)
{
    if( c == 0 )
        return 0;

    c = toupper(c);

    if( uchar(c) == 0xF0 )
        return 0x200;       // special case to handle alt-Space

    int i;
    for( i = 0; i < sizeof( altCodes1 ); i++)
       if( altCodes1[i] == c )
        return (i+0x10) << 8;

    for( i = 0; i < sizeof( altCodes2); i++)
        if (altCodes2[i] == c)
            return (i+0x78) << 8;

    return 0;
}


const char * TEditWindow::clipboardTitle = "Clipboard";
const char * TEditWindow::untitled = "Untitled";

const char * TFileList::tooManyFiles = "Too many files.";

const char * TProgram::exitText = "~Alt-X~ Exit";

const char * MsgBoxText::yesText = "~Y~es";
const char * MsgBoxText::noText = "~N~o";
const char * MsgBoxText::okText = "O~K~";
const char * MsgBoxText::cancelText = "Cancel";
const char * MsgBoxText::warningText = "Warning";
const char * MsgBoxText::errorText = "Error";
const char * MsgBoxText::informationText = "Information";
const char * MsgBoxText::confirmText = "Confirm";

const char * TChDirDialog::changeDirTitle = "Change Directory";
const char * TChDirDialog::dirNameText = "Directory ~n~ame";
const char * TChDirDialog::dirTreeText = "Directory ~t~ree";
const char * TChDirDialog::okText = "O~K~";
const char * TChDirDialog::chdirText = "~C~hdir";
const char * TChDirDialog::revertText = "~R~evert";
const char * TChDirDialog::helpText = "Help";
const char * TChDirDialog::drivesText = "Drives";
const char * TChDirDialog::invalidText = "Invalid directory";

const char * TFileDialog::filesText = "~F~iles";
const char * TFileDialog::openText = "~O~pen";
const char * TFileDialog::okText = "O~K~";
const char * TFileDialog::replaceText = "~R~eplace";
const char * TFileDialog::clearText = "~C~lear";
const char * TFileDialog::cancelText = "Cancel";
const char * TFileDialog::helpText = "~H~elp";
const char * TFileDialog::invalidDriveText = "Invalid drive or directory";
const char * TFileDialog::invalidFileText = "Invalid file name.";

const char * TFileInfoPane::pmText = "p";
const char * TFileInfoPane::amText = "a";
const char * const TFileInfoPane::months[] =
    {
    "","Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
    };

const char * const TSystemError::errorString[] =
{
    "Critical disk error on drive %c",
    "Disk is write-protected in drive %c",
    "Disk is not ready in drive %c",
    "Data integrity error on drive %c",
    "Seek error on drive %c",
    "Unknown media type in drive %c",
    "Sector not found on drive %c",
    "Printer out of paper",
    "Write fault on drive %c",
    "Read fault on drive %c",
    "Hardware failure on drive %c",
    "Bad memory image of FAT detected",
    "Device access error",
    "Insert diskette in drive %c"
};

const char * TSystemError::sRetryOrCancel = "~Enter~ Retry  ~Esc~ Cancel";

const char TDeskTop::defaultBkgrnd = '\xB0';
