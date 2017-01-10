void buttons_init(void);
uint8_t buttons_get(void);

#define BUTTON_CANCEL 8
/* Use the OK later on ... BOTH is a old compat thing... */
#define BUTTON_BOTH 4
#define BUTTON_OK BUTTON_BOTH
#define BUTTON_S1 2
#define BUTTON_S2 1
#define BUTTON_NONE 0
#define BUTTON_NEXT BUTTON_S1
#define BUTTON_PREV BUTTON_S2

#define buttons_hw_count() 4
