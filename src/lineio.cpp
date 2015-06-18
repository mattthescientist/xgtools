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
// lineio.cpp
//
// Contains routines for reading from and writing to XGremlin data files, which
// are used by ftscommonlines, ftscalibrate, ftslinefilter, and ftsplot. The
// primary file format used is the XGremlin 'writelines' format, but additional
// write routines, writeSynLines(...), enable line data to be saved in the 'syn'
// format for use by XGremlin's 'readlines' command.
//
// On input, readLineList(...) extracts the lines from an XGremlin 'writelines'
// file and stores each in a Line object. Conversely, on output, a vector of 
// Line objects is passed to either writeLines(...) or writeSynLines(...) and 
// written in 'writelines' or 'syn' format respectively.
// 
#ifndef LINE_IO_CPP
#define LINE_IO_CPP

#define XG_WRITELINES_HEADER_LENGTH 4 /* rows */
#define XG_WAVCORR_OFFSET 33

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include "ErrDefs.h"
#include "line.h"

// A namespace to store the header from the XGremlin writelines file. This can
// then be used to copy the header to the output line list in writeLines().
namespace writelines_header {
  string WaveCorr;
  string AirCorr;
  string IntCal;
  string Columns;
}


//------------------------------------------------------------------------------
// getWavCorr (string) : Extracts the wavenumber scaling factor from an XGremlin
// 'writelines' header. If no scaling was applied to a line list, a value of
// zero is returned.
//
double getWavCorr (string HeaderLine) throw (int) {
  istringstream iss;
  string NextField;
  char Rubbish [XG_WAVCORR_OFFSET];
  double WavCorr = 0.0;
  
  iss.str (HeaderLine);
  iss >> NextField;
  if (NextField == "NO") return 0.0;
  else if (NextField == "WAVENUMBER") iss.get (Rubbish, XG_WAVCORR_OFFSET);
  else { 
    cout << HeaderLine << endl;
    cout << "Error: Unable to read the wavenumber correction from the line list"
      << " header." << endl;
    throw (LC_FILE_READ_ERROR);
  }
  iss >> WavCorr;
  return WavCorr;
}
    

//------------------------------------------------------------------------------
// readLineList (string, vector <Line>) : Opens and reads an XGremlin writelines
// line list. The string from each individual row in the ascii file is passed to
// the Line object constructor, which extracts the line parameters. The
// resulting Line object is added to the Line vector at arg2, which, being 
// passed in by reference, is returned to the calling function.
//
void readLineList (string Filename, vector <Line> *Lines) throw (int) {
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
    WavCorr = getWavCorr (writelines_header::WaveCorr);
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
}


//------------------------------------------------------------------------------
// writeLines (vector <Line>, ostream) : Requests the XGremlin writelines string
// from each Line in the vector at arg1 and sends this string to the stream at
// arg2.
//
void writeLines (vector <Line> Lines, ostream &Output = std::cout) throw (const char*) {
  if (Lines[0].wavCorr () != 0.0) {
    Output << "  WAVENUMBER CORRECTION APPLIED: wavcorr =   " 
      << Lines[0].wavCorr () << endl;
  }
  else {
    Output << writelines_header::WaveCorr << endl;
  }
  Output << writelines_header::AirCorr << endl;
  Output << writelines_header::IntCal << endl;
  Output << writelines_header::Columns << endl;
  if (Output.fail()) throw "the file header";
  for (unsigned int i = 0; i < Lines.size (); i ++) {
    Output << Lines[i].getLineString() << endl;
    if (Output.fail ()) {
      ostringstream oss;
      oss << "line " << Lines[i].line ();
      throw oss.str().c_str();
    }
  }
}

//------------------------------------------------------------------------------
// writeLines (vector <Line>, string) : Creates an output file stream from
// the filename specified at arg2, then calls writeLines (vector <Line>,
// ostream) to output the XGremlin writelines data to this file.
//
void writeLines (vector <Line> Lines, string Filename) throw (int) {
  ofstream ListFile (Filename.c_str(), ios::out);
  if (! ListFile.is_open()) {
    cout << "Error: Cannot open " << Filename 
      << " for output. List writing ABORTED." << endl;
    throw int (LC_FILE_OPEN_ERROR);
  }
  try {
    writeLines (Lines, ListFile);
  } catch (const char *Err) {
    cout << "Error writing " << Err << " to " << Filename << 
      ". List writing ABORTED." << endl;
    throw int (LC_FILE_WRITE_ERROR);
  }
}


//------------------------------------------------------------------------------
// writeSynLines (vector <Line>, ostream) : Requests the XGremlin 'syn' string
// from each Line in the vector at arg1 and sends this string to the stream at
// arg2.
//
void writeSynLines (vector <Line> Lines, ostream &Output = std::cout) throw (const char*) {
  for (unsigned int i = 0; i < Lines.size (); i ++) {
    Output << Lines[i].getLineSynString() << endl;
    if (Output.fail ()) {
      ostringstream oss;
      oss << "line " << Lines[i].line ();
      throw oss.str().c_str();
    }
  }
}

//------------------------------------------------------------------------------
// writeSynLines (vector <Line>, string) : Creates an output file stream from
// the filename specified at arg2, then calls writeSynLines (vector <Line>,
// ostream) to output the XGremlin 'syn' data to this file.
//
void writeSynLines (vector <Line> Lines, string Filename) throw (int) {
  ofstream ListFile (Filename.c_str(), ios::out);
  if (! ListFile.is_open()) {
    cout << "Error: Cannot open " << Filename 
      << " for output. List writing ABORTED." << endl;
    throw int (LC_FILE_OPEN_ERROR);
  }
  try { 
    writeSynLines (Lines, ListFile);
  } catch (const char *Err) {
    cout << "Error writing " << Err << " to " << Filename << 
      ". List writing ABORTED." << endl;
    throw int (LC_FILE_WRITE_ERROR);
  }
}

#endif // LINE_IO_CPP
