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
// ErrDefs.h
//==============================================================================
// Contains error definitions that are used throughout the FAST code.
//
#ifndef ERR_DEFS_H
#define ERR_DEFS_H

#include <string>

// General FTS Line Tool error codes including file I/O errors.
#define LC_NO_ERROR          0
#define LC_SYNTAX_ERROR      1
#define LC_FILE_OPEN_ERROR   2
#define LC_FILE_HEAD_ERROR   3
#define LC_FILE_READ_ERROR   4
#define LC_FILE_WRITE_ERROR  5
#define LC_PLOT_LINE_MISSING 6
#define LC_PLOT_NO_GNUPLOT   7
#define LC_DIALOG_CANCEL     8
#define LC_SAVE_ABORTED      9

#define NO_COMPARISON_LINES_FOUNDS 10
#define NO_SCALING_RATIO_FOUND     11

// Error codes specific to the Line class
#define LINE_NEGATIVE_WAVENUMBER 10
#define LINE_NEGATIVE_PEAK 11
#define LINE_NEGATIVE_WIDTH 12
#define LINE_NEGATIVE_EQWIDTH 13
#define LINE_NEGATIVE_WAVELENGTH 14
#define LINE_NEGATIVE_SNR 15

// Error codes specific to the XgSpectrum class
#define XGSPEC_OUT_OF_BOUNDS 16
#define XGSPEC_NO_RAD_UNCERTAINTIES 17

// Error codes specific to the ListCal class
#define LC_NO_OVERLAP     18
#define LC_NEGATIVE_VALUE 19
#define LC_NO_DATA        20

// Define an Error type that can be used for reporting errors in the FAST UI.
typedef struct error_type {
  int code;
  std::string message, subtext;
  error_type () { code = 0; message = ""; subtext = ""; }
  error_type (int c) { code = c; message = ""; subtext = ""; }
  error_type (int c, std::string m) { code = c; message = m; subtext = ""; }
  error_type (int c, std::string m, std::string s) 
    { code = c; message = m; subtext = s; }
} Error;

#endif // ERR_DEFS_H
