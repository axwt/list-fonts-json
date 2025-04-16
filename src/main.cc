#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "FontDescriptor.h"
#include "FontQuery.h"

// Platform implementations
ResultSet *getAvailableFonts();
FontDescriptor *substituteFont(const char *postscriptName, const char *string);

void printUsage() {
  std::cout << "Usage: list-fonts-json [command] [options]" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  list                   - List all available fonts (default)" << std::endl;
  std::cout << "  find <query>           - Find fonts matching the query" << std::endl;
  std::cout << "  find-best <query>      - Find the best font matching the query" << std::endl;
  std::cout << "  substitute <ps> <text> - Find a font that can display the given text" << std::endl;
  std::cout << "  families               - List all available font families" << std::endl;
  std::cout << "Query options (for find and find-best):" << std::endl;
  std::cout << "  --family=<name>        - Filter by font family name" << std::endl;
  std::cout << "  --style=<style>        - Filter by font style" << std::endl;
  std::cout << "  --postscript=<name>    - Filter by PostScript name" << std::endl;
  std::cout << "  --monospace            - Filter for monospace fonts" << std::endl;
  std::cout << "  --italic               - Filter for italic fonts" << std::endl;
  std::cout << "  --weight=<weight>      - Filter by weight (100-900)" << std::endl;
  std::cout << "  --width=<width>        - Filter by width (1-9)" << std::endl;
}

// Parse an option like --family=Arial
const char* parseOption(const char* arg, const char* option) {
  size_t optionLen = strlen(option);
  if (strncmp(arg, option, optionLen) == 0 && arg[optionLen] == '=') {
    return arg + optionLen + 1;
  }
  return NULL;
}

// Print a JSON array of strings
void printJsonStringArray(const std::vector<std::string>& strings) {
  std::cout << "[" << std::endl;
  
  for (size_t i = 0; i < strings.size(); i++) {
    std::cout << "  \"" << strings[i] << "\"";
    if (i < strings.size() - 1) {
      std::cout << ",";
    }
    std::cout << std::endl;
  }
  
  std::cout << "]" << std::endl;
}

int main(int argc, char *argv[]) {
  // Default command is to list all fonts
  if (argc <= 1) {
    getAvailableFonts()->printJson();
    return 0;
  }
  
  // Parse command
  const char* command = argv[1];
  
  if (strcmp(command, "list") == 0) {
    getAvailableFonts()->printJson();
  }
  else if (strcmp(command, "families") == 0) {
    std::vector<std::string> families = getAvailableFontFamilies();
    printJsonStringArray(families);
  }
  else if (strcmp(command, "find") == 0 || strcmp(command, "find-best") == 0) {
    // Parse query options
    const char* family = NULL;
    const char* style = NULL;
    const char* postscriptName = NULL;
    bool monospace = false;
    bool italic = false;
    FontWeight weight = FontWeightUndefined;
    FontWidth width = FontWidthUndefined;
    
    for (int i = 2; i < argc; i++) {
      const char* arg = argv[i];
      
      if (const char* val = parseOption(arg, "--family")) {
        family = val;
      }
      else if (const char* val = parseOption(arg, "--style")) {
        style = val;
      }
      else if (const char* val = parseOption(arg, "--postscript")) {
        postscriptName = val;
      }
      else if (strcmp(arg, "--monospace") == 0) {
        monospace = true;
      }
      else if (strcmp(arg, "--italic") == 0) {
        italic = true;
      }
      else if (const char* val = parseOption(arg, "--weight")) {
        weight = (FontWeight)atoi(val);
      }
      else if (const char* val = parseOption(arg, "--width")) {
        width = (FontWidth)atoi(val);
      }
    }
    
    // Create a FontDescriptor from the options
    FontDescriptor* query = new FontDescriptor(
      NULL, // path
      postscriptName,
      family,
      style,
      weight,
      width,
      italic,
      false, // oblique
      monospace
    );
    
    if (strcmp(command, "find") == 0) {
      // Find multiple fonts matching the query
      ResultSet* results = findFonts(query);
      results->printJson();
      delete results;
    }
    else {
      // Find the best font matching the query
      FontDescriptor* result = findFont(query);
      if (result) {
        std::cout << "[" << std::endl;
        result->printJson();
        std::cout << "]" << std::endl;
        delete result;
      }
      else {
        std::cout << "[]" << std::endl;
      }
    }
    
    delete query;
  }
  else if (strcmp(command, "substitute") == 0) {
    // Need postscript name and text
    if (argc < 4) {
      printUsage();
      return 1;
    }
    
    const char* postscriptName = argv[2];
    const char* text = argv[3];
    
    FontDescriptor* result = substituteFont(postscriptName, text);
    if (result) {
      std::cout << "[" << std::endl;
      result->printJson();
      std::cout << "]" << std::endl;
      delete result;
    }
    else {
      std::cout << "[]" << std::endl;
    }
  }
  else {
    printUsage();
    return 1;
  }
  
  return 0;
}
