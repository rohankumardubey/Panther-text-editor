
#include "basictextfield.h"
#include "textview.h"
#include "wrappedtextiterator.h"

TextView::TextView(BasicTextField* textfield) :
	textfield(textfield), wordwrap(false),
	xoffset(0), yoffset(0, 0) {
	cursors.push_back(Cursor(this));
}

TextLineIterator* TextView::GetScrollIterator(BasicTextField* textfield, PGVerticalScroll scroll) {
	if (wordwrap) {
		return new WrappedTextLineIterator(textfield->GetTextfieldFont(), file.get(),
			scroll, wordwrap_width);
	}
	return new TextLineIterator(file.get(), scroll.linenumber);
}

TextLineIterator* TextView::GetLineIterator(BasicTextField* textfield, lng linenumber) {
	if (wordwrap) {
		return new WrappedTextLineIterator(textfield->GetTextfieldFont(), file.get(),
			PGVerticalScroll(linenumber, 0), wordwrap_width);
	}
	return new TextLineIterator(file.get(), linenumber);
}
double TextView::GetScrollPercentage(PGVerticalScroll scroll) {
	if (wordwrap) {
		auto buffer = file->GetBuffer(scroll.linenumber);

		double width = buffer->cumulative_width;
		for (lng i = buffer->start_line; i < scroll.linenumber; i++)
			width += buffer->line_lengths[i - buffer->start_line];

		TextLine textline = file->GetLine(scroll.linenumber);
		lng inner_lines = textline.RenderedLines(buffer, scroll.linenumber, file->GetLineCount(), textfield->GetTextfieldFont(), wordwrap_width);
		width += ((double)scroll.inner_line / (double)inner_lines) * buffer->line_lengths[scroll.linenumber - buffer->start_line];
		return file->total_width == 0 ? 0 : width / file->total_width;
	} else {
		return file->linecount == 0 ? 0 : (double)scroll.linenumber / file->linecount;
	}
}

double TextView::GetScrollPercentage() {
	return (GetScrollPercentage(GetLineOffset()));
}

PGVerticalScroll TextView::GetLineOffset() {
	assert(yoffset.linenumber >= 0 && yoffset.linenumber < file->linecount);
	return yoffset;
}

void TextView::SetLineOffset(lng offset) {
	assert(offset >= 0 && offset < file->linecount);
	yoffset.linenumber = offset;
	yoffset.inner_line = 0;
}

void TextView::SetLineOffset(PGVerticalScroll scroll) {
	assert(scroll.linenumber >= 0 && scroll.linenumber < file->linecount);
	yoffset.linenumber = scroll.linenumber;
	yoffset.inner_line = scroll.inner_line;
}

void TextView::SetScrollOffset(lng offset) {
	if (!wordwrap) {
		SetLineOffset(offset);
	} else {
		double percentage = (double)offset / (double)GetMaxYScroll();
		double width = percentage * file->total_width;
		auto buffer = file->buffers[PGTextBuffer::GetBufferFromWidth(file->buffers, width)];
		double start_width = buffer->cumulative_width;
		lng line = 0;
		lng max_line = buffer->GetLineCount(file->GetLineCount());
		while (line < max_line) {
			double next_width = start_width + buffer->line_lengths[line];
			if (next_width >= width) {
				break;
			}
			start_width = next_width;
			line++;
		}
		line += buffer->start_line;
		// find position within buffer
		TextLine textline = file->GetLine(line);
		lng inner_lines = textline.RenderedLines(buffer, line, file->GetLineCount(), textfield->GetTextfieldFont(), wordwrap_width);
		percentage = buffer->line_lengths[line - buffer->start_line] == 0 ? 0 : (width - start_width) / buffer->line_lengths[line - buffer->start_line];
		percentage = std::max(0.0, std::min(1.0, percentage));
		PGVerticalScroll scroll;
		scroll.linenumber = line;
		scroll.inner_line = (lng)(percentage * inner_lines);
		SetLineOffset(scroll);
	}
}

lng TextView::GetMaxYScroll() {
	if (!wordwrap) {
		return file->GetLineCount() - 1;
	} else {
		return std::max((lng)(file->total_width / wordwrap_width), file->GetLineCount() - 1);
	}
}

PGVerticalScroll TextView::GetVerticalScroll(lng linenumber, lng characternr) {
	if (!wordwrap) {
		return PGVerticalScroll(linenumber, 0);
	} else {
		PGVerticalScroll scroll = PGVerticalScroll(linenumber, 0);
		auto it = GetLineIterator(textfield, linenumber);
		for (;;) {
			it->NextLine();
			if (!it->GetLine().IsValid()) break;
			if (it->GetCurrentLineNumber() != linenumber) break;
			if (it->GetCurrentCharacterNumber() >= characternr) break;
			scroll.inner_line++;
		}
		return scroll;
	}
}

PGVerticalScroll TextView::OffsetVerticalScroll(PGVerticalScroll scroll, double offset) {
	lng lines_offset;
	return OffsetVerticalScroll(scroll, offset, lines_offset);
}

PGVerticalScroll TextView::OffsetVerticalScroll(PGVerticalScroll scroll, double offset, lng& lines_offset) {
	if (offset == 0) {
		lines_offset = 0;
		return scroll;
	}

	// first perform any fractional (less than one line) scrolling
	double partial = offset - (lng)offset;
	offset = (lng)offset;
	scroll.line_fraction += partial;
	if (scroll.line_fraction >= 1) {
		// the fraction is >= 1, we have to advance one actual line now
		lng floor = (lng)scroll.line_fraction;
		scroll.line_fraction -= floor;
		offset += floor;
	} else if (scroll.line_fraction < 0) {
		// the fraction is < 0, we have to go one line back now
		lng addition = (lng)std::ceil(std::abs(scroll.line_fraction));
		scroll.line_fraction += addition;
		offset -= addition;
		if (scroll.linenumber <= 0 && (!wordwrap || scroll.inner_line <= 0)) {
			// if we are at the first line in the file and we go backwards
			// we cannot actually go back one line from this position
			// so we avoid setting the line_fraction back to a value > 0
			scroll.line_fraction = 0;
		}
	}
	// line_fraction should always be a fraction
	assert(scroll.line_fraction >= 0 && scroll.line_fraction <= 1);
	if (!wordwrap) {
		// no wordwrap, simply add offset to the linenumber count
		lng original_linenumber = scroll.linenumber;
		scroll.linenumber += (lng)offset;
		lng max_y_scroll = GetMaxYScroll();
		scroll.linenumber = std::max(std::min(scroll.linenumber, max_y_scroll), (lng)0);
		if (scroll.linenumber >= max_y_scroll) {
			// we cannot have a fraction > 0 if we are at the last line of the file
			scroll.line_fraction = 0;
		}
		lines_offset = std::abs(scroll.linenumber - original_linenumber);
	} else {
		lines_offset = 0;
		lng lines = offset;
		TextLineIterator* it = GetScrollIterator(textfield, scroll);
		if (lines > 0) {
			// move forward by <lines>
			for (; lines != 0; lines--) {
				it->NextLine();
				if (!it->GetLine().IsValid()) {
					break;
				}
				lines_offset++;
			}
		} else {
			// move backward by <lines>
			for (; lines != 0; lines++) {
				it->PrevLine();
				if (!(it->GetLine().IsValid())) {
					break;
				}
				lines_offset++;
			}
		}
		double fraction = scroll.line_fraction;
		scroll = it->GetCurrentScrollOffset();
		it->NextLine();
		if (!it->GetLine().IsValid()) {
			// we cannot have a fraction > 0 if we are at the last line of the file
			fraction = 0;
		}
		scroll.line_fraction = fraction;
	}
	return scroll;
}

void TextView::OffsetLineOffset(double offset) {
	yoffset = OffsetVerticalScroll(yoffset, offset);
}

void TextView::SetCursorLocation(lng line, lng character) {
	ClearExtraCursors();
	cursors[0].SetCursorLocation(line, character);
	if (textfield) textfield->SelectionChanged();
}

void TextView::SetCursorLocation(lng start_line, lng start_character, lng end_line, lng end_character) {
	ClearExtraCursors();
	cursors[0].SetCursorLocation(end_line, end_character);
	cursors[0].SetCursorStartLocation(start_line, start_character);
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::SetCursorLocation(PGTextRange range) {
	ClearExtraCursors();
	cursors[0].SetCursorLocation(range);
	Cursor::NormalizeCursors(this, cursors);

}

void TextView::AddNewCursor(lng line, lng character) {
	cursors.push_back(Cursor(this, line, character));
	active_cursor = cursors.size() - 1;
	std::sort(cursors.begin(), cursors.end(), Cursor::CursorOccursFirst);
	Cursor::NormalizeCursors(this, cursors, false);
}

void TextView::OffsetLine(lng offset) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetLine(offset);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetSelectionLine(lng offset) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetSelectionLine(offset);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetCharacter(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		if (it->SelectionIsEmpty()) {
			it->OffsetCharacter(direction);
		} else {
			auto pos = direction == PGDirectionLeft ? it->BeginCharacterPosition() : it->EndCharacterPosition();
			it->SetCursorLocation(pos.line, pos.character);
		}
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetSelectionCharacter(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetSelectionCharacter(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetWord(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetWord(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetSelectionWord(PGDirection direction) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetSelectionWord(direction);
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetStartOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetStartOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::SelectStartOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectStartOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetStartOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetStartOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::SelectStartOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectStartOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetEndOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetEndOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::SelectEndOfLine() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectEndOfLine();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::OffsetEndOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->OffsetEndOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::SelectEndOfFile() {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		it->SelectEndOfFile();
	}
	Cursor::NormalizeCursors(this, cursors);
}

void TextView::ClearExtraCursors() {
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	active_cursor = 0;
}

void TextView::ClearCursors() {
	cursors.clear();
	active_cursor = -1;
}

lng TextView::GetActiveCursorIndex() {
	if (active_cursor < 0)
		active_cursor = 0;
	return active_cursor;
}

Cursor& TextView::GetActiveCursor() {
	if (active_cursor < 0)
		active_cursor = 0;
	return cursors[active_cursor];
}

void TextView::SelectEverything() {
	ClearCursors();
	this->cursors.push_back(Cursor(this, file->linecount - 1, file->GetLine(file->linecount - 1).GetLength(), 0, 0));
	if (this->textfield) textfield->SelectionChanged();
}

int TextView::GetLineHeight() {
	if (!textfield) return -1;
	return textfield->GetLineHeight();
}
