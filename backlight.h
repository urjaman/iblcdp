void backlight_init(void);
void backlight_set(uint8_t v);
void backlight_set_dv(uint8_t v);
void backlight_set_to(uint8_t to);
void backlight_activate(void);
void backlight_run(void);
uint8_t backlight_get(void);
uint8_t backlight_get_dv(void);
uint8_t backlight_get_to(void);
void backlight_set_contrast(uint8_t contrast);
uint8_t backlight_get_contrast(void);
#define CONTRAST_MAX 64
#define CONTRAST_MIN 0