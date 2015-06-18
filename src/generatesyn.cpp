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
// generatesyn : Generates an XGremlin SYN file from a Kurucz line list
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include "kzline.h"
#include "xgline.h"

using namespace::std;

#define REQ_NUM_ARGS_MODE1 3
#define REQ_NUM_ARGS_MODE2 6
#define REQ_NUM_ARGS_MODE3 5
#define REQ_NUM_ARGS_MODE4 8

#define KURUCZ_INPUT 1
#define ARG_PEAK 2
#define ARG_WIDTH 3
#define ARG_DAMPING 4
#define ARG_MIN_MODE3 2
#define ARG_MAX_MODE3 3
#define ARG_MIN_MODE4 5
#define ARG_MAX_MODE4 6

#define ERR_NO_ERROR           0
#define ERR_INPUT_READ_ERROR   1
#define ERR_OUTPUT_WRITE_ERROR 2
#define ERR_SYNTAX_ERROR       3

#define DEF_LINE_PEAK 100
#define DEF_LINE_WIDTH 30   /* mK */
#define DEF_LINE_DMP 0.0

//------------------------------------------------------------------------------
// showHelp () : Prints syntax help message to the standard output.
//
void showHelp () {
  cout << endl;
  cout << "generatesyn : Generates an XGremlin SYN file from a Kurucz line list" << endl;
  cout << "----------------------------------------------------------------------" << endl;
  cout << "Syntax : generate_syn <kurucz in> [<peak> <width> <damping>] [<min sigma> <max sigma>] <syn out>" << endl << endl;
  cout << "<kurucz in> : A Kurucz line list from which to generate a SYN file" << endl;
  cout << "<peak>      : Line peak height written to the SYN file (default " << DEF_LINE_PEAK << ")" << endl;
  cout << "<width>     : Line width written to the SYN file (default " << DEF_LINE_WIDTH << ")" << endl;
  cout << "<damping>   : Line damping written to the SYN file (default " << DEF_LINE_DMP << ")" << endl;
  cout << "<min sigma> : Minimum wavenumber for lines copied to SYN file" << endl;
  cout << "<max sigma> : Maximum wavenumber for lines copied to SYN file" << endl;
  cout << "<syn out>   : The SYN file generated from <kurucz in>" << endl << endl;
}

//------------------------------------------------------------------------------
// Main program
//
int main (int argc, char* argv[]) 
{
  ostringstream oss;
  istringstream iss;
  string StrNextLine;
  KzLine NextLine;
  float Peak = DEF_LINE_PEAK, Width = DEF_LINE_WIDTH, Damping = DEF_LINE_DMP;
  float MinX = 0, MaxX = 0;
  vector <string> Lines;
  vector <string> Args;
  
  for (unsigned int i = 1; i < argc; i ++) {
    Args.push_back (argv[i]);
  }
  
  
  // Check the user's command line input
  if (argc == REQ_NUM_ARGS_MODE1) {
  } else if (argc == REQ_NUM_ARGS_MODE2) {
    iss.str (argv [ARG_PEAK]); iss >> Peak; iss.clear ();
    iss.str (argv [ARG_WIDTH]); iss >> Width; iss.clear ();
    iss.str (argv [ARG_DAMPING]); iss >> Damping; iss.clear ();
  } else if (argc == REQ_NUM_ARGS_MODE3) {
    iss.str (argv [ARG_MIN_MODE3]); iss >> MinX; iss.clear ();
    iss.str (argv [ARG_MAX_MODE3]); iss >> MaxX; iss.clear ();
  } else if (argc == REQ_NUM_ARGS_MODE4) {
    iss.str (argv [ARG_PEAK]); iss >> Peak; iss.clear ();
    iss.str (argv [ARG_WIDTH]); iss >> Width; iss.clear ();
    iss.str (argv [ARG_DAMPING]); iss >> Damping; iss.clear ();
    iss.str (argv [ARG_MIN_MODE4]); iss >> MinX; iss.clear ();
    iss.str (argv [ARG_MAX_MODE4]); iss >> MaxX; iss.clear ();
  } else {
    showHelp ();
    return ERR_SYNTAX_ERROR;
  }
  
  // Open the Kurucz input list
  ifstream FullKuruczList (argv [KURUCZ_INPUT]);
  if (!FullKuruczList.is_open ())
  {
    cout << "Error Opening " << argv [KURUCZ_INPUT] << endl << 
      "Check the file exists and that you have permission to read it" << endl;
    return ERR_INPUT_READ_ERROR;
  }
  
  // Open the SYN output list
  ofstream SynOutput (argv [argc - 1]);
  if (!SynOutput.is_open ()) {
    cout << "Error Opening " << argv [argc - 1] << endl << 
      "Check that you have permission to write to this location" << endl;
    return ERR_OUTPUT_WRITE_ERROR;
  }

  // Read the Kurucz list and write each line out in SYN format to the SYN file. 
  while (!FullKuruczList.eof ())
  {
    if (FullKuruczList.peek () != '\0')
    {
      getline (FullKuruczList, StrNextLine);
      NextLine.readLine (StrNextLine);
      
      oss.str ("");
      oss.width (15);
      if (NextLine.eUpper () > NextLine.eLower ()) {
        oss << left << NextLine.configUpper () << "  ";
      } else {
        oss << left << NextLine.configLower () << "  ";
      }
      oss << fixed << right;
      oss.width (11); oss.precision (5); oss << NextLine.sigma ();
      oss.width (10); oss.precision (4); oss << Width;
      oss.width (9);  oss.precision (2); oss << Peak;
      oss.width (8);  oss.precision (4); oss << Damping;
      if (argc == REQ_NUM_ARGS_MODE1 || argc == REQ_NUM_ARGS_MODE2) {
        Lines.push_back (oss.str ());
      } else {
        if (NextLine.sigma () >= MinX && NextLine.sigma () <= MaxX) {
          Lines.push_back (oss.str ());
        }
      }
    }
  }
  
  // Output the lines in reverse order so that they are in ascending wavenumber.
  for (int i = Lines.size () - 1; i >= 0; i --)
  {
    SynOutput << Lines [i] << endl;
  }
  
  // Tidy up and quit
  FullKuruczList.close ();
  SynOutput.close ();
  return ERR_NO_ERROR;
}


      
      
  
  
