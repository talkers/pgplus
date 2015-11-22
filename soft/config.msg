#                                                                 SoftMsg file
#     config.msg
#  lotsa talker strings and such
#

##############################################################################
# Main Talker Configurations
#

#
# The name of your talker
#
talker_name:	Playground Plus

#
# The port number of which your talker will run on.
# NOTE: * This must be four digit number (recommend are 1200 - 9900)
#       * The intercom (if defined will run on the port number 1 below) so
#         make sure it is free first.
#       * If you change this whilst the talker is running it will change
#         the port!! Make sure that you set it to a port that is not in
#         use otherwise the talker will come crashing down!
port:		7777

#
# The admininistrators or talkers email address
#
talker_email:	pgplus@pgplus.ewtoo.org

#
# These are the names of the hard coded admins, seperated by a single space
# (you may have any number of names you wish, keep em on the same line tho)
#
hcadmins:       coder sysop admin

#
# Hard coded password for hc admin login without established chars
#
hc_passwd:	silly+rabbit

#
# This is the talker abbrevation on the intercom (if you use it)
#
intercom_abbr:	pgp

#
# This is the possible server alias for the site which your talker is
# running. If left blank then the MOTD will list the _actual_ server
# address.
#
# However if you are on 'kwop29.taolo0.cos_i.yhp-s-s.net.edu.org' and have
# a DNS alias of 'goodtalk.org' then you'll want to set the variable
# below to 'goodtalk.org'
#
site_alias:



##############################################################################
# Misc Low Level Configurations
#

#
# This is how many KiloBytes max for each individual log file
#
max_log_size:   5

#
# This is the Hour of the day in 24 hour time at which to do daily
# backups. (0 is midnight, 12 is noon, so 23 is 11pm and 3 is 3am)
# Set it to -1 if you do not want backups done daily
#
backup_hour:	3

#
# Set the following to 0 if you want timeouts OFF
# if you want timeouts ON, set it to the number of WEEKS
# a player has until time out
#
player_timeout:	12

#
# This is how many non-resident connection are allowed on at once from 
# the same site. This is to prevent connection / newbie spam (must be a number)
#
site_newbies:	7

#
# How many MINS emergency scripting lasts 
#
emergency_len:	3

#
# Set this to either US or UK for either mm/dd/yy or dd/mm/yy respectivly
#
time_format:	uk

#
# These are the options sent to the 'ps' shell command when 'psx'
# is used on the talker... placed here so that machines that use
# diffrent args can be tailored to fit...
# If you do not want to allow the 'psx' command to be used from
# the talker, set this to "off"  (without the quotes)
#
ps_options:	-uxc

#
# This is the command that is sent to system() to send off an email
# if you have email checking on... you should NOT modify this unless
# you know exactly what you are doing...
#
#   It needs exactly three %s   the first is where the players name
#   is substutited, the second is where there site is put, and the
#   third is where their email address thats been set goes
#
emailer_call:	mail -s 'Welcome message to %s@%s' %s < files/email_welcome.msg


##############################################################################
# Staff titles
#

#
# The general title by which staff are referred
#
staff_name:	Staff Member

#
# Staff names set that are shown on lsu
#
hc_name:	HC Admin
coder_name:	Coder
admin_name:	Admin
la_name:	Lower Admin
asu_name:	Advanced SU
su_name:	Superuser
psu_name:	Pseudo SU





##############################################################################
# Various (random and not-so-random) Strings
#

#
# The physical location of the talker
#
server_locate:	Anytown, PG

#
# The default login message for those without one set
#
def_login:	arrives on the scene in a brilliant flash of light

#
# And the default logout message for those without one set
#
def_logout:	is finished with playtime

#
# This is the string shown on login (at the end of the login process)
#
welcome_msg:	~ Welcome to &t - enjoy your stay ~

#
# This is the hourly time message for those seeing the clock
#  (requires exactly one %s where the time is substutited
#
hourly_clock:	-=*> BONG! The time is now %s

#
# These are the Made from strings on examine and finger shown by default
#
neuter_made:	Random bits of electronic flotsam.
male_made:	Snails, nails, and puppy dog tails.
female_made:	Sugar n' spice n' everything nice

#
# This is the message shown when you recieve new mail
#
new_mail:	You have new mail!

#
# This is the prompt for the pager after a command has been done
#
pager_prompt:	PAGER>

#
# This is your name for money
#
cash_name:	pennies

#
# This is your description of that money
#
cash_desc:	shiny gold and silver coins

#
# This is the text that is displayed when a dynatext string is badly
# formed.
#
bad_dynatext:	[spang!?]

#
# The following (logon_prefix, logon_suffix, logoff_prefix,
# logoff_suffix and welcome_msg) may be successfully be left blank
#
#
# This is the string that prepends any logins
#
logon_prefix:	++

#
# This goes after a login 
#
logon_suffix:	++        

#
# This one is before any logouts
#
logoff_prefix:	--

#
# The string after the logoff message
#
logoff_suffix:	--

#
# On 'colorize' this is the example string shown
#
ex_color_str:	The quick brown phreak !@#$%&*() 

# This is how long (in seconds) a player must wait between using socials...
#  Set it to 0 if you want to have a freeforall without any waiting..
#  Set it to -1 to disable socials completely
#
social_index:   3


##############################################################################
# Configure Flags (and related settings)
#

#
# Is online file editing avaible ?
#
can_edit_files:	yes

# Do we send a 'welcome' message whenever anyones email is changed?
#  (the file that is sent is files/email_welcome.msg)
#
do_email_check:	yes

#
#
# Do we use the True Spod Login Time on x/f and spodslist?
#
use_truespod:	no

#
# Spam type defines for the anti-spam code
#
# nospam         - this can be either "on" or "off"
# spam_repeat    - sensitivity of repeated commands (the higher the more
#                  likely to trigger. Recommended is 12)
# spam_different - sensitivity of different commands (the higher the more)
#                  likely to trigger. Recommended is 7 and less than above)
# spam_eject     - number of mins a player is sneezed if they spam
#
nospam:		on
spam_repeat:	12
spam_different:	7
spam_eject:	1

#
# Allow dynamic changing of the priv needed to use a command?
#
privs_change:	yes

#
# Enable the swear filtering code?
#
swear_filter:	yes

#
# Allow ppl to be fingered from login?
#
login_finger:	yes

#
# Have the talker screen newbies from boot (see help screen online for details)
#
screening:	no

#
# Automatically close to newbies when no sus are on?
#
# WARNING: Do not set to "yes" before you have made resident your HCAdmin
# characters! If you do then you won't be able to log in!
#
no_sus_no_new:	no

#
# Allow newbies to resident themselves when no sus are on?
#
allow_selfres:	no

#
# For proposals, marriages, rejections, etc ... set these flags yes or no
# to have the entire talker informed of such...
#
wall_marriage:	yes
wall_accept:	yes
wall_propose:	yes
wall_reject:	yes
wall_divorce:	no

#
# Is idling bad on the talker? (messages and booting for excessive)
#
idle_bad:	yes

#
# Are admin exempt from idleing out?
#
admin_ok_idle:	no

#
# Can the force command be used?
#
can_force:	no

#
# Do we want multis to report verbosely? 
# (that is, report all the players that are on them each time they are used)
#
verbose_multis:	yes

#
# Do we inform about a multi leaving existance?
#
multi_inform:	no

#
# If you want multis to idle out, set this to how many MINS they may
# go idle before being removed... set it to -1 to not have them idleout
#
multi_idle_out:	10

#
# Show a player when someone examines or fingers them?
#
show_when_xd:	no

#
# Do you want to 'look' when reconnecting?
#
reconnect_look:	yes

#
# Do we allow staff members to recap other ppls names?
#
sus_can_recap:	no

#
# Do we allow players to set their names in all caps?
#
capped_names:	no


##############################################################################
# Channel Setup
#

#
# Tag for admin channel
#
admin_chan:	<admin>

#
# Tag for su channel
#
su_chan:	<su>

#
# Tag for main channel
#
main_chan:	<main>

#
# Tag for spod channel
#
spod_chan:	<spod>

#
# Tag for debug channel
#
debug_chan:	<debug>

#
# If you have the hcadmin channel, this is the tag for it
#
hc_chan:	<hc>


##############################################################################
# Slots configs
#

#
# How much it costs to play slots
#
slot_wager:	2

#
# Use old style slots or vegas style?
#
slot_style:	vegas

#
# How much the player is favored, higher is more, between 1 and 100
# (its the chance in 100 that the player will get a free cherry)
#
player_favor:	15

#
# The initial pot value, and its reset when big jackpot is hit
#
initial_pot:	5000


