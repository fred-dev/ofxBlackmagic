# ofxBlackMagic - Blackmagic DeckLink / UltraStudio capture
# Bundles the DeckLink SDK 16.0 headers for macOS, Linux and Windows (libs/DeckLink).
# Needs Blackmagic Desktop Video 16.0 or newer installed on the machine.
# Windows: generate the API header once with scripts/generate_windows_headers.bat (see readme).

meta:
	ADDON_NAME = ofxBlackmagic
	ADDON_DESCRIPTION = Blackmagic DeckLink / UltraStudio capture for openFrameworks (macOS, Windows, Linux)
	ADDON_AUTHOR = Kyle McDonald, Frederick Rodrigues
	ADDON_TAGS = "capture" "Blackmagic" "DeckLink"
	ADDON_URL = https://github.com/fred-dev/ofxBlackmagic

# Each platform excludes the other platforms' SDK folders. A folder has to be listed both as
# itself and as folder/%: "folder/%" alone keeps the folder on the include path, and the wrong
# DeckLinkAPI.h is found first.

common:

osx:
	ADDON_FRAMEWORKS = CoreFoundation
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Linux libs/DeckLink/Linux/% libs/DeckLink/Win libs/DeckLink/Win/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Linux libs/DeckLink/Linux/% libs/DeckLink/Win libs/DeckLink/Win/%

linux64:
	ADDON_LDFLAGS = -ldl -lpthread
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Mac libs/DeckLink/Mac/% libs/DeckLink/Win libs/DeckLink/Win/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Mac libs/DeckLink/Mac/% libs/DeckLink/Win libs/DeckLink/Win/%

linuxaarch64:
	ADDON_LDFLAGS = -ldl -lpthread
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Mac libs/DeckLink/Mac/% libs/DeckLink/Win libs/DeckLink/Win/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Mac libs/DeckLink/Mac/% libs/DeckLink/Win libs/DeckLink/Win/%

vs:
	# ole32 / oleaut32 are linked from DeckLinkController.cpp (#pragma comment)
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Mac libs/DeckLink/Mac/% libs/DeckLink/Linux libs/DeckLink/Linux/% libs/DeckLink/Win/idl libs/DeckLink/Win/idl/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Mac libs/DeckLink/Mac/% libs/DeckLink/Linux libs/DeckLink/Linux/% libs/DeckLink/Win/idl libs/DeckLink/Win/idl/%
