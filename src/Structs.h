//Structs.h - Structures specific to Project - Mosaic.

#ifndef HDR_GRD_PROJECT_MOSAIC_STRUCTS_H_
#define HDR_GRD_PROJECT_MOSAIC_STRUCTS_H_

#include <memory>         //Needed for std::unique_ptr.
#include <string>        
#include <list>
#include <Magick++.h>

struct Geometry {
    float T, B, L, R, I, O; //Top/Bottom/Left/Right/Inner/Outer.

    Geometry();
    Geometry(const struct Geometry &);
    Geometry(float T, float B, float L, float R, float I, float O);
    Geometry(float T, float B, float L, float R);

    struct Geometry & operator=(const struct Geometry &);
};


//--------------------------------------------------------------------------------------------
//------------------------------ Generic Visual Media Struct ---------------------------------
//--------------------------------------------------------------------------------------------
//This struct is a generic visual media struct. It is used to 'wrap' static images and 
// animated images and video.
//
//Specifically, each form of media is treated as a 2D image with a corresponding time
// dimension. In practise this means that ALL media is treated as an animated image.
//
//Timing info is handled internally. There is no need to deal with it outside of the members,
// but everything is public just in case some fine-level control is desired.
//
class Generic_Visual_Media {
    private:

        //---------------------------------- Generic Images ---------------------------------------
        //This will hold all decoded frames in memory. Each element is a single frame.
        std::vector<Magick::Image>  Frame_List; // Note: std::list does not work. Causes seg-faults!

        long int lWidth;        //The width (pixels) of the 2D image.
        long int lHeight;       //The height (pixels) of the 2D image.
        long int lImportance;   //~The distance from the eye, in openGl coords (i.e. ~pixels).

    public:
        //Members.
        bool Is_Static;        //This is a marker which is used to indicate static images,
                               // which can give some optimizations.

        double Frame_Delay_dt; //The time delay between frames of an animation.

        bool Is_Initialized;   //Remains unset while this->data is unallocated.

        double Polling_t0;     //This is a double representing clock ticks (fractions of 
                               // seconds) which is set when the image is initially loaded.

        double Polling_tlast;  //This is a double representing clock ticks (fractions of 
                               // seconds) which is the last time the 2D image was polled.

        std::string Filename;  //This is the URI of the media. It might be a filename or
                               // a URL.

        std::unique_ptr<unsigned short[]> data; //This is pixel data in the format: 
                                                //  RGBA, 16b PER CHANNEL, top left corner of 
                                                //  image is the first coordinate and the first row
                                                //  follows sequentially after the first coordinate.
                                                //  See the indexing members for the packing scheme.

        struct Geometry Geom;       //Directions for drawing on screen.

        //Constructors, Destructors.
        Generic_Visual_Media();
        Generic_Visual_Media(const std::string &);
        Generic_Visual_Media(const Generic_Visual_Media &) = delete;
        Generic_Visual_Media(Generic_Visual_Media &&);
        ~Generic_Visual_Media();

        //Methods.
        void Load_Sane_Defaults(void);
        bool Initialize(void);                  //Decodes and stores the data, if possible.
        void De_Initialize(void);               //Frees the loaded data.

        long int Width(void) const;             //The width (pixels) of the 2D image.
        long int Height(void) const;            //The height (pixels) of the 2D image.
        long int Importance(void) const;        //~The distance from the eye, in openGL coords (i.e. ~pixels).

        long int index(long int i, long int j);
        unsigned short & R(long int i, long int j);
        unsigned short & G(long int i, long int j);
        unsigned short & B(long int i, long int j);
        unsigned short & A(long int i, long int j);

        bool Load_Current_Frame(void);         //This is the most useful member for the user.
};

//--------------------------------------------------------------------------------------------
//--------------------------------- Generic Text Element -------------------------------------
//--------------------------------------------------------------------------------------------
//This struct holds text. Currently, only very simple text is supported. This struct exists to
// make rich text more easily attainable (down the road).
//
//Note that this->Lines may or may not be reflowed, depending on future need/desires. Do not
// require either case.
class Generic_Text_Element {
    private:
        long int lImportance;     //~The distance from the eye, in openGl coords (i.e. ~pixels).

    public:
        std::list<std::string> Lines; //Assumed to be cohesively related somehow.

        struct Geometry Geom;       //Directions for drawing on screen.

        Generic_Text_Element();
        Generic_Text_Element(const std::list<std::string> &);

        long int Width(void) const;  //The width (pixels) of the longest rendered line.
        long int Height(void) const; //The cumulative height (pixels) of the lines.
        long int Height_of_Line(long int) const;
        long int Importance(void) const; //~The distance from the eye, in openGL coords (i.e. ~pixels).
};



//--------------------------------------------------------------------------------------------
//------------------------------ Generic Screen Frame Struct ---------------------------------
//--------------------------------------------------------------------------------------------
//This struct holds elements of a screen frame, including images (static or animated), simple
// shapes, lines, and fills, and text.
//
//It is used to arrange and align items within a scene. In the most simple configuration, a
// single image or text blurb may constitute such a frame. Think of this class as a sort of
// 'window manager' for the children objects.
//
//The idea of this class is to handle the issue of arranging drawing children in some 
// possibly complicated, possibly highly-specific manner. The hope is that the behaviour
// can be tweaked within this class instead of having to hunt down bits of code throughout
// the project.
class Generic_Screen_Frame {
    public:
        
        std::list<Generic_Visual_Media> Images;
        std::list<Generic_Text_Element> Text;
        // .... lines/shapes/gradients/etc...

        std::string UID;                        //Unique ID for flashcards.
        bool IsVirtual = false;                 //Whether a flashcard was created to support review
                                                // (e.g., to present solutions).
        std::string Filename;                   //Holds the filename the frame was loaded from.

        void Arrange(struct Geometry Screen);


        //Special function for flashcards. Used to pass in human's response and check against
        // the solution (as written in the flashcard).

        
};

#endif
