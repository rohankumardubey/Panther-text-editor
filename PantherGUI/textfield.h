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
public:
	TextField(PGWindowHandle, TextFile* file);
	~TextField();

	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGIRect*);
	void MouseWheel(int x, int y, int distance, PGModifier modifier);
	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	void KeyboardCharacter(char character, PGModifier modifier);
	void KeyboardUnicode(char *character, PGModifier modifier);
	
	void InvalidateLine(lng line);
	void InvalidateBeforeLine(lng line);
	void InvalidateAfterLine(lng line);
	void InvalidateBetweenLines(lng start, lng end);
	void InvalidateScrollbar();
	void InvalidateMinimap();

	void RefreshCursors();
	int GetLineHeight();

	void SetTextFile(TextFile* textfile);
	TextFile& GetTextFile() { return *textfile; }

	void OnResize(PGSize old_size, PGSize new_size);

	bool IsDragging() { return drag_type != PGDragNone; }
	PGCursorType GetCursor(PGPoint mouse);

	void MinimapMouseEvent(bool mouse_enter);
	void ScrollbarMouseEvent(bool mouse_enter);
private:
	PGIRect scrollbar_region;
	PGIRect minimap_region;

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

	lng tabwidth;

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
	lng GetMinimapStartLine();
	void SetMinimapOffset(PGScalar offset);

	TextFile* textfile;

	MouseClickInstance last_click;

	void DrawTextField(PGRendererHandle, PGIRect*, bool minimap, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay);

	void GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character, bool clip_character = true);
	bool SetScrollOffset(lng offset);
};