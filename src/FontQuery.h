#ifndef FONT_QUERY_H
#define FONT_QUERY_H

#include "FontDescriptor.h"
#include <set>
#include <string>
#include <vector>
#include <algorithm>

// Forward declarations
ResultSet *findFonts(FontDescriptor *query);
FontDescriptor *findFont(FontDescriptor *query);
FontDescriptor *substituteFont(const char *postscriptName, const char* text);
std::vector<std::string> getAvailableFontFamilies();

// Helper functions
int squareInt(int val);

// Returns a score indicating how well a font matches a query
// Lower score = better match
int matchScore(FontDescriptor *font, FontDescriptor *query);

// Filter a result set by a query
ResultSet *filterResults(ResultSet *fonts, FontDescriptor *query);

// Find the best matching font in a result set
FontDescriptor *findBestMatch(ResultSet *fonts, FontDescriptor *query);

#endif // FONT_QUERY_H 