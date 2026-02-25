#ifndef _matrix_
#define _matrix_

#include <assert.h>


// dichiarazione della classe base MD_MATRIX
// contenitore delle classi di matrici multidimensionali

#include <iostream>

typedef float FLOAT;

class MD_MATRIX
{
protected:

	FLOAT *m;

	int first_el;
	int dim;

	MD_MATRIX() ;
	MD_MATRIX (int d, int first = 0) ;

	virtual void print() ;
	virtual const char * GetClassName() ;

	MD_MATRIX &operator =(MD_MATRIX const &M1);
	void MakeEqual (MD_MATRIX const &M1) ;  // rendi this = M1

public:

	virtual int Dim() const ;

	void SetFirstEl(int first) ;
	int GetFirstEl () const ;

	MD_MATRIX (MD_MATRIX const &M1) ;
	virtual ~MD_MATRIX () ;
};

// funzioni inline classe MD_MATRIX
inline void MD_MATRIX::print()
{
	std::cout << "_____________________" << std::endl;
	std::cout << "Class " << GetClassName() << std::endl;
}

inline const char * MD_MATRIX::GetClassName()
{
	return "MD_MATRIX";
}

inline int MD_MATRIX::GetFirstEl () const
{
	return first_el ;
}

// dichiarazione della classe D1_MATRIX
// matrice 1-dimensionale
class D1_MATRIX : public MD_MATRIX
{
public:
	D1_MATRIX () ;
	D1_MATRIX (int d, int first = 0);
	D1_MATRIX (D1_MATRIX &M1);
	~D1_MATRIX () ;

	void print() ;
	const char * GetClassName() ;

	FLOAT &operator () (int i) const;
	FLOAT& operator[](int i) const;
};

// dichiarazione della classe D2_MATRIX
// matrice 2-dimensionale
class D2_MATRIX : public MD_MATRIX
{
protected:
	int row, col;
public:
	D2_MATRIX () ;
	D2_MATRIX (int i, int j, int first = 0);
	D2_MATRIX (D2_MATRIX const &M1);
	~D2_MATRIX () ;

	void print() ;
	const char * GetClassName() ;

	FLOAT &operator () (int i, int j) const;
	D2_MATRIX &operator =(D2_MATRIX const &M1);
};

class SQRMATRIX;
class VECTOR;

// dichiarazione della classe B_MATRIX
class B_MATRIX : public D2_MATRIX
{
protected:
public:
	B_MATRIX () ;
	B_MATRIX (int r, int c, int first = 0);
	B_MATRIX (B_MATRIX const &M1);
	~B_MATRIX () ;

	FLOAT GetValue(int r, int c) const ;	// inline
	void SetValue(int r, int c, FLOAT val) ;    // inline

	const char * GetClassName() ;

	char *sprint(char *str);
	void swap (int i, int j);

	int Col() const ;	// inline
	int Row() const ;     // inline

	virtual void Del (int, int);
	B_MATRIX Trasp();


	// dichiarazione funzioni friend
	friend B_MATRIX operator *  (B_MATRIX const &m1, B_MATRIX const &m2);
	friend B_MATRIX operator *  (FLOAT const lambda, B_MATRIX const &m1);
	friend B_MATRIX operator *  (B_MATRIX const &m1, FLOAT const lambda);
	friend B_MATRIX operator /  (B_MATRIX const &m1, FLOAT const lambda);
	friend B_MATRIX operator +  (B_MATRIX const &m1, B_MATRIX const &m2);
	friend B_MATRIX operator -  (B_MATRIX const &m1, B_MATRIX const &m2);

	friend bool Condiziona  (B_MATRIX &, B_MATRIX &);
	friend int Scambia  (int s, int cl, B_MATRIX &M, B_MATRIX &V);


};

// funzioni inline classe B_MATRIX
inline FLOAT B_MATRIX::GetValue(int r, int c) const
{
	return m[r * col + c];
}

inline void B_MATRIX::SetValue(int r, int c, FLOAT val)
{
	m[r * col + c] = val;
}

inline int B_MATRIX::Col() const
{
	return col;
}

inline int B_MATRIX::Row() const
{
	return row;
}

inline const char * B_MATRIX::GetClassName()
{
	return "B_MATRIX";
}

// dichiarazione della classe SQRMATRIX
class SQRMATRIX : public B_MATRIX
{
public:

	SQRMATRIX() ;
	SQRMATRIX (int dim, int first = 0) ;
	SQRMATRIX (SQRMATRIX const &M1) ;
	SQRMATRIX (B_MATRIX &M1) ;
	~SQRMATRIX () ;

	const char * GetClassName() ;
	int Dim() const ;	// inline

	FLOAT Det();
	SQRMATRIX Inversa() const;

	virtual void Del (int, int);
	void MakeUnit();


	friend B_MATRIX operator / (B_MATRIX const &m1, SQRMATRIX const &m2);
	friend SQRMATRIX operator / (FLOAT const lambda, SQRMATRIX const &m1);


	SQRMATRIX &operator = (B_MATRIX const &M1);
};

// funzioni inline classe SQRMATRIX
// dichiarazione della classe VECTOR
inline	int SQRMATRIX::Dim() const
{
	return col;
}

inline	const char * SQRMATRIX::GetClassName()
{
	return "SQRMATRIX";
}

// dichiarazione della classe VECTOR
class VECTOR : public B_MATRIX
{
public:
// Vettore é anche un ColVect
	VECTOR () ;
	VECTOR (int n, int first = 0) ;
	VECTOR (VECTOR &v) ;
	VECTOR (B_MATRIX &M) ;
	~VECTOR () ;

	const char * GetClassName() ;

	FLOAT &operator () (int i) const;
	FLOAT& operator[](int i) const;

	VECTOR &operator =(B_MATRIX const &M1);
};

// dichiarazione inline classe VECTOR
inline const char * VECTOR::GetClassName()
{
	return "VECTOR";
}

inline VECTOR & VECTOR::operator =(B_MATRIX const &M1)
{
// verifica che la matrice M1 sia un vettore
//    assert(M1.Col()) ;

    MakeEqual(M1) ;

	return *this;
}

// dichiarazione della classe R_VECTOR
// vettore di una riga
class RVECTOR : public B_MATRIX
{
public:
// Vettore é anche un ColVect
	RVECTOR () ;
	RVECTOR (int n, int first = 0) ;
	RVECTOR (RVECTOR &v) ;
	RVECTOR (B_MATRIX &M) ;
	~RVECTOR () ;

	const char * GetClassName() ;

	FLOAT &operator () (int i) const;
	FLOAT& operator[](int i) const;
};


// dichiarazione inline classe RVECTOR
inline const char * RVECTOR::GetClassName()
{
	return "RVECTOR";
}

typedef B_MATRIX		F_MATRIX;
typedef B_MATRIX		D_MATRIX;

typedef SQRMATRIX		F_SQRMATRIX;
typedef SQRMATRIX		D_SQRMATRIX;

typedef VECTOR			F_VECTOR;
typedef VECTOR			D_VECTOR;

typedef F_VECTOR 		VETTORE ;
typedef F_SQRMATRIX		QMATRICE;
typedef F_MATRIX		MATRICE ;

#endif
