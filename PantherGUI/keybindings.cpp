
#include "keybindings.h"
#include "textfield.h"

#include "json.h"

using namespace nlohmann;

PGKeyBindingsManager::PGKeyBindingsManager() {
	// initialize all keybindings

	// initialize textfield keybindings
	TextField::InitializeKeybindings();

	LoadSettings("default-keybindings.json");
}

bool PGKeyBindingsManager::ParseKeyPress(std::string keys, PGKeyPress& keypress) {
	PGModifier modifier = PGModifierNone;
	size_t pos = 0;

	pos = keys.find("ctrl+");
	if (pos != std::string::npos) {
		modifier |= PGModifierCtrl;
		keys = keys.erase(pos, strlen("ctrl+"));
	}
	pos = keys.find("shift+");
	if (pos != std::string::npos) {
		modifier |= PGModifierShift;
		keys = keys.erase(pos, strlen("shift+"));
	}
	pos = keys.find("alt+");
	if (pos != std::string::npos) {
		modifier |= PGModifierAlt;
		keys = keys.erase(pos, strlen("alt+"));
	}
	pos = keys.find("cmd+");
	if (pos != std::string::npos) {
		modifier |= PGModifierCmd;
		keys = keys.erase(pos, strlen("cmd+"));
	}
	keypress.modifier = modifier;
	if (keys.size() == 1) {
		// character
		char c = keys[0];
		if (c >= 33 && c <= 126) {
			keys = panther::toupper(keys);
			keypress.character = keys[0];
			return true;
		} else {
			return false;
		}
	} else {
		if (keys == "insert") {
			keypress.button = PGButtonInsert;
		} else if (keys == "home") {
			keypress.button = PGButtonHome;
		} else if (keys == "pageup") {
			keypress.button = PGButtonPageUp;
		} else if (keys == "pagedown") {
			keypress.button = PGButtonPageDown;
		} else if (keys == "delete") {
			keypress.button = PGButtonDelete;
		} else if (keys == "end") {
			keypress.button = PGButtonEnd;
		} else if (keys == "printscreen") {
			keypress.button = PGButtonPrintScreen;
		} else if (keys == "scrolllock") {
			keypress.button = PGButtonScrollLock;
		} else if (keys == "break") {
			keypress.button = PGButtonPauseBreak;
		} else if (keys == "escape") {
			keypress.button = PGButtonEscape;
		} else if (keys == "tab") {
			keypress.button = PGButtonTab;
		} else if (keys == "capslock") {
			keypress.button = PGButtonCapsLock;
		} else if (keys == "f1") {
			keypress.button = PGButtonF1;
		} else if (keys == "f2") {
			keypress.button = PGButtonF2;
		} else if (keys == "f3") {
			keypress.button = PGButtonF3;
		} else if (keys == "f4") {
			keypress.button = PGButtonF4;
		} else if (keys == "f5") {
			keypress.button = PGButtonF5;
		} else if (keys == "f6") {
			keypress.button = PGButtonF6;
		} else if (keys == "f7") {
			keypress.button = PGButtonF7;
		} else if (keys == "f8") {
			keypress.button = PGButtonF8;
		} else if (keys == "f9") {
			keypress.button = PGButtonF9;
		} else if (keys == "f10") {
			keypress.button = PGButtonF10;
		} else if (keys == "f11") {
			keypress.button = PGButtonF11;
		} else if (keys == "f12") {
			keypress.button = PGButtonF12;
		} else if (keys == "numlock") {
			keypress.button = PGButtonNumLock;
		} else if (keys == "down") {
			keypress.button = PGButtonDown;
		} else if (keys == "up") {
			keypress.button = PGButtonUp;
		} else if (keys == "left") {
			keypress.button = PGButtonLeft;
		} else if (keys == "right") {
			keypress.button = PGButtonRight;
		} else if (keys == "backspace") {
			keypress.button = PGButtonBackspace;
		} else if (keys == "enter") {
			keypress.button = PGButtonEnter;
		} else {
			return false;
		}
		return true;
	}
	return false;
}

void PGKeyBindingsManager::LoadSettings(std::string filename) {
	lng result_size;
	char* ptr = (char*) panther::ReadFile(filename, result_size);
	if (!ptr) {
		// FIXME:
		assert(0);
		return;
	}
	json j;
	//try {
		j = json::parse(ptr);
	/*} catch(...) {
		return;
	}*/
	
	for (auto it = j.begin(); it != j.end(); it++) {
		if (it.value().is_array()) {
			std::string control = it.key();
			panther::tolower(control);
			auto keys = it.value();
			std::map<PGKeyPress, PGKeyFunctionCall>* functions = nullptr;
			std::map<std::string, PGKeyFunction>* keybindings_noargs = nullptr;
			std::map<std::string, PGKeyFunctionArgs>* keybindings_varargs = nullptr;
			if (control == "textfield") {
				functions = &TextField::keybindings;
				keybindings_noargs = &TextField::keybindings_noargs;
				keybindings_varargs = &TextField::keybindings_varargs;
			}
			if (functions && keybindings_noargs && keybindings_varargs) {
				for (auto key = keys.begin(); key != keys.end(); key++) {
					auto keybinding = key.value();
					if (keybinding.find("key") == keybinding.end()) {
						// no key found
						continue;
					}
					if (keybinding.find("command") == keybinding.end()) {
						// no command found
						continue;
					}
					std::string keys = keybinding["key"];
					std::string command = keybinding["command"];
					panther::tolower(keys);
					panther::tolower(command);
					// parse the key
					PGKeyPress keypress;
					if (!ParseKeyPress(keys, keypress)) {
						// could not parse key press
						continue;
					}

					PGKeyFunctionCall function;
					if (keybinding.find("args") == keybinding.end()) {
						// command without args
						if (keybindings_noargs->count(command) != 0) {
							function.has_args = false;
							function.function = (void*)(*keybindings_noargs)[command];
						} else {
							// command not found
							continue;
						}
					} else {
						// command with args
						if (keybindings_varargs->count(command) != 0) {
							function.has_args = true;
							function.function = (void*)(*keybindings_varargs)[command];
							assert(0);
							// fixme: assign function arguments
						} else {
							// command not found
							continue;
						}
					}
					(*functions)[keypress] = function;
				}
			} else {
				// FIXME: unrecognized control type
			}
		}
	}
}
