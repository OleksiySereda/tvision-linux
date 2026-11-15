/*-------------------------------------------------------*/
/*                                                       */
/*   Turbo Vision 1.0                                    */
/*   Turbo Vision Forms Demo                             */
/*   Copyright (c) 1991 by Borland International         */
/*                                                       */
/*   Formcmds.h: Support header file for TVFORMS Demo    */
/*-------------------------------------------------------*/

#if !defined(__FORMCMDS_H )
#define __FORMCMDS_H

// Misc UI commands 

const int 
   cmAboutBox    = 2000,
   cmChgDir      = 2001,
   cmVideoMode   = 2002,
   cmDosShell    = 2003;

// List & form-oriented commands 
// (Cannot be disabled)          

const int
   cmListOpen    = 3000,
   cmListSave    = 3001,
   cmFormEdit    = 3002,
   cmFormNew     = 3003,
   cmFormSave    = 3004,
   cmFormDel     = 3005;

// Broadcast commands 

const int
   cmTopForm      = 3050,
   cmRegisterForm = 3051,
   cmEditingForm  = 3052,
   cmCanCloseForm = 3053,
   cmCloseForm    = 3054,
   cmTopList      = 3055,
   cmEditingFile  = 3056;

// History list IDs 

const int
   hlChangeDir   = 1,
   hlOpenListDlg = 2;

#endif  // __FORMCMDS_H
