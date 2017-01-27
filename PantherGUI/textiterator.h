#pragma once

#include "textbuffer.h"
#include "textline.h"

class TextLineIterator {
	friend class TextFile;
public:
	virtual TextLine GetLine();
	friend TextLineIterator& operator++(TextLineIterator& element) {
		element.NextLine();
		return element;
	}
	friend TextLineIterator& operator--(TextLineIterator& element) {
		element.PrevLine();
		return element;
	}
	friend TextLineIterator& operator++(TextLineIterator& element, int i) {
		return ++element;
	}
	friend TextLineIterator& operator--(TextLineIterator& element, int i) {
		return --element;
	}

	virtual lng GetCurrentLineNumber() { return current_line; }
	virtual lng GetCurrentCharacterNumber() { return 0; }
	virtual PGVerticalScroll GetCurrentScrollOffset() { return PGVerticalScroll(current_line, 0); }

	PGTextBuffer* CurrentBuffer() { return buffer; }
	TextLineIterator(TextFile* textfile, PGTextBuffer* buffer);
protected:
	TextLineIterator(TextFile* textfile, lng current_line);
	TextLineIterator();

	void Initialize(TextFile* tf, lng current_line);

	virtual void PrevLine();
	virtual void NextLine();

	PGTextBuffer* buffer;
	lng start_position, end_position;
	TextFile* textfile;
	lng current_line;
	TextLine textline;
};