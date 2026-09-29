#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "GFX.h"
#include "TSC2007.h"

typedef enum {
    HOME_SCREEN,
	DATA_SCREEN,
	VISUALIZATION_SCREEN
} Screen;

typedef enum {
    FIRST_FRAME,
	SECOND_FRAME,
	THIRD_FRAME,
	FOURTH_FRAME,
	FIFTH_FRAME,
	SIXTH_FRAME,
	SEVENTH_FRAME
} Frame;

void DrawScreen(GFX_t *gfx, Screen toGoTo, Frame currFrame);

void DrawFrame(GFX_t *gfx, Frame toGoTo);

void CheckScreenPress(GFX_t *gfx, Screen *currPage, TSC2007_Point_t pt, Frame currFrame);

#endif // _SCREEN_H_
