// Xgtools
// Copyright (C) M. P. Ruffoni 2011-2015
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// xgsave : An XGremlin scratch file converter
//
// XGremlin does not provide the option of writing processed line spectra back
// to disk as new spectra (i.e. as a new pair of .dat & .hdr files). It does,
// however, allow the user to create "scratch" spectra with its "save" command.
// These contain all the raw spectrum data points required by a new .dat file,
// but with a different file header.
//
// xgsave removes the header from an XGremlin scratch file and saves the result
// as a line spectrum .dat file. A specified .hdr file is then copied from
// another spectrum and saved as an accompanying .hdr file for this new .dat.
// Care should be taken to ensure that this header file correctly describes the
// scratch spectrum!

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#define HEADER_SIZE 368 /* bytes */
#define REQUIRED_NUM_ARGS 5

#define ERR_CANT_OPEN_SCRATCH 1
#define ERR_CANT_OPEN_HEADER 2
#define ERR_CANT_OPEN_OUTPUT 3
#define ERR_PAD_NOT_NUMERIC 4
#define SYNTAX_ERROR 5

using namespace std;

//------------------------------------------------------------------------------
// is_numeric (string) : Determines whether or not argument 1 is a number.
//
bool is_numeric (string a) {
  for (unsigned int i = 0; i < a.length (); i ++) {
    if (!isalnum(a[i]) || isalpha(a[i])) {
      return false;
    }
  }
  return true;
}

int main (int argc, char *argv[]) {
  ifstream ScratchFileIn, HeaderIn;
  ofstream DatFileOut, HeaderOut;
  string OutputSpectrum, OutputHeader;
  char NextByte;
  float yBegin, yEnd, y;
  int BoxcarSize;
  
  // Check that the correct arguments have been supplied. If not, output a
  // simple program description and quit.
  if (argc != REQUIRED_NUM_ARGS) {
    cout << "xgsave : An XGremlin scratch file converter" << endl;
    cout << "----------------------------------------------------" << endl;
    cout << "Syntax : xgsave <scratch> <header> <padding> <output>" << endl << endl;
    cout << "<scratch> : An XGremlin scratch.? file to be converted into a normal XGremlin line spectrum." << endl;
    cout << "<header>  : An XGremlin line spectrum header file to use for the scratch spectrum." << endl;
    cout << "<padding> : The number of data points in the spectrum will be increased by this" << endl;
    cout << "            factor using linear interpolation (min. value 1.0)." << endl;
    cout << "<output>  : The converted line spectrum will be saved in this file." << endl << endl;
    return SYNTAX_ERROR;
  }
  
  // Open the input scratch file. Output an error message and quite if it
  // failed to open.
  ScratchFileIn.open (argv[1]);
  if (!ScratchFileIn.is_open ()) {
    cout << "Error: Unable to open the scratch file " << argv[1] << ". Check the file exists and is readable." << endl;
    return ERR_CANT_OPEN_SCRATCH;
  }
  
  // Open the input line spectrum header file. Output an error message and quit
  // if it failed to open.
  HeaderIn.open (argv[2]);
  if (!HeaderIn.is_open ()) {
    cout << "Error: Unable to open the header file " << argv[2] << ". Check the file exists and is readable." << endl;
    return ERR_CANT_OPEN_HEADER;
  }
  
  if (!is_numeric (argv[3])) {
    cout << "Error: The padding factor must be an integer greater than 0" << endl;
    return ERR_PAD_NOT_NUMERIC;
  } else {
    istringstream iss;
    iss.str (argv[3]);
    iss >> BoxcarSize;
    if (BoxcarSize < 1.0) {
      cout << "Error: The padding factor must be an integer greater than 0" << endl;
      return ERR_PAD_NOT_NUMERIC;
    }
  }
    
  // Open the output spectrum and header files. Display appropriate error
  // messages and quit if either failed to open.
  OutputHeader = argv[4]; OutputHeader += ".hdr";
  OutputSpectrum = argv[4]; OutputSpectrum += ".dat";
  DatFileOut.open (OutputSpectrum.c_str());
  if (!DatFileOut.is_open ()) {
    cout << "Error: Unable to open " << OutputSpectrum.c_str() << " for output. Check that you have write permissions for that location." << endl;
    return ERR_CANT_OPEN_OUTPUT;
  }
  HeaderOut.open (OutputHeader.c_str());
  if (!HeaderOut.is_open ()) {
    cout << "Error: Unable to open " << OutputHeader.c_str() << " for output. Check that you have write permissions for that location." << endl;
    return ERR_CANT_OPEN_OUTPUT;
  }

  // Now begin the actual process of converting the scratch file to an XGremlin
  // line spectrum.
  
  // First Move the scratch file get pointer to the end of the file header.
  ScratchFileIn.seekg (ios::beg + HEADER_SIZE * sizeof(char));

  // Now copy the scratch spectrum, byte by byte to the output spectrum.
  
  ScratchFileIn.read ((char*)&yBegin, sizeof (float));
  ScratchFileIn.read ((char*)&yEnd, sizeof (float));
//  DatFileOut.write ((char*)&yBegin, sizeof (float));
  for (unsigned int i = 1; i <= BoxcarSize; i ++) {
      DatFileOut.write ((char*)&yBegin, sizeof (float));
    }
  while (!ScratchFileIn.eof()) {
    for (unsigned int i = 1; i <= BoxcarSize; i ++) {
      y = (float (i) / float (BoxcarSize)) * (yEnd - yBegin) + yBegin;
      DatFileOut.write ((char*)&y, sizeof (float));
    }
    yBegin = yEnd;
    ScratchFileIn.read ((char*)&yEnd, sizeof (float));
  }
  
  // Produce an exact copy of the input header for the converted spectrum
  HeaderIn.get (NextByte);
  while (!HeaderIn.eof ()) {
    HeaderOut.put (NextByte);
    HeaderIn.get (NextByte);
  }

  // Finally, close all the open files and quit.
  ScratchFileIn.close ();
  DatFileOut.close ();
  HeaderIn.close ();
  HeaderOut.close ();
  
  return 0;
}
