/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   CONFIG.H                                                              */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   miscellaneous system-wide configuration parameters                    */
/*   FOR INTERNAL USE ONLY                                                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __CONFIG_H )
#define __CONFIG_H

#include <limits.h>

const int eventQSize = 16;
const int maxCollectionSize = INT_MAX / (int)sizeof( void * );

const int maxViewWidth = 1024;    // screen width is clamped to this

const int maxFindStrLen    = 80;
const int maxReplaceStrLen = 80;

#endif	// __CONFIG_H