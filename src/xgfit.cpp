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
// xgfit : An XGremlin line fitting tool
//
// XGremlin's lsqfit function can become unstable if any of the lines being
// fitted differ greatly from the initial parameters (such as if the line being
// fitted is actually absent from the spectrum). This evenutally leads to
// corruption of the loaded spectrum and the loss of any work to that point.
//
// xgfit is a wrapper that exploits XGremlin's script processing functionality
// to run lsqfit in many batches of a few iterations. At the end of each batch,
// the line parameters are examined, and any unstable line dropped from
// subsequent fits. This process is repeated until lsqfit minimises the fit
// parameters for all remaining lines.

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>
#include <cmath>
//#include "line.h"
#include "xgline.h"

#define NUM_REQ_ARGS 4
#define ERR_SYNTAX_ERROR 1
#define ERR_SCRIPT_ERROR 2

#define TEMP_SCRIPT ".xgfit_script"
#define TEMP_LINES ".xgfit_lines"
#define SCRIPT_BACKUP ".xgremlinrc.bak"
#define XGREMLIN_BIN "xgremlin"
#define XGREMLIN_RC "~/.xgremlinrc"

#define NUM_INIT_ITERATIONS 1
#define NUM_STD_ITERATIONS 1
#define DEF_SCALE 1.0
#define MAX_ALLOWED_ITERATIONS 30

using namespace::std;

//#define XG_WRITELINES_HEADER_LENGTH 4 /* rows */
#define XG_WAVCORR_OFFSET 33
#define LIN_HEADER_SIZE 320 /* bytes */

// A namespace to store the header from the XGremlin writelines file. This can
// then be used to copy the header to the output line list in writeLines().
namespace writelines_header {
  string WaveCorr;
  string AirCorr;
  string IntCal;
  string Columns;
}

// In XGremlin's lineio.f, the layout of a .lin file record is explained:
// 
//"* variable    type           size/bytes
// * --------    ----           ----------
// * sig         real*2         8
// * xint        real           4
// * width       real           4
// * dmping      real           4
// * itn         integer*2      2
// * ihold       integer*2      2
// * tags        character*4    4
// * epstot      real           4
// * epsevn      real           4
// * epsodd      real           4
// * epsran      real           4
// * spare       real           4
// * ident       character*32   32"
// 
// struct line_in replicates this record structure, allowing individual lines
// to be read directly from a .lin file in the readLinFile() function below.
//
typedef struct line_in {
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
} LineIn;


void prep_spectrum (char *Filename, char *LineList, vector <string> &Script, double Scale);
void load_spectrum (char *Filename, vector <string> &Script);
void fit_lines (int Iterations, vector <bool> Drop, vector <string> &Script);
void write_lines (vector <string> &Script);
void run_xg_script (vector <string> &Script) throw (string);
//void readLineList (string Filename, vector <Line> *Lines) throw (int);
vector <XgLine> readLinFile (string LinFile) throw (int);
void testArguments (int argc, char *argv[]) throw (string);
void showHelp ();

int main (int argc, char *argv[]) {
  vector <string> XgScript;
  vector <bool> Drop;
  vector <XgLine> FittedLines, InitialLines;
  unsigned int IterationsDone = NUM_INIT_ITERATIONS;
  bool FitIncomplete;
  ostringstream oss;
  
  // Check the command line arguments. Display an error message if they're
  // incorrect.
  try {
    testArguments (argc, argv);
  }
  catch (string &e) {
    cerr << e << endl;
    showHelp ();
    return 1;
  }
  
  // Prepare the XGremlin script for the initial run of lsqfit
  cout << argv[2] << endl;
  prep_spectrum (argv [1], argv [2], XgScript, DEF_SCALE);
  load_spectrum (argv [1], XgScript);
  fit_lines (NUM_INIT_ITERATIONS, Drop, XgScript);
  write_lines (XgScript);

  // Launch XGremlin for the first time to create the .lin file and run lsqfit
  try {
    run_xg_script (XgScript);
  } 
  catch (string &e) {
    cerr << e << endl;
    return ERR_SYNTAX_ERROR;
  }
  
  // Load the initial line fit results into InitialLines and prepare the list
  // of dropped lines.
//  readLineList (TEMP_LINES, &InitialLines);
  InitialLines = readLinFile (string(argv[1]) + ".lin");
  for (unsigned int i = 0; i < InitialLines.size (); i ++) {
    Drop.push_back (false);
  }
  
  // Now iterate, performing multiple calls to lsqfit and checking the results
  // each time. Drop any lines for which the fit parameters begin to differ
  // significantly from those in InitialLines.
  do {
    XgScript.clear ();
    load_spectrum (argv [1], XgScript);
    fit_lines (NUM_STD_ITERATIONS, Drop, XgScript);
    IterationsDone += NUM_STD_ITERATIONS;
    write_lines (XgScript);
    try {
      run_xg_script (XgScript);
    } 
    catch (string &e) {
      cerr << e << endl;
      return ERR_SCRIPT_ERROR;
    }
    cout << "Iterations done: " << IterationsDone << endl;
    
    FitIncomplete = false;
//    readLineList (TEMP_LINES, &FittedLines);
    FittedLines = readLinFile (string(argv[1]) + ".lin");
    for (unsigned int i = 0; i < FittedLines.size (); i ++) {
      if (FittedLines [i].itn () == IterationsDone + 1) {
        FitIncomplete = true;
        if (FittedLines[i].width () >= 100.0 * InitialLines[i].width () ||
          FittedLines[i].width () <= InitialLines [i].width () / 100.0) {
          Drop [i] = true;
          cout << "Dropped line " << i + 1 << ": Width unstable" << endl;
        }
        if (FittedLines[i].peak () >= 1000.0 * InitialLines[i].peak () ||
          FittedLines[i].peak () <= InitialLines [i].peak () / 1000.0) {
          Drop [i] = true;
          cout << "Dropped line " << i + 1 << ": Peak unstable" << endl;
        }
        if (abs(FittedLines[i].wavenumber () - InitialLines[i].wavenumber ()) > 0.3) {
          Drop [i] = true;
          cout << "Dropped line " << i + 1 << ": Wavenumber unstable" << endl;
        }
      }
    }
  } while (FitIncomplete && IterationsDone < MAX_ALLOWED_ITERATIONS);
  
  // Copy the final line list to the user's specified location. Display an error
  // message if this operations fails.
  oss.str (""); oss << "cp " << TEMP_LINES << " " << argv[3];
  if (system (oss.str ().c_str ()) != 0) {
    cout << "Error copying fit results to " << argv[3] << "." << endl 
      << "Please ensure you have write permissions for this location." << endl
      << "The fitted line list should still exist in " << TEMP_LINES <<".";
    return ERR_SCRIPT_ERROR;      
  }
    

  return 0;
}


//------------------------------------------------------------------------------
// testArguments : Checks that the correct command line arguments have been
// passed to xgfit. Throw an error message if a problem is encountered.
//
void testArguments (int argc, char *argv[]) throw (string) {
  ostringstream oss;
  ofstream output;

  if (argc != NUM_REQ_ARGS) {
    throw (string ("Incorrect command line parameters"));
  }
  
  oss << "ls " << argv[1] << ".dat > /dev/null 2> /dev/null";
  if (system (oss.str().c_str())) {
    oss.str (""); oss << "Unable to find .dat file for spectrum " << argv[1];
    throw (oss.str ());
  }
  
  oss.str (""); oss << "ls " << argv[1] << ".hdr > /dev/null 2> /dev/null";
  if (system (oss.str().c_str())) {
    oss.str (""); oss << "Unable to find .hdr file for spectrum " << argv[1];
    throw (oss.str ());
  }
  
  oss.str(""); oss << "ls " << argv[2] << " > /dev/null 2> /dev/null";
  if (system (oss.str().c_str())) {
    oss.str (""); oss << "Unable to find line list " << argv[2];
    throw (oss.str ());
  }
  
  output.open (argv [3], ios::out);
  if (!output.is_open ()) {
    oss.str (""); oss << "Unable to create output file " << argv[3];
    throw (oss.str ());
  }
  output.close ();
}


//------------------------------------------------------------------------------
// showHelp : Displays a help message to standard out if xgfit has been run
// with the incorrect command line parameters.
//
void showHelp () {
  cout << endl << "xgfit : An XGremlin line fitting tool" << endl;
  cout << "----------------------------------------------------" << endl;
  cout << "Syntax : xgfit <spectrum> <syn list> <output>" << endl << endl;
  cout << "<spectrum> : An XGremlin line spectrum containing the lines to be fitted." << endl;
  cout << "<syn list> : A synthetic  XGremlin line list containing the lines to be fitted." << endl;
  cout << "<output>   : The line fit parameters will be saved to this file." << endl << endl;
}


//------------------------------------------------------------------------------
// readLineList (string, vector <Line>) : Opens and reads an XGremlin writelines
// line list. The string from each individual row in the ascii file is passed to
// the Line object constructor, which extracts the line parameters. The
// resulting Line object is added to the Line vector at arg2, which, being 
// passed in by reference, is returned to the calling function.
//
/*void readLineList (string Filename, vector <Line> *Lines) throw (int) {
  string LineString;
  double WavCorr = 0.0;
  unsigned int LineCount = XG_WRITELINES_HEADER_LENGTH;
  // Open the specified line list and abort if it cannot be read.
  ifstream ListFile (Filename.c_str(), ios::in);
  if (! ListFile.is_open()) {
    cout << "Error: Cannot read " << Filename 
      << ". Check the file exists and has read permissions." << endl;
    throw int(LC_FILE_OPEN_ERROR);
  }
  
  // Extract the data from the line list header
  try {
    getline (ListFile, writelines_header::WaveCorr); // wavenumber correction
    if (ListFile.fail()) throw(" wavenumber correction ");
    getline (ListFile, writelines_header::AirCorr);  // air correction
    if (ListFile.fail()) throw("  air correction ");
    getline (ListFile, writelines_header::IntCal);   // intensity calibration
    if (ListFile.fail()) throw(" intensity calibration ");
    getline (ListFile, writelines_header::Columns);  // column headers
    if (ListFile.fail()) throw(" column headers ");
  } catch (const char* Line) {
    cout << "Error reading" << Line << "from the " << Filename << " header.\n"
      << "Check the file was written with XGremlin's 'writelines' command.\n"
      << "Hint: You can also create a dummy header by inserting 4 blank lines "
      << "at the\ntop of the file and placing the first line of data on line 5."
      << endl;
    throw int(LC_FILE_HEAD_ERROR);
  }
      
  // Create a new Line object for each line in the list. Store these in the 
  // Lines vector.
  Lines -> clear ();
  try {
    while (!ListFile.eof ()) {
      LineCount ++;
      getline (ListFile, LineString);
      if (LineString[0] != '\0') {
        Lines -> push_back (Line (LineString, WavCorr));
      }
    }
  } catch (const char* Err) {
    cout << "Error reading " << Err << " from line " << LineCount << " in " 
      << Filename << ". File loading aborted." << endl;
    throw int(LC_FILE_READ_ERROR);
  }
  ListFile.close ();
}*/


//------------------------------------------------------------------------------
// readLinFile
//
vector <XgLine> readLinFile (string LinFile) throw (int) {
  ifstream LinIn;
  int NumLines;
  float Scale, SigCorrection;
  LineIn NextLineIn;
  XgLine NextLine;
  vector <XgLine> RtnLines;
//  VoigtLsqfit V;
  
  LinIn.open (LinFile.c_str (), ios::in|ios::binary);
  if (!LinIn.is_open ()) {
    cout << "Error opening " << LinFile << ". File loading aborted." << endl;
    throw int(LC_FILE_READ_ERROR);
  }
  
  // Read the necessary information from the LIN file header.
  LinIn.read ((char*)&NumLines, sizeof (int));
  LinIn.seekg (sizeof (int) + sizeof (float), ios::cur);
  LinIn.read ((char*)&Scale, sizeof (float));
  LinIn.read ((char*)&SigCorrection, sizeof (float));
  
  // Move to the location of the first line then extract all the line records.
  LinIn.seekg (LIN_HEADER_SIZE, ios::beg);
  for (int i = 0; i < NumLines; i ++) {
    LinIn.read ((char*)&NextLineIn, sizeof (LineIn));
    if (LinIn.good ()) {
      NextLine.line (i + 1);
      NextLine.itn (NextLineIn.itn);
      NextLine.h (NextLineIn.ihold);
      NextLine.wavenumber (NextLineIn.wavenumber);
      NextLine.peak (NextLineIn.peak);
      NextLine.width (NextLineIn.width);
      NextLine.dmp ((NextLineIn.dmp - 1.0) / 25.0);
      NextLine.tags (NextLineIn.tags);
      NextLine.epstot (NextLineIn.epstot);
      NextLine.epsevn (NextLineIn.epsevn);
      NextLine.epsodd (NextLineIn.epsodd);
      NextLine.epsran (NextLineIn.epsran);
      NextLine.id (string (NextLineIn.id));
      NextLine.id (NextLine.id().substr (0, NextLine.id().length () - 4));
      NextLine.name (LinFile.substr (LinFile.find_last_of ("/\\") + 1));
      
      // Calculate the equivalent width of the line using XGremlin's mystical
      // "p" array, as shown in subroutine wrtlin in lineio.f
/*      int DmpInt = int (NextLineIn.dmp);
      float DmpFraction = NextLineIn.dmp - float (DmpInt);
      if (DmpInt == 26) {
        NextLine.eqwidth (V.P (26));
      } else {
        NextLine.eqwidth (V.P(DmpInt) + DmpFraction*(V.P(DmpInt+1)-V.P(DmpInt)));
      }
      NextLine.eqwidth(NextLine.eqwidth() * NextLine.width() * NextLine.peak());*/
      RtnLines.push_back (NextLine);
    }
    else {
      cout << "Error extracting data from " << LinFile << ". File loading aborted." << endl;
      throw int(LC_FILE_READ_ERROR);
    }    
  }
  LinIn.close ();
  return RtnLines;
}


//==============================================================================
// XGREMLIN SCRIPT FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// prep_spectrum
//
void prep_spectrum (char *Filename, char *LineList, vector <string> &Script, double Scale) {
  ostringstream oss;
  string StrFilename (Filename);
  string StrLineList (LineList);
  Script.push_back (string ("open datain " + StrFilename));
  oss << "set scale " << Scale;
  Script.push_back (oss.str ());
  Script.push_back (string ("read; plot"));
  Script.push_back (string ("readlines " + StrLineList + " \"syn\""));
  Script.push_back (string ("cdin"));
}


//------------------------------------------------------------------------------
// load_spectrum
//
void load_spectrum (char *Filename, vector <string> &Script) {
  string StrFilename (Filename);
  Script.push_back (string ("open datain " + StrFilename));
  Script.push_back (string ("read; plot; getlines"));
}


//------------------------------------------------------------------------------
// fit_lines
//
void fit_lines (int Iterations, vector <bool> Drop, vector <string> &Script) {
  ostringstream oss;
  Script.push_back (string ("active; plot"));
  for (unsigned int i = 0; i < Drop.size (); i ++) {
    if (Drop [i]) {
      oss.str (""); oss << "drop " << i + 1;
      Script.push_back (oss.str ());
    }
  }
  Script.push_back (string ("holdamp -1.0; holdwidth -1.0; holdsigma -1.0"));
  Script.push_back (string ("holddamping -1.0"));
  oss.str (""); oss << "lsqfit " << Iterations;
  Script.push_back (oss.str ());
  Script.push_back (string ("putlines"));
}


//------------------------------------------------------------------------------
// write_lines
//
void write_lines (vector <string> &Script) {
  Script.push_back (string ("writelines ") + string (TEMP_LINES) + string(" \"#\""));
}


//------------------------------------------------------------------------------
// run_xg_script
//
void run_xg_script (vector <string> &Script) throw (string) {
  ofstream ScriptFile;
  ostringstream oss;
  bool PreviousXGremlinRcExists = false;
  
  // First complete the XGremlin script and copy it to a temporary text file
  // named by the definition TEMP_SCRIPT. Abort if this file cannot be created.
  Script.push_back (string ("bye"));
  Script.push_back (string ("break"));
  ScriptFile.open (TEMP_SCRIPT);
  if (!ScriptFile.is_open ()) {
    oss << "Error accessing " << TEMP_SCRIPT << ". Please ensure you have " <<
      "read/write permissions for the current directory. xgfit aborted.";
    string ErrMsg (oss.str ());
    throw (ErrMsg);
  }
  for (unsigned int i = 0; i < Script.size (); i ++) {
    ScriptFile << Script [i] << endl;
  }
  ScriptFile.close ();
  
  // Check to see if a .xgremlinrc file already exists in the user's home
  // directory. If one does, create a backup in the current directory. Abort if
  // this backup cannot be created.
  oss.str (""); oss << "ls " << XGREMLIN_RC << " > /dev/null 2> /dev/null";
  if (system (oss.str ().c_str ()) == 0) {
    oss.str (""); oss << "cp " << XGREMLIN_RC << " " << SCRIPT_BACKUP;
    if (system (oss.str ().c_str ()) != 0) {
      oss.str ("");
      oss << "Error backing up the existing " << XGREMLIN_RC << ". Please ensure you have " <<
        "read/write permissions for both " << XGREMLIN_RC << " and the current directory. xgfit aborted.";
      string ErrMsg (oss.str ());
      throw (ErrMsg);      
    }
    PreviousXGremlinRcExists = true;
  }
  
  // Copy the xgfit script to .xgremlinrc in the user's home directory. Check
  // the return status of cp to make sure that it completed this task correctly.
  // Abort if cp failed.
  oss.str (""); oss << "cp " << TEMP_SCRIPT << " " << XGREMLIN_RC;
  if (system (oss.str ().c_str ()) != 0) {
    oss.str ("");
    oss << "Error creating new " << XGREMLIN_RC << ". Please ensure you have " <<
      "write permissions for this location. xgfit aborted.";
    string ErrMsg (oss.str ());
    throw (ErrMsg);      
  }
  
  // Run XGremlin. The system function will not return control until XGremlin
  // has finished running the script and shut itself down.
  system (XGREMLIN_BIN);
  
  if (PreviousXGremlinRcExists) {
    // No need for such severe error checking here. If we have reached this
    // point, we know there are read/write permissions for both SCRIPT_BACKUP
    // and XGREMLIN_RC.
    oss.str (""); oss << "cp " << SCRIPT_BACKUP << " " << XGREMLIN_RC;
    system (oss.str ().c_str ());
    oss.str (""); oss << "rm " << SCRIPT_BACKUP;
    system (oss.str ().c_str ());
  } else {
    oss.str (""); oss << "rm " << XGREMLIN_RC;
    system (oss.str ().c_str ());
  }
  
  oss.str (""); oss << "rm " << TEMP_SCRIPT;
  system (oss.str ().c_str ());
}































