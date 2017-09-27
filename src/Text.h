//Text.h.

#ifndef PROJECT_MOSAIC_TEXT_H
#define PROJECT_MOSAIC_TEXT_H

#include <string>

//Routines for doing math on aspects of text.
int Bitmap_String_Total_Line_Width(const std::string &text);
int Bitmap_String_Max_Line_Height(void);


//Routines for drawing plain, simple text.
void Bitmap_Draw_String(float x, float y, float z, const std::string &text);
void Stroke_Draw_String(float x, float y, const std::string &text, float scale);


#endif
