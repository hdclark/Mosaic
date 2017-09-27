//Flashcard_Parsing.cc
//
//These routines parse a special flashcard file format into frames.

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
#include "YgorPerformance.h"   //Needed for YgorPerformance_Get_Time();
#include "YgorAlgorithms.h"    //Needed for shuffle_list();
//#include "YgorMath.h"        //Needed for vec3 class.

#include "Structs.h"


static std::string Resolve_Flashcard_Image_File(const std::string &Filename, const std::string &Image){
    //'Filename' is the flashcard's filename.
    //Try to find the file described.
        
    //As-is.
    if(Does_File_Exist_And_Can_Be_Read(Image)) return Image;

    //By prepending the path of this file.
    const auto appended = Get_Parent_Directory(Fully_Expand_Filename(Filename)) + Image;
    if(Does_File_Exist_And_Can_Be_Read(appended)) return appended;

    //Else...ignore it. We will issue a warning downstream.
    return std::string();
}

std::list<Generic_Screen_Frame> Parse_Flashcard_File_into_Frames(const std::string &Filename){
    const long int text_width = 100; // chars.
    const long int text_indent = 4; //chars
    std::list<Generic_Screen_Frame> out;

    //Load into memory.
    std::list<std::string> lines = LoadFileToList(Filename);
    if(lines.empty()) return out;

    out.push_back(Generic_Screen_Frame());
    out.back().IsVirtual = false;

    //Canonicalize. Remove extra whitespace on the ends. Remove comment lines.
    for(auto s_it = lines.begin(); s_it != lines.end();      ){
        *s_it = Canonicalize_String2(*s_it, CANONICALIZE::TRIM_ENDS);
        if((*s_it)[0] == '#'){
            s_it = lines.erase(s_it);
        }else{
            ++s_it;
        }
    }

    std::list<std::string> Filenames;
    std::string ques_content;
    bool Ques_Open = false, Soln_Open = false, Img_Open = false, LaTeX_Open = false;

    //Cycle through, finding the 'question' content.
    for(auto s_it = lines.begin(); s_it != lines.end(); ++s_it){
        if(*s_it == "<ques>"){ Ques_Open = true; continue; }
        if(*s_it == "</ques>"){ Ques_Open = false; continue; }
        if(*s_it == "<img>"){ Img_Open = true; continue; }
        if(*s_it == "</img>"){ Img_Open = false; continue; }
        if(*s_it == "<latex>"){ LaTeX_Open = true; continue; }
        if(*s_it == "</latex>"){ LaTeX_Open = false; continue; }

        if(!Ques_Open) continue;
        if(Img_Open){
            Filenames.push_back(*s_it);
        }else{
            ques_content += *s_it + "\n";
        }
        if(LaTeX_Open){
            //Generate a LaTeX equation rendering and store it in a temporary file. 


            Filenames.push_back(*s_it);
        }
    }

    //Cycle through, finding the UID.
    bool UID_Open = false;
    for(auto s_it = lines.begin(); s_it != lines.end(); ++s_it){
        if(*s_it == "<uid>"){ UID_Open = true; continue; }
        if(*s_it == "</uid>"){ UID_Open = false; continue; }

        if(!UID_Open) continue;
        if(!s_it->empty()){
            out.back().UID = *s_it;
        }
    }
    const auto theUID = out.back().UID;

    std::list<std::string> ques_text;
    {
        //Reflow the so the text isn't excessively long.
        std::vector<std::string> paras = Reflow_Text_to_Fit_Width_Left_Just(ques_content, text_width, text_indent);
        for(auto s_it = paras.begin(); s_it != paras.end(); ++s_it){ 
            ques_text.emplace_back(*s_it);
        }

        ques_text.emplace_front("Question: ");

        out.back().Filename = Filename;
        out.back().Text.push_back( Generic_Text_Element(ques_text) );
        for(auto s_it = Filenames.begin(); s_it != Filenames.end(); ++s_it){
            //Try to find the file described.
            const auto Image_Filename = Resolve_Flashcard_Image_File(Filename,*s_it);
            if(Image_Filename.empty()){
                FUNCWARN("Could not find image file '" << *s_it << "' from flashcard '" << Filename << "'. Ignoring image");
            }else{
                out.back().Images.push_back( Generic_Visual_Media(Image_Filename) );
            }
        }
    }

    //Cycle through, finding the 'solution' content.
    //Text.clear();  //Maybe clear these, maybe not... Try it out.
    Filenames.clear();

    std::string soln_content;
    for(auto s_it = lines.begin(); s_it != lines.end(); ++s_it){
        if(*s_it == "<soln>"){ Soln_Open = true; continue; }
        if(*s_it == "</soln>"){ Soln_Open = false; continue; }
        if(*s_it == "<img>"){ Img_Open = true; continue; }
        if(*s_it == "</img>"){ Img_Open = false; continue; }

        if(!Soln_Open) continue;
        if(Img_Open){
            Filenames.push_back(*s_it);
        }else{
            soln_content += *s_it + "\n";
        }
    }

    ques_text.emplace_back("");
    ques_text.emplace_back("Solution: ");
    {
        std::vector<std::string> paras = Reflow_Text_to_Fit_Width_Left_Just(soln_content, text_width, text_indent);
        for(auto s_it = paras.begin(); s_it != paras.end(); ++s_it){ 
            ques_text.emplace_back(*s_it);
        }

        out.push_back(Generic_Screen_Frame());
        out.back().IsVirtual = true;
        out.back().Filename = Filename;
        out.back().UID = theUID;
        out.back().Text.push_back( Generic_Text_Element(ques_text) );
        for(auto s_it = Filenames.begin(); s_it != Filenames.end(); ++s_it){
            //Try to find the file described.
            const auto Image_Filename = Resolve_Flashcard_Image_File(Filename,*s_it);
            if(Image_Filename.empty()){
                FUNCWARN("Could not find image file '" << *s_it << "' from flashcard '" << Filename << "'. Ignoring image");
            }else{
                out.back().Images.push_back( Generic_Visual_Media(Image_Filename) );
            }
        }
    }


    //Dump the frames in the most basic html format. Useful for taking a 'snapshot' of the cards with
    // you as a pdf (or a bunch of webpages).
    if(false){
        long int framecount = 0;
        for(auto it = out.begin(); it != out.end(); ++it){
            ++framecount;
            std::stringstream BasicHTML;
            BasicHTML << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">" << std::endl;
            BasicHTML << "<html><body><center>" << std::endl;

            BasicHTML << "<p align=\"center\">" << std::endl;
            BasicHTML << it->Filename << "<br>" << std::endl;
            BasicHTML << "</p>" << std::endl;

            for(auto i_it = it->Images.begin(); i_it != it->Images.end(); ++i_it){
                const std::string dims = Execute_Command_In_Pipe("identify '"_s + i_it->Filename + "' | cut -d ' ' -f 3");
                std::stringstream ss(dims);
                long int W, H;
                char D;
                ss >> W >> D >> H;
                if(W > 450){
                    float AR = (float)(W)/(float)(H);
                    H = static_cast<long int>(450.0/AR);
                    W = 450;
                }
                BasicHTML << "<img width=\""<< W << "\" height=\"" << H << "\" align=\"center\" src=\"" << i_it->Filename << "\"/><br>" << std::endl;
            }

            BasicHTML << "<p align=\"center\">" << std::endl;
            for(auto t_it = it->Text.begin(); t_it != it->Text.end(); ++t_it){
                for(auto l_it = t_it->Lines.begin(); l_it != t_it->Lines.end(); ++l_it){
                    BasicHTML << *l_it << std::endl;
                    BasicHTML << "<br>" << std::endl;
                }
                BasicHTML << "<br>" << std::endl;
            }
            BasicHTML << "</p>" << std::endl;
            BasicHTML << "</body></html>" << std::endl;

            //BasicHTML can now be dumped somewhere. We will dump it as the original flashcard name with '.[#].autogened.html' appended.
            const std::string HTML_FO = it->Filename + Xtostring(framecount) + ".appended.html";
            if(!WriteStringToFile(BasicHTML.str(),HTML_FO)){
                FUNCWARN("Could not write auto-generated html file to '" << HTML_FO << "'. Ignoring");
            }
        }
    }

    return out;
}

