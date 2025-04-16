#define WINVER 0x0600
#include "FontDescriptor.h"
#include <dwrite.h>
#include <dwrite_1.h>
#include <unordered_set>

// throws a JS error when there is some exception in DirectWrite
#define HR(hr) \
  if (FAILED(hr)) { printf("Font loading error at %s:%i\n", __FILE__, __LINE__); throw "Font loading error"; }

char *utf16ToUtf8(const WCHAR *input) {
  unsigned int len = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
  char *output = new char[len + 1];
  output[len] = '\0';
  WideCharToMultiByte(CP_UTF8, 0, input, -1, output, len, NULL, NULL);
  return output;
}

// returns the index of the user's locale in the set of localized strings
unsigned int getLocaleIndex(IDWriteLocalizedStrings *strings) {
  unsigned int index = 0;
  BOOL exists = false;

  HR(strings->FindLocaleName(L"en-us", &index, &exists));
  if (!exists) {
    index = 0;
  }
  return index;
}

// gets a localized string for a font
char *getString(IDWriteFont *font, DWRITE_INFORMATIONAL_STRING_ID string_id) {
  char *res = NULL;
  IDWriteLocalizedStrings *strings = NULL;

  BOOL exists = false;
  HR(font->GetInformationalStrings(
    string_id,
    &strings,
    &exists
  ));

  if (exists) {
    unsigned int index = getLocaleIndex(strings);
    unsigned int len = 0;
    WCHAR *str = NULL;

    HR(strings->GetStringLength(index, &len));
    str = new WCHAR[len + 1];

    HR(strings->GetString(index, str, len + 1));

    // convert to utf8
    res = utf16ToUtf8(str);
    delete str;

    strings->Release();
  }

  if (!res) {
    res = new char[1];
    res[0] = '\0';
  }

  return res;
}

FontDescriptor *resultFromFont(IDWriteFont *font) {
  FontDescriptor *res = NULL;
  IDWriteFontFace *face = NULL;
  unsigned int numFiles = 0;

  HR(font->CreateFontFace(&face));

  // get the font files from this font face
  IDWriteFontFile *files = NULL;
  HR(face->GetFiles(&numFiles, NULL));
  HR(face->GetFiles(&numFiles, &files));

  // return the first one
  if (numFiles > 0) {
    IDWriteFontFileLoader *loader = NULL;
    IDWriteLocalFontFileLoader *fileLoader = NULL;
    unsigned int nameLength = 0;
    const void *referenceKey = NULL;
    unsigned int referenceKeySize = 0;
    WCHAR *name = NULL;

    HR(files[0].GetLoader(&loader));

    // check if this is a local font file
    HRESULT hr = loader->QueryInterface(__uuidof(IDWriteLocalFontFileLoader), (void **)&fileLoader);
    if (SUCCEEDED(hr)) {
      // get the file path
      HR(files[0].GetReferenceKey(&referenceKey, &referenceKeySize));
      HR(fileLoader->GetFilePathLengthFromKey(referenceKey, referenceKeySize, &nameLength));

      name = new WCHAR[nameLength + 1];
      HR(fileLoader->GetFilePathFromKey(referenceKey, referenceKeySize, name, nameLength + 1));

      char *psName = utf16ToUtf8(name);
      char *postscriptName = getString(font, DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME);
      char *family = getString(font, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES);
      char *style = getString(font, DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES);

      // this method requires windows 7, so we need to cast to an IDWriteFontFace1
      IDWriteFontFace1 *face1 = static_cast<IDWriteFontFace1 *>(face);
      bool monospace = face1->IsMonospacedFont() == TRUE;

      res = new FontDescriptor(
        psName,
        postscriptName,
        family,
        style,
        (FontWeight) font->GetWeight(),
        (FontWidth) font->GetStretch(),
        font->GetStyle() == DWRITE_FONT_STYLE_ITALIC,
        font->GetStyle() == DWRITE_FONT_STYLE_OBLIQUE,
        monospace
      );

      delete psName;
      delete name;
      delete postscriptName;
      delete family;
      delete style;
      fileLoader->Release();
    }

    loader->Release();
  }

  face->Release();
  files->Release();

  return res;
}

ResultSet *getAvailableFonts() {
  ResultSet *res = new ResultSet();
  int count = 0;

  IDWriteFactory *factory = NULL;
  HR(DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(&factory)
  ));

  // Get the system font collection.
  IDWriteFontCollection *collection = NULL;
  HR(factory->GetSystemFontCollection(&collection));

  // Get the number of font families in the collection.
  int familyCount = collection->GetFontFamilyCount();

  // track postscript names we've already added
  // using a set so we don't get any duplicates.
  std::unordered_set<std::string> psNames;

  for (int i = 0; i < familyCount; i++) {
    IDWriteFontFamily *family = NULL;

    // Get the font family.
    HR(collection->GetFontFamily(i, &family));
    int fontCount = family->GetFontCount();

    for (int j = 0; j < fontCount; j++) {
      IDWriteFont *font = NULL;
      HR(family->GetFont(j, &font));

      FontDescriptor *result = resultFromFont(font);
      if (psNames.count(result->postscriptName) == 0) {
        res->push_back(result);
        psNames.insert(result->postscriptName);
      }
      font->Release();
    }

    family->Release();
  }

  collection->Release();
  factory->Release();

  return res;
}

FontDescriptor *substituteFont(const char *postscriptName, const char *string) {
  FontDescriptor *result = NULL;
  
  // Create DirectWrite factory
  IDWriteFactory *factory = NULL;
  HRESULT hr = DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(&factory)
  );
  
  if (FAILED(hr)) {
    return NULL;
  }
  
  // Convert the input string to UTF-16
  int wstrLen = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
  if (wstrLen <= 0) {
    factory->Release();
    return NULL;
  }
  
  WCHAR *wstr = new WCHAR[wstrLen];
  MultiByteToWideChar(CP_UTF8, 0, string, -1, wstr, wstrLen);
  
  // Convert the postscript name to UTF-16
  WCHAR *fontNameW = NULL;
  if (postscriptName) {
    int nameLen = MultiByteToWideChar(CP_UTF8, 0, postscriptName, -1, NULL, 0);
    if (nameLen > 0) {
      fontNameW = new WCHAR[nameLen];
      MultiByteToWideChar(CP_UTF8, 0, postscriptName, -1, fontNameW, nameLen);
    }
  }
  
  // First, try to get the specified font
  IDWriteFontCollection *collection = NULL;
  hr = factory->GetSystemFontCollection(&collection);
  
  if (SUCCEEDED(hr)) {
    if (fontNameW) {
      // Find original font by postscript name
      UINT32 fontIndex = 0;
      BOOL exists = FALSE;
      int familyNameLen = 0;
      
      // Search in all families
      for (unsigned int i = 0; i < collection->GetFontFamilyCount(); i++) {
        IDWriteFontFamily *family = NULL;
        hr = collection->GetFontFamily(i, &family);
        
        if (SUCCEEDED(hr)) {
          for (unsigned int j = 0; j < family->GetFontCount(); j++) {
            IDWriteFont *font = NULL;
            hr = family->GetFont(j, &font);
            
            if (SUCCEEDED(hr)) {
              // Check if this is the requested font by postscript name
              IDWriteLocalizedStrings *names = NULL;
              hr = font->GetInformationalStrings(
                DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME,
                &names,
                &exists
              );
              
              if (SUCCEEDED(hr) && exists && names) {
                WCHAR psName[256] = { 0 };
                hr = names->GetString(0, psName, 256);
                
                if (SUCCEEDED(hr) && wcscmp(psName, fontNameW) == 0) {
                  // We found the requested font
                  // Now check if it can render the string
                  IDWriteFontFace *face = NULL;
                  hr = font->CreateFontFace(&face);
                  
                  if (SUCCEEDED(hr)) {
                    const size_t textLength = wcslen(wstr);
                    UINT16 *glyphIndices = new UINT16[textLength];
                    // Cast with check to ensure no data loss
                    UINT32 safeLength = (textLength > UINT32_MAX) ? UINT32_MAX : static_cast<UINT32>(textLength);
                    hr = face->GetGlyphIndices((UINT32*)wstr, safeLength, glyphIndices);
                    
                    bool canDisplay = true;
                    for (size_t k = 0; k < textLength; k++) {
                      if (glyphIndices[k] == 0) { // 0 means no glyph
                        canDisplay = false;
                        break;
                      }
                    }
                    
                    if (canDisplay) {
                      // Return the original font as it can render the string
                      result = resultFromFont(font);
                    }
                    
                    delete[] glyphIndices;
                    face->Release();
                  }
                }
                
                names->Release();
              }
              
              // If we already found a result, exit early
              if (result) {
                font->Release();
                family->Release();
                break;
              }
              
              font->Release();
            }
          }
          
          family->Release();
          if (result) break;
        }
      }
    }
    
    // If we haven't found a suitable font yet, use the font fallback system
    if (!result) {
      // Create a text format to start with a default font
      IDWriteTextFormat *textFormat = NULL;
      hr = factory->CreateTextFormat(
        L"Arial",
        collection,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-us",
        &textFormat
      );
      
      if (SUCCEEDED(hr)) {
        // Create a text layout for font fallback
        IDWriteTextLayout *textLayout = NULL;
        // Cast size_t to UINT32 safely
        const size_t textLength = wcslen(wstr);
        UINT32 safeLength = (textLength > UINT32_MAX) ? UINT32_MAX : static_cast<UINT32>(textLength);
        hr = factory->CreateTextLayout(
          wstr,
          safeLength,
          textFormat,
          1000.0f, // max width
          100.0f,  // max height
          &textLayout
        );
        
        if (SUCCEEDED(hr)) {
          // Get the font used for the first character (should be fallback if needed)
          IDWriteFontCollection *actualCollection = NULL;
          WCHAR fontName[256] = { 0 };
          UINT32 nameLength = 0;
          DWRITE_FONT_WEIGHT weight;
          DWRITE_FONT_STYLE style;
          DWRITE_FONT_STRETCH stretch;
          float fontSize;
          
          hr = textLayout->GetFontCollection(0, &actualCollection);
          if (SUCCEEDED(hr)) {
            textLayout->GetFontFamilyName(0, fontName, 256);
            textLayout->GetFontWeight(0, &weight);
            textLayout->GetFontStyle(0, &style);
            textLayout->GetFontStretch(0, &stretch);
            textLayout->GetFontSize(0, &fontSize);
            
            // Find the font family
            UINT32 familyIndex = 0;
            BOOL familyExists = FALSE;
            actualCollection->FindFamilyName(fontName, &familyIndex, &familyExists);
            
            if (familyExists) {
              IDWriteFontFamily *family = NULL;
              hr = actualCollection->GetFontFamily(familyIndex, &family);
              
              if (SUCCEEDED(hr)) {
                // Find the specific font
                IDWriteFont *font = NULL;
                hr = family->GetFirstMatchingFont(
                  weight,
                  stretch,
                  style,
                  &font
                );
                
                if (SUCCEEDED(hr)) {
                  // Create descriptor for the fallback font
                  result = resultFromFont(font);
                  font->Release();
                }
                
                family->Release();
              }
            }
            
            actualCollection->Release();
          }
          
          textLayout->Release();
        }
        
        textFormat->Release();
      }
    }
    
    collection->Release();
  }
  
  // Cleanup
  if (fontNameW) {
    delete[] fontNameW;
  }
  
  delete[] wstr;
  factory->Release();
  
  return result;
}
