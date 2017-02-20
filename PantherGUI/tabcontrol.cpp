
#include "tabcontrol.h"
#include "filemanager.h"
#include "style.h"
#include "settings.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(TabControl);

#define SAVE_CHANGES_TITLE "Save Changes?"
#define SAVE_CHANGES_DIALOG "The current file has unsaved changes. Save changes before closing?"


TabControl::TabControl(PGWindowHandle window, TextField* textfield, std::vector<std::shared_ptr<TextFile>> files) :
	Control(window), active_tab(0), textfield(textfield), file_manager(), dragging_tab(nullptr, -1), active_tab_hidden(false), drag_tab(false), current_id(0), temporary_textfile(nullptr) {

	if (files.size() == 0)
		files.push_back(std::shared_ptr<TextFile>(new TextFile(nullptr)));
	for (auto it = files.begin(); it != files.end(); it++) {
		auto ptr = file_manager.OpenFile(*it);
		this->tabs.push_back(OpenTab(ptr));
	}
	this->font = PGCreateFont("myriad", false, true);
	SetTextFontSize(this->font, 12);
	file_icon_width = 0;
	file_icon_height = 0;

	tab_padding = 5;
}


Tab TabControl::OpenTab(std::shared_ptr<TextFile> textfile) {
	return Tab(textfile, current_id++);
}

void TabControl::PeriodicRender() {
	bool invalidate = false;
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		PGScalar current_x = (*it).x;
		PGScalar offset = ((*it).target_x - (*it).x) / 3;
		if (((*it).x < (*it).target_x && (*it).x + offset >(*it).target_x) ||
			(panther::abs((*it).x + offset - (*it).target_x) < 1)) {
			(*it).x = (*it).target_x;
		} else {
			(*it).x = (*it).x < 0 ? (*it).target_x : (*it).x + offset;
		}
		if (panther::abs(current_x - (*it).x) > 0.1)
			invalidate = true;
		index++;
	}
	if (invalidate) {
		this->Invalidate();
	}
}

void TabControl::RenderTab(PGRendererHandle renderer, Tab& tab, PGScalar& position_x, PGScalar x, PGScalar y, bool selected_tab) {
	TextFile* file = tab.file.get();
	assert(file);
	std::string filename = file->GetName();

	PGScalar tab_height = this->height - 2;

	tab.width = MeasureTabWidth(tab);

	PGScalar current_x = tab.x;
	
	PGPolygon polygon;
	polygon.points.push_back(PGPoint(x + current_x, y + tab_height));
	polygon.points.push_back(PGPoint(x + current_x + 12, y + 4));
	polygon.points.push_back(PGPoint(x + current_x + tab.width + 15, y + 4));
	polygon.points.push_back(PGPoint(x + current_x + tab.width + 27, y + tab_height));
	polygon.closed = true;
	RenderPolygon(renderer, polygon, selected_tab ? PGStyleManager::GetColor(PGColorTabControlSelected) :
		PGStyleManager::GetColor(PGColorTabControlBackground));
	polygon.closed = false;
	RenderPolygon(renderer, polygon, PGStyleManager::GetColor(PGColorTabControlBorder), 2);

	current_x += 15;
	std::string ext = file->GetExtension();

	file_icon_height = tab_height * 0.6f;
	file_icon_width = file_icon_height * 0.8f;

	std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

	RenderFileIcon(renderer, font, ext.c_str(), current_x, y + (height - file_icon_height) / 2, file_icon_width, file_icon_height,
		file->GetLanguage() ? file->GetLanguage()->GetColor() : PGColor(255, 255, 255), PGStyleManager::GetColor(PGColorTabControlBackground), PGStyleManager::GetColor(PGColorTabControlBorder));
		
	current_x += file_icon_width + 2.5f;
	if (file->HasUnsavedChanges()) {
		SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlUnsavedText));
	} else {
		SetTextColor(font, PGStyleManager::GetColor(PGColorTabControlText));
	}

	SetTextStyle(font, PGTextStyleBold);
	RenderText(renderer, font, filename.c_str(), filename.length(), x + current_x + tab_padding, y + 6);
	current_x += (tab.width - 25);
	RenderLine(renderer, PGLine(PGPoint(current_x, y + 13), PGPoint(current_x + 8, y + 21)), PGStyleManager::GetColor(PGColorTabControlText), 1);
	RenderLine(renderer, PGLine(PGPoint(current_x, y + 21), PGPoint(current_x + 8, y + 13)), PGStyleManager::GetColor(PGColorTabControlText), 1);
	
	position_x += tab.width + 15;
}

void TabControl::Draw(PGRendererHandle renderer, PGIRect* rectangle) {
	PGScalar x = X() - rectangle->x;
	PGScalar y = Y() - rectangle->y;
	PGScalar position_x = 0;
	PGColor color = PGStyleManager::GetColor(PGColorTabControlBackground);

	bool rendered = false;
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		if (!active_tab_hidden || index != active_tab) {
			if (dragging_tab.file != nullptr && position_x + it->width / 2 > dragging_tab.x && !rendered) {
				rendered = true;
				position_x += MeasureTabWidth(dragging_tab) + 15;
			}
			it->target_x = position_x;
			if (panther::epsilon_equals(it->x, -1)) {
				it->x = position_x;
			}
			RenderTab(renderer, *it, position_x, x, y, dragging_tab.file == nullptr && index == active_tab);
		}
		index++;
	}
	if (dragging_tab.file != nullptr) {
		position_x = dragging_tab.x;
		RenderTab(renderer, dragging_tab, position_x, x, y, true);
	}
	if (temporary_textfile) {
		Tab tab = Tab(temporary_textfile, -1);
		tab.width = MeasureTabWidth(tab);
		position_x = x + this->width - (tab.width + 30);
		tab.x = position_x;
		tab.target_x = position_x;
		RenderTab(renderer, tab, position_x, x, y, true);	
	}
	RenderLine(renderer, PGLine(x, y + this->height - 2, x + this->width, y + this->height - 1), PGColor(80, 150, 200), 2);
}

PGScalar TabControl::MeasureTabWidth(Tab& tab) {
	TextFile* file = tab.file.get();
	assert(file);
	std::string filename = file->GetName();
	return file_icon_width + 25 + MeasureTextWidth(font, filename.c_str(), filename.size());
}

bool TabControl::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(TabControl::keybindings, button, modifier)) {
		return true;
	}
	return false;
}

struct TabDragDropStruct {
	TabDragDropStruct(std::shared_ptr<TextFile> file, TabControl* tabs, PGScalar drag_offset) : file(file), tabs(tabs), accepted(nullptr), drag_offset(drag_offset) {}
	std::shared_ptr<TextFile> file;
	TabControl* tabs;
	TabControl* accepted = nullptr;
	PGScalar drag_offset;
};

void TabControl::MouseMove(int x, int y, PGMouseButton buttons) {
	if (drag_tab) {
		drag_tab = false;

		dragging_tab = tabs[active_tab];
		active_tab_hidden = true;

		PGBitmapHandle bitmap = CreateBitmapFromSize(MeasureTabWidth(tabs[active_tab]) + 30, this->height);
		PGRendererHandle renderer = CreateRendererForBitmap(bitmap);
		PGScalar position_x = 0;
		PGScalar stored_x = tabs[active_tab].x;
		tabs[active_tab].x = 0;
		RenderTab(renderer, tabs[active_tab], position_x, 0, 0, false);
		tabs[active_tab].x = stored_x;
		DeleteRenderer(renderer);
		
		PGStartDragDrop(window, bitmap, [](PGPoint mouse, void* d) {
			if (!d) return;
			TabDragDropStruct* data = (TabDragDropStruct*)d;
			if (data->accepted == nullptr) {
				std::vector<std::shared_ptr<TextFile>> textfiles;
				textfiles.push_back(data->file);
				PGWindowHandle new_window = PGCreateWindow(mouse, textfiles);
				ShowWindow(new_window);
				data->tabs->ActuallyCloseTab(data->file);
			} else if (data->accepted != data->tabs) {
				data->tabs->ActuallyCloseTab(data->file);
			}
			data->tabs->active_tab_hidden = false;
		}, new TabDragDropStruct(tabs[active_tab].file, this, drag_offset), sizeof(TabDragDropStruct));
	}
}


bool TabControl::AcceptsDragDrop(PGDragDropType type) {
	return type == PGDragDropTabs;
}

void TabControl::DragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	assert(type == PGDragDropTabs);
	if (mouse.x >= 0 && mouse.x <= this->width && mouse.y >= -100 & mouse.y <= 100) {
		TabDragDropStruct* tab_data = (TabDragDropStruct*)data;
		dragging_tab.file = tab_data->file;
		dragging_tab.width = MeasureTabWidth(dragging_tab);
		dragging_tab.x = mouse.x - tab_data->drag_offset;
		dragging_tab.target_x = mouse.x;
	} else {
		dragging_tab.x = -1;
		dragging_tab.file = nullptr;
	}
	this->Invalidate();
}

void TabControl::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {
	PGPoint mouse(x - this->x, y - this->y);
	assert(type == PGDragDropTabs);
	TabDragDropStruct* td = (TabDragDropStruct*)data;
	if (mouse.x >= 0 && mouse.x <= this->width && mouse.y >= -100 & mouse.y <= 100) {
		td->accepted = this;
		PGScalar tab_pos = mouse.x - td->drag_offset;
		PGScalar position_x = 0;
		lng index = 0;
		lng current_index = 0;
		lng new_index = this == td->tabs ? tabs.size() - 1 : tabs.size();
		for (auto it = tabs.begin(); it != tabs.end(); it++, index++) {
			// we skip the active tab if the file is from the current tab control
			// because the active tab is the file we are dragging
			if (this == td->tabs && index == active_tab) continue;
			PGScalar width = MeasureTabWidth(*it);
			if (position_x + width / 2 > tab_pos) {
				new_index = current_index;
				break;
			}
			position_x += width + 15;
			current_index++;
		}
		if (this == td->tabs) {
			// if the tab is from this tab control,
			// we only move the tab around
			if (new_index != active_tab) {
				Tab current_tab = tabs[active_tab];
				tabs.erase(tabs.begin() + active_tab);
				tabs.insert(tabs.begin() + new_index, current_tab);
				active_tab = new_index;
			}
			tabs[active_tab].x = tab_pos;
			tabs[active_tab].target_x = tab_pos;
		} else {
			// if the tab is from a different tab control 
			// we have to open the file
			this->OpenFile(td->file, new_index);
		}
		active_tab_hidden = false;
	}
	dragging_tab.x = -1;
	dragging_tab.file = nullptr;
	this->Invalidate();
}

void TabControl::ClearDragDrop(PGDragDropType type) {
	assert(type == PGDragDropTabs);
	dragging_tab.x = -1;
	dragging_tab.file = nullptr;
	this->Invalidate();
}

void TabControl::LoadWorkspace(nlohmann::json& j) {
	if (j.count("tabs") > 0) {
		nlohmann::json& tb = j["tabs"];
		if (tb.count("files") > 0) {
			nlohmann::json& f = tb["files"];
			if (f.is_array() && f.size() > 0) {
				file_manager.ClearFiles();
				tabs.clear();
				for (auto it = f.begin(); it != f.end(); ++it) {
					std::shared_ptr<TextFile> textfile = nullptr;
					std::string path = "";
					if (it->count("file") > 0) {
						path = (*it)["file"].get<std::string>();
					}
					if (it->count("buffer") > 0) {
						// if we have the text stored in a buffer
						// we load the text from the buffer, rather than from the file
						std::string buffer = (*it)["buffer"];
						textfile = std::shared_ptr<TextFile>(new TextFile(textfield, path.c_str(), (char*) buffer.c_str(), buffer.size(), true, false));
						if (path.size() == 0) {
							textfile->name = "untitled";
						}
						textfile->SetUnsavedChanges(true);
						file_manager.OpenFile(textfile);
					} else if (path.size() > 0) {
						// otherwise, if there is a file speciifed
						// we load the text from the file instead
						PGFileError error;
						textfile = file_manager.OpenFile(path, error);
					} else {
						continue;
					}

					// load additional text file settings from the workspace
					// note that we do not apply them to the textfile immediately because of concurrency concerns
					PGTextFileSettings settings;
					
					Cursor::LoadCursors(*it, settings.cursor_data);

					if (it->count("lineending") > 0) {
						std::string ending = (*it)["lineending"];
						if (ending == "windows") {
							settings.line_ending = PGLineEndingWindows;
						} else if (ending == "unix") {
							settings.line_ending = PGLineEndingUnix;
						} else if (ending == "macos") {
							settings.line_ending = PGLineEndingMacOS;
						}
					}
					if (it->count("xoffset") > 0) {
						// if xoffset is specified, set the xoffset of the file
						auto xoffset = (*it)["xoffset"];
						if (xoffset.is_number_float()) {
							settings.xoffset = xoffset;
						}
					} else {
						// if xoffset is not specified, word wrap is enabled
						settings.wordwrap = true;
					}
					if (it->count("yoffset") > 0) {
						auto yoffset = (*it)["yoffset"];
						// yoffset can be either a single number (linenumber)
						// or an array of two numbers ([linenumber, inner_line])
						if (yoffset.is_array()) {
							if (yoffset[0].is_number()) {
								settings.yoffset.linenumber = yoffset[0];
							}
							if (yoffset.size() > 1 && yoffset[1].is_number()) {
								settings.yoffset.inner_line = yoffset[1];
							}
						} else if (yoffset.is_number()) {
							settings.yoffset.linenumber = yoffset;
							settings.yoffset.inner_line = 0;
						}
					}
					if (textfile != nullptr) {
						// apply the settings, either immediately if the file has been loaded
						// or wait until after it has been loaded otherwise
						textfile->SetSettings(settings);
						textfile->textfield = textfield;
						this->tabs.push_back(OpenTab(textfile));
					}
				}
				if (textfield) {
					textfield->SetTextFile(tabs[active_tab].file);
				}
			}
		}
		if (tb.count("selected_tab") > 0 && tb["selected_tab"].is_number()) {
			// if the selected tab is specified, switch to the selected tab
			lng selected_tab = tb["selected_tab"];
			selected_tab = std::max((lng) 0, std::min((lng) tabs.size() - 1, selected_tab));
			SwitchToTab(tabs[selected_tab].file);
		}
		j.erase(j.find("tabs"));
	}
}

void TabControl::WriteWorkspace(nlohmann::json& j) {
	nlohmann::json& tb = j["tabs"];
	nlohmann::json& f = tb["files"];
	int index = 0;
	for (auto it = tabs.begin(); it != tabs.end(); it++) {
		TextFile* file = it->file.get();
		assert(file);
		nlohmann::json& cur = f[index];

		if (file->HasUnsavedChanges()) {
			// the file has unsaved changes
			// so we have to write the file
			auto store_file_type = file->WorkspaceFileStorage();
			if (store_file_type == TextFile::PGStoreFileBuffer) {
				// write the buffer
				cur["buffer"] = file->GetText();
			} else if (store_file_type == TextFile::PGStoreFileDeltas) {
				// write the deltas
				cur["deltas"] = nlohmann::json::array();
				nlohmann::json& deltas = cur["deltas"];
				lng index2 = 0;
				for (auto it = file->deltas.begin(); it != file->deltas.end(); it++) {
					(*it)->WriteWorkspace(deltas[index2++]);
				}
			}
		}

		std::string path = file->GetFullPath();
		if (path.size() != 0) {
			cur["file"] = path;
		}

		auto cursor_data = Cursor::GetCursorData(file->GetCursors());
		Cursor::StoreCursors(cur, cursor_data);

		auto line_ending = file->GetLineEnding();
		if (line_ending == PGLineEndingMixed || line_ending == PGLineEndingUnknown) {
			line_ending = GetSystemLineEnding();
		}
		std::string endings;
		switch (line_ending) {
		case PGLineEndingWindows:
			endings = "Windows";
			break;
		case PGLineEndingUnix:
			endings = "Unix";
			break;
		case PGLineEndingMacOS:
			endings = "MacOS";
			break;
		}
		cur["lineending"] = endings;
		if (file->wordwrap) {
			cur["yoffset"] = { file->yoffset.linenumber, file->yoffset.inner_line };
		} else {
			cur["xoffset"] = file->xoffset;
			cur["yoffset"] = file->yoffset.linenumber ;
		}
		index++;
	}
	tb["selected_tab"] = active_tab;
}

bool TabControl::KeyboardCharacter(char character, PGModifier modifier) {
	if (this->PressCharacter(TabControl::keybindings, character, modifier)) {
		return true;
	}
	return false;
}

void TabControl::NewTab() {
	std::shared_ptr<TextFile> file = file_manager.OpenFile();
	tabs.insert(tabs.begin() + active_tab + 1, OpenTab(file));
	active_tab++;
	SwitchToFile(tabs[active_tab].file);
}

void TabControl::OpenFile(std::string path) {
	PGFileError error;
	std::shared_ptr<TextFile> textfile = file_manager.OpenFile(path, error);
	if (textfile) {
		AddTab(textfile);
	} else if (textfield) {
		this->textfield->DisplayNotification(error);
	}
}

void TabControl::OpenTemporaryFile(std::shared_ptr<TextFile> textfile) {
	this->temporary_textfile = textfile;
	SwitchToFile(temporary_textfile);
}

void TabControl::CloseTemporaryFile() {
	if (textfield->GetTextfilePointer() == temporary_textfile) {
		SwitchToFile(tabs[active_tab].file);
	}
	this->temporary_textfile = nullptr;
}


void TabControl::ReopenFile(PGClosedTab tab) {
	PGFileError error;
	std::shared_ptr<TextFile> textfile = file_manager.OpenFile(tab.filepath, error);
	if (textfile) {
		AddTab(textfile, tab.id, tab.neighborid);
	} else if (textfield) {
		this->textfield->DisplayNotification(error);
	}
}


void TabControl::OpenFile(std::shared_ptr<TextFile> textfile) {
	std::shared_ptr<TextFile> ptr = file_manager.OpenFile(textfile);
	if (ptr) {
		AddTab(ptr);
	}
}

void TabControl::OpenFile(std::shared_ptr<TextFile> textfile, lng index) {
	std::shared_ptr<TextFile> ptr = file_manager.OpenFile(textfile);
	if (ptr) {
		AddTab(ptr, index);
	}
}

void TabControl::AddTab(std::shared_ptr<TextFile> file) {
	assert(file);
	assert(active_tab < tabs.size());
	tabs.insert(tabs.begin() + active_tab + 1, OpenTab(file));
	active_tab++;
	SwitchToFile(tabs[active_tab].file);
}

void TabControl::AddTab(std::shared_ptr<TextFile> file, lng index) {
	assert(file);
	assert(index < tabs.size());
	tabs.insert(tabs.begin() + index, OpenTab(file));
	active_tab = index;
	SwitchToFile(tabs[active_tab].file);
}

void TabControl::AddTab(std::shared_ptr<TextFile> file, lng id, lng neighborid) {
	assert(file);
	if (neighborid < 0) {
		tabs.insert(tabs.begin(), Tab(file, id));
		active_tab = 0;
	} else {
		lng entry = 0;
		for (auto it = tabs.begin(); it != tabs.end(); it++) {
			assert(it->id != id);
			if (it->id == neighborid) {
				break;
			}
			entry++;
		}
		assert(entry != tabs.size());
		tabs.insert(tabs.begin() + entry + 1, Tab(file, id));
		active_tab = entry + 1;
	}
	SwitchToFile(tabs[active_tab].file);
}

int TabControl::GetSelectedTab(int x) {
	for (int i = 0; i < tabs.size(); i++) {
		if (x >= tabs[i].x && x <= tabs[i].x + tabs[i].width) {
			return i;
		}
	}
	return -1;
}

void TabControl::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	x -= this->x; y -= this->y;
	if (button == PGLeftMouseButton) {
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			active_tab = selected_tab;
			SwitchToFile(tabs[selected_tab].file);
			drag_offset = x - tabs[selected_tab].x;
			drag_tab = true;
			this->Invalidate(true);
			return;
		}
	} else if (button == PGMiddleMouseButton) {
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			CloseTab(selected_tab);
			this->Invalidate();
		}
	}
}

void TabControl::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	x -= this->x; y -= this->y;
	if (button & PGLeftMouseButton) {
		if (drag_tab) {
			drag_tab = false;
			this->Invalidate();
		}
	} else if (button & PGRightMouseButton) {
		PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
		int selected_tab = GetSelectedTab(x);
		if (selected_tab >= 0) {
			this->currently_selected_tab = selected_tab;
			PGPopupMenuInsertEntry(menu, "Close", [](Control* control, PGPopupInformation* info) {
				TabControl* tb = dynamic_cast<TabControl*>(control);
				tb->CloseTab(tb->currently_selected_tab);
				tb->Invalidate();
			});
			PGPopupMenuInsertEntry(menu, "Close Other Tabs", [](Control* control, PGPopupInformation* info) {
			}, PGPopupMenuGrayed);
			PGPopupMenuInsertEntry(menu, "Close Tabs to the Right", [](Control* control, PGPopupInformation* info) {
			}, PGPopupMenuGrayed);
			PGPopupMenuInsertSeparator(menu);
		}
		PGPopupMenuInsertEntry(menu, "New File", [](Control* control, PGPopupInformation* info) {
			dynamic_cast<TabControl*>(control)->NewTab();
			control->Invalidate();
		});
		PGPopupMenuInsertEntry(menu, "Open File", [](Control* control, PGPopupInformation* info) {
		}, PGPopupMenuGrayed);
		PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
	}
}

void TabControl::PrevTab() {
	std::vector<std::shared_ptr<TextFile>>& files = file_manager.GetFiles();
	active_tab--;
	if (active_tab < 0) active_tab = files.size() - 1;

	SwitchToFile(files[active_tab]);
}

void TabControl::NextTab() {
	std::vector<std::shared_ptr<TextFile>>& files = file_manager.GetFiles();
	active_tab++;
	if (active_tab >= files.size()) active_tab = 0;

	SwitchToFile(files[active_tab]);
}

bool TabControl::CloseAllTabs() {
	for (size_t i = 0; i < tabs.size(); i++) {
		if (!CloseTabConfirmation(i)) {
			return false;
		}
	}
	return true;
}

bool TabControl::CloseTabConfirmation(int tab) {
	bool hot_exit;
	PGSettingsManager::GetSetting("hot_exit", hot_exit);
	// we only show a confirmation on exit if hot_exit is disabled
	// or if hot_exit is enabled, but the file cannot be saved in the workspace because it is too large
	if (tabs[tab].file->HasUnsavedChanges() && 
		(!hot_exit || (hot_exit && tabs[tab].file->WorkspaceFileStorage() == TextFile::PGFileTooLarge))) {

		// switch to the tab
		active_tab = tab;
		SwitchToFile(tabs[active_tab].file);
		this->Invalidate();

		// then show the confirmation box
		PGResponse response = PGConfirmationBox(window, SAVE_CHANGES_TITLE, SAVE_CHANGES_DIALOG);
		if (response == PGResponseCancel) {
			return false;
		} else if (response == PGResponseYes) {
			tabs[tab].file->SaveChanges();
		}
	}
	return true;
}

void TabControl::ActuallyCloseTab(int tab) {
	bool close_window = false;
	if (tab <= active_tab && active_tab > 0) {
		if (tab == active_tab) {
			SwitchToFile(tabs[active_tab - 1].file);
		}
		active_tab--;
	} else if (tab == active_tab) {
		if (tabs.size() == 1) {
			close_window = true;
		} else {
			SwitchToFile(tabs[1].file);
		}
	}
	if (tabs[tab].file->path.size() > 0) {
		closed_tabs.push_back(PGClosedTab(tabs[tab], tab == 0 ? -1 : tabs[tab - 1].id));
	}
	file_manager.CloseFile(tabs[tab].file);
	tabs.erase(tabs.begin() + tab);
	this->Invalidate();
	if (close_window) {
		PGCloseWindow(this->window);
	}
}

void TabControl::CloseTab(int tab) {
	if (tabs[tab].file->HasUnsavedChanges()) {
		// if there are unsaved changes, we show a confirmation box
		int* tabnumbers = new int[1];
		tabnumbers[0] = tab;
		// we use a callback here rather than a blocking confirmation box
		// because a blocking confirmation box can launch a sub-event queue on windows
		// which could lead to concurrency issues
		PGConfirmationBox(window, SAVE_CHANGES_TITLE, SAVE_CHANGES_DIALOG, 
			[](PGWindowHandle window, Control* control, void* data, PGResponse response) {
			TabControl* tabs = (TabControl*)control;
			int* tabnumbers = (int*)data;
			int tabnr = tabnumbers[0];

			if (response == PGResponseCancel)
				return;
			if (response == PGResponseYes) {
				tabs->tabs[tabnr].file->SaveChanges();
			}
			tabs->ActuallyCloseTab(tabnr);
			delete tabnumbers;
		}, this, (void*)tabnumbers);
	} else {
		ActuallyCloseTab(tab);
	}
}


void TabControl::ActuallyCloseTab(std::shared_ptr<TextFile> textfile) {
	for (size_t i = 0; i < tabs.size(); i++) {
		if (tabs[i].file == textfile) {
			ActuallyCloseTab(i);
			return;
		}
	}
	assert(0);
}

void TabControl::CloseTab(std::shared_ptr<TextFile> textfile) {
	for (size_t i = 0; i < tabs.size(); i++) {
		if (tabs[i].file == textfile) {
			CloseTab(i);
			return;
		}
	}
	assert(0);
}

void TabControl::ReopenLastFile() {
	if (closed_tabs.size() == 0) return;

	PGClosedTab tab = closed_tabs.back();
	closed_tabs.pop_back();

	this->ReopenFile(tab);
}

void TabControl::SwitchToTab(std::shared_ptr<TextFile> textfile) {
	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].file == textfile) {
			SwitchToFile(tabs[i].file);
			active_tab = i;
			this->Invalidate();
			return;
		}
	}
	assert(0);
}


void TabControl::SwitchToFile(std::shared_ptr<TextFile> file) {
	textfield->SetTextFile(file);
}

void TabControl::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = TabControl::keybindings_noargs;
	noargs["open_file"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		std::vector<std::string> files = ShowOpenFileDialog(true, false, true);
		for(auto it = files.begin(); it != files.end(); it++) {
			t->OpenFile(*it);
		}
		t->Invalidate();
	};
	noargs["close_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->CloseTab(t->active_tab);
		t->Invalidate();
	};
	noargs["new_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->NewTab();
		t->Invalidate();
	};
	noargs["next_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->NextTab();
		t->Invalidate();
	};
	noargs["prev_tab"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->PrevTab();
		t->Invalidate();
	};
	noargs["reopen_last_file"] = [](Control* c) {
		TabControl* t = (TabControl*)c;
		t->ReopenLastFile();
		t->Invalidate();
	};
}
