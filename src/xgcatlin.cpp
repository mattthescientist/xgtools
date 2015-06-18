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
// xgcatlin : Concatenates XGremlin LIN files
//
// xgcatlin will concatenate several XGremlin LIN files, as specified by the
// user at the command line, and save them to a single, new LIN file. The header
// from the FIRST LIN file specified at the command line will be copied to the
// new file, with only the number of lines and number of bytes in the file being
// updated. This therefore assumes that all the concatenated LIN files belong to
// THE SAME spectrum.
//
#include <iostream>
#include <fstream>
#include <vector>

using namespace::std;

// Definitions for command line parameters
#define MIN_NUM_ARGS      4

#define LIN_HEADER_SIZE 320 /* bytes */

typedef struct lin {
  double wavenumber;
  float peak;
  float width;
  float dmp;
  short itn;
  short ihold;
  char tags [4];
  float epstot;
  float epsevn;
  float epsodd;
  float epsran;
  float spare;
  char id [32];
} line_in;

const int line_in_size = sizeof (line_in);

//------------------------------------------------------------------------------
// showHelp () : Prints syntax help message to the standard output.
//
void showHelp () {
  cout << endl;
  cout << "xgcatlin : " << endl;
  cout << "---------------------------------------------------------------" << endl;
  cout << "Syntax : xgcatlin <file 1> <file 2> [<file 3> ...] <output>" << endl << endl;
  cout << "<file n> : An XGremlin LIN file." << endl;
  cout << "<output> : Concatenated LIN file will be saved here." << endl << endl;
}


//------------------------------------------------------------------------------
// Main program
//
int main (int argc, char *argv[]) 
{
  ifstream LinIn;
  ofstream Output;
  int FileSize, NumLines;
  line_in NextLineIn;
  vector <line_in> Lines;
  char NextByte;
  bool SomeLinesSwapped;
  
  // Check the user's command line input
  if (argc < MIN_NUM_ARGS) {
    cout << "Syntax error: Too few arguments were specified" << endl;
    showHelp ();
    return 1;
  } 
  
  // Open the output file. Abort if this fails
  Output.open (argv [argc - 1], ios::out|ios::binary);
  if (!Output.is_open ()) {
    cout << "Error: Unable to write output to " << argv [argc - 1] << endl
     << "Aborting" << endl;
    return 1;
  }
  
  // Copy the header from the first LIN file to the output file
  LinIn.open (argv [1], ios::in|ios::binary);
  if (!LinIn.is_open ()) {
    cout << "Error: Unable to open " << argv [1] << endl
      << "Aborting" << endl;
    return 1;
  }
  for (unsigned int i = 0; i < LIN_HEADER_SIZE; i ++) {
    LinIn.get (NextByte);
    Output.put (NextByte);
  }
  LinIn.close ();

  // Read the lines from each of the LIN files specified by the user at the
  // command line. Store the lines in a vector so they can be sorted later.
  for (int File = 1; File < argc - 1; File ++) {
    // Open the next LIN file 
    LinIn.open (argv [File], ios::in|ios::binary);
    if (!LinIn.is_open ()) {
      cout << "Error: Unable to open " << argv [File] << endl << 
        "Only the lines to this point will be saved in " << argv [argc - 1] << endl;
      break;
    }
  
    // Extract the lines from the file
    LinIn.read ((char*)&NumLines, sizeof (int));
    LinIn.seekg (LIN_HEADER_SIZE, ios::beg);
    for (int i = 0; i < NumLines; i ++) {
      LinIn.read ((char*)&NextLineIn, line_in_size);
      Lines.push_back (NextLineIn);
    }
    LinIn.close ();
    cout << "Read " << NumLines << " lines from " << argv [File] << endl;
  }
  
  
  // Now sort all the loaded lines in order of ascending wavenumber
  do {
    SomeLinesSwapped = false;
    for (unsigned int i = 1; i < Lines.size (); i ++) {
      if (Lines [i-1].wavenumber > Lines [i].wavenumber) {
        NextLineIn = Lines [i];
        Lines [i] = Lines [i - 1];
        Lines [i - 1] = NextLineIn;
        SomeLinesSwapped = true;
      }
    }
  } while (SomeLinesSwapped);
  
  // Finally, save the lines to the output file. Update the file header so that
  // it contains the correct number of lines and bytes in the file.
  for (unsigned int i = 0; i < Lines.size (); i ++) {
    Output.write ((char*)&Lines [i], line_in_size);
  }
  FileSize = Output.tellp ();
  Output.seekp (0, ios::beg);
  NumLines = Lines.size ();
  Output.write ((char*)&NumLines, sizeof (int));
  Output.write ((char*)&FileSize, sizeof (int));
  Output.close ();
  cout << "Saved " << NumLines << " lines (" << FileSize << " bytes)" << " to "
    << argv [argc - 1] << endl;
  return 0;
}
