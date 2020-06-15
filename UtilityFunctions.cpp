//
// Created by Pitam Mitra on 2019-07-29.
//
#include "UtilityFunctions.hpp"
#include <map>
/*
Basic Colours:

black        30         40
red          31         41
green        32         42
yellow       33         43
blue         34         44
magenta      35         45
cyan         36         46
white        37         47

Additionally, you can use these:

reset             0  (everything back to normal)
bold/bright       1  (often a brighter shade of the same colour)
underline         4
inverse           7  (swap foreground and background colours)
bold/bright off  21
underline off    24
inverse off      27
*/


std::string ColouredFmtText(std::string text, std::string colour, std::string fmt){

    std::map<std::string, int> colour_to_int;
    colour_to_int["black"] = 30;
    colour_to_int["red"] = 31;
    colour_to_int["green"] = 32;
    colour_to_int["yellow"] = 33;
    colour_to_int["blue"] = 34;
    colour_to_int["magenta"] = 35;
    colour_to_int["cyan"] = 36;
    colour_to_int["white"] = 37;

    std::map<std::string, int> fmt_to_int;
    fmt_to_int["reset"]=0;
    fmt_to_int["bold"]=1;
    fmt_to_int["underline"]=4;
    fmt_to_int["inverse"]=7;
    fmt_to_int["boldoff"]=21;
    fmt_to_int["underlineoff"]=24;
    fmt_to_int["inverseoff"]=27;

    std::string formatted_string;

    if (fmt=="std"){
        formatted_string =  "\033["+std::to_string(colour_to_int[colour])+"m"+text+"\033[0m";
    } else {
        formatted_string =  "\033["+std::to_string(fmt_to_int[fmt])+";"+std::to_string(colour_to_int[colour])+"m"+text+"\033[0m";
    }

    return formatted_string;

}


std::string ColouredFmtText(int text, std::string colour, std::string fmt){
    std::string text_string = std::to_string(text);
    return ColouredFmtText(text_string, colour, fmt);
}
