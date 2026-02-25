#ifndef _cmat_
#define _cmat_

#include <iostream>
#include <sstream>
#include <string.h>

#include <vector>

#include <math.h>

#ifndef _matrix_
   #include <matrix.hpp>
#endif

#ifndef _MAX
   #define _MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef _MIN
   #define _MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define _SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

typedef void (*LOGf)( const std::string & str );

template < class t_function, class t_variable, class t_type >
class Math {


		LOGf fLog = nullptr;
		struct GLOB_VAR {

			// MASSIME ITERAZIONI
			short NEW_RAP_MAXITER 	= 20 ;   // per algoritmo di Newton Rapson
			short BCKTRACK_MAXITER 	= 20 ;   //  per algoritmo di Backtraking
			short RADNEW_MAXITER 	= 20 ;   // per algoritmo ricerca radici con Newton

			// PRECISIONE
			t_type TZERO_ACC 	= 1e-6;    // per algoritmo TZERO
			t_type RADNEW_ACC 	= 1e-6; // per algoritmo ricerca radici Newton
			t_type RADNEW_INC 	= 1e-6; // incremento per algoritmo ricerca radici Newton
			t_type EVAL_ACC 	= 1e-6; // per valutazione sistem
			t_type BCKTRK_ACC	= 1e-6; // per backtraking

			// zbrent
			unsigned short ITMAX = 100 ;			// 	Maximum allowed number of iterations.
			t_type EPS = 3.0e-8 ;		// Machine floating-point precision.


		} G;

		t_type FMin( VETTORE &f ) ;
		t_type   POWL( t_type a, t_type b );

		t_type newLineSearch(
			std::vector< t_variable * > var,
			std::vector< t_function *> f,
			VETTORE &fvec,
			t_type fold,
			VETTORE &g,
			VETTORE &p,
			t_type stpmax ) ;

		void newJacob(
			QMATRICE &J,
			VETTORE &fvec,
			std::vector< t_variable * > x,
			std::vector< t_function *> ptf
		);

		bool newBacktracking (
			std::vector< t_variable * > x,
			std::vector< t_function *> ptf
		);

		bool Gauss(VETTORE &I, QMATRICE &C, VETTORE &T) ;

		t_type IterativeRefinement (VETTORE &X0, QMATRICE &C, VETTORE &T) ;

		t_type zbrent(  t_function & funz, t_variable & v, t_type x1, t_type x2 ) ;
		bool Bracket( t_function  & funz, t_variable & v, t_type &x1, t_type &x2) ;

		void RadNewton ( t_function & funz, t_variable & v ) ;
		t_type TZero (  t_function & funz, t_variable & v  ) ;

	public :

		Math () ;

		bool verbose = true;
		bool risolveEquazione ( t_function & funz, t_variable & var ) ;
		bool execute (
			std::vector< t_variable * > var,
			std::vector< t_function *> f
		) ;

		void MessageOut( const std::string & msg ) ;

		void setLog ( LOGf f ) {
			fLog = f;
		}
		
		void setPrecisione ( t_type system, t_type radix, t_type delta ) {
			G.EVAL_ACC 	= system; // per valutazione sistem
			G.BCKTRK_ACC	= system; // per backtraking
			G.TZERO_ACC 	= radix;    // per algoritmo TZERO
			G.RADNEW_ACC 	= radix; // per algoritmo ricerca radici Newton
			G.RADNEW_INC 	= delta; // per algoritmo ricerca radici Newton
		}

};

/* inizio implementazione classe template */

template < class t_function, class t_variable, class t_type >
Math< t_function, t_variable, t_type >::Math ()
{
}


template < class t_function, class t_variable, class t_type >
t_type Math< t_function, t_variable, t_type >::FMin( VETTORE &f )
{
/*
return f = 0.5 F*F
*/
	t_type sum = 0;
	for ( int i = 0; i < f.Dim(); i++ ) {
		sum += f(i) * f(i);
	}

	return 0.5 * sum;

}

template < class t_function, class t_variable, class t_type >
inline t_type   Math< t_function, t_variable, t_type >::POWL( t_type a, t_type b )
{
	// serve per controllare eventualmente l'overflow
	// e argomento maggiore di zero
//	if (a < 0) a = fabs(a) ;

	return powl ( a, b ) ;

}

template < class t_function, class t_variable, class t_type >
t_type Math< t_function, t_variable, t_type >::
newLineSearch(
	std::vector< t_variable * > var,
	std::vector< t_function *> f,
	VETTORE &fvec,
	t_type fold,
	VETTORE &g,
	VETTORE &p,
	t_type stpmax )

{
// Numerical recipes : 9.7 Globally Convergent Methods
// for NonLinear systems of Equations - page 376

/*
Given a n-dimensional point xold(1:n_var), the value of the
function and gradient there, fold and g(1:n_var),
and a direction p(1:n) along the direction p from xold
where the funcion has decrease "sufficiently".
stpmax is an input quantity that limits the length
of the steps so that you do not try to evaluate the function
in regions where it is undefined or subject o overflow. p is
usually the Newton direction. The output quantity check
is false on a norma exit. It is true when x is too close
 to xold.
In a minmization algorithm, this usually signals convergence
and can be ignored. However, in a zero-finding algorithm the
calling program should check whether the convergence is
 spurious.
 Paraneters: ALF ensures sufficient decrease in function value;
TOLX is the convergence criterion on deltax
*/
	int n_var = fvec.Dim();

	t_type const ALF  = 1e-4 ;
	t_type const TOLX = 1e-7 ;

	VETTORE xnew(n_var);
	VETTORE xold(n_var);

	t_type fold2  = 0,
	       tmplam = 0,
	       alam2  = 0;

	t_type f2 = 0;

	// save TVARIABLE in xold
	for (int i = 0; i < n_var; i++) {
		t_variable &v = *var[i];
		xold(i) = v;
	}

//	bool check = false;

	t_type sum = 0;
	for (int i = 0; i < n_var; i++)
		sum += p(i) * p(i);

	sum = POWL(sum, 0.5);

	// scale if attempted step is too big
	if (sum > stpmax)
		for (int i = 0; i < n_var; i++)
			p(i) = p(i) * stpmax / sum;

	t_type slope = 0;
	for (int i = 0; i < n_var; i++)
		slope += g(i) * p(i);

	// compute lambda min
	t_type test = 0;
	for (int i = 0; i < n_var; i++) {
		auto temp = fabs(p(i)) / _MAX( fabs(xold(i)), 1.0 );
		if (temp > test) test = temp;
	}

	// verifica se test fosse nullo
	if (test == 0) test = 1;
		t_type alamin = TOLX / test;

	// always tray full Newton step first
	t_type alam = 1;

	// start of iteration loop
	do {

		// next x ...
		for ( int i = 0; i < n_var; i++) {
			t_variable &v = *var[i];
			v = xold(i) + alam * p(i);
		}

		// new fvec
		for ( int i = 0; i < n_var; i++) {
			t_function & funz = * f[i] ;
			fvec(i) = funz ();
		}

		t_type fmin = FMin( fvec );
		// verifica se il risultato è nan o inf
		if ( fmin != fmin ) {
			#ifdef _debug_
				MessageOut("nan in LineSearch")
			#endif
	

			return (fold + ALF * alam * slope);
			
		}
			

		// convergence on deltax, for zero finding
		// the calling program should verify the
		// convergence
		if (alam < alamin) {
			// ripristina ...
			for (int i = 0; i < n_var; i++) {
				t_variable &v = *var[i];
				v = xold(i);
			}
			#ifdef _debug_
				MessageOut("alam < alamin in LineSearch")
			#endif
//			check = true;
			return fold;
		}
		else if (fmin <= (fold + ALF * alam * slope)) {
			// sufficient function decrease
			#ifdef _debug_
		for ( size_t i=0; i < var.size(); ++i ) {
			std::cout << var[i]->name << "=" << *var[i] << std::endl;
			
		}
		for ( int i = 0; i < n_var; i++) {
			t_function & funz = * f[i] ;
			fvec(i) = funz ();
			std::cout << f[i]->name  << "=" << fvec(i) << std::endl;
		}
			std::cout << "fmin=" << fmin << std::endl;
			#endif
			return fmin;
		}
		else {
			// backtrack
			if (alam == 1)
				// first time
				tmplam = -slope / (2 * (fmin - fold - slope));
			else {
				// subsequent backtracks
				t_type rhs1 = fmin - fold - slope;
				t_type rhs2 = f2 - fold2 - alam2 * slope;
				t_type a = (rhs1 / POWL(alam, 2) -
						rhs2 / POWL(alam2, 2)) /	(alam - alam2);
				t_type b = (-alam2 * rhs1 / POWL(alam, 2) +
				    alam * rhs2 / POWL(alam2, 2)) / (alam - alam2);
				if (a == 0)
					tmplam = - slope / (2 * b);
				else {
					t_type disc = b * b - 3 * a * slope;
					tmplam = (-b  + POWL(disc, 0.5)) / 3 * a;
				}
				// lambda <= 0.5 lambda1
				if (tmplam > 0.5 * alam)
					tmplam = 0.5 * alam;
			}
		}
		alam2 = alam;
		f2 = fmin;
		fold2 = fold;
                // lambda = 0.1 lambda1
		alam = _MAX(tmplam, 0.1 * alam);

	} while (true);

	return 0;
}


template < class t_function, class t_variable, class t_type >
void Math< t_function, t_variable, t_type >::
newJacob(
	QMATRICE &J,
	VETTORE &fvec,
	std::vector< t_variable * > x,
	std::vector< t_function *> ptf
	)
{
	const int STEP = 10000 ;

	int n = fvec.Dim();

	for (int i = 0; i < n; i++) {

		t_type y1 = fvec(i);

		for (int j = 0; j < n; j++) {

			// incremento
			t_variable &v = *x[j];

			t_type max = v.getMax();
			t_type min = v.getMin();

			t_type delta = (max - min) / STEP ;
			if (!delta) delta = 0.001 ;

			// calcola il gradiente
			t_type savev = v;
			v += delta; // incrementa

			t_function &  funz = * ptf[i];
			t_type y2 = funz();

			J(i, j) = (y2 - y1) / delta;
			// corregge per non avere un determinante nullo
			// questa riga sembra garantire piu facilmente la convergenza
			if (i == j && J(i, j) == 0) J(i, j) = 0.001;
			v = savev; 	// ripristina
		} // end if j

	} // end if i
}

// utilizza Newton Rapson con la correzione backtracking

template < class t_function, class t_variable, class t_type >
bool Math< t_function, t_variable, t_type >::
newBacktracking (
	std::vector< t_variable * > x,
	std::vector< t_function *> ptf
	)
{

	/*
	given an initial guess x(1:n_var) for a root in n_var
	dimensions, find the root by a globally convegent
	Newton's method. The vector of the functions to be zeroed,
	called fvec(1:n_var), is returned by the ptf(1:n_var) functions.
	The output quantity check is false on a normal return
	and true if the routine has converged to a local minimum
	of the function fmin. In this case try restarting from a different
	initial guess.
	Parameters: NP is the maximum expected value of n_var;
	MAX_ITER is the maximum number of iterations; TOLF sets the
	convergence criterion on function values; TOLMIN sets the
	criterion for deciding whether spurious convergence to a minimum
	of fmin has occurred; TOLX is the convergence criterion on deltax;
	STPMX is the scaled maximum step ength allowed in line search
	*/

	t_type n_var = x.size();

        // accuratezza
	t_type ACC = G.BCKTRK_ACC ;

	const int STPMX = 100;

	//	t_type test;

	QMATRICE J (n_var);		// jacobiano
	VETTORE fvec(n_var);	// valore della funzione
	VETTORE b(n_var);       // vettore termini noti
	VETTORE a(n_var);       // vettore incognite
	VETTORE g(n_var);		// gradiente di f

	// calculate stpmax for line searches
	t_type sum = 0;
	for (int i = 0; i < n_var; i++) {
		t_variable &v = *x[i];
		sum += v * v;
	}

	t_type stpmax = STPMX * _MAX(POWL(sum, 0.5), n_var);

	int MAX_ITER = G.BCKTRACK_MAXITER ;
	int iter = 0;


	for (int i = 0; i < n_var; i++) {
		t_function & funz = * ptf[i];
		fvec(i) = funz ();
	}

	t_type fmin = FMin(fvec);

	while (fmin > ACC && iter++ < MAX_ITER) {

		// genera lo Jacobiano
		newJacob(J, fvec, x, ptf);

		// il vettore dei coefficienti
		for (int i = 0; i < n_var; i++)
			b(i) = -fvec(i);

		// calcola il gradiente di f
		g = J * fvec;

		// prima prova con gauss
		if ( !Gauss(a, J, b) ) {

			// calcola il determinante di J
			// come sottoprodotto della triangolazione in Gauss
/*
			t_type Det = 1;
			for (int i = 0; i < n_var; i++) {
				if (!J(i, i)) J(i, i) = 0.001;
				Det *= J(i, i);

			}
*/
			// correzione in caso di ill-conditioned
			MessageOut( "Correzione mal condizionamento");
			int itr = 0;
			const int MAXITER = 20;
			while (IterativeRefinement (a, J, b) > 0.001)
				if (itr++ > MAXITER) break;

		}

		// backtracking


		static int count ;	// contatore quando fmin==save_min
		t_type save_min = fmin;
		fmin = newLineSearch (x, ptf, fvec, fmin, g, a, stpmax);
		if (fmin < save_min) {
			MAX_ITER++;
			count = 0;
		}
		else {

			count++;

			if ( count == 3 && fmin > 0.001) {

				std::ostringstream oss ( "");
				oss << "Correzione valori iniziali" ;
				MessageOut( oss.str() );

				for ( int i = 0; i < n_var; i++ ) {

					t_variable &v = *x[i];

					v.Rand() ;
					fvec[i] = *x[i] ;

				}

				fmin = FMin(fvec);

				count = 0;

			}

		}

		std::ostringstream oss ( "");
		oss << "Iterazione " << iter << " : " << fmin;

		MessageOut( oss.str() );

	}

	{
		std::ostringstream oss ( "" );
		oss << "Dopo " << iter << " iterazioni i residui risultano " << fmin << "! " ;
		MessageOut( oss.str() );

	#ifdef _debug_
		for ( size_t i=0; i < x.size(); ++i ) {
			std::cout << x[i]->name << "=" << *x[i] << std::endl;
		}
	#endif
	}

	if (iter >= MAX_ITER) return false;
	else	return true;

}

template < class t_function, class t_variable, class t_type >
bool Math< t_function, t_variable, t_type >::Gauss(VETTORE &I, QMATRICE &C, VETTORE &T)
{

	// Triangolazine di un sistema di n Equazioni e n Incognite
	// C é la MATRICE dei coeffcienti
	// T é il VETTORE dei termini noti
	// I é il vettore delle incognite

//	t_type const e = 0.00001;

	int n = C.Dim();

	for (int k = 0; k < n - 1; k++) {
		// Pivotal condensation
		// si ricerca la equazione con il max C(m, k)
		// e si scambia con equazione attuale
		t_type max = fabs(C(k , k));
		int p = k;
		for (int m = k + 1; m < n; m++)
			if (fabs(C(m, k)) > max) {
				max = fabs(C(m, k)) ;
				p = m;

			}

//		if (max <= e) {
		if ( max == 0 ) {

			//	C(p, k) = e;
			MessageOut("Mal condizionamento del determinante");
			return false;
		} // end if max

		if (p != k) {

			C.swap(p, k) ;
			T.swap(p, k);
		}

		// fine Pivotal condensation
		for (int i = k + 1; i < n; i++) {

			t_type u = C(i, k) / C(k, k);
			T(i) -= u * T(k);

			for (int j = k; j < n; j++)
				C(i, j) -= u * C(k, j);

		} // end for i ...

	} // end for k ...



	// back substitution

	I(n - 1) = T(n - 1) / C(n - 1, n - 1);

	for (int i = n - 2; i >= 0; i--) {
		t_type sum = 0;
		for (int j = i + 1; j <= n - 1; j++)
			sum += C(i, j) * I(j);
		I(i) = (T(i) - sum) / C(i, i);

	}	// for i...

	return true ;

}

template < class t_function, class t_variable, class t_type >
void Math< t_function, t_variable, t_type >::MessageOut( const std::string & msg )
{

	if ( verbose ) {
		std::cerr << msg << std::endl;
	}
	else if ( fLog != nullptr ) {
		fLog( msg );
	}

}

template < class t_function, class t_variable, class t_type >
t_type Math< t_function, t_variable, t_type >::IterativeRefinement (VETTORE &X0, QMATRICE &C, VETTORE &T)
{
// C é la MATRICE dei coeffcienti
// T é il VETTORE dei termini noti
// X0 la soluzione approssimata ottenuta con Gauss

	int n_var = X0.Dim() ;	// numero di variabili

	VETTORE T0 (n_var) ;	//	termini noti per x = x0
	VETTORE ALPHA (n_var) ; // vari T-T0
	VETTORE E0(n_var) ;	// errore

   // calcola il vettore T0
	for (int i = 0; i < n_var; i++) {
		t_type sum = 0;
		for (int j = 0; j < n_var; j++)
			sum += C(i, j) * X0(j) ;
		T0(i) = sum ;
	}

   // calcola il vettore ALPHA
	for (int i = 0; i < n_var; i++)
		ALPHA(i) = T(i) - T0(i) ;

   // risolve con Gauss

	VETTORE ERR(n_var) ;

	Gauss (ERR, C, ALPHA) ;

	t_type errore = 0;
	for (int i = 0; i < n_var; i++) {

		X0(i) += ERR(i) ;
		errore += ERR (i) * ERR (i);

	}

	return errore ;
}

template < class t_function, class t_variable, class t_type >
bool Math< t_function, t_variable, t_type >::
execute (
	std::vector< t_variable * > var,
	std::vector< t_function *> f
	)
{

	size_t fu = f.size() ;
	size_t va = var.size() ;
	
	if ( fu != va ) {
		std::ostringstream oss ( "" );
		oss << "Il numero di variabili " << va << " risulta diverso dal numero di funzioni " << fu << std::endl;
		throw( oss.str());
	}

	QMATRICE S(va);	// lo jacobiano i sono le funzioni

	MessageOut("Creazione  Jacobiano ...");

	// salva le variabili e gli assegna un numero casuale tra min e max
	std::vector <t_type> save_val;
	for ( size_t i = 0; i < va; i++ ) {
		var[i]->Rand();
		save_val.push_back( t_type( *var[i] ) );
	}

	// genera una matrice equaz/variab che ha
	// l'element i,j <> 0 se j é variabile della eq. i
	for ( size_t i = 0; i < fu; i++) {

		t_function & funz = * f[i];
		t_type y1 ;

		try 	{
			y1 = funz();

		}
		catch ( const std::string & e ) {

			MessageOut(e);
			return false;
		
		}

		for ( size_t j = 0; j < va; j++) {

			// ripristina le precedenti
			for ( size_t k = 0; k < j; k++ ) {
				*var[k] = save_val[k];
			}

			// calcola la variazione modificando casualmente la variabile j
			var[j]->Rand();
//			t_function & funz = * f[i];
			t_type y2 = funz();
/*
std::ostringstream oss ;
oss << f[i]->source << " " << var[j]->name << "=" << var[j]->val << " y1=" << y1 << " y2=" << y2;
MessageOut(oss.str());
*/
			if ( y2 != y1 )	// vuol dire che l'equazione i dipende dalla variabile j
				S(i, j) = 1;

		}	// for j


	}	// for i


	// cerca di risolvere le equazioni che dipendono da
	// una sola variabili
	MessageOut("Verifica se ci sono equazioni che dipendono da una sola variabile ...");
	size_t old_va = va;

	size_t non_risolte = 0;
	std::vector< t_variable * > var_risolte;
	bool flag;	// fai un nuovo giro
	do {

		flag = false;

		for ( size_t i = 0; i < fu; i++) {

			int count = 0;
			int v = -1;

			for ( size_t j = 0; j < va; j++)
				if (S(i, j) == 1)
					v = j, count++;

			if (count == 1) {

				if ( !risolveEquazione ( * f[i], * var[v] ) )
					non_risolte++;
				else {
					var_risolte.push_back(var[v]);
				}

				for ( size_t k = i; k < fu - 1; k++)
					f[k] = f[k + 1];
				fu--;

				for ( size_t k = v; k < va - 1; k++)
					var[k] = var[k + 1];
				va--;

				// elimina la riga i e la colonna v
				S.Del( i, v );
				flag = true;
				i--;

			} // end if count

		} // end for i...

	} while (flag) ;

	// se le ha risolte tutte esci
	if ( !fu  ) {

		std::ostringstream oss ;
		if ( non_risolte > 0 ) {
			oss << non_risolte << " equazioni su " << old_va << " che dipendono da una sola variabile non sono state risolte con successo!!";
			MessageOut( oss.str() );
			return false;
		}
		else {
			oss << "Tutte " << old_va << " equazioni dipendono da una sola variabile e sono state risolte con successo!!";
			for ( auto i = 0; i < old_va - va; i++) {
				auto v = var_risolte[i];
				oss << v->name << "=" << v->val << " " << std::endl;
			}
			MessageOut( oss.str() );
		}

		return true;

	}

	if ( old_va != va ) {

		if ( non_risolte == 0 ) {
			std::ostringstream oss ;
			oss << "Sono state risolte " << old_va - va << " equazioni!" << std::endl;
			for ( auto i = 0; i < old_va - va; i++) {
				auto v = var_risolte[i];
				oss << v->name << "=" << v->val << " " << std::endl;
			}
			MessageOut( oss.str() );
		}
		else {
			std::ostringstream oss ;
			oss << "Risolte " << old_va-va-non_risolte << " equazioni!" << std::endl;
			oss << non_risolte << " equazioni non hanno soluzioni nel campo reale";
			MessageOut( oss.str() );
			throw ( oss.str() );
		}

	}
	else {
		std::ostringstream oss ( "" );
		oss << "Nessuna equazione dipende da una sola variabile!";
		MessageOut( oss.str() );
	}

/*
	se una variabile compare in una sola equazione
	può essere risolta successivamente
*/

	{
		std::ostringstream oss ( "" );
		oss << "Verifica se ci sono variabili che compaiono in una sola equazione, in questo caso le risolve successivamente ..." ;
		MessageOut( oss.str() );
	}

	old_va = va;
	std::vector< t_function * > fz;
	std::vector< t_variable * > vr;

// ************** loop per risoluzione differita

	do {

		flag = false;

		// per tutte le variabili
		for ( unsigned int j = 0; j < va; j++ ) {

			size_t count = 0;
			size_t ff = 0;
			for (unsigned int i = 0; i < fu; i++)
				if (S(i, j) == 1) {
					ff = i;
					count++;
				}
			if (count == 1) {

				// memorizza la funzione e la
				// variabile nella coda


				fz.push_back(  f[ff] );
				vr.push_back ( var[j] );

				// elimina equazione e variabile
				for (unsigned int k = ff; k < fu; k++)
					f[k] = f[k + 1];
				fu--;

				for (unsigned int k = j; k < va; k++)
					var[k] = var[k + 1];
				va--;

				// elimina la riga i e la colonn v
				S.Del( ff, j );
				flag = true;
				j--;

			} // end if count

		} // end for j...

	} while (flag) ;


// ************** fine loop per risoluzione differita

	int no_eq_risolv_dopo = old_va - va;
	if ( old_va != va ) {
		std::ostringstream oss ;
		oss <<  "In " <<old_va-va << " equazioni una variabile compare una sola volta e sarà risolta successivamente!";
		MessageOut( oss.str() );
	}
	else {
		std::ostringstream oss ;
		oss <<  "Nessuna equazione ha una variabile che compare una sola volta!";
		MessageOut( oss.str() );
	}
	// riordina il sistema di equazioni
	// Verifica che sia possibile ottenere la matrice
	//   S in modo che tutti i termini S(i,i) siano
	//   diversi da zero

	MessageOut("Riordinamento Jacobiano ...");

	// crea un vettore da utilizzare per riordinare f
	VETTORE sort_v(fu) ;
	for ( unsigned int i = 0; i < fu; i++ )
		sort_v(i) = i;

	// se Condiziona ritorna false il determinante é zero
	// e quindi le variabili non sono tutte indipendenti

	if ( !Condiziona( S, sort_v ) ) {
		MessageOut("Condizionamento non a buon fine. Ci sono variabili dipendenti?");
	}
	else
		MessageOut("Condizionamento eseguito.");

	// ordiniamo il vettore funzioni in conformitá
	std::vector < t_function * > sorted_f;
	std::vector < t_variable * > sorted_v;

	for (unsigned int i = 0; i < fu; i++) {
		sorted_f.push_back ( f[ (int)sort_v(i) ] );
	}

	// nuovo array senza l'ultimo elemento
	std::vector< t_variable * > new_var;
	for ( unsigned int i = 0; i < va; i++ ) {
		new_var.push_back ( var[i] );
	}

	std::ostringstream oss ;
	oss << "Risolve il sistema di " << fu <<  " equazioni e " << va  << " incognite ...";

	MessageOut( oss.str() );

	if (fu)	{
		if ( !newBacktracking ( new_var, sorted_f ) ) {
			MessageOut("Convergenza non a buon fine. Cambiare i valori di ingresso!!");
			return false;
		}
	}

	bool rtn = true ;

	// risolve le equazioni memorizzate prima della
	// chiamata a _new_rap
	size_t no = no_eq_risolv_dopo ;
	if ( no ) {

		std::vector < t_function * > f1;
		std::vector < t_variable * > var1;

		for ( size_t i = 0; i < no; ++i ) {
			f1.push_back(fz[ i ]);
			var1.push_back(vr[ i ]);
		}

		std::ostringstream oss ;
		oss << no << " equazioni vengono risolte adesso ... ";
		MessageOut( oss.str() );

		// risolve il sistema
		// con una chiamata ricorsiva

		rtn = execute ( var1, f1 );


	}

	return rtn ;

}

template < class t_function, class t_variable, class t_type >
bool Math< t_function, t_variable, t_type >::risolveEquazione ( t_function & funz, t_variable & var )
{

	// risolve la equazione con Newton
	try {
		RadNewton( funz, var );
	}
	catch ( std::string & e ) {

		std::string source = funz.getSource();
		size_t pos = source.find ( ":=" );

		if ( pos != std::string::npos ) {
			pos +=  strlen( ":=" );
			std::ostringstream oss ( "" );
			oss << "f(" << var.GetName() << ")=" << source.substr(pos);
			source = oss.str();
		}


		std::ostringstream oss ( "" );
		oss << "===>> " << e << std::endl;
		oss << "===>> Nella equazione " << source << std::endl;
		oss << "===>> Non è stato possibile risolvere la variabile " << var.GetName() << " iterativamente con l'algoritmo di Newton-Raphson." << std::endl;
		oss << e;
		MessageOut( oss.str() );

		// se non ci sei riuscito
		// prova con il teorema degli zeri
		try {
			TZero( funz, var );
			std::ostringstream oss ( "" );
			oss << "===>> L'equazione " << source << " è stata risolta con TZero!.";
			MessageOut( oss.str() );
		} catch ( std::string & e ) {
			std::ostringstream oss ( "" );
			oss << "===>> " << e <<  " L'equazione " << source << " non ha soluzione nel campo reale.";
//						oss << "Attenzione: l'equazione " << f[i].getSource() << " non ha soluzione nel campo reale.";
			MessageOut( oss.str() );
			return false;
		}

	}

	return true;

}

template < class t_function, class t_variable, class t_type >
void Math< t_function, t_variable, t_type >::RadNewton ( t_function & funz, t_variable & v )
{

	// procedimento di Newton per il calcolo di radici reali
	// di equazioni non lineari
	// si basa sulla ricorsione
	//
	//	xi+1 = xi - f(xi)/f'(xi)

	// valore iniziale



	int maxiter = G.RADNEW_MAXITER ;
	t_type delta = G.RADNEW_INC;
	int	 iter    = 0;

/*
	t_type max = v.getMax();
	t_type min = v.getMin();

	v = min + (max - min) / 2;
*/
/*
	if ( max != min )
		delta = (max - min) / 1e+6;
	else
 */
	t_type y0;

	// calcola la funz in x0=v
	while (fabs( y0 = funz() ) > G.RADNEW_ACC) {

		t_variable savev ;
		savev = v;

		// calcola la derivata prima
		// incremento
		// calcola il gradiente
		v += delta; // incrementa

		t_type y1 = funz ();
		t_type dy = (y1 - y0) / delta;

		if (fabs(dy) <= G.RADNEW_ACC) {
			std::ostringstream oss;
			oss << "Superata la precisione " << G.RADNEW_ACC << " nell'algoritmo di Newton-Raphson";
            throw( oss.str() );
		}
		// fine calcolo derivata

		v = savev - y0 / dy;
		if (iter++ > maxiter) {
			std::ostringstream oss;
			oss << "Superate le iterazione massime (" << maxiter << ") in RadNewton";
            throw( oss.str() );
	    }

	}

//****************
}

template < class t_function, class t_variable, class t_type >
t_type Math< t_function, t_variable, t_type >::TZero (  t_function & funz, t_variable & var  )
{

	/*
	 teorema degli zeri
	y1 valore della funzione a x1
	y2 valore della funzione a x2
	y1 e y2 devono essere di segno contrario
	*/


	t_variable saveval ;
	saveval = var;

	t_type x1 = var.getMax();
	t_type x2 = var.getMin() ;

	// var, x1, x2 sono modificate da Bracket
	MessageOut( "Richiama la routine bracket per trovare gli estremi di segno opposto" );
	if ( !Bracket( funz, var, x1, x2 ) ) {
		var = saveval;
		throw ( std::string( "Non è stato possibile trovare estremi di segno opposto." ) );
	}

	return zbrent( funz, var, x1, x2 );

}


template < class t_function, class t_variable, class t_type >
t_type Math< t_function, t_variable, t_type >::zbrent(  t_function & funz, t_variable & var, t_type x1, t_type x2 )
/*
 * Using the secant method, find the root of a function funz thought to lie between x1 and x2.
 * The root, returned as rtsec, is refined until its accuracy is ±xacc.
*/
{

	t_type a = x1;
	t_type b = x2;

	t_type c = x2;
	t_type d,e,min1,min2;

	var = a;
	t_type fa = funz();
	var = b;
	t_type fb = funz();

	t_type fc, p, q, r, s, tol1, xm;

	if ( ( fa > 0.0 && fb > 0.0 ) || ( fa < 0.0 && fb < 0.0 ) ) {
		throw ( std::string("Root must be bracketed in zbrent") );
	}

	fc = fb;

	for ( size_t iter = 1; iter <= G.ITMAX; iter++ ) {

		if ( (fb > 0.0 && fc > 0.0) || (fb < 0.0 && fc < 0.0) ) {

			c = a; 	// Rename a, b, c and adjust bounding interval d
			fc = fa;
			e = d = b - a;

		}

		if ( fabs(fc) < fabs(fb) ) {

			a = b;
			b = c;
			c = a;
			fa = fb;
			fb = fc;
			fc = fa;
		}

		tol1 = 2.0 * G.EPS * fabs(b) + 0.5 * G.TZERO_ACC; 	// Convergence check.
		xm = 0.5 * (c - b);

		if (fabs(xm) <= tol1 || fb == 0.0) {

				std::ostringstream oss ( "" );
				oss << "zbrent " << iter << " iterazioni " << std::endl;

				MessageOut( oss.str() );

				return b;
		}

		if (fabs(e) >= tol1 && fabs(fa) > fabs(fb)) {

			s=fb/fa; 	// Attempt inverse quadratic interpolation.

			if (a == c) {

				p = 2.0 * xm * s;
				q = 1.0 - s;
			} else {

				q = fa / fc;
				r = fb / fc;
				p = s * ( 2.0 * xm * q * (q-r) - (b-a) * (r-1.0) );
				q = (q - 1.0) * (r - 1.0 ) * ( s - 1.0 );
			}

			if (p > 0.0) q = -q; 	// Check whether in bounds.

			p = fabs(p);
			min1 = 3.0 * xm * q - fabs(tol1*q);
			min2=fabs( e * q );

			if ( 2.0 * p < ( min1 < min2 ? min1 : min2 ) ) {

				e = d; 	// Accept interpolation.
				d = p/q;
				} else {

				d = xm; 	// Interpolation failed, use bisection.
				e = d;
			}
		} else { 	// Bounds decreasing too slowly, use bisection.

			d = xm;
			e = d;

		}

		a = b; 	// Move last best guess to a.
		fa = fb;

		if ( fabs(d) > tol1 ) 	// Evaluate new trial root.
			b +=  d;
		else
			b += _SIGN( tol1, xm );

		var = b;
		fb = funz();

	}

	throw( std::string("Maximum number of iterations exceeded in zbrent") );
	return 0.0; 	// Never get here.

}

template < class t_function, class t_variable, class t_type >
bool Math< t_function, t_variable, t_type >::Bracket( t_function & funz, t_variable & val, t_type &x1, t_type &x2)
{

	/*
	given a function funz and an initial guessed range x1, x2
	the routine expands the range geometrically until the
	root is bracketed by the returned values x1, x2 (in which
	case returns as true) or until the range becomes
	unacceptably large (in which case returns false)
	*/



	const int NTRY   = 500 ;
	const t_type FACTOR = 1.6;

	t_type f1, f2;

	if (x1 == x2) return false;

	val = x1;
	f1 = funz() ;
	val = x2;
	f2 = funz() ;

	for (int j = 0; j < NTRY; j++) {
		if (f1 * f2 < 0) return true;
		if (fabs(f1) < fabs(f2)) {
			x1 += FACTOR * (x1 - x2);
			val = x1;
			f1 = funz() ;
		}
		else {
			x2 += FACTOR * (x2 - x1);
			val = x2;
			f2 = funz();
		}
	} // end for j ...

	return false ;

}



#endif
