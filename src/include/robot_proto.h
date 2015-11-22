/* robot.c */
extern void		init_robots(void);
extern robot	       *robot_start;
extern void		run_action(robot *);
extern void		process_robot_counters(void);
extern void		process_robots(void);
extern void		trans_fn(player *, char *);
