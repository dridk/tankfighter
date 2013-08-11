#include "misc.h"
#ifndef _WIN32
#include <fontconfig/fontconfig.h>
#include <stdlib.h>
#include "parameters.h"
#endif

#ifdef _WIN32
std::string getDefaultFontPath(void) {
	return "c:\\windows\\fonts\\Arial.ttf";
}
#else

static std::string defaultFontPathNFC(void) {
	return "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
}
static std::string getFontPath(const char *pattern_name) {
	std::string file_path = defaultFontPathNFC();
	if (!FcInit()) {
		return file_path;
	}
	FcLangSet *langset=FcLangSetCreate();
	FcLangSetAdd(langset, (const FcChar8*)"en-US");
	FcPattern *pattern = FcNameParse((const FcChar8*)pattern_name);
	FcConfigSubstitute(0, pattern, FcMatchPattern);
        FcDefaultSubstitute (pattern);

	FcObjectSet *props = FcObjectSetCreate();
	FcObjectSetAdd(props, FC_FILE);

	FcResult res=FcResultMatch;
	FcFontSet *fonts = FcFontSort (0, pattern, FcTrue, 0, &res);
	if (res == FcResultMatch && fonts && fonts->nfont > 0) {
		FcValue val;
		val.type = FcTypeString;val.u.s=0;
		FcPatternGet(fonts->fonts[0], FC_FILE, 0, &val);
		if (val.type == FcTypeString) file_path = (const char*)val.u.s;
	}

	FcFontSetDestroy(fonts);
	FcObjectSetDestroy(props);
	FcPatternDestroy(pattern);
	FcLangSetDestroy(langset);
	return file_path;
}
std::string getDefaultFontPath(void) {
	return getFontPath(parameters.defaultFontName().c_str());
}
#endif

