# cpbo
### Armed Assult PBO archive extractor & packer

This is a port of Kegetys's cpbo using Boost's C++ libraries to achieve cross-platform support.

## Usage
Extract a pbo:
```
  cpbo.exe [-y] -e (filename.pbo) [directory]
  directory name is optional, PBO file name used if undefined
  optional -y parameter overwrites directory without asking
```
Make a pbo:
```
  cpbo.exe [-y] -p (directory) [filename.pbo]
  pbo name is optional, directory name used if undefined
  optional -y parameter overwrites pbo without asking
```

## Credits
Original cpbo source code by Keijo "Kegetys" Ruotsalainen, http://www.kegetys.fi/cpbo-source-code-release/
Used to extract and create PBO archives for OFP & ArmA series
Should compile with Visual Studio 2005

## License
Licensed under LGPL v2.1, see LICENSE
