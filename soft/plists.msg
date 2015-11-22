#                                                                 SoftMsg file
#
# plists.msg
#  general messages and strings found throughout plists.c
#  (minus player defaults, those found in pdefaults.msg)
#

# Initial prompt to enter name at login
initial_name:	Please enter a name: 

# Secondard prompt to enter name
second_name:	Please enter a new name:

# If they try to enter a name thats too long
too_long_name:	Sorry, that name is too long, please enter something shorter.
	

# If they try to enter a name thats too short
too_short_name:	Thats a bit short, try something longer.
	

# If they type 'quit' at login
login_quit:	Goodbye!


# The message gotten just after name is entered...
# (needs exactly one %s where the name is substitued)
char_exists:	
	Character '%s' currently exists.
	
	

# The password prompt for player login
passwd_prompt:	Please enter your password (press ENTER for a different name):


# If they farg up there password 3 times they see this...
passwd_fail:	
	Nope, that's still not right...
	

# If they are sneezed, this is the lil message they see when try to login
# (needs exactly one %s where how much longer they have to wait is sub'd)
sneeze_login:
	Sorry, you have been prevented from logging on 
	for another %s  (and counting ...)
	

# If talker is gonna reboot in less than 3 mins, show them this message
talker_rebooting:	
	We are sorry, but &t is currently rebooting for system maintenance.
	We will return in less than 3 minutes, so please try again then.
	


# Message to ask if they are sure about there name
# (requires exactly one %s, where the name is substitued)
name_confirm:	
	You entered the name '%s' when you first logged in.
	Is this the name that you wish to be known as on &t
	

# Seen if they want to change there name
new_name_msg:	Okay, please enter a new name ...
	


# Prompt player gets when they should hit return to continue
hit_return:	Hit <RETURN> to continue: 

# New reconnect string ...
new_reconnect:	
	Restoring your connection to &t ...
	

# Old reconnect string ...
old_reconnect:	
	You were already logged onto &t !!
	Closing other connection.
	

# Message shown when takes too long at login
too_long_time:	
	Connection Timed Out ...
	

# Message seen by staff when a newbie is logging in
# (requires exactly TWO %s ...first is where name is substituted,
#  second is where site is substituted)
newbie_login:	
	-=*> ARROGAH! ARROGAH! NEWBIE ALERT!
	-=*> '%s' is logging in from %s
	

# Message player gets about gendef (in login process)
gender_msg:	
	The program requires that you enter your gender.
	This is used solely for the purposes of correct english and grammer.
	If you object to this, then simply type 'n' for not applicable.
	
	Enter (M)ale, (F)emale or (N)ot applicable:


# Messages shown when try to save a player but missing
# either password or email
no_save_pass:	
	Tried to save this character but failed ...
	Your character will not save until you set a password.
	Simply type 'password' whilst in command mode to set one.
	

no_save_email:	
	Tried to save this character but failed ...
	Your character will not save until you set an email address.
	To set this just type 'email <str>', where <str> is your email address.
	If you do not have an email, please speak to one of the superusers.
	

# The prompt the player recieves after disclaimer message
disclaimer_prompt:	Enter 'continue' or 'end' : 

# The string the player must enter to accept the disclaimer
disclaimer_yes:		continue

# The string the player must enter to reject the disclaimer
disclaimer_no:		end


# The inform message for those that have inform <player> or whutever,
# requires exactly TWO %s, one for the name, the other where
# the rest of the inform info is put
inform_login:		[%s joins us] %s

inform_reconnect:	[%s reconnects] %s

inform_logout:		[%s has left] %s

