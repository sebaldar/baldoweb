#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef AsserzioniH
//   #include <Asserzioni.h>
#endif	// #ifndef AsserzioniH

#ifndef _matrix_
   #include <matrix.hpp>
#endif

// implementazione della classe MD_MATRIX

MD_MATRIX::MD_MATRIX() :
	 m(NULL), dim(0)
{
}


MD_MATRIX::MD_MATRIX (int d, int first) :
	 first_el(first), dim(d)
{
	m = new FLOAT[dim];
	for (int i = 0; i < dim; i++)
		m[i] = 0;
}


MD_MATRIX::MD_MATRIX (MD_MATRIX const &M1) :
	first_el(M1.first_el), dim(M1.dim)
{
	m = new FLOAT[dim];
	for (int i = 0; i < dim; i++)
		m[i] = M1.m[i];
}


MD_MATRIX::~MD_MATRIX ()
{
	delete [] m;
}


int MD_MATRIX::Dim() const
{
	return dim;
}


void MD_MATRIX::MakeEqual(MD_MATRIX const &M1)
{

	delete [] m;
	first_el = M1.first_el;

	dim = M1.dim;

	m = new FLOAT [dim];
	for (int i = 0; i < dim; i++)
		m[i] = M1.m[i];
}


void MD_MATRIX::SetFirstEl (int first)
{
	first_el = first ;
}


MD_MATRIX &MD_MATRIX::operator =(MD_MATRIX const &M1)
{

	MakeEqual (M1) ;

	return *this;
}

// implementazione della classe D1_MATRIX

D1_MATRIX::D1_MATRIX () :
	MD_MATRIX()
{
}


D1_MATRIX::D1_MATRIX (int d, int first) :
	MD_MATRIX (d, first)
{
}


D1_MATRIX::D1_MATRIX (D1_MATRIX &v) :
	MD_MATRIX((MD_MATRIX)v)
{
}


D1_MATRIX::~D1_MATRIX ()
{
}


void D1_MATRIX::print()
{
	MD_MATRIX::print() ;
	for (int i = 0; i < dim; i++)
		std::cout << "M(" << (i + first_el) << ")=" <<
			m[i] << std::endl;
	std::cout << "_____________________" << std::endl;
}


const char * D1_MATRIX::GetClassName()
{
	return "D1_MATRIX";
}


FLOAT& D1_MATRIX::operator () (int i) const
{
	i -= first_el;
//	ASSERT(i >= 0 && i < dim) ;

	return m[i];
}


FLOAT& D1_MATRIX::operator[](int i) const
{
	return (*this)(i);
}

// implementazione della classe D2_MATRIX

D2_MATRIX::D2_MATRIX () :
	MD_MATRIX()
{
}


D2_MATRIX::D2_MATRIX (int r, int c, int first) :
	MD_MATRIX (r * c, first), row(r), col(c)
{
}


D2_MATRIX::D2_MATRIX (D2_MATRIX const &v) :
	MD_MATRIX((MD_MATRIX) v), row(v.row), col(v.col)
{
}


D2_MATRIX::~D2_MATRIX ()
{
}


void D2_MATRIX::print()
{
	MD_MATRIX::print() ;
	for (int i = 0; i < row; i++)
		for (int j = 0; j < col; j++)
			std::cout << "M(" << (i + first_el) <<
				 "," << (j + first_el) <<
				  ")=" <<
				  m[i * col + j] << std::endl;
	std::cout << "_____________________" << std::endl;
}


const char * D2_MATRIX::GetClassName()
{
	return "D2_MATRIX";
}


FLOAT& D2_MATRIX::operator () (int r, int c) const
{
	r -=  first_el;
	c -=  first_el;
//	ASSERT(r >= 0 && r < row && c >= 0 && c < col);
	return m[r * col + c];
}


D2_MATRIX &D2_MATRIX::operator =(D2_MATRIX const &M1)
{
	row = M1.row;
	col = M1.col;
	MD_MATRIX::operator =(M1);

	return *this;
}

// implementazione della classe B_MATRIX

B_MATRIX::B_MATRIX () :
	D2_MATRIX()
{
}


B_MATRIX::B_MATRIX (int r, int c, int first) :
	D2_MATRIX(r, c, first)
{
}


B_MATRIX::B_MATRIX (B_MATRIX const &M1) :
	D2_MATRIX(M1)
{
}


B_MATRIX::~B_MATRIX ()
{
}


void B_MATRIX::swap(int i, int j)
{
	if ( j == i || j >=row || i >= row ) return;

    // scambia la riga i-esima con la j-esima
	for (int k = 0; k < col; k++) {
		FLOAT temp = GetValue(j, k);
		SetValue(j, k, GetValue(i, k));
		SetValue(i, k, temp);
	}
}


void B_MATRIX::Del(int r, int c)
{
/*
// elimina la riga
	for (int i = r; i < row - 1; i++)
		for (int j = 0; j < col; j++)
			SetValue(i, j, GetValue(i + 1, j));
	for (int j = 0; j < col; j++)
		SetValue(row - 1, j, 0);

      // elimina la colonna
	for (int j = c; j < col - 1; j++)
		for (int i = 0; i < row; i ++)
			SetValue(i, j, GetValue(i, j + 1));
	for (int i = 0; i < col; i++)
		SetValue(i, col - 1, 0);


// dimnuisce la dimensione senza rilasciare la memoria
// tanto é poca cosa
	row--;
   col--;
*/

// elimina la riga
	for (int i = r; i < row - 1; i++)
		for (int j = 0; j < col; j++)
			SetValue(i, j, GetValue(i + 1, j));
	for (int j = 0; j < col; j++)
		SetValue(row - 1, j, 0);

      // elimina la colonna
	for (int j = c; j < col - 1; j++)
		for (int i = 0; i < row; i ++)
			SetValue(i, j, GetValue(i, j + 1));

	for (int i = 0; i < col; i++)
		SetValue(i, col - 1, 0);

	int new_dim_col = col - 1;
	int new_dim_row = row - 1;

	// un pó faraginoso
	B_MATRIX N ( new_dim_row, new_dim_col );
	for (int i = 0; i < new_dim_row; i++)
		for (int j = 0; j < new_dim_col; j++)
			N.SetValue(i, j, GetValue(i, j));

	*this = N;

}

char *B_MATRIX::sprint(char *str)
{
	char s[10];
	strcpy(str, "");
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			if (GetValue(i, j) != 0)
				sprintf(s, "%d ", 1);
			else
				sprintf(s, "%d ", 0);
			strcat(str, s);
		}	// end for j ...
		strcat(str, "\n");
	}	// end for i ...
	return str;
}


B_MATRIX operator * (B_MATRIX const &M1, B_MATRIX const &M2 )
{
		  // condizioni di conformitá
//	ASSERT(M1.Col() == M2.Row());
//	ASSERT(M1.first_el  == M2.first_el);

	// la matrice prodotto ha righe=M1, colonne=M2
	B_MATRIX P(M1.Row(), M2.Col(), M1.GetFirstEl());
        // pij = ai1*b1j+ai2*b2j+...+aik*bkj+...+ain*bnj
	for (int i = 0; i < M1.Row(); i++)
		for (int j = 0; j < M2.Col(); j++)
			for (int k = 0; k < M1.Col(); k++)
				P.SetValue(i, j, P.GetValue(i, j) + M1.GetValue(i, k) * M2.GetValue(k, j));
	return P;

}


B_MATRIX operator *(FLOAT const lambda, B_MATRIX const &M1)
{
	B_MATRIX P(M1.Row(), M1.Col(), M1.GetFirstEl());
		  // pij = lambda * mij
	for (int i = 0; i < M1.Row(); i++)
		for (int j = 0; j < M1.Col(); j++)
			P.SetValue(i, j, lambda * M1.GetValue(i, j));
	return P;
}


B_MATRIX operator *(B_MATRIX const &M1, FLOAT const lambda)
{
	return lambda * M1;
}


B_MATRIX operator / (B_MATRIX const &M1, FLOAT const lambda)
{
	return M1 * (1 / lambda);
}


B_MATRIX operator +(B_MATRIX const &M1, B_MATRIX const &M2)
{
        // condizioni di conformitá
//	ASSERT(M1.col  == M2.col && M1.row == M2.row) ;
//	ASSERT(M1.first_el  == M2.first_el);
		  // la matrice somma ha stesse righe e colonne
	B_MATRIX S(M1.row, M1.col, M1.first_el);
        // sij = aij + bij
	for (int i = 0; i < M1.row; i++)
		for (int j = 0; j < M2.col; j++)
				S.SetValue(i,
					   j,
						M1.GetValue(i, j) +
							M2.GetValue(i, j));
	return S;
}


B_MATRIX operator -(B_MATRIX const &M1, B_MATRIX const &M2)
{
        // condizioni di conformitá
//	ASSERT (M1.col  == M2.col && M1.row == M2.row);
//	ASSERT(M1.first_el  == M2.first_el);
  // la matrice differenza ha stesse righe e colonne
	B_MATRIX S(M1.row, M1.col, M1.first_el);
        // sij = aij + bij
	for (int i = 0; i < M1.row; i++)
		for (int j = 0; j < M2.col; j++)
				S.SetValue(i, j, M1.GetValue(i, j) - M2.GetValue(i, j));
	return S;
}


B_MATRIX B_MATRIX::Trasp ()
{
	B_MATRIX M(row, col);
        // aij = aji
	for (int i = 0; i < row; i++)
		for (int j = i; j < col; j++) {
			M.SetValue(i, j, GetValue(j, i));
			M.SetValue(j, i, GetValue(i, j));
                }
	return M;
}

// implementazione della classe SQRMATRIX

SQRMATRIX::SQRMATRIX() :
	B_MATRIX ()
{
}


SQRMATRIX::SQRMATRIX (int dim, int first) :
	B_MATRIX(dim, dim, first)
{
}


SQRMATRIX::SQRMATRIX (SQRMATRIX const &M1) :
	B_MATRIX(M1)
{
}


SQRMATRIX::SQRMATRIX (B_MATRIX &M) :
	B_MATRIX(M)
{
}


SQRMATRIX::~SQRMATRIX ()
{
}


SQRMATRIX & SQRMATRIX::operator = (B_MATRIX const &M1)
{
// richiama	MD_MATRIX &operator =(MD_MATRIX const &M1);
	D2_MATRIX::operator =(M1);

	return *this;
}


FLOAT SQRMATRIX::Det()
{
// calcola il determinante della matrice
	FLOAT det = 0;
	for (int i = 0; i < row; i++)
		;
	return det;
}


void SQRMATRIX::MakeUnit ()
{
	// la rende unitaria
	for (int j = 0; j < col; j++)
		for (int i = 0; i < row ; i++)
			if (i != j) 	SetValue(i, j, 0);
			else		SetValue(i, j, 1);
}


SQRMATRIX SQRMATRIX::Inversa() const
{

// metodo di Gauss-Jordan

	SQRMATRIX I(row, first_el);
	// la rende unitaria
	I.MakeUnit();

	// la matrice inversa inizializz. unitaria
	SQRMATRIX Inv = I;

	// inizia iterazione
	SQRMATRIX M;	// la matrice di lavoro
		  // la matrice originaria che si trasforma
	SQRMATRIX A = *this;

	FLOAT div, cor;
	for (int j = 0; j < col; j++) {
		// il divisore
		div = A.GetValue(j, j);
		if (div == 0) {
// inserire qui messaggio errore
//			MessageBox (NULL, "Div", "B_MATRIX", MB_OK);
			div = 1;
		}
		M = I;
		for (int i = 0; i < row; i++) {
			cor = A.GetValue(i, j);
			if (i == j)
				M.SetValue(i, j, 1 / div);
			else
				M.SetValue(i, j, - cor / div);
		}
                // la nuova A
		A = M * A;
		Inv = M * Inv;
	}
//	Inv.SetFirstEl(first_el);
	return Inv;
}


void SQRMATRIX::Del(int r, int c)
{
/*
// elimina la riga
	for (int i = r; i < row - 1; i++)
		for (int j = 0; j < col; j++)
			SetValue(i, j, GetValue(i + 1, j));
	for (int j = 0; j < col; j++)
		SetValue(row - 1, j, 0);

      // elimina la colonna
	for (int j = c; j < col - 1; j++)
		for (int i = 0; i < row; i ++)
			SetValue(i, j, GetValue(i, j + 1));
	for (int i = 0; i < col; i++)
		SetValue(i, col - 1, 0);


// dimnuisce la dimensione senza rilasciare la memoria
// tanto é poca cosa
	row--;
   col--;
*/

// elimina la riga
	for (int i = r; i < row - 1; i++)
		for (int j = 0; j < col; j++)
			SetValue(i, j, GetValue(i + 1, j));
	for (int j = 0; j < col; j++)
		SetValue(row - 1, j, 0);

      // elimina la colonna
	for (int j = c; j < col - 1; j++)
		for (int i = 0; i < row; i ++)
			SetValue(i, j, GetValue(i, j + 1));
	for (int i = 0; i < col; i++)
		SetValue(i, col - 1, 0);

	int new_dim = col - 1;
      // un pó faraginoso
	SQRMATRIX N (new_dim);
	for (int i = 0; i < new_dim; i++)
		for (int j = 0; j < new_dim; j++)
			N.SetValue(i, j, GetValue(i, j));

	*this = N;

}


B_MATRIX operator / (B_MATRIX const &M1, SQRMATRIX const &M2 )
{
	return M1 * M2.Inversa();
}


SQRMATRIX operator / (FLOAT const lambda, SQRMATRIX const &M1)
{
	B_MATRIX M = lambda * M1.Inversa();
	return M;
}

// implementazione della classe VECTOR

VECTOR::VECTOR () :
	B_MATRIX()
{
}


VECTOR::VECTOR (int n, int first) :
	B_MATRIX (n, 1, first)
{
}


VECTOR::VECTOR (VECTOR &v) :
	B_MATRIX(v)
{
}


VECTOR::VECTOR (B_MATRIX &M) :
	B_MATRIX(M)
{
}


VECTOR::~VECTOR ()
{
}


FLOAT& VECTOR::operator () (int i) const
{
	return B_MATRIX::operator()(i, first_el);
}


FLOAT& VECTOR::operator[](int i) const
{
	return B_MATRIX::operator()(i, first_el);
}

// implementazione della classe R_VECTOR

RVECTOR::RVECTOR () :
	B_MATRIX()
{
}


RVECTOR::RVECTOR (int n, int first) :
	B_MATRIX (1, n, first)
{
}


RVECTOR::RVECTOR (RVECTOR &v) :
	B_MATRIX((B_MATRIX)v)
{
}


RVECTOR::RVECTOR (B_MATRIX &M) :
	B_MATRIX(M)
{
// inverte righe e colonne
	col = M.Row() ;
	row = M.Col() ;
}


RVECTOR::~RVECTOR ()
{
}


FLOAT& RVECTOR::operator () (int i) const
{
	return B_MATRIX::operator()(first_el, i);
}


FLOAT& RVECTOR::operator[](int i) const
{
	return B_MATRIX::operator()(first_el, i);
}
// implementazione delle funzioni friend


int Scambia (int s, int cl, B_MATRIX &M, B_MATRIX &V)
{
		  // per gli scambi
	int row = M.Row();

	VECTOR ui_f(row);
	int nf = 0;
// s é la colonna da scambiare
// cl la colonna da saltare

	// se é successivo a s scambia e esci
	for (int i = s + 1; i < row; i++) {
		if (i == cl) continue;
		if (M.GetValue(i, s) != 0)
			ui_f[nf++] = i;
	}	// for i ...

	// se é possibile lo scambio prendi a caso
        // uno di quelli possibili
	if (nf > 0) {
		int rnd = rand() % nf;
		// lo puoi scambiare
		int g = (int) ui_f[rnd];

		M.swap(g, s);
		V.swap(g, s);
		return s;
	}	// if nf > 0

	// altrimenti vedi se era prima
	nf = 0;
	for (int i = 0; i < s; i++) {
		if (i == cl) continue;
		if (M.GetValue(i, s) != 0 && M.GetValue(s, i) != 0) {
			// lo puoi scambiare
			M.swap(i, s);
			V.swap(i, s);
			return s;
		}
		else if (M.GetValue(i, s) != 0)
			ui_f[nf++] = i;
	}	// for i ...

// se comunque é presente
	if (nf > 0) {
		// scegliene uno a caso
		int rnd = rand() % nf;
		// lo puoi scambiare
		int g = (int) ui_f[rnd];

		M.swap(g, s);
		V.swap(g, s);
		return --g;
	}	// if nf > 0
	return -1;
}


bool Condiziona (B_MATRIX &M, B_MATRIX &V)
{

    // scambia le righe della matrice M in maniera da avere
    // tutta la diagonale diversa da zero
    // ritorna TRUE se ha successo

	int row = M.Row();
	int col = M.Col();

	const int MAX_COUNT = col * 1000;

	// un vettore di int per memorizzare il numero di
	// variabile per ogni equazione

	VECTOR ui_nv( row ) ;
	for (int i = 0; i < row; i++) {
		ui_nv[i] = 0;
		for (int j = 0; j < col; j++)
			if (M.GetValue(i, j)) ui_nv[i]++;
	}   // end for i ..

	// riordina in modo da avere il minimo
	// numero di variabili in ordine crescente
	for (int j = 0; j < col; j++) {

		int n = -1, no = col;
		for (int i = 0; i < row; i++)
			if (M.GetValue(i, j) && ui_nv[i] <= no) {
				n = i ;
				no = (int) ui_nv[i];
         }	// end if M.GetValue ...
		if (n < 0) return false;

		if ( n != j && j ) {
			M.swap(j, n);
			V.swap(j, n);
		}

	}	// end j

	// tutte le colonne (variabili)
	int j = 0;
	// not more than MAX_COUNT times
	int count = 0;
	int cl = -1;

	while (j < col) {
		if (count > MAX_COUNT) {
//			char s[40];
//			sprintf(s, "%d tentativi senza successo", MAX_COUNT);
//			MessageBox(NULL, s, "B_MATRIX", MB_OK);
			return false;
		}	// if count ...
		if (M.GetValue(j, j) == 0) {
			int oldj = j;
			if ((j = Scambia(j, cl, M, V)) < 0) {
// togliendo le seguenti due righe il programma raggiunge la
// convergenza piu spesso
//				MessageBox(NULL, "B_MATRIX", "Scambia < 0", MB_OK);
//				return FALSE;
			}
			if (j < oldj)
				cl = oldj;
			else cl = -1;
		}	// if M.GetValue ...
		j++;
		count++;
	} // while j

	return true ;

}

