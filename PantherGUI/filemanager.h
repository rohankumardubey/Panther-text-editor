#pragma once

#include "textfile.h"
#include "windowfunctions.h"

class FileManager;
class FileManager {
public:
	FileManager();
	~FileManager();

	TextFile* OpenFile();
	TextFile* OpenFile(std::string path);
	TextFile* OpenFile(TextFile* textfile);
	void CloseFile(TextFile*);

	std::vector<TextFile*>& GetFiles() { return open_files; }
private:
	TextField* textfield;
	std::vector<TextFile*> open_files;
};