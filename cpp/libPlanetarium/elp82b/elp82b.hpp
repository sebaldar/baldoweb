

/************************************************************************
    2 
    3 LUNAR SOLUTION ELP2000-82B
    4 by Chapront-Touze M., Chapront J.
    5 ftp://ftp.imcce.fr/pub/ephem/moon/elp82b
    6 
    7 I (Johannes Gajdosik) have just taken the Fortran code and data
    8 obtained from above and used it to create this piece of software.
    9 
   10 I can neigther allow nor forbid the usage of ELP2000-82B.
   11 The copyright notice below covers not the works of
   12 Chapront-Touze M. and Chapront J., but just my work,
   13 that is the compilation and rearrangement of
   14 the Fortran code and data obtained from above
   15 into the software supplied in this file.
   16 
   17 
   18 Copyright (c) 2005 Johannes Gajdosik
   19 
   20 Permission is hereby granted, free of charge, to any person obtaining a
   21 copy of this software and associated documentation files (the "Software"),
   22 to deal in the Software without restriction, including without limitation
   23 the rights to use, copy, modify, merge, publish, distribute, sublicense,
   24 and/or sell copies of the Software, and to permit persons to whom the
   25 Software is furnished to do so, subject to the following conditions:
   26 
   27 The above copyright notice and this permission notice shall be included
   28 in all copies or substantial portions of the Software.
   29 
   30 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   31 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   32 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   33 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   34 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   35 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   36 SOFTWARE.
   37 
   38 My implementation of ELP2000-82B has the following modifications compared to
   39 the original Fortran code:
   40 1) fundamentally rearrange the series into optimized instructions
   41    for fast calculation of the results
   42 2) units are: julian day, AU
   43 
   44 ****************************************************************/

#ifndef _ELP82B_H_
#define _ELP82B_H_
 
#include <vector>
 
std::vector< double > elp82b ( double jd );
std::vector< double > elp82b_ecl_coords ( double jd ) ;


/* Return the rectangular coordinates of the earths moon
on the given julian date jd expressed in dynamical time (TAI+32.184s).
The origin of the xyz-coordinates is the center of the earth.
The reference frame is "dynamical equinox and ecliptic J2000",
which is the reference frame in VSOP87 and VSOP87A.
 
According to vsop87.doc VSOP87 coordinates can be transformed to FK5 by
       X       cos(psi) -sin(psi) 0   1        0         0   X
       Y     = sin(psi)  cos(psi) 0 * 0 cos(eps) -sin(eps) * Y
       Z FK5          0         0 1   0 sin(eps)  cos(eps)   Z VSOP87
with psi = -0.0000275 degrees = -0.099 arcsec and
eps = 23.4392803055556 degrees = 23d26m21.4091sec.
 
http://ssd.jpl.nasa.gov/horizons_doc.html#frames
says:
        "J2000" selects an Earth Mean-Equator and dynamical Equinox of
        Epoch J2000.0 inertial reference system, where the Epoch of J2000.0
        is the Julian date 2451545.0. "Mean" indicates nutation effects are
        ignored in the frame definition. The system is aligned with the
        IAU-sponsored J2000 frame of the Radio Source Catalog of the
        International Earth Rotational Service (ICRF).
        The ICRF is thought to differ from FK5 by at most 0.01 arcsec.

      From this I conclude that in the context of stellarium
      ICRF, J2000 and FK5 are the same, while the transformation
      ICRF <-> VSOP87 must be done with the matrix given above.
*/
      
 
 
#endif
