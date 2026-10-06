# ofxBlackMagic - Blackmagic DeckLink / UltraStudio capture
# Bundles the DeckLink SDK 16.0 headers for macOS, Linux and Windows (libs/DeckLink).
# Needs Blackmagic Desktop Video 16.0 or newer installed on the machine.
# Windows: generate the API header once with scripts/generate_windows_headers.bat (see readme).

meta:
	ADDON_NAME = ofxBlackMagic
	ADDON_DESCRIPTION = Blackmagic DeckLink / UltraStudio capture for openFrameworks (macOS, Windows, Linux)
	ADDON_AUTHOR = Kyle McDonald, Frederick Rodrigues
	ADDON_TAGS = "capture" "Blackmagic" "DeckLink"
	ADDON_URL = https://github.com/fred-dev/ofxBlackmagic

common:

osx:
	ADDON_FRAMEWORKS = CoreFoundation
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Linux/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Linux/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/idl/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Linux/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Linux/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/idl/%

linux64:
	ADDON_LDFLAGS = -ldl -lpthread
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Mac/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Mac/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/idl/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Mac/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Mac/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/idl/%

linuxaarch64:
	ADDON_LDFLAGS = -ldl -lpthread
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Mac/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Mac/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/idl/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Mac/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Mac/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/idl/%

vs:
	# ole32 / oleaut32 are linked from DeckLinkController.cpp (#pragma comment)
	ADDON_SOURCES_EXCLUDE = libs/DeckLink/Mac/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Mac/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Linux/src/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Linux/include/%
	ADDON_SOURCES_EXCLUDE += libs/DeckLink/Win/idl/%
	ADDON_INCLUDES_EXCLUDE = libs/DeckLink/Mac/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Mac/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Linux/src/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Linux/include/%
	ADDON_INCLUDES_EXCLUDE += libs/DeckLink/Win/idl/%
