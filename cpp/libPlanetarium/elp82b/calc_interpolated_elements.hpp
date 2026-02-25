 /************************************************************************
    2 
    3 Copyright (c) 2007 Johannes Gajdosik
    4 
    5 Permission is hereby granted, free of charge, to any person obtaining a
    6 copy of this software and associated documentation files (the "Software"),
    7 to deal in the Software without restriction, including without limitation
    8 the rights to use, copy, modify, merge, publish, distribute, sublicense,
    9 and/or sell copies of the Software, and to permit persons to whom the
   10 Software is furnished to do so, subject to the following conditions:
   11 
   12 The above copyright notice and this permission notice shall be included
   13 in all copies or substantial portions of the Software.
   14 
   15 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   16 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   17 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   18 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   19 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   20 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   21 SOFTWARE.
   22 
 ***************************************************************/
  
 extern
 void CalcInterpolatedElements(const double t,double elem[],
                                const int dim,
                                void (*calc_func)(const double t,double elem[]),
                                const double delta_t,
                                double *t0,double e0[],
                                double *t1,double e1[],
                                double *t2,double e2[]);
  
 /*
   35 Simple interpolation routine with external cache.
   36 The cache consists of 3 sets of values:
   37   e0[0..dim-1] are the cached values at time *t0,
   38   e1[0..dim-1] are the cached values at time *t1,
   39   e2[0..dim-1] are the cached values at time *t2
   40 delta_t is the time step: *t2-*t1 = *t1-*t0 = delta_t,
   41 (*calc_func)(t,elem) calculates the values elem[0..dim-1] at time t,
   42 t is the input parameter, elem[0..dim-1] are the output values.
   43 
   44 The user must supply *t0,*t1,*t2,e0,e1,e2.
   45 The initial values must be *t0 = *t1 = *t2 = -1e100,
   46 the initial values of e0,e1,e2 can be undefined.
   47 The values of *t0,*t1,*t2,e0,e1,e2 belong to this function,
   48 the user must never change them.
   49 
   50 The user must always supply the same delta_t
   51 for one set of (*t0,*t1,*t2,e0,e1,e2),
   52 and of course the same dim and calc_func.
   53 */
