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
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include "listcal.h"
#include "lineio.cpp"

//------------------------------------------------------------------------------
// Default class constructor. Just set default variable values.
//
ListCal::ListCal () {
  WaveCorrection = DEF_WAVE_CORRECTION;
  Discriminator = DEF_DISCRIMINATOR;
  PeakAmpThreshold = DEF_PEAK_THRESHOLD;
  DiscardLimit = DEF_DISCARD_LIMIT;
  PointSpacing = DEF_POINT_SPACING;
  LineListName = "";
  StandardListName = "";
  DiffMean = 0.0;
  DiffStdDev = 0.0;
  DiffStdErr = 0.0;
  WaveCorrectionError = 0.0;
}


//------------------------------------------------------------------------------
// Public SET functions for private class variables. Used to set the internally 
// stored wavenumber correction factor, discriminator, or peak amplitude 
// threshold. In the latter two cases, the input must be checked to ensure it's
// a positive double. Return an LC_NEGATIVE_VALUE error they aren't.
//
void ListCal::setWaveCorrection (double NewWaveCorrection) {
  WaveCorrection = NewWaveCorrection;
}

void ListCal::setDiscriminator (double NewDiscriminator) {
  if (NewDiscriminator >= 0.0) {
    Discriminator = NewDiscriminator;
  } else {
    throw int(LC_NEGATIVE_VALUE);
  }
}

void ListCal::setPeakAmpThreshold (double NewThreshold) {
  if (NewThreshold >= 0.0) {
    PeakAmpThreshold = NewThreshold;
  } else {
    throw int(LC_NEGATIVE_VALUE);
  }
}

void ListCal::setDiscardLimit (double NewDiscardLimit) {
  if (NewDiscardLimit >= 0.0) {
    DiscardLimit = NewDiscardLimit;
  } else {
    throw int(LC_NEGATIVE_VALUE);
  }
}


void ListCal::setPointSpacing (double NewPointSpacing) {
  PointSpacing = NewPointSpacing;
}

//------------------------------------------------------------------------------
// Line list loading procedures. The actual file input is carried out in 
// readLineList(). The other two procedures, loadLineList and loadStandardList,
// act as wrappers so that the correct Line vector is passed to readLineList().
// These wrappers also store the list names in the class object.
//
void ListCal::loadLineList (const char *Filename) {
  readLineList (Filename, &FullLineList);
  LineListName = Filename;
}

void ListCal::loadStandardList (const char *Filename) {
  readLineList (Filename, &StandardList);
  StandardListName = Filename;
}


//------------------------------------------------------------------------------
// findCommonLines (bool) ; Scans through the uncalibrated and standard line
// lists, searching for lines common to both. When a common line is found, a new
// LinePair is created with pointers to its location in each of the two lists.
// This is then pushed onto the CommonLines class vector.
//
void ListCal::findCommonLines (bool Verbose) {
  unsigned int ListIndex = 0;
  unsigned int StdIndex = 0;
  double Difference;
  LinePair NewLinePair;

  if (FullLineList.size () == 0 || StandardList.size () == 0) {
    throw int (LC_NO_DATA);
  }
  
  if (Verbose) { 
    cout << "Lines common to both experimental and reference line lists." << endl;
    cout << "Index" << '\t' << "Wavenumber (K)" << '\t' << "Peak Height" << '\t' << "Ref Wavenumber (K)" << endl;
  }
  CommonLines.clear ();
  while (ListIndex < FullLineList.size () && StdIndex < StandardList.size ()) {
    Difference = 
      StandardList[StdIndex].wavenumber() - FullLineList[ListIndex].wavenumber();
    if (abs(Difference) < Discriminator) {
      // A common line has been found.
      NewLinePair.List = &FullLineList[ListIndex];
      NewLinePair.Standard = &StandardList[StdIndex];
      CommonLines.push_back (NewLinePair);
      if (Verbose) { 
        cout << NewLinePair.List -> line() << '\t' << NewLinePair.List -> wavenumber() << '\t' << '\t'
          << NewLinePair.List -> peak() << '\t' << '\t' << NewLinePair.Standard -> wavenumber() << endl;
      }
      StdIndex ++;
      ListIndex ++;
    } else if (StandardList[StdIndex].wavenumber() < FullLineList[ListIndex].wavenumber()) {
      // One of the standard lines is missing from the experiment
      if (Verbose) {
        cout << "Reference line " << StandardList[StdIndex].line() << " (" 
          << StandardList[StdIndex].wavenumber() << "K) is absent from the experiment." << endl;
      }
      StdIndex ++;
    } else {
      // One of the experiment lines is missing from the standard
      ListIndex ++;
    }
  }
  if (CommonLines.size () == 0) { 
    cout << "Error: No common lines were found between the experimental and reference line lists." << endl;
    throw int (LC_NO_OVERLAP); 
  }
}


//------------------------------------------------------------------------------
// findFittedLines (bool) : Takes the list of lines that are common to both
// input line lists and creates a sub-list from the lines of amplitude greater
// than or equal to PeakAmpThreshold. These are the lines that will subsequently
// be fitted, hence they are stored in FittedLines.
//
void ListCal::findFittedLines (bool Verbose) {
  if (CommonLines.size () == 0) { throw int (LC_NO_DATA); }
  
  if (Verbose) {
    cout << endl;
    cout << "------------------------------------------" << endl;
    cout << "Common lines of amplitude " << PeakAmpThreshold << " or greater." << endl;
    cout << "Index" << '\t' << "Wavenumber (K)" << '\t' << "Peak Height" << endl;
    cout << fixed;
  }
  FittedLines.clear ();
  for (unsigned int i = 0; i < CommonLines.size (); i ++) {
    if (CommonLines[i].List -> peak() >= PeakAmpThreshold) {
      FittedLines.push_back (&CommonLines [i]);
      if (Verbose) { 
        cout.precision (0); cout << CommonLines[i].List -> line() << '\t';
        cout.precision (6); cout << CommonLines[i].List -> wavenumber() << '\t';
        cout.precision (2); cout << CommonLines[i].List -> peak() << endl;
      }
    }
  }
  if (Verbose) 
    cout.unsetf (ios_base::fixed);
    cout.precision (6);
    cout << "------------------------------------------" << endl << endl;
}


//------------------------------------------------------------------------------
// removeBadLines (bool Verbose) : Scans through the list of FittedLines and
// discards all those which differ in wavenumber from the standard by more than
// DiscardLimit * DiffStdDev. This function should be called after 
// findCalibration() so as to check the quality of the Wavenumber correction.
//
int ListCal::removeBadLines (bool Verbose) {
  double Difference = 0.0;
  int LinesRemoved = 0;
  for (int i = (int)FittedLines.size () - 1; i >= 0; i --) {
    Difference = (FittedLines[i] -> List-> wavenumber() * (1.0 + WaveCorrection) -
      FittedLines[i] -> Standard -> wavenumber()) * LC_DATA_SCALE;
    Difference /= FittedLines[i]->Standard->wavenumber();
    if (abs(Difference) > abs(DiffMean) + DiscardLimit * DiffStdDev) {
      if (Verbose) {
        cout << "Removing line " << FittedLines[i] -> List -> line() 
          << ": " << FittedLines[i] -> List -> wavenumber()
          << "K\t(residual dSig/Sig = " << Difference / LC_DATA_SCALE << ", limit = +/-" 
          << (DiffMean + DiscardLimit * DiffStdDev) / LC_DATA_SCALE << ")" << endl;
      }
      DiscardedLines.push_back (FittedLines[i]);
      FittedLines.erase (FittedLines.begin() + i);
      LinesRemoved ++; 
    }
  }
  return LinesRemoved;
}


//------------------------------------------------------------------------------
// printLineList (vector <Line>) : Prints the input vector <Line> to the
// standard output.
//
int ListCal::printLineList (vector <Line> LineList) {
  cout << "Index" << '\t' << "Wavenumber (K)" << '\t' << "Peak Height" << endl;
  for (unsigned int i = 0; i < LineList.size (); i ++) {
    cout << scientific << LineList[i].line() 
      << '\t' << LineList[i].wavenumber() << '\t' << LineList[i].peak() << endl;
  }
  return LC_NO_ERROR;
}


//------------------------------------------------------------------------------
// findCorrection () : Uses a GSL Levenberg-Marquardt algorithm to fit the lines
// in FittedLines to the wavenumbers in the user-specified calibration standard.
// The result is the optimal wavenumber correction factor for the uncalibrated
// data, which is stored in the class variable WaveCorrection. Information about
// the fit residuals are saved by calling calcDiffStats().
//
void ListCal::findCorrection () {

  // Prepare the GSL Solver and associated objects. A non-linear solver is used,
  // the precise type of which is determined by SOLVER_TYPE, defined in 
  // MgstFcn.h. 
  const size_t NumParameters = 1;
  const size_t NumLines = FittedLines.size ();
  
  double GuessArr [NumParameters];
  for (unsigned int i = 0; i < NumParameters; i ++) { GuessArr[i] = WaveCorrection; }

  const gsl_multifit_fdfsolver_type *SolverType;
  gsl_multifit_fdfsolver *Solver;  
  gsl_multifit_function_fdf FitFunction;
  gsl_matrix *Covariance = gsl_matrix_alloc (NumParameters, NumParameters);
  gsl_vector_view VectorView = gsl_vector_view_array (GuessArr, NumParameters);

  FitFunction.f = &fitFn;
  FitFunction.df = &derivFn;
  FitFunction.fdf = &fitAndDerivFns;
  FitFunction.n = NumLines;
  FitFunction.p = NumParameters;
  FitFunction.params = &FittedLines;
 
  SolverType = SOLVER_TYPE;
  Solver = gsl_multifit_fdfsolver_alloc(SolverType, NumLines, NumParameters);
  gsl_multifit_fdfsolver_set (Solver, &FitFunction, &VectorView.vector);

  // Perform the fitting, one iteration at a time until one of the following
  // conditions is met: The absolute and relative changes in the fit parameters
  // become smaller than SOLVER_TOL, or the max number of allowed iterations,
  // SOLVER_MAX_ITERATIONS, is reached.
  unsigned int Iteration = 0;
  int Status;
  do {
    Iteration ++;
    Status = gsl_multifit_fdfsolver_iterate (Solver);
    if (Status) break;
    Status = gsl_multifit_test_delta (Solver->dx, Solver->x, SOLVER_TOL, SOLVER_TOL);
  } while (Status == GSL_CONTINUE && Iteration < SOLVER_MAX_ITERATIONS);

  // Output all the fit parameters with their associated error.
  gsl_multifit_covar (Solver -> J, 0.0, Covariance);
#define FIT(i) gsl_vector_get (Solver -> x, i)
#define ERR(i) sqrt (gsl_matrix_get (Covariance, i, i))

  double chi = gsl_blas_dnrm2 (Solver -> f);
  double dof = NumLines - double(NumParameters);
  double c = chi / sqrt (dof);
  
  cout << "Correction factor: " << FIT(0) << " +/- " << c*ERR(0) << " ("
    << "reduced chi^2 = " << pow(chi, 2) / dof << ", "
    << "lines fitted = " << NumLines << ", c = " << c << ")" << endl;

  // Apply the wavenumber correction to all the lines loaded from the
  // uncalibrated spectrum
  WaveCorrection = FIT(0);
  WaveCorrectionError = c*ERR(0);
  calcDiffStats ();
  cout << "dSig/Sig Mean Residual: " << DiffMean / LC_DATA_SCALE 
    << ", StdDev: " << DiffStdDev / LC_DATA_SCALE
    << ", StdErr: " << DiffStdErr / LC_DATA_SCALE << endl;

  // Clean up the memory and exit
  gsl_multifit_fdfsolver_free (Solver);
  gsl_matrix_free (Covariance);
}


//------------------------------------------------------------------------------
// calcDiffStats () : Calculates the mean wavenumber, and its standard deviation
// and standard error, after the application of the wavenumber correction factor
// stored in 'WaveCorrection'. These are stored in the class variables DiffMean,
// DiffStdDev, and DiffStdErr, respectively. calcDiffStats is called at the end
// of findCorrection().
//
void ListCal::calcDiffStats () {
  DiffMean = 0.0;
  DiffStdDev = 0.0;
  double Difference = 0.0;
  for (unsigned int i = 0; i < FittedLines.size (); i ++) {
    Difference = (FittedLines[i] -> List -> wavenumber() * (1.0 + WaveCorrection)
      - FittedLines[i] -> Standard -> wavenumber()) * LC_DATA_SCALE;
    Difference /= FittedLines[i]->Standard->wavenumber();
    DiffMean += Difference;
  }
  DiffMean /= FittedLines.size ();

  for (unsigned int i = 0; i < FittedLines.size (); i ++) {
    Difference = (FittedLines[i] -> List -> wavenumber() * (1.0 + WaveCorrection)
      - FittedLines[i] -> Standard -> wavenumber()) * LC_DATA_SCALE;
    Difference /= FittedLines[i]->Standard->wavenumber();
    DiffStdDev += pow (Difference - DiffMean, 2);
  }
  DiffStdDev = sqrt (DiffStdDev / FittedLines.size ());
  DiffStdErr = DiffStdDev / sqrt (FittedLines.size());
}


//------------------------------------------------------------------------------
// plotDifferences () : Uses Gnuplot to plot the FittedLines and DiscardedLines.
// Output is first to the screen, and then to the postscipt file Calibration.ps.
//
void ListCal::plotDifferences () {
  FILE *gpPipe, *tempFittedFile, *tempDiscardedFile;
  string tempFitted, tempDiscarded;
  double x, y;
  tempFitted = "tempFitted";
  tempDiscarded = "tempDiscarded";
  gpPipe = popen("/usr/bin/gnuplot","w");
  if (gpPipe) {
    // Prepare the Gnuplot graph and label the axes
    fprintf(gpPipe, "set termoption enhanced\n");
    fprintf(gpPipe, "set xlabel \"Line Wavenumber / cm^{-1}\" \n");
    fprintf(gpPipe, "set ylabel \"dSig/Sig x %1.0e\" \n", LC_DATA_SCALE);
//    fprintf(gpPipe, "set title \"Wavenumber correction factors for each line.\\nOptimal correction = %1.3e \u00B1 %1.3e\"\n",
//      WaveCorrection, WaveCorrectionError);

    // Find the optimal y axis scale and fix the y axis to this scale once done.
    // This must be done before finding the optimal x-axis scale as the dummy
    // points for the std-dev markers will lie outside the optimal x range.
    fprintf(gpPipe, "set yrange [] writeback\n");
    fprintf(gpPipe, "plot \"%s\" lt rgb \"#0000FF\" t \"Fitted Lines (%i)\", \
      \"%s\" lt rgb \"#FF0000\" t \"Discarded Lines (%i)\", \
      \"\" u (48480.):(%1.3e):(0):(100000) w xerror notitle ps 0 lt 0 lw 0.5 lc rgb \"#909090\", \
      \"\" u (48480.):(%1.3e):(0):(100000) w xerror notitle ps 0 lt 0 lw 0.5 lc rgb \"#909090\"\n", 
      tempFitted.c_str(), FittedLines.size(), tempDiscarded.c_str(), 
      DiscardedLines.size(), DiffStdDev * DiscardLimit - WaveCorrection * LC_DATA_SCALE, 
      -1.0 * DiffStdDev * DiscardLimit - WaveCorrection * LC_DATA_SCALE);
    fprintf(gpPipe, "set yrange restore\n");

    // Find the optimal x axis scale by plotting the Fitted and Discarded data
    // points only. Fix the x axis to this scale once done.
    fprintf(gpPipe, "set xrange [] writeback\n");
    if (DiscardedLines.size () > 0) {
      fprintf(gpPipe, "plot \"%s\" lt rgb \"#0000FF\" t \"Fitted Lines (%i)\", \
        \"%s\" lt rgb \"#FF0000\" t \"Discarded Lines (%i)\"\n",
        tempFitted.c_str(), FittedLines.size(), tempDiscarded.c_str(), 
        DiscardedLines.size());
    } else {
      fprintf(gpPipe, "plot \"%s\" lt rgb \"#0000FF\" t \"Fitted Lines (%i)\"\n",
        tempFitted.c_str(), FittedLines.size());
    }
    fprintf(gpPipe, "set xrange restore\n");
    fprintf(gpPipe, "set style line 1 lt 2 lw 1 lc rgb \"#000000\"\n");
    fprintf(gpPipe, "set key box linestyle 1\n");
    
    // Finally, plot the real data again, but also add two dummy points with
    // huge x errors to denote the cutoff lines between Fitted and Discarded
    // lines.
    fprintf(gpPipe, "plot \"%s\" lt rgb \"#0000FF\" t \"Fitted Lines (%i)\", \
      \"%s\" lt rgb \"#FF0000\" t \"Discarded Lines (%i)\", \
      \"\" u (48480.):(%1.3e):(0):(100000) w xerror notitle ps 0 lt 0 lw 0.5 lc rgb \"#909090\", \
      \"\" u (48480.):(%1.3e):(0):(100000) w xerror notitle ps 0 lt 0 lw 0.5 lc rgb \"#909090\"\n", 
      tempFitted.c_str(), FittedLines.size(), tempDiscarded.c_str(), 
      DiscardedLines.size(), DiffStdDev * DiscardLimit - WaveCorrection * LC_DATA_SCALE, 
      -1.0 * DiffStdDev * DiscardLimit - WaveCorrection * LC_DATA_SCALE);
    fflush(gpPipe);
    
    // Now generate two temporary files containing the data points for Gnuplot
    // to plot - one for the FittedLines, and a second for the DiscardedLines.
    // If the temporary files cannot be created, throw an error to the user.
    tempFittedFile = fopen(tempFitted.c_str(),"w");
    if (!tempFittedFile) {
      cout << "Error: Cannot create Gnuplot temporary file " << tempFitted 
      << ".\nCheck you have write access to the current directory. Plotting ABORTED." << endl;
      throw int (LC_FILE_OPEN_ERROR);
    }
    for (unsigned i = 0; i < FittedLines.size (); i ++) {
      x = FittedLines[i] -> Standard -> wavenumber();
      y = (FittedLines[i] -> List -> wavenumber() /** (1.0 + WaveCorrection)*/
        - FittedLines[i] -> Standard -> wavenumber()) * LC_DATA_SCALE;
      y /= FittedLines[i]->Standard->wavenumber();
      fprintf(tempFittedFile,"%1.12e %1.12e\n", x, y);
    }
    tempDiscardedFile = fopen(tempDiscarded.c_str(),"w");
    if (!tempDiscardedFile) {
      cout << "Error: Cannot create Gnuplot temporary file " << tempDiscarded 
      << ".\nCheck you have write access to the current directory. Plotting ABORTED." << endl;
      throw int (LC_FILE_OPEN_ERROR);
    }
    if (DiscardedLines.size () > 0) {
      for (unsigned i = 0; i < DiscardedLines.size (); i ++) {
        x = DiscardedLines[i] -> Standard -> wavenumber();
        y = (DiscardedLines[i] -> List -> wavenumber() /** (1.0 + WaveCorrection)*/
          - DiscardedLines[i] -> Standard -> wavenumber()) * LC_DATA_SCALE;
        y /= DiscardedLines[i]->Standard->wavenumber();
        fprintf(tempDiscardedFile,"%1.12e %1.12e\n", x, y);
      }
    } else {
      fprintf(tempDiscardedFile, "0.0 0.0\n");
    }
    fclose(tempFittedFile);
    fclose(tempDiscardedFile);

    // Wait for the user to finish with the on-screen graph, then save the plot
    // to 'Calibration.ps', discard the temp files, and shut down Gnuplot.
    printf("press enter to continue...");          
    getchar();

    fprintf(gpPipe, "set size 0.9, 0.5\n");
    fprintf(gpPipe, "set terminal postscript portrait enhanced color solid lw 1 \"Times\" 11\n");
    fprintf(gpPipe, "set output \"Calibration.ps\"\n");
    fprintf(gpPipe, "replot\n");
    fprintf(gpPipe,"exit \n");
    pclose(gpPipe);
    
    remove(tempFitted.c_str());
    remove(tempDiscarded.c_str());
  } else {
    printf("gnuplot not found...");
    throw int (LC_PLOT_NO_GNUPLOT);
  }     
}  


//------------------------------------------------------------------------------
// saveLineList (const char *Filename) : Produces a calibrated line list in the
// XGremlin writelines format and a calibration results files. The latter
// contains all the calibration settings and then lists the calibrated wave-
// numbers with all the associated error components.
//
int ListCal::saveLineList (const char *Filename) {
  if (FullLineList.size () == 0) { return LC_NO_DATA; }
  ostringstream oss;
  oss.str ("");
  oss << Filename << ".cln";

  // First save the calibrated line list to an XGremlin writelines formatted
  // file. Work on a copy of the FullLineList so as not to modify its wavcorr.
  vector <Line> SavedLines;
  for (unsigned int i = 0; i < FullLineList.size (); i ++) {
    SavedLines.push_back (FullLineList[i]);
    SavedLines[i].wavCorr (getWaveCorrection ());
  }
  writeLines (SavedLines, oss.str().c_str());

  // Now prepare to save the calibration results themselves.
  oss.str ("");
  oss << Filename << ".cal";
  double FullErrorStdDev, FullErrorBrault;
  FILE *LineFile;
  LineFile = fopen (oss.str().c_str(), "w");
  if (! LineFile) {
    return LC_FILE_OPEN_ERROR;
  }
  
  // Write the calibration output header
  fprintf (LineFile, "# Fitted lines from %s against standards in %s\n", 
    LineListName.c_str(), StandardListName.c_str());
  fprintf (LineFile, "# Discriminator / K : %f\n", Discriminator);
  fprintf (LineFile, "# Peak Amp Threshold: %f\n", PeakAmpThreshold);
  fprintf (LineFile, "# Discard Limit     : %f\n", DiscardLimit); 
  fprintf (LineFile, "# Point Spacing     : %f\n#\n", PointSpacing);
  fprintf (LineFile, "# Correction factor : %e +/- %e\n", WaveCorrection, WaveCorrectionError);
  fprintf (LineFile, "# Mean fit residual : %e\n", DiffMean / LC_DATA_SCALE);
  fprintf (LineFile, "# Residual std dev  : %e\n#\n", DiffStdDev / LC_DATA_SCALE);
  fprintf (LineFile, "#  n  Wavenumber    Scale Error   StdDev Error  Brault Error  Full Error\n");

  // Output the calibrated wavenumber for each line, the individual error
  // components, and the total wavenumber error. All units are cm^-1.
  FullErrorStdDev = sqrt (pow (getWaveCorrectionError (), 2) 
    + pow (DiffStdDev / LC_DATA_SCALE, 2));
  for (unsigned int i = 0; i < SavedLines.size (); i ++) {
    FullErrorBrault = sqrt (pow (SavedLines[i].wavenumber() * getWaveCorrectionError (), 2) 
      + pow (SavedLines[i].getCentroidError (PointSpacing), 2));
    fprintf (LineFile, "%4d  %11.6f  %11.6e  %11.6e  %11.6e  %11.6e\n", 
      SavedLines[i].line(),
      SavedLines[i].wavenumber(),
      SavedLines[i].wavenumber() * getWaveCorrectionError (),
      SavedLines[i].wavenumber() * DiffStdDev / LC_DATA_SCALE,
      SavedLines[i].getCentroidError (PointSpacing),
      max (SavedLines[i].wavenumber() * FullErrorStdDev, FullErrorBrault));
  }
  fclose (LineFile);
  return LC_NO_ERROR;
}


//------------------------------------------------------------------------------
// GSL Fitting functions
//
// fitFn (const gsl_vector *, void *, gsl_vector) : Calculates the difference
// between the uncalibrated and standard line lists after an offset, Step, has
// been applied to the uncalibrated list. 
//
int fitFn (const gsl_vector *x, void *data, gsl_vector *f) {
  double Step = gsl_vector_get (x, 0);
  vector <LinePair*> *FittedLines = (vector <LinePair*> *) data;  
  for (unsigned int i = 0; i < FittedLines->size (); i ++) {
    gsl_vector_set (f, i, (FittedLines->at(i)->List->wavenumber() * (1.0 + Step) 
      - FittedLines->at(i)->Standard->wavenumber()) * LC_DATA_SCALE 
      / FittedLines->at(i)->Standard->wavenumber());
  }
  return GSL_SUCCESS;
}

//
// derivFn (const gsl_vector *, void *, gsl_vcector *) : In principle, this
// function should calculate the derivatives of the list differences with
// respect to changes in each fit parameter and return a Jacobian matrix.
// However, since the wavenumber calibration is linear, the derivatives will all
// be zero.
//
int derivFn (const gsl_vector *x, void *data, gsl_matrix *J) {  
  // First initalise the Jacobian (J) so all elements are zero
  for (unsigned int i = 0; i < J -> size1; i ++) {
    for (unsigned int j = 0; j < J -> size2; j ++) {
      gsl_matrix_set (J, i, j, LC_DATA_SCALE);
    }
  }
  return GSL_SUCCESS;  
}

//
// fitAndDerivFns (const gsl_vector*, void*, gsl_vector*, gsl_matrix*) : Calls
// fitFn (...) and then derivFn (...) in turn.
//
int fitAndDerivFns (const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J) {
  int ErrCode;
  if ((ErrCode = fitFn (x, data, f)) != LC_NO_ERROR) { return ErrCode; }
  if ((ErrCode = derivFn (x, data, J)) != LC_NO_ERROR) { return ErrCode; }
  return GSL_SUCCESS;
}
