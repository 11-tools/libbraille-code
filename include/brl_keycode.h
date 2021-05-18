/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _BRL_KEYCODE_H
#define _BRL_KEYCODE_H

typedef enum {
	/* The keyboard syms have been cleverly chosen to map to ASCII */
	BRLK_UNKNOWN		= 0,
	BRLK_NONE		= 0,
	BRLK_BACKSPACE		= 8,
	BRLK_TAB		= 9,
	BRLK_CLEAR		= 12,
	BRLK_RETURN		= 13,
	BRLK_PAUSE		= 19,
	BRLK_ESCAPE		= 27,
	BRLK_SPACE		= 32,
	BRLK_EXCLAIM		= 33,
	BRLK_QUOTEDBL		= 34,
	BRLK_HASH		= 35,
	BRLK_DOLLAR		= 36,
	BRLK_AMPERSAND		= 38,
	BRLK_QUOTE		= 39,
	BRLK_LEFTPAREN		= 40,
	BRLK_RIGHTPAREN		= 41,
	BRLK_ASTERISK		= 42,
	BRLK_PLUS		= 43,
	BRLK_COMMA		= 44,
	BRLK_MINUS		= 45,
	BRLK_PERIOD		= 46,
	BRLK_SLASH		= 47,
	BRLK_0			= 48,
	BRLK_1			= 49,
	BRLK_2			= 50,
	BRLK_3			= 51,
	BRLK_4			= 52,
	BRLK_5			= 53,
	BRLK_6			= 54,
	BRLK_7			= 55,
	BRLK_8			= 56,
	BRLK_9			= 57,
	BRLK_COLON		= 58,
	BRLK_SEMICOLON		= 59,
	BRLK_LESS		= 60,
	BRLK_EQUALS		= 61,
	BRLK_GREATER		= 62,
	BRLK_QUESTION		= 63,
	BRLK_AT			= 64,
	/* 
	   Skip uppercase letters
	 */
	BRLK_LEFTBRACKET	= 91,
	BRLK_BACKSLASH		= 92,
	BRLK_RIGHTBRACKET	= 93,
	BRLK_CARET		= 94,
	BRLK_UNDERSCORE		= 95,
	BRLK_BACKQUOTE		= 96,
	BRLK_a			= 97,
	BRLK_b			= 98,
	BRLK_c			= 99,
	BRLK_d			= 100,
	BRLK_e			= 101,
	BRLK_f			= 102,
	BRLK_g			= 103,
	BRLK_h			= 104,
	BRLK_i			= 105,
	BRLK_j			= 106,
	BRLK_k			= 107,
	BRLK_l			= 108,
	BRLK_m			= 109,
	BRLK_n			= 110,
	BRLK_o			= 111,
	BRLK_p			= 112,
	BRLK_q			= 113,
	BRLK_r			= 114,
	BRLK_s			= 115,
	BRLK_t			= 116,
	BRLK_u			= 117,
	BRLK_v			= 118,
	BRLK_w			= 119,
	BRLK_x			= 120,
	BRLK_y			= 121,
	BRLK_z			= 122,
	BRLK_DELETE		= 127,
	/* End of ASCII mapped keysyms */

	/* International keyboard syms */
	BRLK_WORLD_0		= 160,		/* 0xA0 */
	BRLK_WORLD_1		= 161,
	BRLK_WORLD_2		= 162,
	BRLK_WORLD_3		= 163,
	BRLK_WORLD_4		= 164,
	BRLK_WORLD_5		= 165,
	BRLK_WORLD_6		= 166,
	BRLK_WORLD_7		= 167,
	BRLK_WORLD_8		= 168,
	BRLK_WORLD_9		= 169,
	BRLK_WORLD_10		= 170,
	BRLK_WORLD_11		= 171,
	BRLK_WORLD_12		= 172,
	BRLK_WORLD_13		= 173,
	BRLK_WORLD_14		= 174,
	BRLK_WORLD_15		= 175,
	BRLK_WORLD_16		= 176,
	BRLK_WORLD_17		= 177,
	BRLK_WORLD_18		= 178,
	BRLK_WORLD_19		= 179,
	BRLK_WORLD_20		= 180,
	BRLK_WORLD_21		= 181,
	BRLK_WORLD_22		= 182,
	BRLK_WORLD_23		= 183,
	BRLK_WORLD_24		= 184,
	BRLK_WORLD_25		= 185,
	BRLK_WORLD_26		= 186,
	BRLK_WORLD_27		= 187,
	BRLK_WORLD_28		= 188,
	BRLK_WORLD_29		= 189,
	BRLK_WORLD_30		= 190,
	BRLK_WORLD_31		= 191,
	BRLK_WORLD_32		= 192,
	BRLK_WORLD_33		= 193,
	BRLK_WORLD_34		= 194,
	BRLK_WORLD_35		= 195,
	BRLK_WORLD_36		= 196,
	BRLK_WORLD_37		= 197,
	BRLK_WORLD_38		= 198,
	BRLK_WORLD_39		= 199,
	BRLK_WORLD_40		= 200,
	BRLK_WORLD_41		= 201,
	BRLK_WORLD_42		= 202,
	BRLK_WORLD_43		= 203,
	BRLK_WORLD_44		= 204,
	BRLK_WORLD_45		= 205,
	BRLK_WORLD_46		= 206,
	BRLK_WORLD_47		= 207,
	BRLK_WORLD_48		= 208,
	BRLK_WORLD_49		= 209,
	BRLK_WORLD_50		= 210,
	BRLK_WORLD_51		= 211,
	BRLK_WORLD_52		= 212,
	BRLK_WORLD_53		= 213,
	BRLK_WORLD_54		= 214,
	BRLK_WORLD_55		= 215,
	BRLK_WORLD_56		= 216,
	BRLK_WORLD_57		= 217,
	BRLK_WORLD_58		= 218,
	BRLK_WORLD_59		= 219,
	BRLK_WORLD_60		= 220,
	BRLK_WORLD_61		= 221,
	BRLK_WORLD_62		= 222,
	BRLK_WORLD_63		= 223,
	BRLK_WORLD_64		= 224,
	BRLK_WORLD_65		= 225,
	BRLK_WORLD_66		= 226,
	BRLK_WORLD_67		= 227,
	BRLK_WORLD_68		= 228,
	BRLK_WORLD_69		= 229,
	BRLK_WORLD_70		= 230,
	BRLK_WORLD_71		= 231,
	BRLK_WORLD_72		= 232,
	BRLK_WORLD_73		= 233,
	BRLK_WORLD_74		= 234,
	BRLK_WORLD_75		= 235,
	BRLK_WORLD_76		= 236,
	BRLK_WORLD_77		= 237,
	BRLK_WORLD_78		= 238,
	BRLK_WORLD_79		= 239,
	BRLK_WORLD_80		= 240,
	BRLK_WORLD_81		= 241,
	BRLK_WORLD_82		= 242,
	BRLK_WORLD_83		= 243,
	BRLK_WORLD_84		= 244,
	BRLK_WORLD_85		= 245,
	BRLK_WORLD_86		= 246,
	BRLK_WORLD_87		= 247,
	BRLK_WORLD_88		= 248,
	BRLK_WORLD_89		= 249,
	BRLK_WORLD_90		= 250,
	BRLK_WORLD_91		= 251,
	BRLK_WORLD_92		= 252,
	BRLK_WORLD_93		= 253,
	BRLK_WORLD_94		= 254,
	BRLK_WORLD_95		= 255,		/* 0xFF */

	/* Numeric keypad */
	BRLK_KP0		= 256,
	BRLK_KP1		= 257,
	BRLK_KP2		= 258,
	BRLK_KP3		= 259,
	BRLK_KP4		= 260,
	BRLK_KP5		= 261,
	BRLK_KP6		= 262,
	BRLK_KP7		= 263,
	BRLK_KP8		= 264,
	BRLK_KP9		= 265,
	BRLK_KP_PERIOD		= 266,
	BRLK_KP_DIVIDE		= 267,
	BRLK_KP_MULTIPLY	= 268,
	BRLK_KP_MINUS		= 269,
	BRLK_KP_PLUS		= 270,
	BRLK_KP_ENTER		= 271,
	BRLK_KP_EQUALS		= 272,

	/* Arrows + Home/End pad */
	BRLK_UP			= 273,
	BRLK_DOWN		= 274,
	BRLK_RIGHT		= 275,
	BRLK_LEFT		= 276,
	BRLK_INSERT		= 277,
	BRLK_HOME		= 278,
	BRLK_END		= 279,
	BRLK_PAGEUP		= 280,
	BRLK_PAGEDOWN		= 281,

	/* Function keys */
	BRLK_F1			= 282,
	BRLK_F2			= 283,
	BRLK_F3			= 284,
	BRLK_F4			= 285,
	BRLK_F5			= 286,
	BRLK_F6			= 287,
	BRLK_F7			= 288,
	BRLK_F8			= 289,
	BRLK_F9			= 290,
	BRLK_F10		= 291,
	BRLK_F11		= 292,
	BRLK_F12		= 293,
	BRLK_F13		= 294,
	BRLK_F14		= 295,
	BRLK_F15		= 296,

	/* Key state modifier keys */
	BRLK_NUMLOCK		= 300,
	BRLK_CAPSLOCK		= 301,
	BRLK_SCROLLOCK		= 302,
	BRLK_RSHIFT		= 303,
	BRLK_LSHIFT		= 304,
	BRLK_RCTRL		= 305,
	BRLK_LCTRL		= 306,
	BRLK_RALT		= 307,
	BRLK_LALT		= 308,
	BRLK_RMETA		= 309,
	BRLK_LMETA		= 310,
	BRLK_LSUPER		= 311,		/* Left "Windows" key */
	BRLK_RSUPER		= 312,		/* Right "Windows" key */
	BRLK_MODE		= 313,		/* "Alt Gr" key */
	BRLK_COMPOSE		= 314,		/* Multi-key compose key */

	/* Miscellaneous function keys */
	BRLK_HELP		= 315,
	BRLK_PRINT		= 316,
	BRLK_SYSREQ		= 317,
	BRLK_BREAK		= 318,
	BRLK_MENU		= 319,
	BRLK_POWER		= 320,		/* Power Macintosh power key */
	BRLK_EURO		= 321,		/* Some european keyboards */

	/* Add any other keys here */
	BRLK_BACKWARD           = 401,        /* go left one full window */
	BRLK_FORWARD            = 402,        /* go right one full window */
	BRLK_DOT7               = 403,
	BRLK_DOT8               = 404,

	BRLK_ACCORD_a           = 411,
	BRLK_ACCORD_b           = 412,
	BRLK_ACCORD_c           = 413,
	BRLK_ACCORD_d           = 414,
	BRLK_ACCORD_e           = 415,
	BRLK_ACCORD_f           = 416,
	BRLK_ACCORD_g           = 417,
	BRLK_ACCORD_h           = 418,
	BRLK_ACCORD_i           = 419,
	BRLK_ACCORD_j           = 420,
	BRLK_ACCORD_k           = 421,
	BRLK_ACCORD_l           = 422,
	BRLK_ACCORD_m           = 423,
	BRLK_ACCORD_n           = 424,
	BRLK_ACCORD_o           = 425,
	BRLK_ACCORD_p           = 426,
	BRLK_ACCORD_q           = 427,
	BRLK_ACCORD_r           = 428,
	BRLK_ACCORD_s           = 429,
	BRLK_ACCORD_t           = 430,
	BRLK_ACCORD_u           = 431,
	BRLK_ACCORD_v           = 432,
	BRLK_ACCORD_w           = 433,
	BRLK_ACCORD_x           = 434,
	BRLK_ACCORD_y           = 435,
	BRLK_ACCORD_z           = 436,

	BRLK_ABOVE,     // Read line above (without moving cursor)
	BRLK_BELOW,   // Read line below (without moving cursor)
	//TODO: move close to BACKWARD - FORWARD

	BRLK_LAST
} brl_keycode;

#endif
