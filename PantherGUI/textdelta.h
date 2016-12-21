#pragma once

#include "cursor.h"
#include "textline.h"
#include "utils.h"

#include <string>
#include <vector>

class TextFile;

typedef enum {
	PGDeltaAddText,
	PGDeltaRemoveText,
	PGDeltaAddLine,
	PGDeltaAddManyLines,
	PGDeltaRemoveLine,
	PGDeltaRemoveManyLines,
	PGDeltaMultiple,
	PGDeltaCursorMovement,
	PGDeltaSwapLines,
	PGDeltaUnknown
} PGTextType;

class TextDelta {
public:
	lng linenr;
	lng characternr;
	TextDelta* next = nullptr;

	TextDelta() : next(nullptr) {}
	virtual ~TextDelta() { }

	virtual PGTextType TextDeltaType() { return PGDeltaUnknown; }
	TextDelta(lng linenr, lng characternr) : linenr(linenr), characternr(characternr), next(nullptr) { }
};

class CursorDelta : public TextDelta {
public:
	Cursor* cursor;
	Cursor stored_cursor;

	CursorDelta(Cursor* cursor, lng linenr, lng characternr) : cursor(cursor), stored_cursor(nullptr), TextDelta(linenr, characternr) {
		if (cursor) stored_cursor = Cursor(*cursor);
	}

	virtual PGTextType TextDeltaType() { return PGDeltaCursorMovement; }
};

class AddText : public CursorDelta {
public:
	std::string text;

	PGTextType TextDeltaType() { return PGDeltaAddText; }
	AddText(Cursor* cursor, lng linenr, lng characternr, std::string text) : CursorDelta(cursor, linenr, characternr), text(text) {}
};

class RemoveText : public CursorDelta {
public:
	lng charactercount;
	std::string removed_text;

	PGTextType TextDeltaType() { return PGDeltaRemoveText; }
	RemoveText(Cursor* cursor, lng linenr, lng characternr, lng charactercount) : CursorDelta(cursor, linenr, characternr), charactercount(charactercount) {}
};

class RemoveLine : public CursorDelta {
public:
	TextLine line;

	PGTextType TextDeltaType() { return PGDeltaRemoveLine; }
	RemoveLine(Cursor* cursor, lng linenr, TextLine line) : CursorDelta(cursor, linenr, characternr), line(line) {}
};

class RemoveLines : public CursorDelta {
public:
	std::vector<TextLine> lines;
	std::string extra_text;
	lng last_line_offset;

	PGTextType TextDeltaType() { return PGDeltaRemoveManyLines; }
	RemoveLines(Cursor* cursor, lng linenr) : CursorDelta(cursor, linenr, 0), last_line_offset(0) {}
	void AddLine(TextLine line) {
		lines.push_back(line);
	}
};

class AddLine : public CursorDelta {
public:
	int cursor_position;
	std::string line;
	std::string extra_text;
	RemoveText* remove_text;


	PGTextType TextDeltaType() { return PGDeltaAddLine; }
	AddLine(Cursor* cursor, lng linenr, lng characternr, std::string text) : CursorDelta(cursor, linenr, characternr), line(text), cursor_position(0), remove_text(nullptr) {}
	AddLine(Cursor* cursor, lng linenr, lng characternr, std::string text, int cursor_position) : CursorDelta(cursor, linenr, characternr), line(text), cursor_position(cursor_position), remove_text(nullptr) {}
};

class AddLines : public CursorDelta {
public:
	std::vector<std::string> lines;
	std::string extra_text;
	RemoveText* remove_text;


	PGTextType TextDeltaType() { return PGDeltaAddManyLines; }
	AddLines(Cursor* cursor, lng linenr, lng characternr, std::vector<std::string> lines) : CursorDelta(cursor, linenr, characternr), lines(lines), remove_text(nullptr) {}
};

class SwapLines : public TextDelta {
public:
	std::vector<Cursor*> cursors;
	std::vector<Cursor> stored_cursors;
	std::vector<TextLine> lines;
	int offset;

	PGTextType TextDeltaType() { return PGDeltaSwapLines; }
	SwapLines(lng linenr, int offset, TextLine line) : TextDelta(linenr, 0), offset(offset) { lines.push_back(line); }
};

class MultipleDelta : public TextDelta {
public:
	std::vector<TextDelta*> deltas;

	PGTextType TextDeltaType() { return PGDeltaMultiple; }
	MultipleDelta() : deltas() {}
	~MultipleDelta() {
		for (auto it = deltas.begin(); it != deltas.end(); it++) {
			delete *it;
		}
	}

	void AddDelta(TextDelta* delta) {
		deltas.push_back(delta);
	}
};

