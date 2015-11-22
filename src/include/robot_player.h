
/*
 * robot.h
 */
 
/* flags for the robots */
#define WANDER (1<<0)
#define LOCAL_WANDER (1<<1)
#define INTELLIGENT (1<<2) /* NOT IMPLEMENTED!! */
#define STORED (1<<3)
#define FIXED (1<<4)

/* actions for robots list struct */
struct moves_struct
{
   char		   	move_string[IBUFFER_LENGTH-10];
   struct moves_struct 	*next;
};
typedef struct moves_struct move;

/* robot list + actions struct */
struct robot_struct
{
   char		   	lower_name[MAX_NAME];
   int		   	speed;
   int			counter;
   int		   	flags;
   int			max_moves;
   struct moves_struct	*moves_top;
   struct p_struct	*actual_player;
   struct robot_struct  *next;
};
typedef struct robot_struct robot;
