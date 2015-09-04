#include "sys.h"
#include "input.h"
#include <3ds.h>

typedef struct {
	char	*name;
	int		keynum;
	char	*strId;	// localized string id
} keyname_t;

// keys that can be set without a special name
static const char unnamedkeys[] = "*,-=./[\\]1234567890abcdefghijklmnopqrstuvwxyz";

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
keyname_t keynames[] =
{
	{"TAB",				K_TAB,				"#str_07018"},
	{"ENTER",			K_ENTER,			"#str_07019"},
	{"ESCAPE",			K_ESCAPE,			"#str_07020"},
	{"SPACE",			K_SPACE,			"#str_07021"},
	{"BACKSPACE",		K_BACKSPACE,		"#str_07022"},
	{"UPARROW",			K_UPARROW,			"#str_07023"},
	{"DOWNARROW",		K_DOWNARROW,		"#str_07024"},
	{"LEFTARROW",		K_LEFTARROW,		"#str_07025"},
	{"RIGHTARROW",		K_RIGHTARROW,		"#str_07026"},

	{"ALT",				K_ALT,				"#str_07027"},
	{"RIGHTALT",		K_RIGHT_ALT,		"#str_07027"},
	{"CTRL",			K_CTRL,				"#str_07028"},
	{"SHIFT",			K_SHIFT,			"#str_07029"},

	{"LWIN", 			K_LWIN, 			"#str_07030"},
	{"RWIN", 			K_RWIN, 			"#str_07031"},
	{"MENU", 			K_MENU, 			"#str_07032"},

	{"COMMAND",			K_COMMAND,			"#str_07033"},

	{"CAPSLOCK",		K_CAPSLOCK,			"#str_07034"},
	{"SCROLL",			K_SCROLL,			"#str_07035"},
	{"PRINTSCREEN",		K_PRINT_SCR,		"#str_07179"},
	
	{"F1", 				K_F1, 				"#str_07036"},
	{"F2", 				K_F2, 				"#str_07037"},
	{"F3", 				K_F3, 				"#str_07038"},
	{"F4", 				K_F4, 				"#str_07039"},
	{"F5", 				K_F5, 				"#str_07040"},
	{"F6", 				K_F6, 				"#str_07041"},
	{"F7", 				K_F7, 				"#str_07042"},
	{"F8", 				K_F8, 				"#str_07043"},
	{"F9", 				K_F9, 				"#str_07044"},
	{"F10", 			K_F10, 				"#str_07045"},
	{"F11", 			K_F11, 				"#str_07046"},
	{"F12", 			K_F12, 				"#str_07047"},

	{"INS", 			K_INS, 				"#str_07048"},
	{"DEL", 			K_DEL, 				"#str_07049"},
	{"PGDN", 			K_PGDN, 			"#str_07050"},
	{"PGUP", 			K_PGUP, 			"#str_07051"},
	{"HOME", 			K_HOME, 			"#str_07052"},
	{"END",				K_END,				"#str_07053"},

	{"MOUSE1", 			K_MOUSE1, 			"#str_07054"},
	{"MOUSE2", 			K_MOUSE2, 			"#str_07055"},
	{"MOUSE3", 			K_MOUSE3, 			"#str_07056"},
	{"MOUSE4", 			K_MOUSE4, 			"#str_07057"},
	{"MOUSE5", 			K_MOUSE5, 			"#str_07058"},
	{"MOUSE6", 			K_MOUSE6, 			"#str_07059"},
	{"MOUSE7", 			K_MOUSE7, 			"#str_07060"},
	{"MOUSE8", 			K_MOUSE8, 			"#str_07061"},

	{"MWHEELUP",		K_MWHEELUP,			"#str_07131"},
	{"MWHEELDOWN",		K_MWHEELDOWN,		"#str_07132"},

	{"JOY1", 			K_JOY1, 			"#str_07062"},
	{"JOY2", 			K_JOY2, 			"#str_07063"},
	{"JOY3", 			K_JOY3, 			"#str_07064"},
	{"JOY4", 			K_JOY4, 			"#str_07065"},
	{"JOY5", 			K_JOY5, 			"#str_07066"},
	{"JOY6", 			K_JOY6, 			"#str_07067"},
	{"JOY7", 			K_JOY7, 			"#str_07068"},
	{"JOY8", 			K_JOY8, 			"#str_07069"},
	{"JOY9", 			K_JOY9, 			"#str_07070"},
	{"JOY10", 			K_JOY10, 			"#str_07071"},
	{"JOY11", 			K_JOY11, 			"#str_07072"},
	{"JOY12", 			K_JOY12, 			"#str_07073"},
	{"JOY13", 			K_JOY13, 			"#str_07074"},
	{"JOY14", 			K_JOY14, 			"#str_07075"},
	{"JOY15", 			K_JOY15, 			"#str_07076"},
	{"JOY16", 			K_JOY16, 			"#str_07077"},
	{"JOY17", 			K_JOY17, 			"#str_07078"},
	{"JOY18", 			K_JOY18, 			"#str_07079"},
	{"JOY19", 			K_JOY19, 			"#str_07080"},
	{"JOY20", 			K_JOY20, 			"#str_07081"},
	{"JOY21", 			K_JOY21, 			"#str_07082"},
	{"JOY22", 			K_JOY22, 			"#str_07083"},
	{"JOY23", 			K_JOY23, 			"#str_07084"},
	{"JOY24", 			K_JOY24, 			"#str_07085"},
	{"JOY25", 			K_JOY25, 			"#str_07086"},
	{"JOY26", 			K_JOY26, 			"#str_07087"},
	{"JOY27", 			K_JOY27, 			"#str_07088"},
	{"JOY28", 			K_JOY28, 			"#str_07089"},
	{"JOY29", 			K_JOY29, 			"#str_07090"},
	{"JOY30", 			K_JOY30, 			"#str_07091"},
	{"JOY31", 			K_JOY31, 			"#str_07092"},
	{"JOY32", 			K_JOY32, 			"#str_07093"},

	{"AUX1", 			K_AUX1, 			"#str_07094"},
	{"AUX2", 			K_AUX2, 			"#str_07095"},
	{"AUX3", 			K_AUX3, 			"#str_07096"},
	{"AUX4", 			K_AUX4, 			"#str_07097"},
	{"AUX5", 			K_AUX5, 			"#str_07098"},
	{"AUX6", 			K_AUX6, 			"#str_07099"},
	{"AUX7", 			K_AUX7, 			"#str_07100"},
	{"AUX8", 			K_AUX8, 			"#str_07101"},
	{"AUX9", 			K_AUX9, 			"#str_07102"},
	{"AUX10", 			K_AUX10, 			"#str_07103"},
	{"AUX11", 			K_AUX11, 			"#str_07104"},
	{"AUX12", 			K_AUX12, 			"#str_07105"},
	{"AUX13", 			K_AUX13, 			"#str_07106"},
	{"AUX14", 			K_AUX14, 			"#str_07107"},
	{"AUX15", 			K_AUX15, 			"#str_07108"},
	{"AUX16", 			K_AUX16, 			"#str_07109"},

	{"KP_HOME",			K_KP_HOME,			"#str_07110"},
	{"KP_UPARROW",		K_KP_UPARROW,		"#str_07111"},
	{"KP_PGUP",			K_KP_PGUP,			"#str_07112"},
	{"KP_LEFTARROW",	K_KP_LEFTARROW, 	"#str_07113"},
	{"KP_5",			K_KP_5,				"#str_07114"},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW,	"#str_07115"},
	{"KP_END",			K_KP_END,			"#str_07116"},
	{"KP_DOWNARROW",	K_KP_DOWNARROW,		"#str_07117"},
	{"KP_PGDN",			K_KP_PGDN,			"#str_07118"},
	{"KP_ENTER",		K_KP_ENTER,			"#str_07119"},
	{"KP_INS",			K_KP_INS, 			"#str_07120"},
	{"KP_DEL",			K_KP_DEL, 			"#str_07121"},
	{"KP_SLASH",		K_KP_SLASH, 		"#str_07122"},
	{"KP_MINUS",		K_KP_MINUS, 		"#str_07123"},
	{"KP_PLUS",			K_KP_PLUS,			"#str_07124"},
	{"KP_NUMLOCK",		K_KP_NUMLOCK,		"#str_07125"},
	{"KP_STAR",			K_KP_STAR,			"#str_07126"},
	{"KP_EQUALS",		K_KP_EQUALS,		"#str_07127"},

	{"PAUSE",			K_PAUSE,			"#str_07128"},
	
	{"SEMICOLON",		';',				"#str_07129"},	// because a raw semicolon separates commands
	{"APOSTROPHE",		'\'',				"#str_07130"},	// because a raw apostrophe messes with parsing

	{NULL,				0,					NULL}
};

static const unsigned char s_scantokey[256] = { 
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        '-',    '=',          K_BACKSPACE, 9, // 0
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '[',       ']',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      ';', 
	'\'',       '`',    K_SHIFT,   '\\',       'z',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '/',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           0,        K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,      // 7
// shifted
	0,           27,    '!',       '@',        '#',    '$',         '%',      '^', 
	'&',        '*',    '(',       ')',        '_',    '+',          K_BACKSPACE, 9, // 0
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '[',       ']',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      ';', 
	'\'',       '~',    K_SHIFT,   '\\',       'z',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '/',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           0,        K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 


/*
=======
MapKey

Map from windows to Doom keynums
=======
*/
int MapKey (int key, bool shift)
{
	int result;
	int modified;
	bool is_extended;

	return key;

	modified = ( key >> 16) & 255;

	if ( modified > 127 )
		return 0;

	if ( key & ( 1 << 24 ) ) {
		is_extended = true;
	}
	else {
		is_extended = false;
	}

	//Check for certain extended character codes.
	//The specific case we are testing is the numpad / is not being translated
	//properly for localized builds.
	if(is_extended) {
		switch(modified) {
			case 0x35: //Numpad /
				return K_KP_SLASH;
		}
	}

	const unsigned char *scanToKey = &s_scantokey[shift ? 128 : 0];
	result = scanToKey[modified];

	// common->Printf( "Key: 0x%08x Modified: 0x%02x Extended: %s Result: 0x%02x\n", key, modified, (is_extended?"Y":"N"), result);

	if ( is_extended ) {
		switch ( result )
		{
		case K_PAUSE:
			return K_KP_NUMLOCK;
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		case K_KP_STAR:
			return K_PRINT_SCR;
		case K_ALT:
			return K_RIGHT_ALT;
		}
	}
	else {
		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		}
	}

	return result;
}

/*ACTION_NONE = 0,
ACTION_FORWARD,
ACTION_BACKWARD,
ACTION_LEFT,
ACTION_RIGHT,
ACTION_JUMP,
ACTION_ATTACK,
ACTION_STRAFE_LEFT,
ACTION_STRAFE_RIGHT,
ACTION_UP,
ACTION_DOWN,
ACTION_SPEED,
ACTION_MOVE_AWAY,
ACTION_MOVE_NEAR,
ACTION_MAX_NUM*/

const action_t actions[] = {
	{ "", ACTION_NONE },
	{ "forward", ACTION_FORWARD },
	{ "backward", ACTION_BACKWARD },
	{ "left", ACTION_LEFT },
	{ "right", ACTION_RIGHT },
	{ "jump", ACTION_JUMP },
	{ "attack", ACTION_ATTACK },
	{ "strafeleft", ACTION_STRAFE_LEFT },
	{ "straferight", ACTION_STRAFE_RIGHT },
	{ "up", ACTION_UP },
	{ "down", ACTION_DOWN },
	{ "speed", ACTION_SPEED },
	{ "moveaway", ACTION_MOVE_AWAY },
	{ "movenear", ACTION_MOVE_NEAR }
};

sEvHandler::sEvHandler(void) {
	memset(m_keys,0,sizeof(m_keys));
	memset(m_actions,0,sizeof(m_actions));
}

int sEvHandler::handleEvent(event_t ev) {
	bool down = false;
	int key;

	ACTIONTYPE action;
	switch(ev.type) {
	case EV_MOUSE_BUTTON:
		for (int i = 0; i < 8; i++) {
			key = K_MOUSE1+i;
			action = m_keys[key].action;
			if (action == ACTION_NONE) {
				return 0;
			}
			down = (ev.value & (1 << i)) ? true : false;
			m_actions[action] = down ? 1 : 0;
		}
		break;
	case EV_KEY_DOWN:
		down = true;
	case EV_KEY_UP:
		key = MapKey(ev.value,false);
		action = m_keys[key].action;
		if(action == ACTION_NONE) {
			return 0;
		}
		m_actions[action] = down ? 1 : 0;
		return 1;
	}
	return 0;
}

bool sEvHandler::actionState(ACTIONTYPE action) {
	if(action <= ACTION_NONE || action >= ACTION_MAX_NUM) {
		return false;
	}
	return m_actions[action] > 0 ? true : false;
}

int sEvHandler::bind(char *key,char *action) {
	int num;
	int keynum = 0;
	ACTIONTYPE actionnum = ACTION_NONE;

	if(key[1] == 0) {
		num = sizeof(unnamedkeys);
		ACTIONTYPE actionnum = ACTION_NONE;
		for(int i=0;i<num;i++) {
			if(unnamedkeys[i] == key[0]) {
				keynum = key[0];
				break;
			}
		}
	}
	if(keynum == 0) {
		num = sizeof(keynames)/sizeof(keyname_t) - 1;
		for(int i=0;i<num;i++) {
			if(strcmp(keynames[i].name,key) == 0) {
				keynum = keynames[i].keynum;
				break;
			}
		}
	}
	num = sizeof(actions)/sizeof(action_t);
	for(int i=0;i<num;i++) {
		if(strcasecmp(actions[i].name,action) == 0) {
			actionnum = actions[i].type;
			break;
		}
	}

	if(keynum && actionnum) {
		m_keys[keynum].action = actionnum;
		return 0;
	}

	return -1;
}

void sEvHandler::save(FILE *f) {
	if (f == 0) {
		return;
	}
	int num = sizeof(keynames) / sizeof(keyname_t) - 1;
	for (int i = 0; i<num; i++) {
		int key = keynames[i].keynum;
		int action = m_keys[key].action;
		if (action) {
			//printf("key %d %d %d %s %s\n", i, key, action, keynames[i].name, actions[action].name);
			fprintf(f, "bind %s %s\n", keynames[i].name, actions[action].name);
		}
	}
	num = sizeof(unnamedkeys);
	ACTIONTYPE actionnum = ACTION_NONE;
	for (int i = 0; i<num; i++) {
		int key = unnamedkeys[i];
		int action = m_keys[key].action;
		if (action) {
			//printf("key %d %d %d %s %s\n", i, key, action, keynames[i].name, actions[action].name);
			fprintf(f, "bind %c %s\n", unnamedkeys[i], actions[action].name);
		}
	}
}