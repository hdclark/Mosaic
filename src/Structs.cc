//Structs.cc.

#include <memory>           //Needed for std::unique_ptr.
#include <string>
#include <list>
#include <iterator>         //Needed for std::distance().
#include <cmath>            //Needed for std::ceil().

#include <Magick++.h>       //NOTE: We make use of http://www.imagemagick.org/Magick++/STL.html (Aug 2012.)

#include "Structs.h"
#include "Text.h"

#include "YgorFilesDirs.h"    //Needed for Is_File... 
#include "YgorMisc.h"         //Needed for FUNCERR, FUNCWARN, FUNCINFO macro functions.
#include "YgorPerformance.h"  //Needed for YgorPerformance_Get_Time();
#include "YgorString.h"       //Needed for Xtostring.


Geometry::Geometry(){
    this->T = this->B = this->L = this->R = this->I = this->O = -1.0;
}
Geometry::Geometry(const struct Geometry &rhs){
    *this = rhs;
}
Geometry::Geometry(float iT, float iB, float iL, float iR, float iI, float iO){
    this->T = iT;
    this->B = iB;
    this->L = iL;
    this->R = iR;
    this->I = iI;
    this->O = iO;
}
Geometry::Geometry(float iT, float iB, float iL, float iR){
    this->T = iT;
    this->B = iB;
    this->L = iL;
    this->R = iR;
    this->I = -1.0;
    this->O = -1.0;
}
struct Geometry & Geometry::operator=(const struct Geometry &rhs){
    //Check if it is itself.
    if(this == &rhs) return *this;
    this->T = rhs.T;
    this->B = rhs.B;
    this->L = rhs.L;
    this->R = rhs.R;
    this->I = rhs.I;
    this->O = rhs.O;
    return *this;
}


static struct Geometry Embed_Object_in_Geometry_Maintaining_Aspect(struct Geometry Screen, struct Geometry Object, float max_scale){
    //This routine takes a screen geometry (i.e., a spatial container) and an object's geometry 
    // (e.g., an image's bounding box) and returns the object's geometry which has been scaled, 
    // centered, and fit into the available space.
    //
    //This routine is suitable for embedding images into a fixed region. 
    //
    //NOTE: The maximum scale factor can be chosen to keep visual pixelation to a minimum.
    // For most images, 1.5 to 2.0 is a suitable max scaling factor. At this point, pixels
    // are generally VERY noticeable. 
    //
    //NOTE: This assumes that the T,B,L,R are coordinates of a 2D space. This routine will
    // NOT work if positive B and L denote negative coordinates. In other words, T and B 
    // must share a common axis. L and R must share a common axis. These common axes 
    // cannot be inverted.

    //----- First, work out the numbers without dealing with where the origin is ----
//    const float sH = YGORABS(Screen.T) + YGORABS(Screen.B);
//    const float sW = YGORABS(Screen.L) + YGORABS(Screen.R);
//    const float orig_oH = YGORABS(Object.T) + YGORABS(Object.B);
//    const float orig_oW = YGORABS(Object.L) + YGORABS(Object.R);

    const float sH = Screen.T - Screen.B;  
    const float sW = Screen.R - Screen.L;
    const float orig_oH = Object.T - Object.B;
    const float orig_oW = Object.R - Object.L;
    float oH = orig_oH, oW = orig_oW;

    //If the object is too big for the screen, shrink it to fit, maintaining aspect.
    if((oW > sW) || (oH > sH)){
        float scale = sW/oW;
        if(scale*oH > sH) scale = sH/oH;
        oH *= scale;
        oW *= scale;
    }

    //If the object is small, scale it up to fit the shortest edge maintaining aspect.
    const float Hfill = oH/sH, Wfill = oW/sW;
    if((Hfill < 0.9) && (Wfill < 0.9)){
        if(Hfill > Wfill){
            oH /= Hfill;
            oW /= Hfill;
        }else{
            oH /= Wfill;
            oW /= Wfill;
        }
    }

    //If the object has been stretched too much, clamp the enlargement.
    if((oH > max_scale*orig_oH) && (oW > max_scale*orig_oW)){
        oH = max_scale*orig_oH;
        oW = max_scale*orig_oW;
    }

    //----- Now centre. Here we deal with the origin -----
    struct Geometry out;
    out.T = Screen.B + 0.5*sH + 0.5*oH;
    out.B = Screen.B + 0.5*sH - 0.5*oH;
    out.L = Screen.L + 0.5*sW - 0.5*oW;
    out.R = Screen.L + 0.5*sW + 0.5*oW;
    return out;


//--(I) In function: DrawGLScene: Screen Geom: T B L R I O = 980 0 0 1918 1 9000.
//--(I) In function: DrawGLScene: Image Geom: T B L R I O = 6.63202 1065.37 0 -1918 -1 -1.

}




//--------------------------------------------------------------------------------------------
//------------------------------ Generic Visual Media Struct ---------------------------------
//--------------------------------------------------------------------------------------------
//Constructors, Destructors.
Generic_Visual_Media::Generic_Visual_Media(){
    this->Load_Sane_Defaults();
};

Generic_Visual_Media::Generic_Visual_Media(const std::string &Filename_In){
    this->Load_Sane_Defaults(); 
    this->Filename = Filename_In;
};

Generic_Visual_Media::Generic_Visual_Media(Generic_Visual_Media &&to_move){
    this->Filename       = std::move(to_move.Filename);
    this->Is_Static      = std::move(to_move.Is_Static);
    this->Is_Initialized = std::move(to_move.Is_Initialized);
    this->Frame_Delay_dt = std::move(to_move.Frame_Delay_dt);
    this->Polling_t0     = std::move(to_move.Polling_t0);
    this->Polling_tlast  = std::move(to_move.Polling_tlast);
    this->lWidth         = std::move(to_move.lWidth);
    this->lHeight        = std::move(to_move.lHeight);
    this->lImportance      = std::move(to_move.lImportance);
    this->data           = std::move(to_move.data);    
};

Generic_Visual_Media::~Generic_Visual_Media(){
    //Die happily?
};

//Members.
void Generic_Visual_Media::Load_Sane_Defaults(void){
    this->Is_Static      = true;
    this->Is_Initialized = false;
    this->Frame_Delay_dt = -1.0;
    this->Polling_t0     = -1.0;
    this->Polling_tlast  = -1.0;
    this->lWidth         = -1;
    this->lHeight        = -1;
    this->lImportance    = -1;
    this->data           = nullptr;

    this->Geom = Geometry(-1.0,-1.0,-1.0,-1.0,-1.0,-1.0);
//    this->Geom.T     = -1.0; 
//    this->Geom.B     = -1.0;
//    this->Geom.L     = -1.0;
//    this->Geom.R     = -1.0;
//    this->Geom.I     = -1.0;
//    this->Geom.O     = -1.0;
    return;
}

bool Generic_Visual_Media::Initialize(void){
    //This routine prepares the struct by attempting to read the file.
    // All failures result in returning a false. Returning a true means everything has
    // been successfully initialized.

    if(this->Is_Initialized == true) return true;

    if(!Does_File_Exist_And_Can_Be_Read(this->Filename)){
//        FUNCWARN("Unable to open file '" << this->Filename << "'. If it exists, we may not have proper permissions to open it");
        FUNCWARN("Assuming file '" << this->Filename << "' is a URL");
//        return false;
    }

    //Open the file and read some metadata from the file.
    Magick::Image magicimg;
    magicimg.read( this->Filename.c_str() );

//    this->lWidth  = static_cast<long int>(magicimg.rows());
//    this->lHeight = static_cast<long int>(magicimg.columns());

    this->lWidth  = static_cast<long int>(magicimg.columns());
    this->lHeight = static_cast<long int>(magicimg.rows());
    this->lImportance = 50; //Something arbitrary. Positive, but near 1.

    //Allocate space for the 2D image.
    this->data.reset( new unsigned short[ this->lWidth * this->lHeight * 4 ] );
    if( this->data == nullptr ){
        FUNCWARN("Unable to allocate space for the image data");
        return false;
    }

    //Handle GIF's specifically by offloading to the Initialize_Gif() member.
    if((magicimg.magick() == "GIF") || (magicimg.magick() == "PDF")){
        this->Is_Static      = false;
        this->Frame_Delay_dt = static_cast<double>(magicimg.animationDelay())/100.0;
        if( this->Frame_Delay_dt <= 0.0 ){
            this->Frame_Delay_dt = 0.08;
            FUNCWARN("Could not determine the gif's animation delay. Assuming a delay of " << this->Frame_Delay_dt);
        }
    }else{
        this->Is_Static = true;
    }

    //Load the file into memory.
    Magick::readImages(&this->Frame_List, this->Filename);

    //Perform some transformations on the images.
    if(!(this->Is_Static)){
        //Coalesce the images: CoalesceImages() composites a set of images while respecting any page offsets 
        // and disposal methods. GIF, MIFF, and MNG animation sequences typically start with an image background 
        // and each subsequent image varies in size and offset. CoalesceImages() returns a new sequence where 
        // each image in the sequence is the same size as the first and composited with the next image in the 
        // sequence.
        // See: http://www.graphicsmagick.org/api/transform.html#coalesceimages
        Magick::coalesceImages(&this->Frame_List, this->Frame_List.begin(), this->Frame_List.end()); 
    }

//    Magick::quantizeImages(this->Frame_List.begin(), this->Frame_List.end());
//    for_each(this->Frame_List.begin(), this->Frame_List.end(), Magick::negateImage());
//    for_each(this->Frame_List.begin(), this->Frame_List.end(), Magick::normalizeImage());

//    for_each(this->Frame_List.begin(), this->Frame_List.end(), Magick::quantizeColorSpaceImage(Magick::GRAYColorspace));
//    Magick::quantizeImages(this->Frame_List.begin(), this->Frame_List.end());

//    for_each(this->Frame_List.begin(), this->Frame_List.end(), Magick::densityImage("300x300"));


/*
    //Fuck around reloading the data so we can specify density and quality. Sheesh!
    std::string prefix(this->Filename + "[");
    for(auto it = this->Frame_List.begin(); it != this->Frame_List.end(); ++it){
        *it = Magick::Image();
        it->density("150x150");
        it->quality(99);
        it->read( (prefix + Xtostring<size_t>(std::distance(this->Frame_List.begin(), it)) + "]").c_str() ); 
    }
*/

    //Signal that we have initialized.
    this->Is_Initialized = true;
    return true;
}


void Generic_Visual_Media::De_Initialize(void){
    this->Load_Sane_Defaults();
    this->Frame_List.clear();
    return;
}

long int Generic_Visual_Media::Width(void) const {
    if( !this->Is_Initialized || (this->data == nullptr)) return -1;
    return this->lWidth;
}

long int Generic_Visual_Media::Height(void) const {
    if( !this->Is_Initialized || (this->data == nullptr)) return -1;
    return this->lHeight;
}
long int Generic_Visual_Media::Importance(void) const {
    if( !this->Is_Initialized || (this->data == nullptr)) return -1;
    return this->lImportance;
}

long int Generic_Visual_Media::index(long int col, long int row){    // (x,y) or (i,j)
//    return 4*(row + col*this->lHeight);
    return 4*(row + col*this->lWidth);
//    return 4*(row*this->lWidth  + col);
}
unsigned short & Generic_Visual_Media::R(long int col, long int row){    // (x,y) or (i,j)
    return (this->data[ this->index(col, row) + 0 ]);
}
unsigned short & Generic_Visual_Media::G(long int col, long int row){    // (x,y) or (i,j)
    return (this->data[ this->index(col, row) + 1 ]);
}
unsigned short & Generic_Visual_Media::B(long int col, long int row){    // (x,y) or (i,j)
    return (this->data[ this->index(col, row) + 2 ]);
}
unsigned short & Generic_Visual_Media::A(long int col, long int row){    // (x,y) or (i,j)
    return (this->data[ this->index(col, row) + 3 ]);
}


bool Generic_Visual_Media::Load_Current_Frame(void){
    //Check if we have initialized. Attempt to initialize if not so.
    if( (this->Is_Initialized != true) || (this->data == nullptr) ){
        if(!this->Initialize()){
            FUNCWARN("Unable to load current frame. Attempted silent initializatin failed");
            return false;
        }
    }

    //Check if the data has ever been loaded. If it has not, skip ahead and load the data.
    if( !(this->Polling_t0 < 0.0) ){
 
        //If we get here, we have previously loaded some data. If the image is static we happily
        // return.    
        if(this->Is_Static == true) return true;

        //If the animation delay time has been exceeded then we will continue to load the next
        // frame. Otherwise we happily return.
        if( (YgorPerformance_Get_Time() - this->Polling_tlast) < this->Frame_Delay_dt ) return true;

    }

    //If this is the first time loading the data, specify the t0 time as now.
    if( this->Polling_t0 < 0.0 ){
        this->Polling_t0 = YgorPerformance_Get_Time();
    }

    //Start of image decoding/loading into memory.
    Magick::Image magicimg;

    const auto numb_of_loaded_frames = Frame_List.size();
    if(numb_of_loaded_frames == 0){
        FUNCWARN("Attempted to read image data from an empty list. Ignoring");
        return false;
    }

    //Prepare the Magick::Image we are going to use.
    //
    //If the image is a Gif, then we have to select the proper frame.
    if(this->Is_Static != true){
        double framenumb_d   = (YgorPerformance_Get_Time() - this->Polling_t0) / this->Frame_Delay_dt;
        long int framenumb_i = static_cast<long int>(framenumb_d) % static_cast<long int>(numb_of_loaded_frames);
        //FUNCINFO("There are " << numb_of_loaded_frames << " frames in this image");
        //FUNCINFO("The frame delay is " << this->Frame_Delay_dt);
        //FUNCINFO("Loading frame number " << framenumb_d << " in double or " << framenumb_i);

        //Iterate over the list to the current frame.
        auto mi_it = Frame_List.begin();
        for(long int i = 0; i < framenumb_i; ++i) ++mi_it;

        magicimg = *(mi_it);

    //Otherwise, just load the file. We will only do this once for a static image.
    }else{
        magicimg = *(Frame_List.begin());
    }

    for(long int i=0; i<this->lHeight; ++i) for(long int j=0; j<this->lWidth; ++j){
        auto pxl = magicimg.pixelColor(j, i);
        const auto pxl_r = pxl.quantumRed();
        const auto pxl_g = pxl.quantumGreen();
        const auto pxl_b = pxl.quantumBlue();
        const auto pxl_a = pxl.quantumAlpha();
    
        this->R(i,j) = pxl_r;
        this->G(i,j) = pxl_g;
        this->B(i,j) = pxl_b;
        this->A(i,j) = pxl_a;
    }

    //Set the polling times.
    this->Polling_tlast = YgorPerformance_Get_Time();

    //Return successfully.
    return true;
};


//--------------------------------------------------------------------------------------------
//--------------------------------- Generic Text Element -------------------------------------
//--------------------------------------------------------------------------------------------
Generic_Text_Element::Generic_Text_Element(){
    this->lImportance = 40; //Something positive and arbitrary. Closer to 1 than inf.
}

Generic_Text_Element::Generic_Text_Element(const std::list<std::string> &in){
    this->lImportance = 40; //Something positive and arbitrary. Closer to 1 than inf.
    this->Lines = in;
}

long int Generic_Text_Element::Width(void) const {
    long int max = 0;
    for(auto it = this->Lines.begin(); it != this->Lines.end(); ++it){
        long int W = Bitmap_String_Total_Line_Width(*it);
        if(max < W) max = W;
    }
    return max;
}

long int Generic_Text_Element::Height(void) const {
    long int tot = 0;
    for(auto it = this->Lines.begin(); it != this->Lines.end(); ++it){
        tot += Bitmap_String_Max_Line_Height(); //*it);
    }
    return tot;
}
long int Generic_Text_Element::Height_of_Line(long int n) const {
    //All lines for this font are same-height.
    return Bitmap_String_Max_Line_Height();
}
long int Generic_Text_Element::Importance(void) const {
    return this->lImportance;
}

//--------------------------------------------------------------------------------------------
//------------------------------ Generic Screen Frame Struct ---------------------------------
//--------------------------------------------------------------------------------------------
void Generic_Screen_Frame::Arrange(struct Geometry Screen){
    //NOTE: This assumes that the T,B,L,R are coordinates of a 2D space. This routine will
    // NOT work if positive B and L denote negative coordinates. In other words, T and B 
    // must share a common axis. L and R must share a common axis. These common axes 
    // cannot be inverted.
    //const float sH = YGORABS(Screen.T) + YGORABS(Screen.B);
    //const float sW = YGORABS(Screen.L) + YGORABS(Screen.R);
//Moved this down locally. When Screen geom was being updated (eg. regions being marked 
// reserved for text) the changes in sW and sH were not being reflected.
//    const float sH = Screen.T - Screen.B;
//    const float sW = Screen.R - Screen.L;

    //----- Text first -----
    //Text usually hugs the corners of the screen. Therefore, we handle it first and 'lop off'
    // the portions of the screen which are occupied by text for the image routines.

    //Special case: no images, 1 text.
    if(this->Images.empty() && (this->Text.size() == 1)){
        //Center the text in the centre of the screen, vertically and horizontally.
        const float sH = Screen.T - Screen.B;
        const float sW = Screen.R - Screen.L;
        const auto tW = this->Text.front().Width();
        const auto tH = this->Text.front().Height();

        this->Text.front().Geom = Geometry(Screen.B + 0.5*sH + 0.5*tH, 0.0, Screen.L + 0.5*sW - 0.5*tW, 0.0);

        //TODO: verify the text will not extend out of the screen dimensions.
        // ...

        return;

    //Case: >0 images, 1 text.
    }else if(this->Text.size() == 1){
        //Place the text at the bottom left corner.
        const float sH = Screen.T - Screen.B;
        const float sW = Screen.R - Screen.L;
        const auto tW = this->Text.front().Width();
        const auto tH = this->Text.front().Height();
        this->Text.front().Geom = Geometry(Screen.B + tH, 0.0, Screen.L, 0.0);

        //With the remaining space, scale the image to fit (horizontally centered).
        //Give the text an extra line. Some fonts render above (some below) the point.
        const auto extraH = Bitmap_String_Max_Line_Height(); //"dummy");
        Screen.B += tH + extraH;

        //TODO: verify the text will not extend out of the screen dimensions.
        // ...

        //Do not return. Make sure the images are properly arranged.

    }

    if(this->Text.size() > 1){
        FUNCERR("Not sure how to handle multiple texts. Fix me");
        //Just stack at bottom like multiple images?
    }


    //----- Images -----
    //We can now ignore text and use the remaining space for images.

    //Case: 1 image.
    if(this->Images.size() == 1){
        const float sH = Screen.T - Screen.B;
        const float sW = Screen.R - Screen.L;
        const auto iW = static_cast<float>(this->Images.front().Width());
        const auto iH = static_cast<float>(this->Images.front().Height());

        Geometry orig(iH, 0.0, 0.0, iW);
        this->Images.front().Geom = Embed_Object_in_Geometry_Maintaining_Aspect(Screen,orig,2.0);

    //Case: up to 3 images.
    }else if(this->Images.size() <= 3){
        //Divide space stupidly: give each image a vertical section of the screen (if sW > sH)
        // or a horizontal section (otherwise).
        const float sH = Screen.T - Screen.B;
        const float sW = Screen.R - Screen.L;
        const float d = (sW > sH ? sW : sH)/static_cast<float>(this->Images.size());
        int N = 0;
        for(auto i_it = this->Images.begin(); i_it != this->Images.end(); ++i_it, ++N){
            const auto n = static_cast<float>(N);
            const auto iW = static_cast<float>(i_it->Width());
            const auto iH = static_cast<float>(i_it->Height());

            Geometry orig(iH, 0.0, 0.0, iW);
            Geometry slice(Screen);
            if(sW > sH){
                slice.L = 0.0;
                slice.R = d;
            }else{
                slice.B = 0.0;
                slice.T = d;
            }
            i_it->Geom = Embed_Object_in_Geometry_Maintaining_Aspect(slice,orig,2.0);

            if(sW > sH){
                i_it->Geom.L += Screen.L + n*d;
                i_it->Geom.R += Screen.L + n*d;
            }else{
                i_it->Geom.B += Screen.B + n*d;
                i_it->Geom.T += Screen.B + n*d; 
            }
        }

    //Case: up to 9 images.
    }else if(this->Images.size() <= 9){
        //There must be >3 panels, but no more than 3x3 panels. Make one dimension 3, 
        // so the other must be 2 or 3.
        const float sH = Screen.T - Screen.B;
        const float sW = Screen.R - Screen.L;
        const long int n = static_cast<long int>(this->Images.size());
        long int nx = 0, ny = 0;
        const float dW = sW/3.0, dH = sH/(n > 6 ? 3.0 : 2.0);

        for(auto i_it = this->Images.begin(); i_it != this->Images.end(); ++i_it, ++nx){
            if(nx >= 3){ //1-based, but nx and ny are 0-based. 
                ++ny;
                nx = 0;
            }
            const auto iW = static_cast<float>(i_it->Width());
            const auto iH = static_cast<float>(i_it->Height());
            Geometry orig(iH, 0.0, 0.0, iW);
            Geometry slice(dH, 0.0, 0.0, dW);

            i_it->Geom = Embed_Object_in_Geometry_Maintaining_Aspect(slice,orig,2.0);
            i_it->Geom.L += Screen.L + static_cast<float>(nx)*dW;
            i_it->Geom.R += Screen.L + static_cast<float>(nx)*dW;
            i_it->Geom.B += Screen.B + static_cast<float>(ny)*dH;
            i_it->Geom.T += Screen.B + static_cast<float>(ny)*dH;
        }

    //Case: more than 9 images.
    }else{
        //Most simple: divide space into rows of sqrt(N) equal sections.
        // NOTE: A better technique would be to try find integer factors of n
        // and then split up the space without waste (though some images would
        // then, maybe unfairly, get more screen space). FIXME.
        //
        //NOTE: A better technique would "smartly" decide to split horiz or vert
        // based on the dimensions of the remaining space. FIXME.
        const float sH = Screen.T - Screen.B;
        const float sW = Screen.R - Screen.L;
        const long int n = static_cast<long int>(this->Images.size());
        const long int Panels = std::ceil(std::sqrt(static_cast<float>(n)));
        const auto Panelsf = static_cast<float>(Panels);
        long int nx = 0, ny = 0;
        const float dW = sW/Panelsf, dH = sH/Panelsf;
        
        for(auto i_it = this->Images.begin(); i_it != this->Images.end(); ++i_it, ++nx){
            if(nx >= Panels){ //Panels is 1-based. nx and ny are 0-based. 
                ++ny; 
                nx = 0; 
            }
            const auto iW = static_cast<float>(i_it->Width());
            const auto iH = static_cast<float>(i_it->Height());
            Geometry orig(iH, 0.0, 0.0, iW);
            Geometry slice(dH, 0.0, 0.0, dW);

            i_it->Geom = Embed_Object_in_Geometry_Maintaining_Aspect(slice,orig,2.0);
            i_it->Geom.L += Screen.L + static_cast<float>(nx)*dW;
            i_it->Geom.R += Screen.L + static_cast<float>(nx)*dW;
            i_it->Geom.B += Screen.B + static_cast<float>(ny)*dH;
            i_it->Geom.T += Screen.B + static_cast<float>(ny)*dH;
        }
    }

    return;
}
