//this MOSTLY is my code
//the goal of these data structures and classes is to hold text in memory when reading and editting existing journal entries
//using ONLY C-style arrays
#pragma once

//header files
#include "TextElement.h"
#include "FieldType.h"
#include "FieldValue.h"


using namespace std;

//holds entry data for a given jopurnal entry
class TextArray {
private:
	TextElement* elements;
	unsigned int size;
public:
	//constructor -- initialize elements to nullptr and size to 0 //corrected with claude5
	TextArray() : elements(nullptr), size(0) {}

	//copy constructor //corrected with claude5
	//same reasoning as the copy assignment operator below: this class owns raw dynamic
	//memory (elements) and has a destructor, so the compiler's default copy constructor
	//(which just copies the pointer) would leave two TextArray objects pointing at the
	//SAME memory. this version allocates a fresh array and deep-copies each element,
	//so "TextArray a = b;" or passing a TextArray by value both produce fully independent copies.
	TextArray(const TextArray& other) {
		size = other.size;
		elements = (size > 0) ? new TextElement[size] : nullptr;
		for (unsigned int i = 0; i < size; i++) {
			elements[i] = other.elements[i];
		}
	}

	//copy assignment operator //corrected with claude5
	//needed because this class manages raw dynamic memory (elements) and has a destructor.
	//without this, "myArray = TextArray();" (like JournalAPI does when loading a new record)
	//would just copy the POINTER, not the data -- leaving two TextArray objects pointing at
	//the SAME memory. when both get destroyed, the destructor calls delete[] twice on the
	//same pointer -> double-free -> undefined behavior / crash.
	//this version instead allocates its OWN array and copies each element over individually,
	//so each TextArray fully owns its own memory ("deep copy").
	TextArray& operator=(const TextArray& other) {
		if (this == &other) return *this; //guard against "x = x;" self-assignment

		delete[] elements; //free whatever this object currently owns

		size = other.size;
		elements = (size > 0) ? new TextElement[size] : nullptr;
		for (unsigned int i = 0; i < size; i++) {
			elements[i] = other.elements[i];
		}
		return *this;
	}

	//uses REFERENCE (alias) data for string (preventing unnecessary copying) -- courtessy of claude
	void addElement(const string& data, unsigned int offset, unsigned int sizeInBytes) {
		//increment size of array.
		size++;
		//if size is greater than zero create new dynamic array of elements containing <size-1> elements
		if (size>0) {

			//copy existing elements over from old array into placeholder array (if array is of size greater than 1)
			if (size>1) {

				//copy values into placeholder array
				TextElement* placeHolder = new TextElement[size-1];
				for (unsigned int i=0;i<size-1;i++) {
						placeHolder[i]=elements[i]; //corrected with claude5 -- missing semicolon
				}
				//delete old array
				delete[] elements;
				elements = nullptr;

				//create new dynamic array (this one is full size)
				elements = new TextElement[size];
				//iterate through it
				for (unsigned int i=0;i<size;i++) {
					//if its less than size-1 copy back from placeholder
					if (i<(size-1)) {
						elements[i] = placeHolder[i];
					} else {
						//otherwise add new data members provided as args to function
						elements[i].data = data;
						elements[i].length = sizeInBytes;
					}
				}
				delete[] placeHolder;
				placeHolder = nullptr;
			} else {
				//size==1 case: this is the very first element ever added, so elements is still nullptr.
				//must allocate here or nothing ever gets stored. //corrected with claude5
				elements = new TextElement[1];
				elements[0].data = data;
				elements[0].length = sizeInBytes;
			}
		}
	}
	unsigned int getSize() {
		return size;
	}

	//returns the text data of a single line by index -- read-only accessor //corrected with claude5
	string getLine(unsigned int index) {
		return elements[index].data;
	}

	//returns the byte length of a single line by index -- read-only accessor //corrected with claude5
	unsigned int getLength(unsigned int index) {
		return elements[index].length;
	}

	//TAKES
	void updateElement(unsigned int index, FieldType field, const FieldValue& value) {	//takes field value by reference
		//determine the datatype of the value based on the provided enum type
		switch (field) {
			case FieldType::DATA:
				elements[index].data = value.getString();
				break;
			case FieldType::LENGTH: //corrected with claude5 -- was a semicolon instead of a colon
				elements[index].length = value.getUInt();
				break;
		}
	}

	void removeElement(unsigned int index) {
		//if the size is already zero, there is nothing to do so return
		if (size==0) {return;}
		//if the index is less than zero or greater than/equal to
		//decrement size of array
		size--;
		//create placeholder array if size <size> is greater than zero
		if (size>0) {
			if (size>1) {
				TextElement* placeHolder = new TextElement[size+1];
				//save elements to placeholder
				for (unsigned int i=0;i<size+1;i++) {
					placeHolder[i] = elements[i];
				}
				delete[] elements;
				elements=nullptr;
				unsigned int counterOffset=0; //counter offset (zero before i==index, -1 after
				int offsetDecrementCounter=0;
				elements = new TextElement[size];
				for (unsigned int i=0; i<size+1;i++) {
					//skip over index of element being removed
					if (i==index) {
						offsetDecrementCounter++; //corrected with claude5 -- moved before continue, was unreachable
						continue;
					}
					//mechanism for delaying decrement of iterator offset
					if (offsetDecrementCounter>0) {
						offsetDecrementCounter--;
						if (offsetDecrementCounter==0) { //corrected with claude5 -- fixed typo (was offsetDecrementCountdown)
							//if this value gets changed AND reaches zero after said change, decrement the countdown offset
							counterOffset++;
						}
					}
					//now do logic with the newly calculated index, which should guarentee no overlap of indices following skip
					unsigned int actualIndex = i - counterOffset;	//has to use subtraction operation BECAUSE int is implicitly converted to unsigned int when performing operation between int and unsigned int
					elements[actualIndex] = placeHolder[i];
				}
				delete[] placeHolder;
				placeHolder = nullptr;
			} else {
				delete[] elements;
				elements = nullptr;
			}

		}

	}

	//destructor -- frees dynamically allocated array, prevents memory leak //corrected with claude5
	~TextArray() {
		delete[] elements;
		elements = nullptr;
	}
};
//referred to as 'FieldValue' in TextArray class. alias necessary
typedef SimpleVariant FieldValue;


//corrected with claude5
