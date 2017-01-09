void rgbbl_init(void);
void rgbbl_off(void);

/* 0bRGB */
enum colors {
	C_BLUE = 1,
	C_GREEN = 2,
	C_CYAN = 3,
	C_RED = 4,
	C_MAGENTA = 5,
	C_YELLOW = 6,
	C_WHITE = 7
};

void rgbbl_set_intensity(uint8_t in);
void rgbbl_set_color(enum colors color);


/* for debug purposes. */
void rgbbl_set(uint8_t r, uint8_t g, uint8_t b);

