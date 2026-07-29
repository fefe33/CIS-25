//JournalAPI - manages the on-disk journal format:
//  indexFile:  [uint32 numRecords][uint32 length_0][uint32 length_1]...[uint32 length_N-1]
//  dataFile:   <record0 bytes>\xFF<record1 bytes>\xFF...<recordN-1 bytes>\xFF
//
//holds ONE TextArray at a time representing the record currently loaded for editing.
//each TextElement in that TextArray is one LINE of the current record.

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include "TextArrays.h"

using namespace std;

class JournalAPI {
private:
	string indexPath;
	string dataPath;
	vector<uint32_t> lengths; //length (in bytes) of each full record, mirrors index file
	TextArray currentRecord;  //the record currently loaded for editing (lines as TextElements)
	int loadedIndex;          //which record index currentRecord holds, -1 if none/new

	//computes byte offset of a given record within the data file
	uint64_t offsetForRecord(unsigned int index) const {
		uint64_t offset = 0;
		for (unsigned int i = 0; i < index; i++) {
			offset += lengths[i] + 1; //+1 for the 0xFF delimiter
		}
		return offset;
	}

	//rewrites the index file from the in-memory `lengths` vector
	bool writeIndex() {
		ofstream out(indexPath, ios::binary | ios::trunc);
		if (!out) return false;

		uint32_t numRecords = static_cast<uint32_t>(lengths.size());
		out.write(reinterpret_cast<const char*>(&numRecords), sizeof(numRecords));
		for (uint32_t len : lengths) {
			out.write(reinterpret_cast<const char*>(&len), sizeof(len));
		}
		return true;
	}

	//reads the raw bytes (whole record, newlines and all) for a record index straight off disk
	string readRawRecord(unsigned int index) {
		ifstream in(dataPath, ios::binary);
		in.seekg(offsetForRecord(index));

		string raw(lengths[index], '\0');
		in.read(&raw[0], lengths[index]);
		return raw;
	}

	//splits raw record text into lines and loads them into currentRecord
	void loadIntoCurrentRecord(const string& raw) {
		currentRecord = TextArray(); //reset buffer

		size_t start = 0;
		size_t pos;
		while ((pos = raw.find('\n', start)) != string::npos) {
			string line = raw.substr(start, pos - start);
			currentRecord.addElement(line, 0, static_cast<unsigned int>(line.size()));
			start = pos + 1;
		}
		//last line (or the whole thing if there were no newlines at all)
		if (start < raw.size()) {
			string line = raw.substr(start);
			currentRecord.addElement(line, 0, static_cast<unsigned int>(line.size()));
		}
	}

	//joins currentRecord's lines back into one raw string (lines separated by \n)
	string flattenCurrentRecord() {
		string raw;
		unsigned int n = currentRecord.getSize();
		for (unsigned int i = 0; i < n; i++) {
			raw += currentRecord.getLine(i); //see note below re: TextArray needing a getter
			if (i + 1 < n) raw += '\n';
		}
		return raw;
	}

public:
	JournalAPI(const string& indexPath_, const string& dataPath_)
		: indexPath(indexPath_), dataPath(dataPath_), loadedIndex(-1) {}

	//reads the index file into memory (call once at startup)
	bool loadIndex() {
		ifstream in(indexPath, ios::binary);
		if (!in) return false;

		uint32_t numRecords = 0;
		in.read(reinterpret_cast<char*>(&numRecords), sizeof(numRecords));
		lengths.resize(numRecords);
		for (uint32_t i = 0; i < numRecords; i++) {
			in.read(reinterpret_cast<char*>(&lengths[i]), sizeof(uint32_t));
		}
		return true;
	}

	unsigned int recordCount() const {
		return static_cast<unsigned int>(lengths.size());
	}

	//public read-only access to a record's full raw text (all lines joined), for search/sort //corrected with claude5
	string getRecordText(unsigned int index) {
		return readRawRecord(index);
	}

	//searches all records for a keyword; returns indices of matches //corrected with claude5
	vector<unsigned int> searchRecords(const string& keyword) {
		vector<unsigned int> matches;
		for (unsigned int i = 0; i < recordCount(); i++) {
			string text = getRecordText(i);
			if (text.find(keyword) != string::npos) {
				matches.push_back(i);
			}
		}
		return matches;
	}

	//returns record indices sorted by length, ascending -- manual selection sort //corrected with claude5
	//(no std::sort -- written by hand so the sorting logic is my own)
	vector<unsigned int> sortedByLength() {
		vector<unsigned int> indices;
		for (unsigned int i = 0; i < recordCount(); i++) indices.push_back(i);

		unsigned int n = static_cast<unsigned int>(indices.size());
		//selection sort: for each position, find the smallest remaining element and swap it in
		for (unsigned int i = 0; i < n; i++) {
			unsigned int smallest = i;
			for (unsigned int j = i + 1; j < n; j++) {
				if (lengths[indices[j]] < lengths[indices[smallest]]) {
					smallest = j;
				}
			}
			if (smallest != i) {
				unsigned int temp = indices[i];
				indices[i] = indices[smallest];
				indices[smallest] = temp;
			}
		}
		return indices;
	}

	//returns record indices sorted alphabetically by their text -- manual selection sort //corrected with claude5
	vector<unsigned int> sortedAlphabetically() {
		vector<unsigned int> indices;
		for (unsigned int i = 0; i < recordCount(); i++) indices.push_back(i);

		unsigned int n = static_cast<unsigned int>(indices.size());
		for (unsigned int i = 0; i < n; i++) {
			unsigned int smallest = i;
			for (unsigned int j = i + 1; j < n; j++) {
				//string has its own < operator (lexicographic compare), so this works directly
				if (getRecordText(indices[j]) < getRecordText(indices[smallest])) {
					smallest = j;
				}
			}
			if (smallest != i) {
				unsigned int temp = indices[i];
				indices[i] = indices[smallest];
				indices[smallest] = temp;
			}
		}
		return indices;
	}

	//deletes a record entirely (not just a line within it) -- rewrites the data file
	//without that record, and removes its entry from the lengths list. //corrected with claude5
	bool removeRecord(unsigned int index) {
		if (index >= lengths.size()) return false;

		//read every OTHER record's raw text first (using the OLD lengths/offsets,
		//same reasoning as in save()'s update path -- must read before mutating lengths)
		vector<string> remaining;
		for (unsigned int i = 0; i < lengths.size(); i++) {
			if (i == index) continue;
			remaining.push_back(readRawRecord(i));
		}

		//remove this record's length entry
		lengths.erase(lengths.begin() + index);

		//rewrite the data file with everything except the removed record
		ofstream out(dataPath, ios::binary | ios::trunc);
		if (!out) return false;
		for (const auto& rec : remaining) {
			out.write(rec.data(), rec.size());
			out.put(static_cast<char>(0xFF));
		}
		out.close();

		//if the record we just deleted was currently loaded for editing, clear the buffer
		if (loadedIndex == static_cast<int>(index)) {
			newRecord();
		}

		return writeIndex();
	}

	//loads record `index` off disk into currentRecord for editing
	bool loadRecord(unsigned int index) {
		if (index >= lengths.size()) return false;
		string raw = readRawRecord(index);
		loadIntoCurrentRecord(raw);
		loadedIndex = static_cast<int>(index);
		return true;
	}

	//resets currentRecord to empty, for adding a brand new entry
	void newRecord() {
		currentRecord = TextArray();
		loadedIndex = -1; //-1 means "not an existing record" -> save() will append
	}

	//gives the menu direct access to manipulate lines while editing
	TextArray& getCurrentRecord() {
		return currentRecord;
	}

	//flushes currentRecord to disk: appends if new, rewrites data file if updating existing
	bool save() {
		string raw = flattenCurrentRecord();

		if (loadedIndex == -1) {
			//APPEND new record
			ofstream out(dataPath, ios::binary | ios::app);
			if (!out) return false;
			out.write(raw.data(), raw.size());
			out.put(static_cast<char>(0xFF));
			out.close();

			lengths.push_back(static_cast<uint32_t>(raw.size()));
			loadedIndex = static_cast<int>(lengths.size() - 1);
		} else {
			//UPDATE existing record -- read all OTHER records first (using OLD lengths),
			//then rewrite entire data file with the new text swapped in.
			unsigned int idx = static_cast<unsigned int>(loadedIndex);
			vector<string> allRaw(lengths.size());
			for (unsigned int i = 0; i < lengths.size(); i++) {
				allRaw[i] = (i == idx) ? raw : readRawRecord(i);
			}

			lengths[idx] = static_cast<uint32_t>(raw.size());

			ofstream out(dataPath, ios::binary | ios::trunc);
			if (!out) return false;
			for (const auto& rec : allRaw) {
				out.write(rec.data(), rec.size());
				out.put(static_cast<char>(0xFF));
			}
		}

		return writeIndex();
	}
};

//corrected with claude5
