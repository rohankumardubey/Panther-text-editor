#pragma once

#include "control.h"
#include "cursor.h"
#include "textfile.h"
#include "time.h"

struct TextSelection {
	int line_start;
	int character_start;
	int line_end;
	int character_end;
};

#define SCROLLBAR_BASE_OFFSET 16
#define SCROLLBAR_WIDTH 16

class TextField : public Control {
	friend class Cursor;
public:
	TextField(PGWindowHandle, std::string filename);

	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGIRect*);
	void MouseWheel(int x, int y, int distance, PGModifier modifier);
	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	void KeyboardButton(PGButton button, PGModifier modifier);
	void KeyboardCharacter(char character, PGModifier modifier);
	void KeyboardUnicode(char *character, PGModifier modifier);
	
	void InvalidateLine(ssize_t line);
	void InvalidateBeforeLine(ssize_t line);
	void InvalidateAfterLine(ssize_t line);
	void InvalidateBetweenLines(ssize_t start, ssize_t end);
	void InvalidateScrollbar();
	void InvalidateMinimap();

	ssize_t GetLineOffset() { return lineoffset_y; }
	void SetLineOffset(ssize_t offset) { lineoffset_y = offset; }
	int GetLineHeight();
	void DisplayCarets();

	void ClearCursors(std::vector<Cursor*>&);

	Cursor*& GetActiveCursor() { return active_cursor; }
	TextFile& GetTextFile() { return textfile; }
	std::vector<Cursor*>& GetCursors() { return cursors; }
private:
	int offset_x;
	ssize_t lineoffset_y;
	std::vector<Cursor*> cursors;
	PGScalar text_offset;
	PGScalar line_height;
	PGScalar minimap_line_height;
	PGScalar character_width;
	int display_carets_count;
	bool display_carets;
	enum PGDragType {
		PGDragNone,
		PGDragSelection,
		PGDragSelectionCursors,
		PGDragScrollbar,
		PGDragMinimap
	};
	PGDragType drag_type;

	ssize_t tabwidth;

	bool current_focus;

	bool display_linenumbers;

	bool display_scrollbar;
	PGScalar drag_offset;

	PGScalar GetScrollbarHeight();
	PGScalar GetScrollbarOffset();
	void SetScrollbarOffset(PGScalar offset);

	bool display_minimap;
	PGScalar GetMinimapWidth();
	PGScalar GetMinimapHeight();
	PGScalar GetMinimapOffset();
	ssize_t GetMinimapStartLine();
	void SetMinimapOffset(PGScalar offset);

	Cursor* active_cursor;
	TextFile textfile;

	MouseClickInstance last_click;

	void DrawTextField(PGRendererHandle, PGIRect*, bool minimap, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay);

	void ClearExtraCursors();
	void GetLineCharacterFromPosition(PGScalar x, PGScalar y, ssize_t& line, ssize_t& character, bool clip_character = true);
	bool SetScrollOffset(ssize_t offset);
};