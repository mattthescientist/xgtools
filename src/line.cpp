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

#include "line.h"
#include <iostream>
#include <sstream>
#include <cmath>

#define XG_OVERLOAD "**********"

//------------------------------------------------------------------------------
// Default Line constructor : Sets all the line properties to default values
//
Line::Line () {
  Index = 0; Itn = 0; H = 0; Wavenumber = 0.0; Peak = 0.0;  Width = 0.0; 
  Dmp = 0.0; EqWidth = 0.0; EpsTot = 0.0; EpsEvn = 0.0; EpsOdd = 0.0; 
  EpsRan = 0.0; Wavelength = 0.0; Tags = '.'; Identification = "";
  WavenumberCorrection = 0.0; AirCorrection = 0.0; IntensityCalibration = 0.0;
}


//------------------------------------------------------------------------------
// Create constructor : Creates a Line from an XGremlin "writelines" string. The
// header parameters are not contained in the input string, and so must be set
// manually afterwards.
//
Line::Line (string LineData, double NewWaveCorr, double NewAirCorr, 
  double NewIntCal) {
  WavenumberCorrection = NewWaveCorr;
  AirCorrection = NewAirCorr;
  IntensityCalibration = NewIntCal;
  createLine (LineData);
}


//------------------------------------------------------------------------------
// Line = operator : Copies the operator Line into the current class instance
//
void Line::operator= (Line Operator) {
  Index = Operator.line ();
  Itn = Operator.itn ();
  H = Operator.h ();
  Wavenumber = Operator.wavenumber ();
  Peak = Operator.peak ();
  Width = Operator.width ();
  Dmp = Operator.dmp ();
  EqWidth = Operator.eqwidth ();
  EpsTot = Operator.epstot ();
  EpsEvn = Operator.epsevn ();
  EpsOdd = Operator.epsodd ();
  EpsRan = Operator.epsran ();
  Wavelength = Operator.wavelength ();
  Tags = Operator.tags ();
  Identification = Operator.id ();
  WavenumberCorrection = Operator.wavCorr ();
  AirCorrection = Operator.airCorrection ();
  IntensityCalibration = Operator.intensityCalibration ();
}

//------------------------------------------------------------------------------
// checkInput (isstringstream &, const char *) : If a read error occurs in the
// createLine function below, checkInput is called to examine the failed input.
// If the error was caused by XGremlin writing ********** in a column rather
// than a real value, a warning is printed and input is allowed to continue. Any
// other error will cause an exception to be thrown.
//
void Line::checkInput (istringstream &iss, const char* Err) throw (const char *) {
  iss.clear ();
  string TestInput;
  iss >> TestInput;
  if (TestInput == XG_OVERLOAD) {
    cout << "Warning: " << XG_OVERLOAD << " has been found in the " << Err
      << " column. A value of zero has been taken instead." << endl;
    return;
  } else {
    throw Err;
  }
}


//------------------------------------------------------------------------------
// createLine (string) : Creates a Line from an XGremlin "writelines" string
//
void Line::createLine (string LineString) throw (const char*) {
  istringstream iss;
  iss.str (LineString);
  char IdCharString [LINE_ID_STRING_LEN];

  // Read the contents of the Line string
  iss >> skipws >> Index; if (iss.fail ()) { Index = 0; checkInput (iss, "index"); }
  iss >> Wavenumber; if (iss.fail ()) { Wavenumber = 0.0; checkInput (iss, "wavenumber"); }
  iss >> Peak; if (iss.fail ()) { Peak = 0.0; checkInput (iss, "peak height"); }
  iss >> Width; if (iss.fail ()) { Width = 0.0; checkInput (iss, "width"); }
  iss >> Dmp; if (iss.fail ()) { Dmp = 0.0; checkInput (iss, "dmp"); }
  iss >> EqWidth; if (iss.fail ()) { EqWidth = 0.0; checkInput (iss, "eqwidth"); }
  iss >> Itn; if (iss.fail ()) { Itn = 0; checkInput (iss, "itn"); }
  iss >> H; if (iss.fail ()) { H = 0; checkInput (iss, "h"); }
  iss >> Tags; if (iss.fail ()) if (iss.fail ()) throw ("tags");
  iss >> EpsTot; if (iss.fail ()) { EpsTot = 0.0; checkInput (iss, "epstot"); }
  iss >> EpsEvn; if (iss.fail ()) { EpsEvn = 0.0; checkInput (iss, "epsevn"); }
  iss >> EpsOdd; if (iss.fail ()) { EpsOdd = 0.0; checkInput (iss, "epsodd"); }
  iss >> EpsRan; if (iss.fail ()) { EpsRan = 0.0; checkInput (iss, "epsran"); }
  iss >> ws;
  
  // Read the line identification field based on a fixed length string. This is 
  // needed as the field may contain several words that could be interpreted as
  // multiple fields in a simple istringstream input operation.
  iss.get (IdCharString, LINE_ID_STRING_LEN);
  Identification = IdCharString;
  iss >> Wavelength; if (iss.fail ()) { Wavelength = 0.0; checkInput (iss, "wavelength"); }

  // Remove whitespace at the end of the ID string
  for (int i = Identification.length () - 1; i >= 0; i --) {
    if (Identification [i] == ' ') Identification.erase (i);
    else break;
  }
  
  // Finally,remove the wavenumber correction from the internally stored params.
  Wavenumber /= 1.0 + WavenumberCorrection;
  Width /= 1.0 + WavenumberCorrection;
  Wavelength *= 1.0 + WavenumberCorrection;
}


//------------------------------------------------------------------------------
// print (ostream&) : Prints the line properties to the ostream specified at
// arg1. If nothing is passed in at arg1, the information will be output to
// std::cout by default.
//
void Line::print (ostream& Output) {
  Output << "Line " << Index << " (" << Identification << "):" << endl;
  Output.precision (6);
  Output << " Wavenumber : " << fixed << Wavenumber << endl;
  Output.precision (4);
  Output << " Peak Height: " << scientific << Peak << endl;
  Output.precision (2);
  Output << " Line Width : " << fixed << Width << endl;
  Output.precision (4);
  Output << " Damping    : " << fixed << Dmp << endl;
  Output << " EqWidth    : " << scientific << EqWidth << endl;
  Output << " Itn        : " << Itn << endl;
  Output << " H          : " << H << endl;
  Output << " Tags       : " << Tags << endl;
  Output.precision (5);
  Output << " Residuals  : total = " << EpsTot << ", even = " << EpsEvn <<
    ", odd = " << EpsOdd << ", random = " << EpsRan << endl;
  Output.precision (6);
  Output << " Wavelength : " << fixed << Wavelength << endl;
}


//------------------------------------------------------------------------------
// getLineSynString () : Returns the line properties in a formatted string for 
// use with XGremlin's readlines command in 'syn' mode. 
//
string Line::getLineSynString () {
  ostringstream oss;
  oss.width (15); oss << left << Identification << "  ";
  oss.width (12); oss.precision (5); oss << fixed << right << wavenumber ();
  oss.width (10); oss.precision (4); oss << right << peak ();
  oss.width (9);  oss.precision (2); oss << right << width ();
  oss.width (8);  oss.precision (4); oss << right << dmp ();
  return oss.str();
}


//------------------------------------------------------------------------------
// getLineString () : Returns the line properties in a formatted string matching
// the XGremlin writelines file format.
//
string Line::getLineString () {
  string IdLong = id();
  IdLong.resize (LINE_ID_STRING_LEN, ' ');
  ostringstream oss;
  oss.width (6);  oss << right << Index << "  ";
  oss.width (12); oss.precision (6); oss << fixed << right << wavenumber ();
  oss.width (10); oss.precision (3); oss << scientific << right << peak ();
  oss.width (9);  oss.precision (2); oss << fixed << right << width ();
  oss.width (9);  oss.precision (4); oss << fixed << right << dmp ();
  oss.width (11); oss.precision (4); oss << scientific << right << eqwidth ();
  oss.width (6); oss << right << itn ();
  oss.width (4); oss << right << h ();
  oss.width (5); oss << right << tags ();
  oss.width (11); oss.precision (4); oss << scientific << right << epstot ();
  oss.width (11); oss.precision (4); oss << scientific << right << epsevn ();
  oss.width (11); oss.precision (4); oss << scientific << right << epsodd ();
  oss.width (11); oss.precision (4); oss << scientific << right << epsran ();
  oss << left << " " << IdLong;
  oss.width (11); oss.precision (6); oss << fixed << right << wavelength ();
  return oss.str();
}


//------------------------------------------------------------------------------
// getCentroidError () : Returns the estimated error in locating the centroid of
// a line based on the equation given by Whaling: dWN_{LC}=FWHM / (sqrt(N)*SNR).
// For the result to be meaningful, the peak amplitude must be normalised.
// Remember to convert the width from mK to K.
double Line::getCentroidError (double PointSpacing) {
  double PointsInFwhm = Width / (1000.0 * PointSpacing);
  return Width / (1000.0 * sqrt (PointsInFwhm) * Peak);
}


//------------------------------------------------------------------------------
// Line SET functions that require error checking. Simple set functions that can
// take any value from the input type are in line.h.
//
void Line::wavenumber (double NewWavenumber) {
  if (NewWavenumber < 0.0) {
    cout << "Error: Cannot set line wavenumber to " << NewWavenumber
     << ". The wavenumber must be positive." << endl;
    throw int (LINE_NEGATIVE_WAVENUMBER);
  }
  Wavenumber = NewWavenumber;
}

void Line::peak (double NewPeakHeight) {
  if (NewPeakHeight < 0.0) {
    cout << "Error: Cannot set line height to " << NewPeakHeight
     << ". The peak height must be positive." << endl;
    throw int (LINE_NEGATIVE_PEAK);
  }
  Peak = NewPeakHeight;
}

void Line::width (double NewWidth) {
  if (NewWidth < 0.0) {
    cout << "Error: Cannot set line width to " << NewWidth
     << ". The width must be positive." << endl;
    throw int (LINE_NEGATIVE_WIDTH);
  }
  Width = NewWidth;
}

void Line::eqwidth (double NewEqWidth) {
  if (NewEqWidth < 0.0) {
    cout << "Error: Cannot set line eqwidth to " << NewEqWidth
     << ". The eqwidth must be positive." << endl;
    throw int (LINE_NEGATIVE_EQWIDTH);
  }
  EqWidth = NewEqWidth;
}

void Line::wavelength (double NewWavelength) {
  if (NewWavelength < 0.0) {
    cout << "Error: Cannot set line wavelength to " << NewWavelength
     << ". The wavelength must be positive." << endl;
    throw int (LINE_NEGATIVE_WAVELENGTH);
  }
  Wavelength = NewWavelength;
}

