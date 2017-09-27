//Text.cc.

#include <string>

#include <GL/freeglut.h>    // Header File For The GLUT Library.
#include <GL/gl.h>          // Header File For The OpenGL32 Library.
#include <GL/glu.h>         // Header File For The GLu32 Library.

#include "Text.h"


int Bitmap_String_Total_Line_Width(const std::string &text){
    //Given a line of text, this routine returns the total width (in pixels).
    int tot = 0;
    for(auto it = text.begin(); it != text.end(); ++it){
        tot += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18,(int)(*it));
    }
    return tot;
}

int Bitmap_String_Max_Line_Height(void){
    //Given a line of text, this routine returns the height/spacing (in pixels) between lines.
    return glutBitmapHeight(GLUT_BITMAP_HELVETICA_18);
}

void Bitmap_Draw_String(float x, float y, float z, const std::string &text){
    int i;
    int xoffset=0;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClear(GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glLoadIdentity();

    for(auto it = text.begin(); it != text.end(); ++it){
        glRasterPos3f(x+(float)(xoffset),y,z);

        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,(int)(*it));
        xoffset += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18,(int)(*it));
    }

    glPopMatrix();
    glEnable(GL_BLEND);
    glEnable( GL_DEPTH_TEST );

}

//Scale = 1/10 is a good starting point.
void Stroke_Draw_String(float x, float y, const std::string &text, float scale){
    int i;
    float xoffset=0.0;
    for(auto it = text.begin(); it != text.end(); ++it){
        glPushMatrix();
        glLoadIdentity();

        glTranslatef(x + scale*xoffset,y,0.0);
        //glRotatef(180.0,1.0,0.0,0.0);
        glScalef(scale, scale, scale);

        glutStrokeCharacter(GLUT_STROKE_ROMAN,(int)(*it));
        xoffset += (float)(glutStrokeWidth(GLUT_STROKE_ROMAN,(int)(*it)));

        glPopMatrix();
    }
}
