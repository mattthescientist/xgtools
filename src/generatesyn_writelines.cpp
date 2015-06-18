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
// generatesyn_writelines : Generates an XGremlin SYN file from writelines output
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include "xgline.h"

using namespace::std;

#define REQ_NUM_ARGS 3
#define WRITELINES_INPUT 1
#define SYN_OUTPUT 2

#define ERR_NO_ERROR           0
#define ERR_INPUT_READ_ERROR   1
#define ERR_OUTPUT_WRITE_ERROR 2

#define HEADER_SIZE 4

//------------------------------------------------------------------------------
// showHelp () : Prints syntax help message to the standard output.
//
void showHelp () {
  cout << endl;
  cout << "generatesyn_writelines : Generates an XGremlin SYN file from writelines output" << endl;
  cout << "----------------------------------------------------------------------------------" << endl;
  cout << "Syntax : generate_syn <kurucz in> <syn out>" << endl << endl;
  cout << "<writelines in>  : An XGremlin 'writelines' list from which to generate a SYN file" << endl;
  cout << "<syn out>    : The SYN file generated from <writelines in>" << endl << endl;
}

//------------------------------------------------------------------------------
// Main program
//
int main (int argc, char* argv[]) 
{
  // Check the user's command line input
  if (argc != REQ_NUM_ARGS) {
    cout << "Syntax error: Too few arguments were specified" << endl;
    showHelp ();
    return 1;
  }  
  
  ifstream WriteLinesList (argv [WRITELINES_INPUT]);
  ofstream SynOutput (argv [SYN_OUTPUT]);
  string StrNextLine;
  XgLine NextLine;
  vector <string> Lines;
  if (WriteLinesList.is_open ())
  {
    if (SynOutput.is_open ()) 
    {
      // Discard the writelines file header
      for (unsigned int i = 0; i < HEADER_SIZE; i ++) {
        getline (WriteLinesList, StrNextLine);
      }
      
      // Read the line data from the writelines file
      while (!WriteLinesList.eof ()) 
      {
        if (WriteLinesList.peek () != '\0')
        {
          getline (WriteLinesList, StrNextLine);
          NextLine.createLine (StrNextLine);
          Lines.push_back (NextLine.getLineSynString ());
        }
      }
      for (unsigned int i = 0; i < Lines.size (); i ++)
      {
        SynOutput << Lines [i] << endl;
      }
    }
    else 
    {
      cout << "Error: Unable to write to " << argv [WRITELINES_INPUT] << endl;
      return ERR_OUTPUT_WRITE_ERROR;
    }
  } 
  else 
  {
    cout << "Error: Unable to open " << argv [WRITELINES_INPUT] << endl;
    return ERR_INPUT_READ_ERROR;
  }
  WriteLinesList.close ();
  SynOutput.close ();
  return ERR_NO_ERROR;
}


      
      
  
  
