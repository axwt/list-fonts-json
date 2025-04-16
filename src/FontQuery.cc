#include "FontQuery.h"
#include <cstring>
#include <climits> // For INT_MAX

// Forward declaration of platform-specific function
extern ResultSet *getAvailableFonts();

// Utility function for computing match scores
int squareInt(int val) {
  return val * val;
}

// Compare a string with case-insensitivity, returning true if they match (ignoring case)
bool caseInsensitiveMatch(const char *a, const char *b) {
  if (a == NULL || b == NULL)
    return false;
    
  while (*a && *b) {
    if (tolower(*a) != tolower(*b))
      return false;
    a++;
    b++;
  }
  
  return *a == *b; // Both should be at the end
}

// Returns a score indicating how well a font matches a query
// Lower score = better match (0 = perfect match)
int matchScore(FontDescriptor *font, FontDescriptor *query) {
  int score = 0;
  
  // PostScript name match is most important
  if (query->postscriptName && !caseInsensitiveMatch(font->postscriptName, query->postscriptName)) {
    score += 1000;
  }
  
  // Family name match
  if (query->family && !caseInsensitiveMatch(font->family, query->family))
    score += 100;
  
  // Style match
  if (query->style && !caseInsensitiveMatch(font->style, query->style))
    score += 50;
  
  // Weight match (weighted difference)
  if (query->weight != FontWeightUndefined)
    score += squareInt((font->weight - query->weight) / 100) * 10;
  
  // Width match (weighted difference)
  if (query->width != FontWidthUndefined)
    score += squareInt(font->width - query->width) * 10;
  
  // Italic/oblique/monospace properties
  if (query->italic != font->italic)
    score += 5;
  
  if (query->oblique != font->oblique)
    score += 5;
  
  if (query->monospace != font->monospace)
    score += 5;
  
  return score;
}

// Filter a result set by a query - returns a new ResultSet
ResultSet *filterResults(ResultSet *fonts, FontDescriptor *query) {
  ResultSet *results = new ResultSet();
  
  // If no query, return all fonts
  if (!query) {
    for (auto font : *fonts) {
      results->push_back(new FontDescriptor(font));
    }
    return results;
  }
  
  // Filter the fonts by each field in the query
  for (auto font : *fonts) {
    bool matches = true;
    
    // Special case for exact postscript matching
    if (query->postscriptName && font->postscriptName) {
      if (strcmp(font->postscriptName, query->postscriptName) == 0) {
        results->push_back(new FontDescriptor(font));
        continue; // Skip other checks for exact matches
      }
    }
    
    // PostScript name
    if (query->postscriptName && !caseInsensitiveMatch(font->postscriptName, query->postscriptName))
      matches = false;
    
    // Family name
    if (matches && query->family && !caseInsensitiveMatch(font->family, query->family))
      matches = false;
    
    // Style
    if (matches && query->style && !caseInsensitiveMatch(font->style, query->style))
      matches = false;
    
    // Weight - allow some variance
    if (matches && query->weight != FontWeightUndefined) {
      int weightDiff = abs((int)font->weight - (int)query->weight);
      if (weightDiff > 100) // Allow 1 weight grade difference
        matches = false;
    }
    
    // Width - allow some variance
    if (matches && query->width != FontWidthUndefined) {
      int widthDiff = abs((int)font->width - (int)query->width);
      if (widthDiff > 1) // Allow 1 width grade difference
        matches = false;
    }
    
    // Italic/oblique/monospace properties
    if (matches && query->italic != font->italic)
      matches = false;
    
    if (matches && query->oblique != font->oblique)
      matches = false;
    
    if (matches && query->monospace != font->monospace)
      matches = false;
    
    if (matches) {
      results->push_back(new FontDescriptor(font));
    }
  }
  
  return results;
}

// Find the best matching font in a result set
FontDescriptor *findBestMatch(ResultSet *fonts, FontDescriptor *query) {
  if (!fonts || fonts->empty() || !query)
    return NULL;
  
  FontDescriptor *bestMatch = NULL;
  int bestScore = INT_MAX;
  
  for (auto font : *fonts) {
    int score = matchScore(font, query);
    if (score < bestScore) {
      bestScore = score;
      bestMatch = font;
    }
  }
  
  return bestMatch ? new FontDescriptor(bestMatch) : NULL;
}

// Extract unique font family names
std::vector<std::string> extractFontFamilies(ResultSet *fonts) {
  std::set<std::string> familySet;
  
  for (auto font : *fonts) {
    if (font->family && strlen(font->family) > 0) {
      familySet.insert(font->family);
    }
  }
  
  std::vector<std::string> families(familySet.begin(), familySet.end());
  std::sort(families.begin(), families.end());
  return families;
}

// Implementation for common platform functions

ResultSet *findFonts(FontDescriptor *query) {
  // Get all available fonts and filter them
  ResultSet *allFonts = getAvailableFonts();
  ResultSet *result = filterResults(allFonts, query);
  delete allFonts;
  return result;
}

FontDescriptor *findFont(FontDescriptor *query) {
  if (!query)
    return NULL;
    
  // Get all available fonts
  ResultSet *allFonts = getAvailableFonts();
  
  // Find the best match
  FontDescriptor *result = findBestMatch(allFonts, query);
  
  delete allFonts;
  return result;
}

std::vector<std::string> getAvailableFontFamilies() {
  ResultSet *allFonts = getAvailableFonts();
  std::vector<std::string> families = extractFontFamilies(allFonts);
  delete allFonts;
  return families;
} 