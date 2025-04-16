# list-fonts-json

This is a tiny cross-platform utility which lists the available installed fonts in a JSON format to stdout. It now implements all the features from the original [font-manager](https://github.com/foliojs/font-manager) NodeJS module.

This code is derived from the [font-manager](https://github.com/foliojs/font-manager) NodeJS module, but makes it a separate executable and not a node module which needs to be rebuilt everytime you install it or change node versions.

Example output:

```
[
  {
    "path": "/usr/share/fonts/truetype/noto/NotoSansDisplay-Bold.ttf",
    "postscriptName": "NotoSansDisplay-Bold",
    "family": "Noto Sans Display",
    "style": "Bold",
    "weight": 700,
    "width": 5,
    "italic": false,
    "oblique": false,
    "monospace": true
  }
]
```
The output is a JSON array of objects where each object describes a font with the following fields:

* **path** - The path to the font file on the file system.
* **postscriptName** - The PostScript name of the font.
* **family** - The name of the font.
* **style** - The general style of the font.
* **weight** - The weight or thickness of the font. This number ranges from 100 for very thin, to 400 for normal, up to 900 for maximum heavy.
* **width** - The width of the font. This number ranges from 1 for ultra condensed, to 5 for normal, up to 9 for ultra expanded.
* **italic** - (boolean) Whether the font is an italic font.
* **oblique** - (boolean) Whether the font is oblique.
* **monospace** - (boolean) Whether the font is monospace.

Note that italic, oblique and monospace fields tend not to be reliably reported by most operating systems.


## Platforms

* Mac OS X 10.5 and later supported via [CoreText](https://developer.apple.com/library/mac/documentation/Carbon/reference/CoreText_Framework_Ref/_index.html)
* Windows 7 and later supported via [DirectWrite](http://msdn.microsoft.com/en-us/library/windows/desktop/dd368038(v=vs.85).aspx)
* Linux supported via [fontconfig](http://www.freedesktop.org/software/fontconfig)

## Build

The build system is based on [CMake](https://cmake.org/). Make sure you have a recent version installed.


### macOS

Make sure you have Xcode's C/C++ compiler installed, and then from the root of this git repository run:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
The executable is left in the build folder.


### Linux

First ensure you have a working C/C++ compiler and the fontconfig development files installed. Now run:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
The executable is left in the build directory.


### Linux in Docker

It is also possible to build the Linux binary via Docker. That advantage is that you don't need any extra tooling installed and the resulting executable should have greater compatibilitywith older Linux distributions.

Run the script:

```
./build_linux_in_docker.sh
```

The executable is left in the build directory.


### Windows

Make sure you have CMake installed and a "Visual Studio Visual C/C++ Build Tools 2015" or similar installed.

From PowerShell run:

```
  mkdir build
  cd build
  cmake .. -G "Visual Studio 16 2019" -A x64
```

Now open up a suitable "Developer Command Promp", go to the build directory and run:

```
msbuild ALL_BUILD.vcxproj /p:Configuration=Release
```
The exe should now be in the `Release/` directory.


## License

MIT

Simon Edwards
<simon@simonzone.com>

## Usage

The tool supports multiple commands for different font-related operations:

```bash
# List all available fonts (default behavior)
list-fonts-json

# Or explicitly specify the command
list-fonts-json list

# Get a list of all font families
list-fonts-json families

# Find fonts matching specific criteria
list-fonts-json find --family="Arial" --italic --weight=700

# Find the best matching font
list-fonts-json find-best --family="Helvetica" --weight=400

# Find a font that can substitute for another when displaying specific text
list-fonts-json substitute "Arial-Regular" "こんにちは"
```

### Command Line Options

For the `find` and `find-best` commands, the following filter options are available:

* `--family=<name>` - Filter by font family name
* `--style=<style>` - Filter by font style
* `--postscript=<name>` - Filter by PostScript name
* `--monospace` - Filter for monospace fonts
* `--italic` - Filter for italic fonts
* `--weight=<weight>` - Filter by weight (100-900)
* `--width=<width>` - Filter by width (1-9)