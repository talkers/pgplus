/*
 * Playground+ - aliases.h
 * Aliases for the internal library
 * ---------------------------------------------------------------------------
 */

alias_library library[] = {

	{"all", "tlist %1 noisy,inform,grabme,friend,invite,beep,key,find,shareroom", "Toggles ALL the good flags on someone.\nFormat : all <player(s)>", "idea from void", 0}, 
	{"bop", ";bops you with a %{rusty@slimy@green@wet@moldy} %{spoon@fork@frog@piano@fish@carrot@stick} |%1", "Bop a person in the same room with a random item.\nFormat : bop <player>", "traP", 0},
	{"frin", "tlist %1 inform,friend", "Toggles friend and inform at the same time on someone.\nFormat : frin <player(s)>", "idea from void", 0}, 
	{"grin", ";grins at you. =) |%1", "Grin at a user.\nFormat : grin <someone in the same room>", "traP", 0},
	{"rbop", "<%1 bops you with a %{rusty@slimy@green@wet@moldy} %{spoon@fork@frog@piano@fish@carrot@stick}", "Bop a silly person anywhere with a random item.\nFormat : rbop <player>", "traP", 0},
	{"resq", ">%1 Do you have a valid email address, and have you read the rules?%;su obj %1?", "Asks a newbie if they have email and read the rules.\nFormat : resq <player>", "traP", 1},
	{"rsnog", "<%1 snogs you deeply, until you can hardly breathe.", "Snog someone... sort of a real passionate kiss.\nFormat : rsnog <lovebird>", "traP", 0},
	{"rsog", "<%1 sogs you wetly on the nose -- ewwwwww!", "Sog someone... A real wet lick on the nose \nFormat : rsog <lovebird>", "traP", 0},
	{"sendmail", "mail post %1 Random subject # %{1@6@9@3}%{2@6@8@1}%{4@9@8@0}%{3@2@4@5@1}", "Send mail to someone with a random title\nFormat : sendmail <player>", "traP", 0}, 
	{"snog", ";snogs you deeply, leaving you breathless|%1", "Snog someone... sort of a real passionate kiss.\nFormat : snog <lovebird>", "traP", 0},
	{"sog", ";runs up and sogs you wetly on the nose -- ewwwwww!", "Sog someone... A real wet lick on the nose \nFormat : sog <lovebird>", "traP", 0},
	{"www", "url http://%1/%2", "Sets your home page.. \nFormat : www <site of your server> <Path to your file>\nExample: www.w3.org ~web", "traP", 0},
	{0,0,0,0,0}
};

alias_library no_library_here = {0, 0, 0, 0, 0};

