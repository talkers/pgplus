------------------------------- New Code ----------------------------------

Welcome to Playground+ version 1.0.x

Below is a list of additional things that have been added, bug fixed or
changed since Playground v5.0.0 in no particular order:

- Cleaned up presentation of code
- Added Linux Kernal style "make configure" option
- Removed the incrediably poor "plural" gender
- Code tells everyone "<name> sets a session comment" when one is set
- Renamed angel and talker processes to something more useful
- Fixed "weild" bug, "remove" bug, "trace" bug, "time last logged on" bug
- Fixed "backup" code
- Removed annoying "hit return" after setting terminal type on login
- Cleaned up lines to the more standard "------"
- Fixed the LSU "problem" (not so much of a bug but not good)
- Stopped admin being able to cheat at PRS
- Added pager and command counter to "commands ..."
- Made dibbs more useable
- Added a decent newbie inform for superusers
- Added admin sing (was it worth it?)
- Cleaned up presentation of admin and SU channels
- Added Grims intercom code
- Added subtle's anti-crash code
- Added dynamic staff list (woo woo)
- Added Oolons get_int_safe and get_string_safe code
- Stopped bug and idea logs from being trimmed
- Added "longest connection" to time and stats
- Cleaned up "res_count" - more informative for residents
- Added a disconnect message feature
- Allowed people to have 3 attempts at getting their password right
- Added a purge_list command to remove bad entries
- Added anti-spam code
- Removed annoying "Character saved" message
- Screenlock command now uses players own password
- Complicated PG channels removed
- Main talker channel added
- Spod channel added (uses both pu, pe, ps, pt AND du, de, ds, dt)
- Z Channels (user definable) added
- "Make" command fixed
- Added aliases so people can use TFO, RFO, SFO etc.
- Added support for Summink commands { and }
- Added number of times article has been read to "news list" & "snews list"
- Cleaned up "inventory","lso" and "lsa" 
- Changed "PG time" to "<talker name> time" in finger and examine
- Fixed "redtape" bug, "lsg" bug and "git" bug
- Fixed bug in res_count which reported more ressies than there was
- Cleaned up presentation of "pinfo"
- Fixed pager bug
- Added mailing list code
- FIFO logging code added. If logs get too big they don't get deleted but
   the just the first item removed (first in - first out)
- Added ability to turn timeouts on/off from "make config"
- Added login welcome message
- Fixed "nuke" bug
- Allowed user definable names for staff through "make config"
- Added code to prevent HCAdmins nuking other HCAdmins
- Fixed blockmsg bug (should be base command not HCAdmin only)
- Added auto-shutdown code
- Added choice of UK (DD-MM-YY) or US (MM-DD-YY) date formats
- Cleaned up "comments" command
- Added zchannel colour
- Added last connections code
- Added hourly clock command
- Added "muffle" command to silence channels, sessions and clock
- Fixed random logout bug
- Added new dynamic kRad social code
- Added new pager code
- Added prefixes to the logon and logoff messages
- Edited dependances on Makefile
- Added timeprompt code
- Sorted out alias splicing bug (well patched it)
- Removed duplicated session code and sorted presentation out
- Fixed "medit" bug
- Added global "proto.h" file contain standard list of externs
- Fixed Admin "transfer" bug -- tell_player("");  Okaaaaay ...?!?
- Removed unecessary third argument on "pager" function
- Large proportion of the bad language removed in the code -- tut tut ;o)
- Added dynatext code (see "help dynatext" for more info)
- Removed horrible "No such name - -" message because of sloppy coding
- Cleaned up presentation of "pinfo"
- Fixed newbie nuke bug
- Fixed "shutdown"/"abort" bug
- Cleaned, updated and mildly spell checked help files (took 3 days!!)
- Added Linux/DOS style help system - use "-h" or "/?" on any command
- Allowed newbies to pick whether they have colour and syscolour on
- Fixed the mailer bug
- Added extra help in "help room"
- Added code to stop talker being spammed with newbies
- Removed @ from LSU and changed main room to say "main" not "room"
- Dodgy recap changed so only the person can do it (stops abuse)
- Cleaned up pstats
- Fixed "who" bug from the login prompt
- Added ping code for checking to see how lagged a player is
- Added "grep" command for easy log searching
- Added TicTacToe (noughts and crosses) game
- Removed annoying code that prevented you from doing tells etc. to a
    person by their first letter only
- Cleaned up "privs" command
- Added Slaines robots code
- Merged "y" and "z" commands into the one "y"
- Added dynamic changing of command priv level
- Fixed "disconnect and change last seen" bug
- Fixed multiple sigpipe entries on dead angel bug
- Cleaned up some horrible memory leakage in the angel code
- Added a "muffle dynatext" option
- Added ability to set monetary type and description from config file
- Added ability to do a seamless reboot of the code (keeping the players
    still connected)
- Added option to enable partial command (and sub command) matching
- Added Athans and Oolons ident server for reverse lookups
- Added swear filtering code
- Fixed "where" and "login info" bugs
- Added "prefer" command and toggle
- Added "number of owners" to Ix
- Added code to timeout old items
- Fixed reset_session, cancel engagement, assist, dibbs, privs and 
    forcehelp bugs
- Made it so whole talker is told of proposal, engagement and rejection
- Changed momsg's so that you get a decent "you can't do that" message
- Added "number of new news/snews" to finger, examine and login info
- Removed "twall" and "ewall" becuase they're useless
- Set "news inform" on as default for new users
- A trailing ^^ will cancel a command
- Setting a blank logon and logoff message resorts to defaults
- Added "mquit" command for leaving a quick message after quitting in your
    finger.
- Added priv checking for "nomsgs", "off_duty" and "on_duty".
- Stopped admin making superusers go off_lsu on the staff list
- Fixed "barge" bug and sorted out the minister and builder bugs
- Viewing your own articles doesn't increase the "times read" counter.
- Removed the pointless "crash" command :o)
- Fixed the notorious login alias bad stack bug (thanks to Shaggy for the
    fix)
- Added astyanax's SU history command
- Added itrace command
- Allowed users to hit [ENTER] and try a different username
- Added saved warning (swarn) command
- Prevented users from sneakily logging in as "who"
- Corrected terminal definitions for vt100 and ANSI
- Stupid "Tis done..." messages replaced with something more informative 
- Fixed notorious "bedit" bug
- Allowed admin to kill the angel from within the talker
- Cleaned up format of mail postings
- Added "mail readsent" code
  -- 1.0.1 --
- Support for the always-seen-but-never-used sys_flags & VERBOSE is
    included as an option in the Makefiles - horay!
- Fixed missing prompts when logging in bug when using Tinyfugue
- Brand new dynamic spodlist (woo!)
- Grant available to PSU's and above with limits on what can be granted
- Fixed the "finger" and "examine" prefix bugs
- Add dynamic saved channels
- Removed "forest channel" priv and aliased all channels with forest
    equivilants
- Added support for multi's/chains for group communication
- Added support for 'debugging' mode (include additional 'verbose' mode) 
    including visual warnings from within the Makefile
- Soft messages support included allowing many messages and defaults to be
    edited and reloaded on the fly
- "vlog" lists only the logs which are available to be viewed
- Logs can have specific permissions required before they can be viewed
    (can be changed on the fly too)
- Changed logs are listed upon login for su's
- Added ability to define messages to the whole talker should specific
    things happen (in soft/config.msg)
- Added coder rank (no additional commands)
- Added "vemerg" and "vscript" commands for viewing emergency logs and
    scripts.
- Removed ancient "tfo <name>  <message>" bug (note 2 spaces).
- Configure script automatically works out path to the talker so you don't
    have to set it.
- Removed stupid prompt changing colours bug which annoyed the hell out of
    me :o)
- Added "average logins per hour and day" to time since most people seem
    to be adding the former to their code these days.
 -- 1.0.2 --
- enhanced lsr, added nice privs output line string for other calls
- enhanced dynatext to be able to refer to current_player, output player,
    random player in room, and random player on talker
- added newbie screening
- fixed stack bug in pinfo
- fixed missing #ifdef IDENT in link_to_program
- fixed birthday to take the format for time set in soft config
- fixed intercom examine and finger when informing
- fixed display bug on liblist
- fixed display bug on idle
- fixed display bug on nwho
- added 'uname' stat to version
- fixed rename log bug
- enhanced banish_player
 -- 1.0.3 --
- got rid of STANDARD_ROOMS residency define ....it was never actually 
    Set anywhere within the code..only checked for....it would Always 
    be nonexistant......
- fixed newbie screening bug
- fixed dud links to show only to room owner
- added aliases "adpost", "adnews", and "adread" for admin news group
- added log "ammend" command
- fixed IN_EDITOR bug
- added errors shown on varible argument functions with wrong format
- fixed obscure social bug
- fixed "help drugs" helpfile
- fixed display log for newconn
- fixed item delete bug
- fixed color bleed in various functions
- enhanced netstat (again)
- added "psx" to view process stats online
- added soft config for sus to recap others
- fixed blankpass, totally rewritten
- enhanced rooms, exits default to owner if not in format owner.id
- fixed small bugs in res_count
- enhanced bug and idea to echo the text back to the player
- added logging of failed help file requests
- fixed bug in multis when used as an alias
- fixed display bug in time when talker was up less than an hour
- added "blank_alias"
- added "fix_list"
- added ^^X and ^^x for random color
- added -f option to "reboot" to force it (even if others in editor)
 -- 1.0.4 --
- fixed color bleed when player did not have term info (mainly login screen)
- fixed debug priv incompatiblity
- fixed boot 'typo', missing sys_room_id call
- fixed news id problem for ppl posting at the same time
- fixed mail overflow bug
 -- 1.0.5 --
- added expandable search engine, fixes possible bugs in itrace/etrace,
    adds extra functionality for searching files
- fixed admin channel shortcuts availble to everyone
- fixed recap bug allowing anyone to recap anyone if sus_can_recap on
- fixed color bleed on 'where' with colored room names
- fixed fingering bug for show_when_xed at login
- prevented talker from booting if soft config files missing
- cleaned up log(), default log size if possibly missing
 -- 1.0.6 --
- fixed major legacy bug, user exploitable from suicide
 -- 1.0.7 --
- fixed many typographical errors and possible pfile corruption problems
 -- 1.0.8 --
- prevented talker being rebooted when in 'autoshutdown' mode
- prevented 'news followup' from showing previous private messages
- made intercom able to handle talkers with dynamic ip addresses
- removed several stack problems
- allowed users to use capitals in 'spods' command
 -- 1.0.9 --
- intercom fixed to prevent trailing ^^M's
- users password are accidently logged in a crash
- compilation flags corrected for redhat 5
- trailing . in the 'time' command removed
- gitting newbies used to make them residents, but not anymore
- socket code bug which meant that sigpipes could kill the talker fixed
---------------------------------------------------------------------------
