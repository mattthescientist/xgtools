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
// ftsresponse : A FTS Response Function Generator
//
// Based upon the GSL spline fitting example in section 39.7 of the GSL manual.
// Make sure the GNU Scientific Library (GSL) development package is installed
// on your system, then compile this code using the following command:
//
// g++ ftsresponse.cpp -lgsl -lgslcblas -o ftsresponse
//
// In the output file, column 1 is the wavenumber, column 2 the response 
// function, and column 3 the log of the relative spectral radiance.
//
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics.h>
#include <vector>
#include <cctype>

using namespace::std;

// ftsresponse version
#define VERSION "1.0"

// Default number of fit coefficients
#define DEFAULT_NUM_COEFFS  40

// Definitions for command line parameters
#define REQUIRED_NUM_ARGS_MODE1 4
#define REQUIRED_NUM_ARGS_MODE2 5
#define ARG_SPECTRUM 1
#define ARG_CALIBRATION 2
#define ARG_OUTPUT 3
#define ARG_COEFFS 4


//------------------------------------------------------------------------------
// showHelp () : Prints syntax help message to the standard output.
//
void showHelp () {
  cout << endl;
  cout << "ftsresponse : Calculates a spectrometer response function" << endl;
  cout << "---------------------------------------------------------" << endl;
  cout << "Syntax : ftsresponse <spectrum> <cal> <output> [<coeffs>]" << endl << endl;
  cout << "<spectrum>  : The standard lamp spectrum (in cm^{-1}) written with writeasc in XGremlin." << endl;
  cout << "<cal>       : The standard lamp spectral radiance (in nm) supplied by e.g. NPL." << endl;
  cout << "<output>    : The spectrometer response function will be saved here." << endl;
  cout << "<coeffs>    : Number of spline fit coefficients. Larger values will reduce smoothing, allowing" << endl;
  cout << "              higher frequencies to be fitted, but could cause fit instabilities if too high" << endl;
  cout << "              (default " << DEFAULT_NUM_COEFFS << ")." << endl << endl;
}


//------------------------------------------------------------------------------
// is_numeric (string) : Determines whether or not argument 1 is a number.
//
bool is_numeric (string a) {
  for (unsigned int i = 0; i < a.length (); i ++) {
    if (!isalnum(a[i]) || isalpha(a[i])) {
      return false;
    }
  }
  return true;
}


//------------------------------------------------------------------------------
// Main program
//
int main (int argc, char *argv[]) {

  // Check the user's command line input
  if (argc != REQUIRED_NUM_ARGS_MODE1 && argc != REQUIRED_NUM_ARGS_MODE2) {
    showHelp (); 
    return 1;
  }
  
  // Print introductory message to the standard output
  cout << "FTS Response Function Generator v" << VERSION 
    << " (built " << __DATE__ << ")" << endl;
  cout << "--------------------------------------------------------" << endl;
  cout << "Spectrum file : " << argv [ARG_SPECTRUM] << endl;
  cout << "Lamp radiance : " << argv [ARG_CALIBRATION] << endl;
  cout << "Output file   : " << argv [ARG_OUTPUT] << endl;
  
  istringstream iss;
  double xmin, xmax;
  
  // Determine how many coefficients are to be used for the spline fit
  size_t ncoeffs;
  if (argc == REQUIRED_NUM_ARGS_MODE1) {
    ncoeffs = DEFAULT_NUM_COEFFS;
  } else {
    if (is_numeric (argv[ARG_COEFFS])) {
      iss.str (argv[ARG_COEFFS]);
      iss >> ncoeffs;
      iss.clear ();
    } else {
      cout << "ERROR: Argument " << ARG_COEFFS << " must be a number." << endl;
      return 1;
    }
    if (ncoeffs < 4) {
      cout << "ERROR: The spline fit must contain at least 4 coefficients." << endl;
      return 1;
    }
  }
  cout << "Spline Coeffs : " << ncoeffs << endl;
  
  // Variables for the spline fit and response function calculation
  const size_t nbreak = ncoeffs - 2; // nbreak = ncoeffs+2-k = ncoeffs-2 as k=4
  size_t n = 0;
  size_t i, j;
  gsl_bspline_workspace *bw;
  gsl_vector *B;
  double dy;
  gsl_rng *r;
  gsl_vector *c, *w;
  gsl_vector *x, *y;
  gsl_matrix *X, *cov;
  gsl_multifit_linear_workspace *mw;
  double chisq, Rsq, dof, tss;
  vector <double> xVec, yVec, xResponse, yResponse, yLogRad;
  double xi, yi, yerr, ySpline, wlen, ymax;

  // Variables for file input/output
  ifstream calData (argv [ARG_CALIBRATION], ios::in);
  ifstream spectrum (argv [ARG_SPECTRUM], ios::in);
  ofstream response (argv [ARG_OUTPUT], ios::out);
  string LineString;

  // Load the calibrated standard lamp spectral radiance file
  if (calData.is_open ()) {
    getline (calData, LineString);
    while (!calData.eof ()) {
      n ++;
      iss.str (LineString);
      iss >> xi >> yi;

      xVec.push_back (xi);
      yVec.push_back (log(yi));
      iss.clear ();

      getline (calData, LineString);
    }
    calData.close ();
  } else {
    cout << "ERROR: Unable to open " << argv [ARG_CALIBRATION] << "." << endl;
    return 1;
  }
  xmin = xVec[0];
  xmax = xVec[xVec.size () - 1];
  
  // Check there are sufficient data points in the spectral radiance file
  if (n <= ncoeffs) {
    cout << "ERROR: There must be more data points in " << argv [ARG_CALIBRATION] 
      << " than spline fit coefficients." << endl;
    return 1;
  }
  
  // Prepare the GSL environment
  gsl_rng_env_setup();
  r = gsl_rng_alloc(gsl_rng_default);

  bw = gsl_bspline_alloc(4, nbreak); // allocate a cubic bspline workspace (k=4)
  B = gsl_vector_alloc(ncoeffs);

  x = gsl_vector_alloc(n);
  y = gsl_vector_alloc(n);
  X = gsl_matrix_alloc(n, ncoeffs);
  c = gsl_vector_alloc(ncoeffs);
  w = gsl_vector_alloc(n);
  cov = gsl_matrix_alloc(ncoeffs, ncoeffs);
  mw = gsl_multifit_linear_alloc(n, ncoeffs);
  
  for (i = 0; i < n; i ++) {
    gsl_vector_set (x, i, xVec[i]);
    gsl_vector_set (y, i, yVec[i]);
    gsl_vector_set (w, i, 1.0);
  }

  // use uniform breakpoints between xmin and xmax
  gsl_bspline_knots_uniform(xmin, xmax, bw);

  // construct the fit matrix X
  for (i = 0; i < n; ++i)
   {
     double xi = gsl_vector_get(x, i);

     // compute B_j(xi) for all j
     gsl_bspline_eval(xi, B, bw);

     // fill in row i of X
     for (j = 0; j < ncoeffs; ++j)
       {
         double Bj = gsl_vector_get(B, j);
         gsl_matrix_set(X, i, j, Bj);
       }
   }

  // do the spline fit
  gsl_multifit_wlinear(X, w, y, c, cov, &chisq, mw);
  dof = n - ncoeffs;
  tss = gsl_stats_wtss(w->data, 1, y->data, 1, y->size);
  Rsq = 1.0 - chisq / tss;
  printf("chisq/dof = %e, Rsq = %f\n", chisq / dof, Rsq);

  // Read in the measured lamp spectrum
  if (spectrum.is_open ()) {
    getline (spectrum, LineString);
    ymax = 0;
    while (!spectrum.eof ()) {
      iss.str (LineString);
      if (LineString[0] != '#' && LineString[0] != '!') {
        iss >> xi >> yi;
        wlen = 1e7 / xi;    // Convert wavenumber to vacuum wavelength

        // Only proceed if xi is within the valid spline interpolation range
        if (wlen >= xmin && wlen <= xmax) {
          gsl_bspline_eval(wlen, B, bw);
          gsl_multifit_linear_est(B, c, cov, &ySpline, &yerr);
          
          // Calculate the response function based on 'photon' in XGremlin
          yi = xi * xi * xi * yi / exp(ySpline);
          xResponse.push_back (xi);
          yResponse.push_back (yi);
          yLogRad.push_back (ySpline);
          if (yi > ymax) ymax = yi;
        } else {
          xResponse.push_back (xi);
          yResponse.push_back (0.0);
        }
        iss.clear ();
      }
      getline (spectrum, LineString);
    }
    spectrum.close ();
    
    // Output the normalised response function
    if (response.is_open ()) {
      cout << "Outputting " << yResponse.size () << " data points to "
        << argv [ARG_OUTPUT] << endl;
      for (i = 0; i < yResponse.size (); i ++) {
        yResponse [i] /= ymax;
        response << xResponse [i] << " " << yResponse [i] 
          /*<< " " << yLogRad [i]*/ << endl;
      }
      response.close ();
    } else {
      cout << "ERROR: Unable to write to " << argv [ARG_OUTPUT] << endl;
    }
  } else {
    cout << "ERROR: Unable to open " << argv [ARG_SPECTRUM] << endl;
    return 1;
  }
  
  // Free up the GSL environment and terminate the program
  gsl_rng_free(r);
  gsl_bspline_free(bw);
  gsl_vector_free(B);
  gsl_vector_free(x);
  gsl_vector_free(y);
  gsl_matrix_free(X);
  gsl_vector_free(c);
  gsl_vector_free(w);
  gsl_matrix_free(cov);
  gsl_multifit_linear_free(mw);
  return 0;
}
