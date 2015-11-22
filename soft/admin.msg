#                                                                 SoftMsg file
#
#   admin.msg
#  soft messages for admin.c, repelaces many compile time macros
#

# When staff is offduty and they try to do something staffish
go_on_duty:	Hey pal, get back on_duty first ...

# These arent relaly impletemented, but should be...;)
nojoy:		No such player in saved files ...
noton:		That person isn't on &t at the moment ...

# this needs exactly one %s where the plaeyrs name is substituted
sneezed:	-=*> %s is sneezed upon ... ewwwwwwww !

dragged:	-=*> You are dragged away by a thousand creepy crawlies ...

# this needs exactly one %s where the plaeyrs name is substituted
drag_msg:	-=*> %s starts crying as mommy grabs them away by the ear ...

rm_shout:	-=*> You throat starts burning and feels so very sore ...
un_rm_shout:	-=*> Some kind soul hands you a cough sweet ...

rm_sing:	-=*> You notice how out of tune you are and get highly embarrassed.
un_rm_sing:	-=*> Someone pays for your singing lessions ...

assist_msg:
	-=*> &c->n is a superuser, and would be
	     happy to assist you in any problems you may have, including
	     gaining residency, so that you can have your own rooms, and
	     have your character saved between visits (type 'help residency' 
	     for more information about that.)
	
	     To talk to &c->i, type 'tell &c->n <whatever>',
	     short forms of names usually work as well.
	
	


res_header:	
	+------------------------------------------------------------------------+
	|                    You have been made a resident!                      |
	|                                                                        |
	| Please answer a few short questions so that we can save your character |
	+------------------------------------------------------------------------+
	

