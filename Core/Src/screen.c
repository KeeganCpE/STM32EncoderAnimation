#include "GFX.h"
#include "screen.h"
#include "TSC2007.h"

void DrawScreen(GFX_t *gfx, Screen toGoTo, Frame currFrame) {
	switch(toGoTo) {
		case HOME_SCREEN:
			// Whole screen black
			GFX_FillScreen(gfx, GFX_BLACK);

			// Title
			GFX_DrawString(gfx, 55, 40, "Welcome", GFX_CYAN, GFX_BLACK, 3);
			GFX_DrawString(gfx, 65, 65, "to the", GFX_CYAN, GFX_BLACK, 3);
			GFX_DrawString(gfx, 30, 90, "Responsive", GFX_CYAN, GFX_BLACK, 3);
			GFX_DrawString(gfx, 25, 115, "Visualizer!", GFX_CYAN, GFX_BLACK, 3);

			// Rectangle for Data Screen button
			GFX_WritePerfRect(gfx, 20, 200, 220, 250, GFX_PURPLE);

			// Data Screen Text
			GFX_DrawString(gfx, 57, 217, "Data Screen", GFX_PURPLE, GFX_BLACK, 2);

			// Rectangle for Visualization Screen button
			GFX_WritePerfRect(gfx, 20, 260, 220, 310, GFX_PURPLE);

			// Data Screen Text
			GFX_DrawString(gfx, 45, 277, "Visualization", GFX_PURPLE, GFX_BLACK, 2);

			break;

		case DATA_SCREEN:

			// Whole screen black
			GFX_FillScreen(gfx, GFX_BLACK);

			// Rectangle to enclose data region
			GFX_WritePerfRect(gfx, 20, 10, 220, 250, GFX_CYAN);

			// Title
			GFX_DrawString(gfx, 55, 20, "Data Screen", GFX_CYAN, GFX_BLACK, 2);

			// 3 Data fields
			// Distance Traveled
			GFX_DrawString(gfx, 35, 70, "Distance Traveled:", GFX_WHITE, GFX_BLACK, 1);
			// Direction Traveling
			GFX_DrawString(gfx, 35, 130, "Direction Traveling:", GFX_WHITE, GFX_BLACK, 1);
			// Current Speed
			GFX_DrawString(gfx, 35, 190, "Speed:", GFX_WHITE, GFX_BLACK, 1);


			// Rectangle for Home Screen button
			GFX_WritePerfRect(gfx, 20, 260, 220, 310, GFX_PURPLE);

			// Data Screen Text
			GFX_DrawString(gfx, 50, 277, "Back to Home", GFX_PURPLE, GFX_BLACK, 2);

			break;

		case VISUALIZATION_SCREEN:

			// Whole screen black
			GFX_FillScreen(gfx, GFX_BLACK);

			// Rectangle to enclose visualization region
			GFX_WritePerfRect(gfx, 20, 10, 220, 250, GFX_CYAN);

			// Title
			GFX_DrawString(gfx, 35, 20, "Visualization", GFX_CYAN, GFX_BLACK, 2);

			// Rectangle for Home Screen button
			GFX_WritePerfRect(gfx, 20, 260, 220, 310, GFX_PURPLE);

			// Data Screen Text
			GFX_DrawString(gfx, 50, 277, "Back to Home", GFX_PURPLE, GFX_BLACK, 2);

			// Draw the visualization frame
			DrawFrame(gfx, currFrame);

			break;

		default:

	}
}

void DrawFrame(GFX_t *gfx, Frame toGoTo) {
	// Clear visualization area
	GFX_FillRectDMA(gfx, 55, 60, 131, 176, GFX_BLACK);

	switch(toGoTo) {
		case FIRST_FRAME:
			// Front face
			GFX_WriteRect(gfx, 65, 70, 175, 80, 175, 215, 65, 205, GFX_WHITE);

			// Right Face
			GFX_WriteRect(gfx, 175, 80, 185, 70, 185, 205, 175, 215, GFX_WHITE);

			// Top Face
			GFX_WriteRect(gfx, 65, 70, 75, 60, 185, 70, 175, 80, GFX_WHITE);

			break;

		case SECOND_FRAME:
			// Front face
			GFX_WriteRect(gfx, 85, 65, 155, 80, 155, 220, 85, 190, GFX_WHITE);

			// Right Face
			GFX_WriteRect(gfx, 155, 80, 170, 75, 170, 215, 155, 220, GFX_WHITE);

			// Top Face
			GFX_WriteRect(gfx, 85, 65, 95, 62, 170, 75, 155, 80, GFX_WHITE);

			break;
		case THIRD_FRAME:
			// Front face
			GFX_WriteRect(gfx, 105, 60, 135, 80, 135, 230, 105, 190, GFX_WHITE);

			// Top Face
			GFX_WriteRect(gfx, 105, 60, 120, 60, 155, 80, 135, 80, GFX_WHITE);

			// Right Face
			GFX_WriteRect(gfx, 135, 80, 155, 80, 155, 230, 135, 230, GFX_WHITE);

			break;
		case FOURTH_FRAME:
			// Front face
			GFX_WriteRect(gfx, 115, 80, 135, 80, 135, 235, 115, 235, GFX_WHITE);

			// Top Face
			GFX_WriteRect(gfx, 115, 80, 120, 60, 130, 60, 135, 80, GFX_WHITE);

			break;
		case FIFTH_FRAME:
		    // Front face
		    GFX_WriteRect(gfx, 135, 60, 105, 80, 105, 230, 135, 190, GFX_WHITE);

		    // Top Face
		    GFX_WriteRect(gfx, 135, 60, 120, 60, 85, 80, 105, 80, GFX_WHITE);

		    // Right Face (now becomes LEFT face)
		    GFX_WriteRect(gfx, 105, 80, 85, 80, 85, 230, 105, 230, GFX_WHITE);

		    break;

		case SIXTH_FRAME:
		    // Front face
		    GFX_WriteRect(gfx, 155, 65, 85, 80, 85, 220, 155, 190, GFX_WHITE);

		    // Right face (becomes left)
		    GFX_WriteRect(gfx, 85, 80, 70, 75, 70, 215, 85, 220, GFX_WHITE);

		    // Top face
		    GFX_WriteRect(gfx, 155, 65, 145, 62, 70, 75, 85, 80, GFX_WHITE);

		    break;

		case SEVENTH_FRAME:
		    // Front face
		    GFX_WriteRect(gfx, 175, 70, 65, 80, 65, 215, 175, 205, GFX_WHITE);

		    // Right face (becomes left)
		    GFX_WriteRect(gfx, 65, 80, 55, 70, 55, 205, 65, 215, GFX_WHITE);

		    // Top face
		    GFX_WriteRect(gfx, 175, 70, 165, 60, 55, 70, 65, 80, GFX_WHITE);

		    break;
	}
}

void CheckScreenPress(GFX_t *gfx, Screen *currPage, TSC2007_Point_t pt, Frame currFrame) {

	uint16_t y = pt.y;

	if (y > 2300 && y < 2750) {
		if (*currPage == HOME_SCREEN) {
			*currPage = DATA_SCREEN;
			DrawScreen(gfx, *currPage, currFrame);
		}
	} else if (y > 3000 && y < 3500) {

		switch (*currPage) {
		case HOME_SCREEN:
			*currPage = VISUALIZATION_SCREEN;
			DrawScreen(gfx, *currPage, currFrame);

			break;

		default:
			*currPage = HOME_SCREEN;
			DrawScreen(gfx, *currPage, currFrame);

			break;
		}

	}
}
