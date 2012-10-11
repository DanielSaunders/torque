/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/

#include <pbs_config.h>   /* the master config generated by configure */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "portability.h"
#include "log.h"
#include "pbs_log.h"
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>

#ifdef __CYGWIN__

#include <ctype.h>
#include <wchar.h>
#include <windows.h>
#include <io.h>
#include <sys/cygwin.h>
#include <getopt.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <ntsecapi.h>
#include <ntdef.h>
#include <sys/fcntl.h>
#include <lmerr.h>
#include <lmcons.h>

SID_IDENTIFIER_AUTHORITY sid_world_auth = {SECURITY_WORLD_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY sid_nt_auth = {SECURITY_NT_AUTHORITY};

NET_API_STATUS WINAPI (*netapibufferfree)(PVOID);
NET_API_STATUS WINAPI (*netuserenum)(LPWSTR,DWORD,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI (*netgroupenum)(LPWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI (*netlocalgroupenum)(LPWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI (*netlocalgroupgetmembers)(LPWSTR,LPWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI (*netgroupgetusers)(LPWSTR,LPWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);
NET_API_STATUS WINAPI (*netgetdcname)(LPWSTR,LPWSTR,PBYTE*);
NET_API_STATUS WINAPI (*netusergetinfo)(LPWSTR,LPWSTR,DWORD,PBYTE*);

NTSTATUS NTAPI (*lsaclose)(LSA_HANDLE);
NTSTATUS NTAPI (*lsaopenpolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
NTSTATUS NTAPI (*lsaqueryinformationpolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
NTSTATUS NTAPI (*lsafreememory)(PVOID);

LPWSTR servername;

#endif  /* __CYGWIN__ */


#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

int chk_file_sec_stderr = 0;


#ifdef __CYGWIN__

/* ----------------------------- HELPERS ---------------------------------------- */

BOOL load_netapi (HANDLE hNetapi,HANDLE hAdvapi)
{
    if ((!hNetapi) || (!hAdvapi))
	return FALSE;

    if (!(netapibufferfree = (void *) GetProcAddress (hNetapi, "NetApiBufferFree")))
	return FALSE;
    if (!(netuserenum = (void *) GetProcAddress (hNetapi, "NetUserEnum")))
	return FALSE;
    if (!(netlocalgroupenum = (void *) GetProcAddress (hNetapi, "NetLocalGroupEnum")))
	return FALSE;
    if (!(netgetdcname = (void *) GetProcAddress (hNetapi, "NetGetDCName")))
	return FALSE;
    if (!(netusergetinfo = (void *) GetProcAddress (hNetapi, "NetUserGetInfo")))
	return FALSE;
    if (!(netgroupenum = (void *) GetProcAddress (hNetapi, "NetGroupEnum")))
	return FALSE;
    if (!(netgroupgetusers = (void *) GetProcAddress (hNetapi, "NetGroupGetUsers")))
	return FALSE;  
    if (!(netlocalgroupgetmembers = (void *) GetProcAddress (hNetapi, "NetLocalGroupGetMembers")))
	return FALSE;
    if (!(lsaclose = (void *) GetProcAddress (hAdvapi, "LsaClose")))
	return FALSE;
    if (!(lsaopenpolicy = (void *) GetProcAddress (hAdvapi, "LsaOpenPolicy")))
	return FALSE;
    if (!(lsaqueryinformationpolicy = (void *) GetProcAddress (hAdvapi, "LsaQueryInformationPolicy")))
	return FALSE;
    if (!(lsafreememory = (void *) GetProcAddress (hAdvapi, "LsaFreeMemory")))
	return FALSE;  

    return TRUE;
}

void uni2ansi (LPWSTR wcs, char *mbs, int size)
{
	if (wcs)
		WideCharToMultiByte (CP_ACP, 0, wcs, -1, mbs, size, NULL, NULL);
	else
		*mbs = '\0';
}

void uni2utf8 (LPWSTR wcs, char *mbs, int size)
{
	if (wcs)
		WideCharToMultiByte (CP_UTF8, 0, wcs, -1, mbs, size, NULL, NULL);
	else
		*mbs = '\0';
}

/* ----------------------------- BASIC FUNCTIONS ----------------------------------- */

int enum_local_users (LPWSTR groupname,char *username)
{
	GROUP_USERS_INFO_0 *buf0; 
	LOCALGROUP_MEMBERS_INFO_1 *buf1;
	DWORD entries = 0;
	DWORD total = 0;
	DWORD reshdl = 0;
	int i,ret=-1;
	char grp_username[128];

	/* Print local users*/
		if (!netlocalgroupgetmembers (NULL, groupname, 1, (void *) &buf1, MAX_PREFERRED_LENGTH, &entries, &total, &reshdl))
		{
			ret=0;
			for (i = 0; i < entries; ++i)
				if (buf1[i].lgrmi1_sidusage == SidTypeUser)
				{
					uni2utf8 (buf1[i].lgrmi1_name, grp_username, sizeof (grp_username));
					if (strcmp(grp_username,username)==0)
					{
						ret=1;
						break;
					}
				}
		  netapibufferfree (buf1);
		} 

	return ret;
}

int enum_domain_users (LPWSTR server_name, LPWSTR groupname,char *username)
{
	GROUP_USERS_INFO_0 *buf0; 
	LOCALGROUP_MEMBERS_INFO_1 *buf1;
	DWORD entries = 0;
	DWORD total = 0;
	DWORD reshdl = 0;
	int i,ret=-1;
	char grp_username[128];

		if (!netgroupgetusers (server_name, groupname, 0, (void *) &buf0,  MAX_PREFERRED_LENGTH, &entries, &total, &reshdl))
		{
			ret=0;
			for (i = 0; i < entries; ++i)
			{
				uni2utf8 (buf0[i].grui0_name, grp_username, sizeof (grp_username));


				if (strcmp(grp_username,username)==0)
				{
					ret=1;
					break;
				}
			}
			netapibufferfree (buf0);
		}

	return ret;
}

int check_local_user_privileges (char *username_utf8, int usertype)
{

	LOCALGROUP_INFO_0 *buffer;
	DWORD entriesread = 0;
	DWORD totalentries = 0;
	DWORD resume_handle = 0;
	DWORD rc;

	char errbuf[1024];
	int user=-1,admin=-1,ret;

	do
    {
		DWORD i;
		rc = netlocalgroupenum (NULL, 0, (void *) &buffer, 1024, &entriesread, &totalentries, &resume_handle);
		switch (rc)
		{
			case ERROR_ACCESS_DENIED:
				return 1;
			case ERROR_MORE_DATA:
			case ERROR_SUCCESS:
				break;
			default:
				return 1;
		}

		for (i = 0; i < entriesread; i++)
		{
			char localgroup_name_acp[128];
			char domain_name[128];
			DWORD domain_name_len = 128;
			char psid_buffer[1024];	

			DWORD sid_length = 1024;
			int gid;
			SID_NAME_USE acc_type;

			uni2ansi (buffer[i].lgrpi0_name, localgroup_name_acp, sizeof (localgroup_name_acp));

			if (!LookupAccountName (NULL, localgroup_name_acp, &psid_buffer, &sid_length, domain_name, &domain_name_len, &acc_type))
			{
				continue;
			}

			gid = *GetSidSubAuthority (&psid_buffer, *GetSidSubAuthorityCount(&psid_buffer) - 1);

			if (gid==544)
			{
				ret = enum_local_users (buffer[i].lgrpi0_name,username_utf8);
					if (ret>admin)
						admin=ret;
			}

			if (gid==545)
			{
				ret = enum_local_users (buffer[i].lgrpi0_name, username_utf8);
					if (ret>user)
						user=ret;
			}

		}
		netapibufferfree (buffer);
	}
	while (rc == ERROR_MORE_DATA);

	/* check if user is Admin */
	if (usertype==0) 
		return (admin==1)?1:0;

	/* check if user is Simple User */
	return (admin==0 && user==1)?1:0;
}

int check_domain_user_privileges (LPWSTR servername, char *username_utf8, int usertype)
{
	GROUP_INFO_2 *buffer;
	DWORD entriesread = 0;
	DWORD totalentries = 0;
	DWORD resume_handle = 0;
	DWORD rc;

	char errbuf[1024];
	int user=-1,admin=-1,ret;

	do
	{
	DWORD i;
	rc = netgroupenum (servername, 2, (void *) &buffer, 1024, &entriesread, &totalentries, &resume_handle);

        switch (rc)
		{
			case ERROR_ACCESS_DENIED: 
				return;
			case ERROR_MORE_DATA:
			case ERROR_SUCCESS:
				break;
			default: 
				return;
		}

		for (i = 0; i < entriesread; i++)
		{

			int gid = buffer[i].grpi2_group_id;

			if (gid==512)
			{
				ret = enum_domain_users (servername, buffer[i].grpi2_name,username_utf8);
					if (ret>admin)
						admin=ret;
			}
			if (gid==513)
			{
				ret = enum_domain_users (servername, buffer[i].grpi2_name, username_utf8);
					if (ret>user)
						user=ret;
			}
		}
		netapibufferfree (buffer);
	}
	while (rc == ERROR_MORE_DATA);

	/* check if user is Admin */
	if (usertype==0) 
		return (admin==1)?1:0;
	/* check if user is Simple User */
	return (admin==0 && user==1)?1:0;
}

/* ----------------------------- TORQUE FUNCTIONS ----------------------------------- */

/* 
 * IamRoot returns 1 if current user has root (Administrator) account, 
 * else returns 0
*/

int IamRoot()
{
	struct passwd *p;   
	int uid;
	HANDLE hAdvapi, hNetapi;

	servername=NULL;
	hNetapi = LoadLibrary ("netapi32.dll");
	hAdvapi = LoadLibrary ("advapi32.dll");

	if (!load_netapi (hNetapi,hAdvapi))
	{
		log_err(-1, "IamRoot","Cann`t load netapi32.dll and advapi32.dll libraries\n");          				
		return 0;
	}

	if (netgetdcname (NULL, NULL, (void *) &servername) != ERROR_SUCCESS)
	{
		log_err(-1, "IamRoot","Cann`t get the name of the primary domain controller\n");
	}

	uid=getuid();

	if (uid==18) 
		return 1;

	if ((p = getpwuid(uid))==NULL)
	{
		log_err(-1, "IamRoot","WARNING!!! No password entry for currient user. Check your /etc/passwd file.\n");
  		return 0;
	}

	if (check_local_user_privileges(p->pw_name,0) || check_domain_user_privileges(servername,p->pw_name,0))
        return 1;  
  
	log_err(-1, "IamRoot","WARNING!!! Must be run with Administrator privileges.\n");
	return 0;
}


/* 
 * IamAdminByName returns 1 if user <userName> has Administrator account, 
 * else returns 0 
*/

int IamAdminByName(char *userName)
{
	return (check_local_user_privileges(userName,0) || check_domain_user_privileges(servername,userName,0))?1:0;
}


/*
 * IamUser returns 1 if current user isn't included to Administrators group
 * (i.e. has a limited account), else returns 0 
*/

int IamUser()
{  
	struct passwd *p;

	if ((p = getpwuid(getuid())) != NULL)
    {
		printf("Check %s\n",p->pw_name);
    	if (check_local_user_privileges(p->pw_name,1) || check_domain_user_privileges(servername,p->pw_name,1))
			return 1;
    }

	log_err(-1, "IamUser","WARNING!!! Check your /etc/group and /etc/passwd files.\n");
	return 0;
}  /* END IamUser() */


/*
 * IamUserByName returns 1 if current user isn't included to Administrators group
 * (i.e. has a limited account), else returns 0 
*/

int IamUserByName(char *userName)
{  
	char buff[512];	


	if (check_local_user_privileges(userName,1) || check_domain_user_privileges(servername,userName,1))
	{
		return 1;
	}
	else
		if (IamAdminByName(userName))
		{
			sprintf(buff, "WARNING!!! Can`t run job with Administrator privileges. Your should limit preveleges for \"%s\"!",userName);
			log_err(-1, "IamUserByName", buff);
			return 0;
		}
	sprintf(buff, "WARNING!!! Can`t find user \"%s\"!",userName);
	log_err(-1, "IamUserByName", buff);
    return 0;
} 


#else /* not def __CYGWIN__ */

int IamRoot()
{
	if ((getuid() == 0) && (geteuid() == 0))
		return 1;
	fprintf(stderr, "Must be run as root\n");
	return 0;
}

#endif /* __CYGWIN__ */






/*
 * chk_file_sec() - Check file/directory security
 *      Part of the PBS System Security "Feature"
 *
 * To be secure, all directories (and final file) in path must be:
 *  owned by uid < 10
 *  owned by group < 10 if group writable
 *  not have world writable unless stick bit set & this is allowed.
 *
 * Returns 0 if ok
 *      errno value if not ok, including:
 *              EPERM if not owned by root
 *              ENOTDIR if not file/directory as specified
 *              EACCESS if permissions are not ok
 */

int chk_file_sec(

  char *path,   /* path to check */
  int   isdir,   /* 1 = path is directory, 0 = file */
  int   sticky,   /* allow write on directory if sticky set */
  int   disallow, /* perm bits to disallow */
  int   fullpath, /* recursively check full path */
  char *SEMsg)    /* O (optional,minsize=1024) */

  {
  int    i;
  char  *error_buf;
  char  *pc;
  int    rc = 0;

  struct stat sbuf;
  char   shorter[_POSIX_PATH_MAX];
  char   symlink[_POSIX_PATH_MAX];

  char   tmpLine[1024];

  char  *EMsg;

  if (SEMsg != NULL)
    EMsg = SEMsg;
  else
    EMsg = tmpLine;

  EMsg[0] = '\0';

  if ((*path == '/') && fullpath)
    {
    /* check full path starting at root */

    snprintf(shorter, _POSIX_PATH_MAX, "%s", path);

    pc = strrchr(shorter, '/');

    if ((pc != NULL) && (pc != shorter))
      {
      /*
       * push "dirname" onto stack, stack will pop back from
       * root to the given file/directory
       */

      *pc = '\0';

      if ((rc = chk_file_sec(shorter, 1, sticky, S_IWGRP | S_IWOTH, 1, EMsg)) != 0)
        {
        return(rc);
        }
      }
    }

  if (lstat(path, &sbuf) == -1)
    {
    rc = errno;

    /* FAILURE */

    if (EMsg != NULL)
      snprintf(EMsg, 1024, "%s cannot be lstat'd - errno=%d, %s",
               path,
               rc,
               strerror(rc));

    goto chkerr;
    }

  if (S_ISLNK(sbuf.st_mode) != 0)
    {
    i = readlink(path, symlink, sizeof(symlink) - 1);

    if (i < 0)
      {
      rc = errno;

      /* FAILURE */

      snprintf(EMsg, 1024, "%s cannot be read as link, errno=%d, %s",
               path,
               rc,
               strerror(rc));

      goto chkerr;
      }

    if (i == sizeof(symlink) - 1)
      symlink[i] = '\0';
    else
      symlink[i + 1] = '\0';

    if (symlink[0] == '/')
      {
      return(chk_file_sec(symlink, isdir, sticky, disallow, fullpath, EMsg));
      }

    snprintf(shorter, _POSIX_PATH_MAX, "%s", path);

    /* terminate string after final directory delimiter */

    pc = strrchr(shorter, '/');

    if (pc != NULL)
      {
      pc[1] = '\0';
      }

    /* now figure out how to follow the symlink */

    if (stat(path, &sbuf) == -1)
      {
      rc = errno;

      /* FAILURE */

      snprintf(EMsg, 1024, "%s cannot be stat'd - errno=%d, %s",
               path,
               rc,
               strerror(rc));

      goto chkerr;
      }

    if (S_ISDIR(sbuf.st_mode) != 0)
      {
      if (strlen(shorter) + strlen(symlink) > _POSIX_PATH_MAX)
        {
        snprintf(EMsg, 1024, "buffer length exceeded in chk_file_sec");
        /* This isn't the right error */
        rc = EPERM;
        goto chkerr;
        }
      strcat(shorter, symlink);
      }
    else
      {
      snprintf(shorter, _POSIX_PATH_MAX, "%s", symlink);
      }

    return(chk_file_sec(shorter, isdir, sticky, disallow, fullpath, EMsg));
    }

  i = sbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

#ifndef __CYGWIN__
  if (sbuf.st_uid > 10)
    {
    rc = EPERM;

    /* FAILURE */

    snprintf(EMsg, 1024, "%s is not owned by admin user",
             path);
    }
  else 
#endif  /* __CYGWIN__ */

    if (((isdir == 1) && (S_ISDIR(sbuf.st_mode) == 0)) ||
           ((isdir == 0) && (S_ISREG(sbuf.st_mode) == 0)))
    {
    /* FAILURE */

    snprintf(EMsg, 1024, "%s is not directory",
             path);

    rc = ENOTDIR;
    }
  else if (isdir && sticky && !fullpath)
    {
    if ((S_ISDIR(sbuf.st_mode) == 0) ||
        ((sbuf.st_mode & S_ISVTX) == 0) ||
        (i != (S_IRWXU | S_IRWXG | S_IRWXO)))
      {
      /* FAILURE */

      snprintf(EMsg, 1024, "%s cannot be accessed",
               path);

      rc = EACCES;
      }
    }
  else if (i & disallow)
    {
    /* if group write, gid must be less than 10 */

#ifndef __CYGWIN__
    if ((i & disallow & S_IWGRP) && (sbuf.st_gid > 9))
      {
      /* FAILURE */

      snprintf(EMsg, 1024, "%s is group writable",
               path);

      rc = EPERM;
      }
#endif  /* __CYGWIN__ */

    /* if world write, sticky bit must be set and "sticky" ok */

    if (i & disallow & S_IWOTH)
      {
      if ((S_ISDIR(sbuf.st_mode) == 0) ||
          (((sbuf.st_mode & S_ISVTX) == 0) || (sticky != 1)))
        {
        /* FAILURE */

        snprintf(EMsg, 1024, "%s is world writable and not sticky",
                 path);

        rc = EACCES;
        }
      }

    /* check any remaining bits */

    if (i & disallow & ~(S_IWGRP | S_IWOTH))
      {
      /* FAILURE */

      snprintf(EMsg, 1024, "%s is writable by group or other",
               path);

      rc = EACCES;
      }
    }

chkerr:

  if (rc != 0)
    {
    if ((error_buf = calloc(1, LOG_BUF_SIZE)) == NULL)
      {
      if (chk_file_sec_stderr)
        {
        fprintf(stdout, "chk_tree: calloc failed: error #%d: (%s)\n",
                rc,
                strerror(rc) ? strerror(rc) : "UNKNOWN");
        }
      else
        {
        log_err(rc, "chk_file_sec", "calloc failed");
        }
      }
    else
      {
      if (EMsg[0] != '\0')
        {
        snprintf(error_buf, _POSIX_PATH_MAX, "Security violation with \"%s\" - %s",
                path,
                EMsg);
        }
      else
        {
        snprintf(error_buf, _POSIX_PATH_MAX, "Security violation with \"%s\", errno=%d, %s",
                path,
                rc,
                strerror(rc));
        }

      if (chk_file_sec_stderr)
        {
        fprintf(stdout, "chk_tree: %s: error #%d: (%s)\n",
                error_buf,
                rc,
                strerror(rc) ? strerror(rc) : "UNKNOWN");
        }
      else
        {
        log_err(rc, "chk_file_sec",
                error_buf);
        }

      free(error_buf);
      }
    }    /* END if (rc != 0) */

  return(rc);
  }  /* END chk_sec_file.c */

/* END chk_file_sec.c */
