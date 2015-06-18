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
// extractlevel : Extracts lines from a Kurucz gf*.lines file based on the 
// energy of a target upper or lower level.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

using namespace::std;

#define REQ_NUM_ARGS 4
#define OPTIONS      1
#define KURUCZ_INPUT 1
#define LEVEL_ENERGY 2
#define LEVEL_TYPE   3

#define ERR_NO_ERROR         0
#define ERR_INPUT_READ_ERROR 1

#define DISCRIMINATOR 0.005 // cm^{-1}

//------------------------------------------------------------------------------
// showHelp () : Prints syntax help message to the standard output.
//
void showHelp () {
  cout << endl;
  cout << "extractlevel : Extracts lines from a Kurucz gf*.lines file based on level energy" << endl;
  cout << "--------------------------------------------------------------------------------" << endl;
  cout << "Syntax : extractlevel [options] <list> <level> <l/u>" << endl << endl;
  cout << "<list>  : A Kurucz line list containing many upper levels" << endl;
  cout << "<level> : Extract transitions from <list> involving this level (in cm^{-1})" << endl;
  cout << "<l/u>   : Use 'l' to specify that <level> should be the transition lower level" <<endl;
  cout << "          or 'u' to specify that it should be the transition upper level." << endl << endl;
  cout << "[options] :" << endl;
  cout << "  -m : Strip the minus sign prefix on predicted energy levels." << endl;
  cout << "  -p : Remove predicted energy levels altogether." << endl;
}

//------------------------------------------------------------------------------
// Main program
//
int main (int argc, char* argv[]) 
{
  double LevelEnergy;
  bool IgnoreMinus = false, RemovePredicted = false;
  string Options;

  // Check the user's command line input
  if (argc != REQ_NUM_ARGS && argc != REQ_NUM_ARGS + 1) {
    cout << "Syntax error: Too few arguments were specified" << endl;
    showHelp ();
    return 1;
  }
  {
    istringstream iss;
    iss.str (argv [OPTIONS]);
    iss >> Options;
    if (Options == "-m") { 
      IgnoreMinus = true; 
      for (unsigned int i = OPTIONS; i < argc - 1; i ++) {
        argv[i] = argv[i + 1];
      }
      argc --;
    }
    if (Options == "-p") { 
      RemovePredicted = true; 
      for (unsigned int i = OPTIONS; i < argc - 1; i ++) {
        argv[i] = argv[i + 1];
      }
      argc --;
    }
  }
   
  {  
    istringstream iss;
    iss.str (argv [LEVEL_ENERGY]);
    iss >> LevelEnergy;
  }

  ifstream FullKuruczList (argv [KURUCZ_INPUT]);

  if (FullKuruczList.is_open ()) 
  {
    istringstream iss;
    ostringstream oss;
    string NextLine;
    oss << fixed << right;
    double LowerLevel, UpperLevel, Temp;
    double *TargetLevel;
    if (argv [LEVEL_TYPE][0] == 'l') { TargetLevel = &LowerLevel; }
    else if (argv [LEVEL_TYPE][0] == 'u') { TargetLevel = &UpperLevel; }
    else {
      cout << "Syntax error: 3rd argument must be either 'l' or 'u'" << endl;
      showHelp ();
      return 1;
    }
    while (!FullKuruczList.eof ()) 
    {
      if (FullKuruczList.peek () != '\0')
      {
        getline (FullKuruczList, NextLine);
        iss.clear (); iss.str (NextLine.substr (24, 12));
        iss >> LowerLevel;
        iss.clear (); iss.str (NextLine.substr (52, 12));
        iss >> UpperLevel;
        if (IgnoreMinus && (LowerLevel < 0 || UpperLevel < 0)) {
          LowerLevel = abs(LowerLevel);
          UpperLevel = abs(UpperLevel);
          oss.str (""); oss.clear ();
          oss << NextLine.substr (0, 24);
          oss.precision (3); oss.width (12);
          oss << LowerLevel;
          oss << NextLine.substr (36, 16);
          oss.precision (3); oss.width (12);
          oss << UpperLevel;
          oss << NextLine.substr (64);
          NextLine = oss.str ();
        }
            
        if (abs(LowerLevel) > abs(UpperLevel))
        {
          Temp = LowerLevel;
          LowerLevel = UpperLevel;
          UpperLevel = Temp;
        }
        
        if (abs(abs(*TargetLevel) - LevelEnergy) < DISCRIMINATOR) {
          if (!(RemovePredicted && (LowerLevel < 0 || UpperLevel < 0))) {
            cout << NextLine << endl;
          }
        }
      }
    }
  } 
  else 
  {
    cout << "Error: Unable to open " << argv [KURUCZ_INPUT] << endl;
    return ERR_INPUT_READ_ERROR;
  }
  FullKuruczList.close ();
  return ERR_NO_ERROR;
}


      
      
  
  
