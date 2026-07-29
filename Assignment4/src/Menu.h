//had to speedrun this file. gave it over to claude5 to handle (due to extreme time crunch)

#include <iostream>
#include <map>	//learned about maps and unordered maps & their pros and cons from claude5
#include <string>
#include <functional> //corrected with claude5 -- needed for std::function used in the command map

#include "Files.h"
#include "TextArrays.h"

using namespace std; //corrected with claude5 -- needed since cin/cout/string are used unqualified below

const string DEFAULT_BYTEFILE = "Bytes.data";
const string DEFAULT_SAVEFILE = "data.journal";


//corrected with claude5 -- "NULL" cannot be used as an enumerator name, it's a macro (usually 0 or nullptr)
//defined in standard headers and will clash/fail to compile. renamed to NONE.
enum class MenuState { MAIN, SELECT, TEXT, TEXTUPDATE, NONE };
enum class SelectionContext { VIEW, UPDATE, REMOVE, NONE };

class Menu {
private:
	JournalAPI JAPI;	//use the journalAPI (written by me and claude5) to interact with the file (while both on disk and in memory)
	bool running;
	MenuState state;
	SelectionContext context;

	//corrected with claude5 -- switched from a fixed-size array (which didn't have enough slots
	//for all 5 MenuState values and would read out of bounds) to a map. ties keys directly to
	//the enum instead of relying on matching array index order by hand.
	map<MenuState, string> MenuPrompts = {
		{MenuState::MAIN, "Select an option:\n-------------\n1: View Entries\n2: Create Entry\n3: Remove Entry\n4: Update Entry\n5: Exit\n\n?>"},
		{MenuState::SELECT, "#>"},
		{MenuState::TEXT, "+>"},
		{MenuState::TEXTUPDATE, "+>"}
	};

	//index the user picked in SELECT state, carried over into TEXTUPDATE //corrected with claude5
	unsigned int selectedIndex = 0;

	//command dispatch map for TEXT/TEXTUPDATE states //corrected with claude5
	//"<>"  -> finish and save the record, return to MAIN
	//"<->" -> remove the previously entered line
	//(note: "<+>" is handled separately below since it's a SUFFIX on a real line
	//of content, not a standalone command, so it can't be an exact-match map key)
	map<string, function<void()>> textCommands = {
		{"<>", [this](){
			JAPI.save();
			state = MenuState::MAIN;
		}},
		{"<->", [this](){
			unsigned int size = JAPI.getCurrentRecord().getSize();
			if (size > 0) {
				JAPI.getCurrentRecord().removeElement(size - 1);
			} else {
				cout << "nothing to remove\n";
			}
		}}
	};

	//shared handler for TEXT and TEXTUPDATE -- both states collect lines into the
	//current record the same way, so there's no need to duplicate this logic. //corrected with claude5
	void handleTextInput() {
		auto it = textCommands.find(UIBuffer);
		if (it != textCommands.end()) {
			it->second(); //run the matched command (save-and-exit, or remove-last-line)
			return;
		}

		//check for the "<+>" suffix marker on an otherwise normal line of content
		const string marker = "<+>";
		if (UIBuffer.size() >= marker.size() &&
		    UIBuffer.compare(UIBuffer.size() - marker.size(), marker.size(), marker) == 0) {
			string line = UIBuffer.substr(0, UIBuffer.size() - marker.size());
			JAPI.getCurrentRecord().addElement(line, 0, static_cast<unsigned int>(line.size()));
			return;
		}

		//default: no command matched, no marker found -- just add the line as-is
		JAPI.getCurrentRecord().addElement(UIBuffer, 0, static_cast<unsigned int>(UIBuffer.size()));
	}

public:
	//constructor -- initializes JAPI with real file paths, sets starting state //corrected with claude5
	Menu() : Menu(DEFAULT_BYTEFILE, DEFAULT_SAVEFILE) {}

	//(JournalAPI has no default constructor, so JAPI must be initialized here via the member init list)
	Menu(const string& indexPath, const string& dataPath)
		: JAPI(indexPath, dataPath), running(true), state(MenuState::MAIN), context(SelectionContext::NONE) {
		JAPI.loadIndex();
	}

	void clearScreanANSI() {
		//clear the screen using ANSI escape sequences
		//generated/recommended by claude4
		cout << "\033[2J\033[1;1H";
	}

	//buffer to hold input text
	string UIBuffer = "";

	void flushBuffer() {
		UIBuffer = "";
	}

	//called after getInput()
	void processBuffer() {
		//check the menu state
		switch(state) {
			//if its in null state, silently return
			case MenuState::NONE:
				return;

			case MenuState::MAIN:
				//process main menu input
				//primary purpose: redirect user to other menus
				if (UIBuffer=="5") {
					//exit
					cout << "Goodbye...";
					running = false;
				} else if (UIBuffer=="1") {
					//view
					//set selection context
					context = SelectionContext::VIEW;
					state = MenuState::SELECT;
				} else if (UIBuffer=="2") {
					//create new record
					JAPI.newRecord();
					state = MenuState::TEXT;
				} else if (UIBuffer=="3") {
					//route to SELECT so the user can pick WHICH entry to remove //corrected with claude5
					context = SelectionContext::REMOVE;
					state = MenuState::SELECT;
				} else if (UIBuffer=="4") {
					context = SelectionContext::UPDATE; //corrected with claude5 -- was "==" (comparison), needed "=" (assignment)
					state = MenuState::SELECT; //corrected with claude5 -- was missing semicolon, and routed to SELECT first (need to pick WHICH entry before editing it)
				} else {
					cout << "invalid input\n";
				}
				break;

			case MenuState::SELECT: {
				//process journal selection input from buffer
				unsigned int idx;
				try {
					idx = static_cast<unsigned int>(stoi(UIBuffer));
				} catch (...) {
					cout << "invalid selection\n";
					state = MenuState::MAIN;
					break;
				}

				if (idx >= JAPI.recordCount()) {
					cout << "no such entry\n";
					state = MenuState::MAIN;
					break;
				}

				JAPI.loadRecord(idx);
				selectedIndex = idx;

				if (context == SelectionContext::VIEW) {
					//just display it, then return to main menu
					for (unsigned int i = 0; i < JAPI.getCurrentRecord().getSize(); i++) {
						cout << JAPI.getCurrentRecord().getLine(i) << "\n";
					}
					state = MenuState::MAIN;
				} else if (context == SelectionContext::UPDATE) {
					//move into editing mode for this record
					state = MenuState::TEXTUPDATE;
				} else if (context == SelectionContext::REMOVE) {
					//delete the record outright, then return to main menu //corrected with claude5
					JAPI.removeRecord(idx);
					cout << "entry removed\n";
					state = MenuState::MAIN;
				}
				break;
			}

			case MenuState::TEXT:
			case MenuState::TEXTUPDATE:
				//both states collect lines the same way -- see handleTextInput() //corrected with claude5
				handleTextInput();
				break;
		}
	}

	void getInput() {
		//corrected with claude5 -- was "cin >> UIBuffer", which stops at the first whitespace.
		//that's fine for menu numbers, but breaks multi-word journal text. getline reads the
		//WHOLE line instead.
		getline(cin, UIBuffer);
	}

	void setMenuState(MenuState newState) {
		state = newState;
	}

	void run() {
		while (running) {
			//output determined by menu state
			cout << MenuPrompts[state];
			getInput();
			processBuffer();
			flushBuffer();
		}
	}
};

//corrected with claude5
