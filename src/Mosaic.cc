//Project - MosaicFlashcards. Hal Clark, 2013, 2017.

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>    
#include <vector>
#include <list>
#include <getopt.h>      //Needed for 'getopts' argument parsing.
#include <cstdlib>       //Needed for exit() calls.
#include <utility>       //Needed for std::pair.

#include "YgorMisc.h"          //Needed for FUNCINFO, FUNCWARN, FUNCERR macros.
#include "YgorFilesDirs.h"     //Needed for file and directory checking.
#include "YgorString.h"        //Needed for stringtoX<...>(...).
//#include "YgorPerformance.h"   //Needed for YgorPerformance_Get_Time();
#include "YgorAlgorithms.h"    //Needed for shuffle_list();
#include "YgorTime.h"

#include "Structs.h"         //Needed for Generic_Visual_Media class.
#include "Text.h"            //Needed for rendering text on screen.
#include "Flashcard_Parsing.h" 

#include <GL/freeglut.h>    // Header File For The GLUT Library.
#include <GL/gl.h>          // Header File For The OpenGL32 Library.
#include <GL/glu.h>         // Header File For The GLu32 Library.

#include <sqlite3.h> 

#include <unistd.h>         // Needed to sleep.
#include <stdlib.h>
#include <stdio.h>
#include <climits>


//Forward declarations.
class Generic_Visual_Media; //Defined in Structs.h.


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Windowing constants and globals.

//GLUT does NOT have compatability with scroll wheels. This is here to cope with it.
#ifndef GLUT_WHEEL_UP
    #define GLUT_WHEEL_UP (3)
#endif

#ifndef GLUT_WHEEL_DOWN
    #define GLUT_WHEEL_DOWN (4)
#endif

#define ESCAPE 27               //ASCII code for the escape key.
int window;                     //GLUT window number.
int Screen_Pixel_Width  = 768;  //Initial screen width. Can be resized during runtime.
int Screen_Pixel_Height = 768;  //Initial screen height. Can be reseized during runtime.
int vertpixcount;               //This is the number of vert pixels
int horizpixcount;              //This is the number of horiz pixels.

float ZOOM         =  1.0; 
float Ortho_Left   = -2000.0;
float Ortho_Right  =  2000.0;
float Ortho_Top    =  2000.0;
float Ortho_Bottom = -2000.0;
float Ortho_Inner  =     1.0;  //Should be positive (distance from eye).
float Ortho_Outer  =  9000.0;  //Should be positive (distance from eye).
float Ortho_Middle_Inner_Outer = (Ortho_Inner + Ortho_Outer)/2.0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//'Business logic' constants and globals.

const std::string Version = "0.1.3 - Beta. Use at your own risk!"; 

std::list<Generic_Screen_Frame> Frames; //Each frame contains a list of images and text to display.
std::list<Generic_Screen_Frame>::iterator F_it;

long int Flashcards_Answered = 0;
long int Flashcards_Poor = 0;
long int Flashcards_Ok = 0;
long int Flashcards_Great = 0;

long int New_Card_Limit = 30;  // Maximum number of previously-unseen cards to consider in one session.
                               // What is 'reasonable' depends on your goals and deadlines.
                               // Note: Setting to zero disables the limit altogether.

long int Seen_Card_Limit = 30;  // Maximum number of previously-seen cards to consider in one session.
                                // What is 'reasonable' depends on your goals and deadlines.
                                // Note: Setting to zero disables the limit altogether.

long int Interval_Cap = 14; // The maximum re-review delay in days. Can be >=1. Setting this value larger will result in
                            // fewer re-reviews in each session, but you will cover cards less frequently. So it depends
                            // on your goals:
                            // - If you are a student, you should use something small (~days or weeks). 
                            //   Your aim to learn and retain the info as much as possible in a finite time.
                            // - If you do not have a deadline and simply want to persist some knowledge,
                            //   you may want to use something larger to reduce workload (~weeks or months).
                            //
                            // The algorithm default is open-ended and can rapidly grow to 10 years if you've viewed a 
                            // card ~10 times and answered 4 or 5 each time. That's a long time!

long int Session_Fudge = 5; // The number of minutes prior to the beginning of the most recent session to group into the
                            // session. This is useful if no cards were reviewed, or there was a runtime failure that
                            // cut a session short. If you perform several review sessions per day, keep it to 5-15
                            // minutes. Otherwise, keep it around 60-120 minutes.

sqlite3 *db;
std::string DB_Filename = "/home/hal/.Flashcards.db";
char *sqlite_err = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Advance_F_it(void){
    for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it) i_it->De_Initialize();
    if(F_it == --(Frames.end())){
        F_it = Frames.begin();
    }else{
        ++F_it;
    }
}

void Purge_F_it(void){
    for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it) i_it->De_Initialize();
    F_it = Frames.erase(F_it);
    if((F_it == Frames.end()) && !Frames.empty()){
        F_it = Frames.begin();
    }
}

void Deadvance_F_it(void){
    for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it) i_it->De_Initialize();
    if(F_it == Frames.begin()){
        F_it = --(Frames.end());
    }else{
        --F_it;
    }
}

long int interval(const double E, long int n){
    //This routine computes the re-training interval required for a given card. The result is in seconds (86400s == 1d).
    if(n == 0) return 0*86400; // seconds.
    if(n == 1) return 1*86400; // seconds.
    if(n == 2) return 6*86400; // seconds.

    //Modification to the algorithm: do not permit interval to grow too large -- cap it at something reasonable.
    auto I = interval(E,n-1)*E;
    if(I > (Interval_Cap*86400)) I = (Interval_Cap*86400);
    return I;
}

static int corral_records(void *data, int argc, char **argv, char **col_names){
    //This routine is a callback for sqlite3 exec statements.
    //Note that it gets called once per row, so can only append to the output.
    auto r_ptr = static_cast<std::list<std::map<std::string,std::string>>*>(data); // Records.
    std::map<std::string,std::string> record;
    for(int i = 0; i < argc; ++i){
        record[col_names[i]] = ((argv[i] != nullptr) ? argv[i] : "NULL");
    }
    r_ptr->emplace_back( std::move(record) );
    return 0;
}

//Insert the response into the DB responses table. No extra logic required.
// ...
void Update_Response_Record(long int q, std::string uid){
    std::list<std::map<std::string,std::string>> records;
    std::string query;
    int ret;

    //Get the current values needed for re-grading.
    records.clear();
    query = "SELECT E, n, n_seen FROM sm2 WHERE ( uid = '"_s + uid + "' );";
    ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
    if(ret != SQLITE_OK){
        sqlite3_close(db);
        sqlite3_free(sqlite_err);
        glutDestroyWindow(window);
        FUNCERR("sqlite3 error: '" << sqlite_err << "'");
    }else if(records.empty()){
        sqlite3_close(db);
        glutDestroyWindow(window);
        FUNCERR("Logic error -- no record found for this card");
    }
    //for(auto &r : records){
    //    for(auto &p : r){
    //        FUNCINFO(p.first << " --- " << p.second);
    //    }
    //}
    //FUNCINFO("The UID is " << uid);

    const auto n_seen = std::stol(records.front()["n_seen"]);
    const auto n      = std::stol(records.front()["n"]);
    const auto E      = std::stod(records.front()["E"]);

    const auto n_seenp = n_seen + 1;
    auto Ep = E + 0.1 - (5.0 - q)*(0.08 + 0.02*(5.0 - q));
    if(Ep <= 1.3) Ep = 1.3;
    auto np = n + 1;
    if(q < 3){
        np = 1;
        Ep = E;
    }

    records.clear();
    query = "INSERT OR REPLACE INTO sm2 (uid, E, n, n_seen) VALUES ( '"_s + uid + "', "_s
          + std::to_string(Ep) + ", "_s + std::to_string(np) + ", "_s + std::to_string(n_seenp)
          + " ); ";
    ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
    if(ret != SQLITE_OK){
        sqlite3_close(db);
        sqlite3_free(sqlite_err);
        glutDestroyWindow(window);
        FUNCERR("sqlite3 error: '" << sqlite_err << "'");
    }

    records.clear();
    query = "INSERT INTO sm2_responses (uid, q, d_t) VALUES ( '"_s 
          + uid + "' , "_s + std::to_string(q) + ", datetime('now', 'localtime'));";
    ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
    if(ret != SQLITE_OK){
        sqlite3_close(db);
        sqlite3_free(sqlite_err);
        glutDestroyWindow(window);
        FUNCERR("sqlite3 error: '" << sqlite_err << "'");
    }

    return;
}

void ReSizeGLScene(int Width, int Height){
    if(Height==0) Height=1;
    glViewport(0, 0, Width, Height);              // Reset The Current Viewport And Perspective Transformation
    Screen_Pixel_Width  = Width;
    Screen_Pixel_Height = Height;

    //For easier bitmap pixel math, keep the ortho coordinates clamped to 
    // the available screen pixels. Also do NOT make both the leftward and
    // rightward directions positive for the respective variables. Keep 
    // R>L, T>B, and O>I to keep everything in a sensible R3 space.
    Ortho_Left   =  0.0;
    Ortho_Right  =  static_cast<double>(Screen_Pixel_Width);
    Ortho_Top    =  static_cast<double>(Screen_Pixel_Height);
    Ortho_Bottom =  0.0;
    Ortho_Inner  =  1.0;     //Should be positive (distance from eye).
    Ortho_Outer  =  9000.0;  //Should be positive (distance from eye).
    Ortho_Middle_Inner_Outer = (Ortho_Inner + Ortho_Outer)/2.0;

    //For 2-D.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(Ortho_Left/ZOOM, Ortho_Right/ZOOM, Ortho_Bottom/ZOOM, Ortho_Top/ZOOM, Ortho_Inner, Ortho_Outer);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.0f, 0.0f, 0.0f, 0.3f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH); //Enables Smooth Color Shading
    glEnable(GL_BLEND); //Make sure to supply a fourth coordinate with each glColor4f() call.
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE);
    //glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    return;
}

void DrawGLScene(){

    //Initialize all images which have not yet been.
    for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it){
        if(!i_it->Is_Initialized){
            if(!i_it->Initialize()){
                sqlite3_close(db);
                FUNCERR("Unable to initialize image");
            }
        }
    }

    //Get the text line height. Useful for drawing HUD.
    const auto glyphH = Bitmap_String_Max_Line_Height(); //"DUMMY");

    //Perform automatic arrangement on the content. Leave room at bottom for text entry line.
    struct Geometry ScreenGeom(Ortho_Top,Ortho_Bottom+glyphH,Ortho_Left,Ortho_Right,Ortho_Inner,Ortho_Outer);
    //FUNCINFO("Screen Geom: T B L R I O = " << ScreenGeom.T << " " << ScreenGeom.B << " " << ScreenGeom.L << " " << ScreenGeom.R << " " << ScreenGeom.I << " " << ScreenGeom.O);
    F_it->Arrange(ScreenGeom);

    //Clear the back color and depth buffers. Reset the view.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glEnable(GL_DEPTH_TEST);

    //------- Render images ----------
    std::list<GLuint> TextureIDs;
    for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it){

        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        TextureIDs.push_back(GLuint()); //Generate one texture ID.
        GLuint g_uTextureID; //Generate one texture ID
        glGenTextures(1,&(TextureIDs.back()));
        glBindTexture(GL_TEXTURE_2D,TextureIDs.back()); //Bind the texture type.
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i_it->Width(), i_it->Height(), 0, GL_RGBA,GL_UNSIGNED_SHORT, i_it->data.get());
    
        //glScalef(2.0f,2.0f,0.0f); //Make the sprite 2 times bigger.
        //glEnable(GL_BLEND); //Blend the color key into oblivion.
        //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f ); //Set the primitive color to white.
        glBindTexture(GL_TEXTURE_2D,TextureIDs.back()); //Bind the texture to the polygons.

        //FUNCINFO("Image Geom: T B L R I O = " << i_it->Geom.T << " " << i_it->Geom.B << " " << i_it->Geom.L << " " << i_it->Geom.R << " " << i_it->Geom.I << " " << i_it->Geom.O);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0,1.0); glVertex3f( i_it->Geom.R, i_it->Geom.B, -Ortho_Middle_Inner_Outer);
        glTexCoord2f(1.0,0.0); glVertex3f( i_it->Geom.R, i_it->Geom.T, -Ortho_Middle_Inner_Outer);
        glTexCoord2f(0.0,0.0); glVertex3f( i_it->Geom.L, i_it->Geom.T, -Ortho_Middle_Inner_Outer);
        glTexCoord2f(0.0,1.0); glVertex3f( i_it->Geom.L, i_it->Geom.B, -Ortho_Middle_Inner_Outer);
        glEnd();

        //Figure out if we need to re-load the image data.
        if(!i_it->Load_Current_Frame()){
            FUNCERR("Unable to load current frame");
            sqlite3_close(db);
        }
    }

    //------ Render text ------
    //Draw some text on screen.
    for(auto t_it = F_it->Text.begin(); t_it != F_it->Text.end(); ++t_it){
        glColor4f( 0.1f, 1.0f, 0.1f, 1.0f );
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        //FUNCINFO("Text  Geom: T B L R I O = " << t_it->Geom.T << " " << t_it->Geom.B << " " << t_it->Geom.L << " " << t_it->Geom.R << " " << t_it->Geom.I << " " << t_it->Geom.O);
        float dy = 0.0;
        for(auto l_it = t_it->Lines.begin(); l_it != t_it->Lines.end(); ++l_it){
            Bitmap_Draw_String(t_it->Geom.L, t_it->Geom.T - dy, -Ortho_Middle_Inner_Outer, *l_it);
            dy += static_cast<float>(t_it->Height_of_Line(0));
        }
    }

    //----- HUD/Dash text rendering ------
    {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        std::string nums;

        //Top: flashcard number / total.
        glColor4f(0.1f, 1.0f, 0.1f, 1.0f);
        //nums = "Cards: "_s + Xtostring((1+std::distance(Frames.begin(),F_it))/2) + " / "_s + Xtostring(Frames.size()/2);
        nums = "Remaining cards: "_s + Xtostring(Frames.size()/2);
        Bitmap_Draw_String(ScreenGeom.L, ScreenGeom.T - 1.0*glyphH, -Ortho_Middle_Inner_Outer, nums);

        //# seen/answered
        glColor4f(0.65f, 0.1f, 0.85f, 1.0f);
        nums = "Great: "_s + Xtostring(Flashcards_Great);
        Bitmap_Draw_String(ScreenGeom.L, ScreenGeom.T - 2.0*glyphH, -Ortho_Middle_Inner_Outer, nums);
        nums = "Ok:     "_s + Xtostring(Flashcards_Ok);
        Bitmap_Draw_String(ScreenGeom.L, ScreenGeom.T - 3.0*glyphH, -Ortho_Middle_Inner_Outer, nums);
        nums = "Poor:  "_s + Xtostring(Flashcards_Poor);
        Bitmap_Draw_String(ScreenGeom.L, ScreenGeom.T - 4.0*glyphH, -Ortho_Middle_Inner_Outer, nums);

        // Display the options for self-grading.
        if(F_it->IsVirtual){
            glColor4f(0.1f, 0.85f, 0.85f, 1.0f); 
            nums.clear();
            nums += "How'd you do?  (0-1) for poor,  (2-3) for ok,  (4-5) for great.";
            Bitmap_Draw_String(ScreenGeom.L, ScreenGeom.B + 0.0*glyphH, -Ortho_Middle_Inner_Outer, nums);
        }

    }

    //Swap the render and display buffers, drawing the render buffer to screen.
    glutSwapBuffers();

    //Eagerly delete the textures. (If this is too inefficient, devise a "cache and eventual reap" strategy...)
    for(auto it = TextureIDs.begin(); it != TextureIDs.end(); ++it) glDeleteTextures(1, &(*it));

    return;
}

void keyPressed(unsigned char key, int x, int y){
    usleep(100);   //Debounce!

    if(key == ESCAPE){
        //Exit the program by destroying the window and dying.
        sqlite3_close(db);
        glutDestroyWindow(window);
        std::exit(0);

    }else if(key == ' '){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }

    }else if(key == '\r'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }

    }else if(key == '~'){
    }else if(key == '0'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }
        Flashcards_Poor++;
        Update_Response_Record(0, F_it->UID);
        Advance_F_it();

    }else if(key == '1'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }
        Flashcards_Poor++;
        Update_Response_Record(1, F_it->UID);
        Advance_F_it();

    }else if(key == '2'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }
        Flashcards_Ok++;
        Update_Response_Record(2, F_it->UID);
        Advance_F_it();

    }else if(key == '3'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }
        Flashcards_Ok++;
        Update_Response_Record(3, F_it->UID);
        Advance_F_it();

    }else if(key == '4'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }
        Flashcards_Great++;
        Update_Response_Record(4, F_it->UID);

        //Remove the flashcard from this session.
        for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it) i_it->De_Initialize();
        std::advance(F_it,-1);
        Purge_F_it(); // Purge the question card.
        Purge_F_it(); // Then purge the solution card.
        if(Frames.empty()){
            FUNCINFO("No cards remaining. Session complete");
            sqlite3_close(db);
            glutDestroyWindow(window);
            std::exit(0);
        }


    }else if(key == '5'){
        if(!F_it->IsVirtual){
            Advance_F_it();
            return;
        }
        Flashcards_Great++;
        Update_Response_Record(5, F_it->UID);

        //Remove the flashcard from this session.
        for(auto i_it = F_it->Images.begin(); i_it != F_it->Images.end(); ++i_it) i_it->De_Initialize();
        std::advance(F_it,-1);
        Purge_F_it(); // Purge the question card.
        Purge_F_it(); // Then purge the solution card.
        if(Frames.empty()){
            FUNCINFO("No cards remaining. Session complete");
            sqlite3_close(db);
            glutDestroyWindow(window);
            std::exit(0);
        }

    }else if(key == '6'){
    }else if(key == '7'){
    }else if(key == '8'){
    }else if(key == '9'){
    }else if(key == '-'){
    }else if(key == '='){
    }else if(key == '!'){
    }else if(key == '`'){
    }else if(key == '@'){
    }else if(key == '#'){
    }else if(key == '$'){
    }else if(key == '%'){
    }else if(key == '^'){
    }else if(key == '&'){
    }else if(key == '*'){
    }else if(key == '('){
    }else if(key == ')'){
    }else if(key == '_'){
    }else if(key == '+'){
    }else if(key == 'q'){
    }else if(key == 'w'){
    }else if(key == 'e'){
    }else if(key == 'r'){
    }else if(key == 't'){
    }else if(key == 'y'){
    }else if(key == 'u'){
    }else if(key == 'i'){
    }else if(key == 'o'){
    }else if(key == 'p'){
        Deadvance_F_it();

    }else if(key == '['){
    }else if(key == ']'){
    }else if(key == 'a'){
    }else if(key == 's'){
    }else if(key == 'd'){
    }else if(key == 'f'){
    }else if(key == 'g'){
    }else if(key == 'h'){
    }else if(key == 'j'){
    }else if(key == 'k'){
    }else if(key == 'l'){
    }else if(key == ';'){
    //}else if(key == '''){
    }else if(key == 'z'){
        ZOOM *= 1.25;
        ReSizeGLScene(Screen_Pixel_Width, Screen_Pixel_Height);

    }else if(key == 'x'){
    }else if(key == 'c'){
    }else if(key == 'v'){
    }else if(key == 'b'){
    }else if(key == 'n'){
        Advance_F_it();

    }else if(key == 'm'){
    }else if(key == ','){
    }else if(key == '.'){
    }else if(key == 'Q'){
    }else if(key == 'W'){
    }else if(key == 'E'){
    }else if(key == 'R'){
    }else if(key == 'T'){
    }else if(key == 'Y'){
    }else if(key == 'U'){
    }else if(key == 'I'){
    }else if(key == 'O'){
    }else if(key == 'P'){
    }else if(key == '{'){
    }else if(key == '}'){
    }else if(key == 'A'){
    }else if(key == 'S'){
    }else if(key == 'D'){
    }else if(key == 'F'){
    }else if(key == 'G'){
    }else if(key == 'H'){
    }else if(key == 'J'){
    }else if(key == 'K'){
    }else if(key == 'L'){
    }else if(key == ':'){
    }else if(key == '"'){
    }else if(key == '|'){
    }else if(key == 'Z'){
        ZOOM *= 0.75;
        ReSizeGLScene(Screen_Pixel_Width, Screen_Pixel_Height);

    }else if(key == 'X'){
    }else if(key == 'C'){
    }else if(key == 'V'){
    }else if(key == 'B'){
    }else if(key == 'N'){
    }else if(key == 'M'){
    }else if(key == '<'){
    }else if(key == '>'){
    }else if(key == '?'){
        FUNCINFO("Currently viewing flashcard: '" << F_it->Filename << "'");

    }
    
    return;
}

void processMouse(int button, int state, int x, int y) {

    if(button == GLUT_LEFT_BUTTON){
        if(state == GLUT_DOWN){
            if(glutGetModifiers() & GLUT_ACTIVE_CTRL){

            }else if( glutGetModifiers() & GLUT_ACTIVE_SHIFT){

            }
        }
    }

    if(button == GLUT_RIGHT_BUTTON){
        if(state == GLUT_DOWN){

        }
    }

    if(button == GLUT_WHEEL_UP){
        if(glutGetModifiers() & GLUT_ACTIVE_CTRL){  
            
        }
    }

    if(button == GLUT_WHEEL_DOWN){
        if(glutGetModifiers() & GLUT_ACTIVE_CTRL){

        }
    }

    return;
}

int main(int argc, char* argv[]){
    std::list<std::string> Filenames_In;
    bool flashcard_require_fcard_suffix = false;  //Useful if you have thousands of cards and need to pass in (mixed) directories.

    //---------------------------------------------------------------------------------------------------------------------
    //------------------------------------------------ Option parsing -----------------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------
    //These are fairly common options. Run the program with -h to see them formatted properly.

    int next_options;
    const char* const short_options    = "hVi:Sd:s:n:"; //This is the list of short, single-letter options.
                                                      //The : denotes a value passed in with the option.
    //This is the list of long options. Columns:  Name, BOOL: takes_value?, NULL, Map to short options.
    const struct option long_options[] = { { "help",          0, NULL, 'h' },
                                           { "version",       0, NULL, 'V' },
                                           { "in",            1, NULL, 'i' },
                                           { "seen-limit",    1, NULL, 's' },
                                           { "new-limit",     1, NULL, 'n' },
                                           { "strict-filenames", 0, NULL, 'S' },
                                           { "database",      1, NULL, 'd' },
                                           { NULL,            0, NULL, 0   }  };

    do{
        next_options = getopt_long(argc, argv, short_options, long_options, NULL);
        switch(next_options){
            case 'h': 
                std::cout << std::endl;
                std::cout << "-- " << argv[0] << " Command line switches: " << std::endl;
                std::cout << "------------------------------------------------------------------------------------------------------------" << std::endl;
                std::cout << "   Short              Long                 Default            Description" << std::endl;
                std::cout << "------------------------------------------------------------------------------------------------------------" << std::endl;
                std::cout << "   -h                 --help                                  Display this message and exit." << std::endl;
                std::cout << "   -V                 --version                               Display program version and exit." << std::endl;
                std::cout << "   -i myfilename      --in myfilename       <none>            Flashcard name. If directory, recurses looking for cards." << std::endl;
                std::cout << "   -s 30              --seen-limit 30       30                Number of previously-seen cards to review in a session. (0 --> infinite.)." << std::endl;
                std::cout << "   -n 20              --new-limit 20        30                Number of previously-unseen cards to review in a session. (0 --> infinite.)." << std::endl;
                std::cout << "   -S                 --strict-filenames    <false>           In Flashcard mode, ensure all input ends with '.fcard'." << std::endl;
                std::cout << "   -d                 --database            ~/.Flashcards.db  The database file to use. It is created if necessary." << std::endl;
                std::cout << std::endl;
                std::cout << "Note that all other arguments are treated as filenames." << std::endl;

                std::cout << std::endl;
                std::exit(0);
                break;

            case 'V': 
                FUNCINFO( "Version: " + Version );
                std::exit(0);
                break;

            case 'n':
                New_Card_Limit = std::stod(optarg);
                break;

            case 's':
                Seen_Card_Limit = std::stod(optarg);
                break;

            case 'i':
                Filenames_In.push_back(optarg);
                break;

            case 'S':
                flashcard_require_fcard_suffix = true;
                break;

            case 'd':
                DB_Filename = optarg;
                break;
        }
    }while(next_options != -1);
    //From the optind man page:
    //  "If the resulting value of optind is greater than argc, this indicates a missing option-argument,
    //         and getopt() shall return an error indication."
    for( ; optind < argc; ++optind){
        //We treat everything else as input files. this is OK (but not safe) because we will test each file's existence.
        Filenames_In.push_back(argv[optind]);
    }

    //---------------------------------------------------------------------------------------------------------------------
    //----------------------------------------------- Filename Testing ----------------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------

    //Determine which of the filenames is a file, which is a directory, and which is garbage.
    for(auto f_it = Filenames_In.begin(); f_it != Filenames_In.end();    ){
        if(Does_File_Exist_And_Can_Be_Read(*f_it)){
            ++f_it;
            continue;
        }else if(Does_Dir_Exist_And_Can_Be_Read(*f_it)){
            //If we have a directory, grab all the filenames and place them in the collection.
            // We will assume they are all files now and will sort out the cruft recursively.
            auto names = Get_List_of_File_and_Dir_Names_in_Dir(*f_it);
            for(auto i=names.begin(); i!=names.end(); ++i) Filenames_In.push_back(*f_it + "/" + *i);
            f_it = Filenames_In.erase(f_it);
            continue;
        }

        //At this point, the file doesn't exist or is a URL. We assume URL for now because the image libs can handle them.
        FUNCWARN("Assuming '" << *f_it << "' is a URL");
        ++f_it;
    }

    if(Filenames_In.size() == 0){
        FUNCERR("No valid files were found. Verify files can be read and/or execute '" << argv[0] << " -h' for help");
    }

    shuffle_list_randomly(Filenames_In);

    //Filenames_In now contains a list of filenames which are ready to be opened/loaded/processed.

    //---------------------------------------------------------------------------------------------------------------------
    //------------------------------------------- File Parsing / Data Loading ---------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------
    for(auto f_it = Filenames_In.begin(); f_it != Filenames_In.end();    ){
        //In this mode, we assume the input files are special flashcard files. They will be text files with special
        // directives inside, such as file URIs and text to render on screen.
        bool IsOK = true;
        if(flashcard_require_fcard_suffix){
            const std::string ReqSfx(".fcard");
            if(f_it->size() < ReqSfx.size()) IsOK = false;

            if(IsOK){
                const std::string TheSfx = f_it->substr(f_it->size() - ReqSfx.size(),ReqSfx.size());
                if(TheSfx != ReqSfx) IsOK = false;
            }
        }

        if(IsOK) Frames.splice(Frames.begin(),Parse_Flashcard_File_into_Frames(*f_it));
        ++f_it;
        continue;
    }

    //Clear the input filenames so that we do not accidentally use them.
    Filenames_In.clear();

    const auto N_found = Frames.size()/2;

    //---------------------------------------------------------------------------------------------------------------------
    //--------------------------------------------------- DB Loading ------------------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------
    const auto rc = sqlite3_open(DB_Filename.c_str(), &db);
    if(rc){
        FUNCERR("Unable to open or create sqlite3 db: '" << sqlite3_errmsg(db) << "'");
    }

    std::list<std::map<std::string,std::string>> records;
    std::string query;
    int ret;

    //Create the necessary DB schema if needed.
    {
        records.clear();
        query = "CREATE TABLE IF NOT EXISTS sm2 (   "  // SuperMemo 2 algorithm.
                "    uid TEXT PRIMARY KEY NOT NULL, "  // Unique card ID.
                "    E REAL NOT NULL,               "  // SM2 model parameter E.
                "    n INT NOT NULL,                "  // SM2 model parameter for ~# times a card has been seen. Can be reset though.
                "    n_seen INT NOT NULL );         "  // Monotonic/lifetime counter for # times a card has been seen.
                " \n "
                "CREATE TABLE IF NOT EXISTS sm2_responses (  " // Ongoing/persistent record of previous responses.
                "    uid TEXT NOT NULL,                      " // Unique card ID.
                "    q INT NOT NULL,                         " // How the card was received: worst -->  [0:5] <-- best.
                "    d_t TEXT NOT NULL );                    " // Datetime of response (localtime).
                " \n "
                "CREATE TABLE IF NOT EXISTS sessions (  "  // Record of individual sessions.
                "    b TEXT NOT NULL );                 "; // Session begin time (localtime).
        ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
        if(ret != SQLITE_OK){
            sqlite3_close(db);
            sqlite3_free(sqlite_err);
            FUNCERR("sqlite3 error: '" << sqlite_err << "'");
        }

    }

    //---------------------------------------------------------------------------------------------------------------------
    //-------------------------------------------------- File Ordering ----------------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------
    // Query the DB for a list of cards to review. Starting with the complete set,
    //    - for those not previously seen, randomly select a few (~20 or so) to avoid overloading sessions.
    //    - for those previously seen, 
    //      - repeat all cards that got a q<4 at least once in the last session, and
    //      - any card that is 'due' according to the SM2-algorithm interval 'I'.

    // Cycle through the cards, identifying unseen cards. After so many, start trimming them.
    {
        auto f_it = Frames.begin();
        long int New_Card_Count = 0;
        long int Seen_Card_Count = 0;
        while(f_it != Frames.end()){
            records.clear();
            query = "SELECT uid FROM sm2 WHERE (uid = '"_s + f_it->UID + "') AND NOT (n_seen = 0) ;";
            ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
            if(ret != SQLITE_OK){
                sqlite3_close(db);
                sqlite3_free(sqlite_err);
                FUNCERR("sqlite3 error: '" << sqlite_err << "'");
            }
            const bool previously_unseen = records.empty();
            if( previously_unseen ){
                //Previously unseen cards.

                //Add a default record to the DB for the card.
                records.clear();
                query = "INSERT OR REPLACE INTO sm2 (uid, E, n, n_seen) VALUES ( '"_s + f_it->UID + "', 2.5, 0, 0 );";
                ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
                if(ret != SQLITE_OK){
                    sqlite3_close(db);
                    sqlite3_free(sqlite_err);
                    FUNCERR("sqlite3 error: '" << sqlite_err << "'");
                }

                //Trim them if needed.
                if( (New_Card_Limit != 0)
                &&  (++New_Card_Count > 2*New_Card_Limit )){ // Twice because of virtual cards.
                    f_it = Frames.erase(f_it);
                }else{
                    ++f_it;
                }
                continue;
            }

            // --- Previously seen cards only beyond this point. ---

            //Get the current values needed for re-grading.
            records.clear();
            query = "SELECT E, n, n_seen FROM sm2 WHERE ( uid = '"_s + f_it->UID + "' );";
            ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
            if(ret != SQLITE_OK){
                sqlite3_close(db);
                sqlite3_free(sqlite_err);
                FUNCERR("sqlite3 error: '" << sqlite_err << "'");
            }else if(records.empty()){
                sqlite3_close(db);
                FUNCERR("Logic error -- no record found for this card");
            }
            const auto n_seen = std::stol(records.front()["n_seen"]);
            const auto n      = std::stol(records.front()["n"]);
            const auto E      = std::stod(records.front()["E"]);
            const auto I      = interval(E,n); // in seconds.


            //Figure out if review is due yet.
            records.clear();
            query = "SELECT (strftime('%s', 'now', 'localtime') - strftime('%s', d_t)) AS dt FROM sm2_responses WHERE ( uid = '"_s + f_it->UID + "' );";
            ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
            if(ret != SQLITE_OK){
                sqlite3_close(db);
                sqlite3_free(sqlite_err);
                FUNCERR("sqlite3 error: '" << sqlite_err << "'");
            }else if(records.empty()){
                sqlite3_close(db);
                FUNCERR("Logic error -- no record found for this card");
            }
            const auto dt = std::stol(records.front()["dt"]);
            const auto review_due = (dt >= I);
            //FUNCINFO("   dt = " << dt << "    and   I = " << I );
            //FUNCINFO("   Review due? ----------------- " << !!review_due );

            //Figure out if any (q<3) were given in the last session.
            records.clear();
            query = "SELECT b FROM sessions ORDER BY strftime('%s',b) DESC LIMIT 1 ;";
            ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
            if(ret != SQLITE_OK){
                sqlite3_close(db);
                sqlite3_free(sqlite_err);
                FUNCERR("sqlite3 error: '" << sqlite_err << "'");
            }else if(records.empty()){
                sqlite3_close(db);
                FUNCERR("Logic error -- no record found for this card");
            }
            const auto b = records.front()["b"];

            records.clear();
            query = "SELECT uid FROM sm2_responses WHERE ( uid = '"_s + f_it->UID + "' )"_s
                  + " AND (q < 3) AND ( strftime('%s', d_t) > strftime('%s','"_s + b 
                  + "', '-"_s + std::to_string(Session_Fudge) + " minutes') ) ;";
            ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
            if(ret != SQLITE_OK){
                sqlite3_close(db);
                sqlite3_free(sqlite_err);
                FUNCERR("sqlite3 error: '" << sqlite_err << "'");
            }
            const bool got_a_low_q_last_session = !(records.empty());
            //FUNCINFO("   Got a low q last session? --- " << !!got_a_low_q_last_session );

            if( ( got_a_low_q_last_session || review_due )
            &&  ( (Seen_Card_Limit == 0) || !(++Seen_Card_Count > 2*Seen_Card_Limit ) ) ){ // Twice because of virtual cards.
                //FUNCINFO("Added a card");
                ++f_it;
            }else{
                //FUNCINFO("Card was not eligible for review right now. Removed it");
                f_it = Frames.erase(f_it);
            }

        }
    }

    //If there are no cards available for review, terminate execution before beginning the session.
    if(Frames.empty()){
        sqlite3_close(db);
        FUNCERR("There are no cards ready to review yet. Terminating");
    }else{
        FUNCINFO("There are " << Frames.size()/2 << " cards for review");
    }

    // Provide an estimate of the number of ongoing cards you can support with the chosen constants.
    {
        const auto N_cards = Frames.size()/2;
        FUNCINFO("I found " << N_found << " cards and " << N_cards << " will be reviewed this session."
                 " If you review only these session cards today, and keep up the rate indefinitely"
                 " (e.g., steady-state), you'll be able to support " 
                 << N_cards * Interval_Cap << " cards at a time");
        FUNCINFO("Assuming a steady-state, setting Interval_Cap to " << N_found / N_cards << " days"
                 " may be more appropriate for this card collection"
                 " (it's currently set to " << Interval_Cap << " days)");
    }


    // Insert this session's begin time.
    {
        query = "INSERT INTO sessions (b) VALUES (datetime('now', 'localtime'));";
        ret = sqlite3_exec(db, query.c_str(), corral_records, static_cast<void*>(&records), &sqlite_err);
        if(ret != SQLITE_OK){
            sqlite3_close(db);
            FUNCERR("sqlite3 error: '" << sqlite_err << "'");
            sqlite3_free(sqlite_err);
        }
    }

    F_it = Frames.begin();

    //---------------------------------------------------------------------------------------------------------------------
    //------------------------------------------------- Visualization -----------------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------
    //Launch the viewer. This is something we do not recover from, so this is last.
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
    glutInitWindowSize(Screen_Pixel_Width, Screen_Pixel_Height);
    glutInitWindowPosition(0, 0);
    window = glutCreateWindow("MosaicFlashcards");
    glutDisplayFunc(&DrawGLScene);
    glutIdleFunc(&DrawGLScene);
    glutReshapeFunc(&ReSizeGLScene);
    glutKeyboardFunc(&keyPressed);
    glutMouseFunc(&processMouse);
    ReSizeGLScene(Screen_Pixel_Width, Screen_Pixel_Height);
    glutMainLoop();

    //---------------------------------------------------------------------------------------------------------------------
    //---------------------------------------------------- Cleanup --------------------------------------------------------
    //---------------------------------------------------------------------------------------------------------------------
    //Try avoid doing anything that will require you to explicitly clean anything up here (ie. please use smart pointers
    // where ever they are possible.)

    //Sure hope nothing is here, because glut won't return!
    return 0;
}
