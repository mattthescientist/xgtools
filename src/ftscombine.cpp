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
// ftscombine   : Combines several spectral .dat files using + - x or / operators
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>
#include <sstream>

using namespace::std;

// ftsresponse version
#define VERSION "1.0"

// Default number of fit coefficients
#define DEFAULT_NUM_COEFFS  200

// Definitions for command line parameters
#define MIN_NUM_ARGS      5
#define OPERATOR_ADD      '+'
#define OPERATOR_SUBTRACT '-'
#define OPERATOR_MULTIPLY 'x'
#define OPERATOR_DIVIDE   '/'

const int float_size = sizeof (float);

//------------------------------------------------------------------------------
// showHelp () : Prints syntax help message to the standard output.
//
void showHelp () {
  cout << endl;
  cout << "ftscombine : " << endl;
  cout << "---------------------------------------------------------------" << endl;
  cout << "Syntax : ftscombine <file 1> <operator> <file 2> [<operator> <file 3> ...] <output>" << endl << endl;
  cout << "<file 1>    : Binary file for the left operand." << endl;
  cout << "<operator>  : one of + - * or / for addition, subtraction, multiplication, or division" << endl;
  cout << "<output>    : The calibrated line spectrum will be saved here." << endl;
  cout << "<coeffs>    : Number of spline fit coefficients. A larger value will reduce" << endl;
  cout << "              smoothing, allowing higher frequencies to be fitted, but" << endl;
  cout << "              could cause fit instabilities if too high (default "
    << DEFAULT_NUM_COEFFS << ")." << endl << endl;
}


vector <char> processCommandLine (int argc, char *argv[]) throw (string) 
{
  string NextArgument;
  vector <char> Operators;
  ostringstream oss;

  if (argc < MIN_NUM_ARGS) 
  {
    throw (string("Syntax error: Too few arguments were specified"));
  } 
  if ((argc) % 2 != 1) 
  {
    throw (string("Syntax error: Incorrect command line parameters"));
  }
  
  for (int i = 2; i < argc - 1; i += 2) 
  {
    NextArgument = argv [i];
    if (NextArgument.length () != 1)
    {
      oss << "Syntax error: Incorrect operator at argument " << i;
      throw (oss.str ());
    }
    switch (NextArgument [0]) 
    {
      case OPERATOR_ADD:      Operators.push_back (OPERATOR_ADD); break;
      case OPERATOR_SUBTRACT: Operators.push_back (OPERATOR_SUBTRACT); break;
      case OPERATOR_MULTIPLY: Operators.push_back (OPERATOR_MULTIPLY); break;
      case OPERATOR_DIVIDE:   Operators.push_back (OPERATOR_DIVIDE); break;
      default: 
        oss << "Syntax error: Incorrect operator at argument " << i;
        throw (oss.str ());
    }
  }
  return Operators;
}
  

//------------------------------------------------------------------------------
// Main program
//
int main (int argc, char *argv[]) 
{
  ifstream OperandFile;
  ofstream Output;
  int FileSize, NumFloats;
  float *Result, NextFloat;
  vector <char> Operators;

  // Check the user's command line input
  try 
  { 
    Operators = processCommandLine (argc, argv);
  }
  catch (string Err) 
  {
    cout << Err << endl;
    showHelp ();
    return 1;
  }
  
  Output.open (argv [argc - 1], ios::out|ios::binary);
  if (!Output.is_open ())
  {
    cout << "Error: Unable to write output to " << argv [argc - 1] << endl
     << "Aborting" << endl;
    return 1;
  }
  
  OperandFile.open (argv [1], ios::in|ios::binary);
  if (!OperandFile.is_open ())
  {
    cout << "Error: Unable to open " << argv [1] << endl
      << "Aborting" << endl;
    return 1;
  }
  OperandFile.seekg (0, ios::end);
  FileSize = OperandFile.tellg ();
  NumFloats = FileSize / float_size;
  OperandFile.seekg (0, ios::beg);
  Result = new float [NumFloats];
  cout << "Reading " << FileSize << " bytes from " << argv [1] <<" (" <<
    NumFloats << " floating point numbers)" << endl;
  
  for (int i = 0; i < NumFloats; i ++)
  {
    OperandFile.read ((char*)&Result [i], float_size);
  }
  OperandFile.close ();
  
  for (int i = 3; i < argc; i += 2) 
  {
    OperandFile.open (argv [i], ios::in);
    if (!OperandFile.is_open ())
    {
      cout << "Error: Unable to open " << argv [i] << endl 
        << "Saving result up to this point and aborting" << endl;
      break;
    }
    OperandFile.seekg (0, ios::end);
    if (OperandFile.tellg () != FileSize) 
    {
      cout << "Error: " << argv [i] << " is not the same size as " << argv [1] << endl 
        << "Saving result up to this point and aborting" << endl;
      OperandFile.close ();
      break;
    }
    
    OperandFile.seekg (ios::beg);
    cout << "   " << argv [i - 1] << " " << argv [i] << endl;
    switch (Operators [(i - 1) / 2 - 1]) {
      case OPERATOR_ADD:
        for (int j = 0; j < NumFloats; j ++)
        {
          OperandFile.read ((char*)&NextFloat, float_size);
//          if (j == 2) cout << Result [j] << " + " << NextFloat << " = " << flush;
          Result [j] += NextFloat;
//          if (j == 2) cout << Result [j] << endl;
        }
        break;
      case OPERATOR_SUBTRACT:
        for (int j = 0; j < NumFloats; j ++)
        {
          OperandFile.read ((char*)&NextFloat, float_size);
          Result [j] -= NextFloat;
        }
        break;
      case OPERATOR_MULTIPLY:
        for (int j = 0; j < NumFloats; j ++)
        {
          OperandFile.read ((char*)&NextFloat, float_size);
          Result [j] *= NextFloat;
        }
        break;
      case OPERATOR_DIVIDE:
        for (int j = 0; j < NumFloats; j ++)
        {
          OperandFile.read ((char*)&NextFloat, float_size);
          Result [j] /= NextFloat;
        }
        break;
      default:
        cout << "An internal error has occurred with the operator at argument "
          << i - 1 << endl << "Saving result up to this point and aborting" << endl;
        i = argc;
    }
    OperandFile.close ();
  }
  
  cout << "Writing the result to " << argv [argc - 1] << endl << endl;
  for (int i = 0; i < NumFloats; i ++)
  {
    Output.write ((char*)&Result [i], float_size);
  }
  Output.close ();
  delete [] Result;
  return 0;
}
