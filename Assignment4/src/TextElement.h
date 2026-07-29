#pragma once
//this is my code
#include <string>

//hold an element
struct TextElement {
       	std::string data="\0";       //ascii null terminator as placeholder for empty string
        unsigned int length;    //unsigned integers (or size_t) used to prevent overflow on extra LONG journal entries (although it shouldnt be a problem since elements will be newline delimitted)
};
