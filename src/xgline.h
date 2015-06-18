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
//==============================================================================
// XgLine class (xgline.h)
//==============================================================================
// Describes a spectral emission line. The class properties match all those 
// listed in an XGremlin 'writelines' output file. A new line can be created by
// passing a full line string from an XGremlin 'writelines' file to the 
// createLine() function. Individual properties may also be changed with their
// separate GET functions.
//
// The Line class can also handle wavenumber correction factors. By default, it
// will be assumed that no correction is applied to the line. If, however, one
// is set with wavCorr(double), it will be applied when getting the wavenumber,
// width, or wavelength of the line. The values then returned from wavenumber(),
// width() and wavelength() are the corrected values.
//
// If a correction factor is set prior to calling createLine(), it will be
// assumed that this factor has ALREADY BEEN APPLIED to the line being read.
// createLine() will then remove the factor such that the uncorrected properties
// are saved in the class variables. This allows the GET functions to return the
// correct values, but also permits the correction factor to be changed after
// creating the line, if so desired.
//
// At any time, the line properties may be extracted in an XGremlin friendly
// format. getLineString() returns all the line properties in an XGremlin
// 'writelines' string. getLineSynString() returns them in a 'syn' format for
// use with the 'readlines' command. The line properties may also be printed to
// a specified stream (or standard output by default) with the print() function.
//
// Finally, getCentroidError(double) can be used to estimate the error in 
// determining the line centroid, as calculated from the equation given by
// Brault. This equation requires the line width and S/N ratio, and the spacing
// between individual data points. This last term isn't a line property given by
// XGremlin, and so must be passed in at arg1. A default spacing is given by
// DEF_POINT_SPACING.
//
#ifndef XG_LINE_H
#define XG_LINE_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "ErrDefs.h"

// The default spacing between spectru data points, in cm^-1. This is used by
// getCentroidError() when calculating the Brault line centre error.
#define DEF_POINT_SPACING   0.03

// Define the width of the line identification field in an XGremlin 'writelines'
// line list. This is needed as the field may contain several words that could
// be interpreted as multiple fields in a simple istringstream input operation.
#define LINE_ID_STRING_LEN 30
#define LINE_TAG_STRING_LEN 4

using namespace::std;

class XgLine {
  public:
  
    // Constructors and destructor
    XgLine ();                
    XgLine (string LineData, double NewWaveCorr = 0.0, double NewAirCorr = 0.0, 
      double NewIntCal = 0.0);
    ~XgLine () {}
  
    // GET functions to access line properties. Apply the wavenumber correction
    // factor to any properties that require it.
    int line () { return Index; }
    int itn () { return Itn; }
    int h () { return H; }
    double wavenumber () { return Wavenumber * (1.0 + WavenumberCorrection); }
    double peak () { return Peak; }
    double snr ();
    double width () { return Width * (1.0 + WavenumberCorrection); }
    double dmp () { return Dmp; }
    double eqwidth () { return EqWidth; }
    double epstot () { return EpsTot; }
    double epsevn () { return EpsEvn; }
    double epsodd () { return EpsOdd; }
    double epsran () { return EpsRan; }
    double spare () { return Spare; }
    double wavelength () { return 1.0e7 / wavenumber (); }
    double airWavelength ();
    string tags () { return Tags; }
    string id () { return Identification; }
    double wavCorr () { return WavenumberCorrection; }
    double airCorrection () { return AirCorrection; }
    double intensityCalibration () { return IntensityCalibration; }
    string name () { return SourceFilename; }
    
    // SET functions to modify line properties
    void line (int NewIndex) { Index = NewIndex; }
    void itn (int NewItn) { Itn = NewItn; }
    void h (int NewH) { H = NewH; }
    void wavenumber (double NewWavenumber) throw (Error);
    void peak (double NewPeakHeight) throw (Error);
    void snr (double NewSNR) throw (Error);
    void width (double NewWidth) throw (Error);
    void dmp (double NewDamping) { Dmp = NewDamping; }
    void eqwidth (double NewEqWidth) throw (Error);
    void epstot (double NewEpstot) { EpsTot = NewEpstot; }
    void epsevn (double NewEpsevn) { EpsEvn = NewEpsevn; }
    void epsodd (double NewEpsodd) { EpsOdd = NewEpsodd; } 
    void epsran (double NewEpsran) { EpsRan = NewEpsran; }
    void spare (double NewSpare) { Spare = NewSpare; }
    void wavelength (double NewWavelength) throw (Error);
    void tags (string NewTags) { Tags = NewTags; }
    void id (string NewId) { Identification = NewId; }
    void wavCorr (double NewCorr){ WavenumberCorrection = NewCorr;}
    void airCorrection (double NewCorrection) { AirCorrection = NewCorrection; }
    void intensityCalibration (double NewCal) { IntensityCalibration = NewCal; }
    void name (string NewName) { SourceFilename = NewName; }

    // Allow the user to create a Line from a string read from an XGremlin
    // "writelines" output file.
    void createLine (string LineString) throw (const char*);
    
    // An = operator to copy the contents of one line to another.
    void operator= (XgLine Operator);
    
    // Output functions. print (...) writes the line properties to a specified
    // stream or to std::cout by default. getLineSynString() and getLineString()
    // return the line properties in a format matching XGremlin's 'syn' and 
    // 'old' formats. See 'readlines' in the XGremlin manual for more info.
    void print (ostream& Output = std::cout);
    string getLineSynString ();
    string getLineString ();
    void save (ofstream& BinOut);
    void load (ifstream& BinIn);
    
    // Calculates the error in the line centroid position using the Brault eqn.
    double getCentroidError (double PointsInFwhm = DEF_POINT_SPACING);
   
  private:
    // Line properties. Follows the naming convention used in the XGremlin
    // "writelines" output files.
    int Index, Itn, H;
    double Wavenumber, Peak, Width, Dmp, EqWidth, EpsTot, 
      EpsEvn, EpsOdd, EpsRan, Wavelength, SNR;
    string Tags, Identification, SourceFilename;
    double Spare;
    bool CustomSNR;
    
    // Header parameters from an XGremlin "writelines" file
    double WavenumberCorrection;
    double AirCorrection;
    double IntensityCalibration;
    
    // If an error is thrown while reading an XGremlin writelines file, check
    // the nature of the error for known problems. If these can be handled, fix
    // the problem. If not, continue to throw the error.
    void checkInput (istringstream &iss, const char* Err) throw (const char *);
};
    
#endif // XG_LINE_H
