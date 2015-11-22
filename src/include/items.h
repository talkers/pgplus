/*
 * Playground+ - items.h
 * All item defines
 * ---------------------------------------------------------------------------
 */

/* item flags */
#define INSIDE_PREVIOUS (1<<0)
#define WORN (1<<1)
#define WIELDED (1<<2)
#define OPEN_ITEM (1<<3)
#define ACTIVE (1<<4)
#define BROKEN (1<<5)

/* s_item flags */

#define NOT_READY_TO_BUY (1<<0)
#define EDIBLE (1<<1)
#define WEARABLE (1<<2)
#define WIELDABLE (1<<3)
#define CONTAINER (1<<4)
#define UNIQUE (1<<5)
#define PLAYTHING (1<<6)

/* unique happenings */
#define UNPURCHASEABLE (1<<7)
#define MAKE_BUILDER (1<<8)
#define MAKE_MINISTER (1<<9)
#define MAKE_SPOD (1<<10)
#define MAKE_PSU (1<<11)
#define MAKE_SU (1<<12)

/* more things you can do with a s_item */
#define DRINKABLE (1<<13)
