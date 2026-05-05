
/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1994, Silicon Graphics, Inc.               *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright  law.  They  may not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 *************************************************************************/

/**************************************************************************
 *
 *  $Revision: 1.10 $
 *  $Date: 1997/02/11 08:37:33 $
 *  $Source: /disk6/Master/cvsmdev2/PR/include/ultra64.h,v $
 *
 **************************************************************************/

#ifndef _ULTRA64_H_
#define _ULTRA64_H_

#ifdef __SWITCH__
/* <PR/os.h> includes <stddef.h> without __need_wint_t.  GCC's built-in
 * stddef.h does not honour newlib's __need_wint_t extension, so
 * <sys/_types.h> never sees wint_t defined.  Provide it globally here
 * before <stddef.h> is first consumed. */
#ifndef _WINT_T
#define _WINT_T
typedef __WINT_TYPE__ wint_t;
#endif
#endif

#include <PR/ultratypes.h>
#include <PR/rcp.h>
#include <PR/os.h>
#include <PR/region.h>
#include <PR/rmon.h>
#include <PR/sptask.h>
#include <PR/mbi.h>
#include <PR/libaudio.h>
#include <PR/gu.h>
#include <PR/ramrom.h>
#include <PR/sp.h>
#include <PR/ucode.h>
#include <PR/ultraerror.h>
#include <PR/ultralog.h>

#endif
