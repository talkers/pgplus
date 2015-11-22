/*
 * Playground+ - admin.h
 * Information about HCAdmins and privs
 * --------------------------------------------------------------------------
 */
 
 
const char *HCAdminList[] = { "admin", "coder", "sysop", 0};

/* The actual privs - note that a good proportion of them have been ditched
   becuase they were dodgy when used from here or have specific commands for
   them. */

flag_list       permission_list[] = {
   {"admin",		ADMIN | LOWER_ADMIN | ASU | SU | PSU | WARN | DUMB},
   {"admin_channel",	ADC},
   {"asu",		SU | ASU | PSU | WARN | DUMB},
   {"base",		BASE},
   {"build",		BUILD},
   {"builder",		BUILDER},
   {"coder",		CODER},/* grant admin/lower_admin/asu/etc, then coder */
   {"condom",		PROTECT},
   {"creator",		SPECIALK},
   {"debug",		DEBUG},
   {"echo",		ECHO_PRIV},
   {"hcadmin",		HCADMIN|ADMIN|LOWER_ADMIN|ASU|SU|PSU|WARN|DUMB},
   {"house",		HOUSE},
   {"list",		LIST},
   {"lower_admin",	LOWER_ADMIN | SU | ASU | PSU | WARN | DUMB},
   {"mail",		MAIL},
   {"minister",		MINISTER},
   {"nosync",		NO_SYNC},
   {"no_timeout",	NO_TIMEOUT},
   {"psu",		PSU},
   {"residency",	BASE | BUILD | LIST | ECHO_PRIV | MAIL | SESSION },
#ifdef ROBOTS
   {"robot",		ROBOT_PRIV},
#endif
   {"script",		SCRIPT},
   {"session",		SESSION},
   {"spod",		SPOD},
   {"sysroom",		SYSTEM_ROOM},
   {"trace",		TRACE},
   {"su",		SU | PSU | WARN | DUMB},
   {"warn",		WARN},
   {"", 0}
};


/* These is the privs list containing the single priv that is required 
   to grant from the above array
*/
flag_list       perm_required[] =
{
      /* staff privs */
   {"hcadmin",		HCADMIN},
   {"coder",		HCADMIN},
   {"admin",		HCADMIN},
   {"admin_channel",	LOWER_ADMIN},
   {"lower_admin",	ADMIN},
   {"asu",		ADMIN},
   {"su",		LOWER_ADMIN},
   {"psu",		LOWER_ADMIN},
   {"warn",		HCADMIN},

      /* misc privs */
   {"creator",		ASU},
   {"builder",		ASU},
   {"debug",		HCADMIN},
   {"spod",		ASU},
   {"condom",		HCADMIN},
   {"house",		LOWER_ADMIN},
   {"minister",		ADMIN},
   {"script",		LOWER_ADMIN},
   {"trace",		LOWER_ADMIN},
   {"no_timeout",	LOWER_ADMIN},
#ifdef ROBOTS
   {"robot",		HCADMIN},
#endif

     /* safe res privs */
   {"session",		LOWER_ADMIN},
   {"echo",		LOWER_ADMIN},

      /* These should NEVER by granted or removed via the
         "grant" command since it could fargle pfiles.
	 However it is required in order for code to work */
   {"residency",	HCADMIN},
   {"base",		HCADMIN},
   {"build",		HCADMIN},
   {"list",		HCADMIN},
   {"mail",		HCADMIN},
   {"nosync",		HCADMIN},
   {"sysroom",		HCADMIN},
   {"", 0}
};
