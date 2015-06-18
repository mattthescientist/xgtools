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
// ftscalibrate
//
// Calculates the optimal wavenumber scaling factor, epsilon, for a list of 
// uncalibrated lines by comparing their wavenumbers with those in a calibrated
// standard list. Both of these line lists must be in an XGremlin 'writelines' 
// format. The scaling factor is obtained by using a least-squares algorithm to
// fit epsilon and so minimise the difference in  wavenumbers between the two 
// lists. The calibrated wavenumbers are given by:
//
// \sigma_{cal} = \sigma_{measured} * ( 1 + \epsilon )
//
// This program is essentially just a re-implementation of the older DIFFLIST
// code with the addition of a Levenberg-Marquardt fitting algorithm from the
// GNU Scientific Library.
//
// In order to be included in the fitting, each line in the uncalibrated list
// must have a S/N ratio of at least SNR_{min}, and be no further than D cm^-1
// from the corresponding standard line. SNR_{min} and D have default values,
// found in at the top of listcal.h, but can also be changed by the user from
// the command line.
//
// Once the optimal scaling factor has been found for the subset of suitable
// lines in the uncalibrated list, the fit residuals and their standard 
// deviation are calculated. If any single line gives a scaling factor that lies
// further than N standard deviations from the mean scaling factor, it is
// removed, and the fit repeated. Again, a default value of N is given at the
// top of listcal.h, but may be changed by the user from the command line.
//
// When no further lines need rejecting, the fit is complete. The results are
// printed to the standard output, and two results files generated. The first 
// is a copy of the original uncalibrated list, but with the wavenumber scaling
// factor applied to each line. The second contains a record of the calibration
// settings, the key results, and the calibration errors for each individual
// line. 
//
// Following the approach of Whaling et al., J. Quant.  Spectrosc. Radiat. 
// Transfer 53 pp. 1 (1995), there are two error components: the error in 
// \epsilon, as returned by the fitting algorithm, and the error in determining
// the line centre position. The latter is calculated in two ways. Firstly, by 
// using the equation given by Brault in Mikrochim. Acta (Wien) 3 pp.215 (1987),
// and secondly, by using the standard deviation in the fit residuals.
//
#include "listcal.h"
#include <iostream>
#include <string>
#include <sstream>

#define LC_VERSION "1.0"

// Definitions for command line arguments
#define REQ_NUM_ARGS_1 8    /* 6 arguments are needed (plus 1 for the binary) */
#define REQ_NUM_ARGS_2 4    /* 3 arguments are needed (plus 1 for the binary) */
#define ARG_LIST_FILE 1     /* 1st arg is for the list to be calibrated       */
#define ARG_STD_FILE 2      /* 2nd arg is for the calibration standard list   */
#define ARG_DISCRIMINATOR 3 /* 3rd arg is the discriminator in units of K     */
#define ARG_THRESHOLD 4     /* 4rd arg is the minimum allowed line S/N ratio  */
#define ARG_DISCARD_LIMIT 5 /* 5rd arg is the max allowed std devs from mean  */
#define ARG_POINT_SPACING 6 /* 6th arg is the spacing between data points in K*/
#define ARG_OUT_FILE_1 7    /* 7th arg is the output for the calibrated list  */
#define ARG_OUT_FILE_2 3    /* as above, but for the second argument form     */

// Error codes
#define LC_NO_ERROR     0
#define LC_SYNTAX_ERROR 1

//==============================================================================
// main
//
int main (int argc, char *argv[]) {
  ListCal ListFitter;
  string OutputName;
  ostringstream oss;
  
  cout << "FTS Line List Calibrator v" << LC_VERSION << " (built " << __DATE__ << ")" << endl << endl;

  
  // Check the command line syntax. Output a help message if it's incorrect
  // and abort, returning a non-zero error code
  if (argc != REQ_NUM_ARGS_1 && argc != REQ_NUM_ARGS_2) {
    cout << "ftscalibrate: Calibrates the wavenumbers of lines saved in an XGremlin ASCII (writelines) line list" << endl;
    cout << "---------------------------------------------------------------------------------------------------" << endl;
    cout << "Syntax: ftscalibrate <list> <standards> [<discriminator> <min S/N> <discard limit> <spacing>] <output file>" << endl << endl;
    cout << "<list>         : An XGremlin ASCII line list containing the lines to be calibrated (written with writelines)." << endl;
    cout << "<standards>    : An XGremlin ASCII line list to act as the calibration standard (also in writelines format)." << endl;
    cout << "<discriminator>: The maximum allowed wavenumber difference (in cm^-1) when searching for common lines in" << endl;
    cout << "                 <list> and <standards>. Any line without a partner within this limit will be ignored." << endl;
    cout << "<min S/N>      : The minimum allowed S/N ratio for any line used in the calibration." << endl;
    cout << "<discard limit>: All lines with dSig/Sig greater than <discard limit> times the std. dev. in the mean dSig/Sig" << endl;
    cout << "                 will be discarded from the calibration." << endl;
    cout << "<spacing>      : The separation, in cm^-1, between data points in the spectrum." << endl;
    cout << "<output file>  : A file name where the calibrated <list> will be saved." << endl;
    cout << endl;
    return LC_SYNTAX_ERROR;
  }
  
  // Process the command line options
  if (argc == REQ_NUM_ARGS_1) {
    ListFitter.setDiscriminator (atof (argv[ARG_DISCRIMINATOR]));
    ListFitter.setPeakAmpThreshold (atof (argv[ARG_THRESHOLD]));
    ListFitter.setDiscardLimit (atof (argv[ARG_DISCARD_LIMIT]));
    ListFitter.setPointSpacing (atof (argv[ARG_POINT_SPACING]));
    OutputName = argv[ARG_OUT_FILE_1];
  } else {
    OutputName = argv[ARG_OUT_FILE_2];
  }
  
  // Output the calibration parameters before continuing  
  cout << "Line list to be calibrated: " << argv[ARG_LIST_FILE] << endl;
  cout << "Calibration standard list : " << argv[ARG_STD_FILE] << endl;
  cout << "Discriminator             : " << ListFitter.getDiscriminator() << endl;
  cout << "Minimum line amplitude    : " << ListFitter.getPeakAmpThreshold() << endl;
  cout << "Discard beyond x Std Dev  : " << ListFitter.getDiscardLimit() << endl;  
  cout << "Calibrated list saved to  : " << OutputName << endl;

  // Prepare the calibration. Pass the list files to the ListCal object
  // and find all the lines common to both lists. From these common lines,
  // discard any of amplitude less than DEF_PEAK_THRESHOLD
  cout << endl << "Starting calibration..." << endl;
  try {
    ListFitter.loadLineList (argv[ARG_LIST_FILE]);
    ListFitter.loadStandardList (argv[ARG_STD_FILE]);
    ListFitter.findCommonLines (false);
    ListFitter.findFittedLines (true);
  } catch (int Err) {
    return Err;
  }

  // Now fit the uncalibrated list to the standard lines. If any lines remain
  // beyond DEF_DISCARD_LIMIT standard deviations of the mean after fitting,
  // remove them and refine the fit. Stop when all the fitted lines are within
  // DEF_DISCARD_LIMIT standard deviations of the mean.
  unsigned int NumLinesRemoved;
  do {
  
    // Do the fitting here
    ListFitter.findCorrection ();
    
    // Remove any bad lines and output the results to the user
    if ((NumLinesRemoved = ListFitter.removeBadLines (true))) {
      cout << "Removed " << NumLinesRemoved << " bad line" << flush;
      if (NumLinesRemoved > 1) cout << "s" << flush;
      cout << " from the fit." << endl;
      cout << endl << "Refining the calibration..." << endl;
    } else {
      cout << "All lines are within " << DEF_DISCARD_LIMIT << " standard deviations of the mean." << endl;
      cout << endl << "Calibration complete." << endl;
    }
    
    // Continue until no lines are removed by ListFitter.removeBadLines()
  } while (NumLinesRemoved);
  
  // The calibration is now complete. Output the results to the user
  cout << endl;
  cout << "Residual Mean dSig/Sig   : " << ListFitter.getDiffMean () / LC_DATA_SCALE << endl;
  cout << "Residual StdDev dSig/Sig : " << ListFitter.getDiffStdDev () / LC_DATA_SCALE << endl;
  cout << "Residual StdErr dSig/Sig : " << ListFitter.getDiffStdErr () / LC_DATA_SCALE << endl;
  cout << "--------------------------------------------------" << endl;
  cout << "Optimal dSig/Sig : " << ListFitter.getWaveCorrection () << " +/- "
    << ListFitter.getWaveCorrectionError() << endl;
  cout << "--------------------------------------------------" << endl;
  cout << endl;
  ListFitter.saveLineList (OutputName.c_str());
  
  // Finally, plot the results with GNUPlot
  ListFitter.plotDifferences ();
  return LC_NO_ERROR;
}
