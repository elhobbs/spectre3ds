#pragma once

typedef void(*keyboard_func)(void);

typedef struct {
	int x, y, type, dx, key;
	char *text, *shift_text;
	keyboard_func func;
} sregion_t;

void keyboard_init();
void keyboard_bind(int key, keyboard_func func);
void keyboard_draw();
void keyboard_input();
int keyboard_scankeys();
