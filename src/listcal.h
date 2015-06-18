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
#ifndef LIST_CAL_H
#define LIST_CAL_H

#include <vector>
#include <string>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_deriv.h>
#include "ErrDefs.h"
#include "line.h"

// Default spectrum processing parameters
#define DEF_WAVE_CORRECTION 0.0  /* wavenumbers                               */
#define DEF_DISCRIMINATOR   0.1  /* wavenumbers                               */
#define DEF_PEAK_THRESHOLD  50.0 /* eqvalent to SNR if spectrum normalised    */
#define DEF_DISCARD_LIMIT   2.0  /* times the residual difference std dev     */

// Output parameters
#define LC_DATA_SCALE   1.0e6    /* scale the output amplitude by this factor */

// GSL Fitting parameters
#define SOLVER_TYPE gsl_multifit_fdfsolver_lmsder
#define SOLVER_TOL 1.0e-12
#define SOLVER_MAX_ITERATIONS 500
#define SOLVER_DERIV_STEP 0.1

using namespace::std;

// Define a structure in which a matched pair of lines can be stored. One of
// these will come from the uncalibrated list, the other from the calibration
// standard.
typedef struct td_LinePair {
  Line *List;
  Line *Standard;
} LinePair;

// Create the ListCal class
class ListCal {
public:
  ListCal ();
  ~ListCal () { /* Do nothing */ };
  
  // File I/O functions
  void loadStandardList (const char *Filename);
  void loadLineList (const char *Filename);
  int saveLineList (const char *Filename);

  // Class variable GET and SET functions
  void setWaveCorrection (double NewWaveCorr);
  void setDiscriminator (double NewDiscriminator);
  void setPeakAmpThreshold (double NewThreshold);
  void setDiscardLimit (double NewDiscardLimit);
  void setPointSpacing (double NewPointSpacing);
  double getWaveCorrection () { return WaveCorrection; }
  double getWaveCorrectionError () { return WaveCorrectionError; }
  double getDiscriminator () { return Discriminator; }
  double getPeakAmpThreshold () { return PeakAmpThreshold; }
  double getDiscardLimit () { return DiscardLimit; }
  double getDiffMean () { return DiffMean; }
  double getDiffStdDev () { return DiffStdDev; }
  double getDiffStdErr () { return DiffStdErr; }
  
  // Calibration and list manipulation functions
  void findCorrection ();
  void findCommonLines (bool Verbose = false);
  void findFittedLines (bool Verbose = false);
  int removeBadLines (bool Verbose = false);
  void calcDiffStats ();
  
  // Output functions
  int printLineList (vector <Line> LineList);
  void plotDifferences ();

private:
  vector <Line> FullLineList;   // All the lines from the uncalibrated line list
  vector <Line> StandardList;   // All the lines from the standard line list
  vector <LinePair> CommonLines;  // Lines from FullLineList that exist in StandardList
  vector <LinePair*> FittedLines; // Lines from CommonLines to be fitted (weak lines omitted)
  vector <LinePair*> DiscardedLines; // Lines removed from FittedLines
  double WaveCorrection;
  double WaveCorrectionError;
  double Discriminator;
  double PeakAmpThreshold;
  double DiscardLimit;
  string LineListName;
  string StandardListName;
  double DiffMean;
  double DiffStdDev;
  double DiffStdErr;
  double PointSpacing;
};

int fitFn (const gsl_vector *x, void *data, gsl_vector *f);
int derivFn (const gsl_vector *x, void *data, gsl_matrix *J);
int fitAndDerivFns (const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J);
double dSigma (double Step, void *data);


#endif // LIST_CAL_H
