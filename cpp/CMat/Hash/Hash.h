#ifndef _hash_
#define _hash_

#include <string.h>
#include <stdlib.h>

#define TABSIZE 100

typedef int	BOOL;
#define FALSE 	0
#define TRUE 	1

// lista linkata
class LINKLIST {
	friend class LISTITERATOR;
	friend class INTITERATOR;

	LINKLIST *next;
	void *data;
public:
	LINKLIST(void *data, size_t size) ;
	~LINKLIST();

	LINKLIST *Add(void *data, size_t size);
};

// iteratore della lista linkata
class LISTITERATOR {
protected:
	LINKLIST *ref;
public:
	LISTITERATOR(LINKLIST *lst);

	BOOL AtEnd();

	operator void * () ;
	LISTITERATOR *operator ++() ;
};

// lista linkata di interi
class INTLIST : public LINKLIST{
public:
	INTLIST(int i) ;

	INTLIST *Add(int i);
};

// iteratore della lista linkata di interi
class INTITERATOR : public LISTITERATOR{
public:
	INTITERATOR(INTLIST *lst);

	operator int () ;
	INTITERATOR *operator ++() ;
};

struct Tab { 		// elemento della tabella

		struct Tab *next;		// prossimo elemento della catena

		char *key;		// chiave
      void *data;		// puntatore generico ad una struttura
	   			// di dati associati
} ;



class HashClass
{
friend class HashIterator;

   const int HASHSIZE;
   Tab *HashTab[TABSIZE];

   unsigned Hash(const char *s) const;	// calcola il codice hash
   Tab *LookUp(const char *s) const;	// verifica se é un membro

public:

	HashClass(int hashsize = 100);
   ~HashClass();

	bool Exist (const char *key) const ;
	void  * Insert(const char *key, const void *data, size_t size);
	void * GetData(const char *key) const;
	void *Cancel(const char *key);
	void Clear();
	void Print() const;
};

class HashIterator
{

	HashClass *tbl;
	int count;

	Tab *tab;
	bool eof;

public:

	HashIterator (HashClass *hash);

	void *GetData () const;
	char * GetKey () const;

	void First() ;
	void Next() ;
	bool Eof() const;

	void *Clear () ;	// elimina tutti gli elementi della tabella

} ;

struct TNode {	// il nodo dell'albero binario
      char *key;	// il codice di accesso
      int count;	// numero di occorrenze

      void *data;	// la struttura dati assocciata

      TNode *Parent;	// nodo padre
      TNode *Left;	// figlio di sinistra
      TNode *Right;	// figlio di destra
} ;

class TreeClass
{
protected:

   TNode *root ;

   TNode *CreateLeaf(TNode *parent, const char *key, const void *data, size_t size);

   virtual TNode *AddTree(TNode *p, const char *key, const void *data, size_t size);
   TNode *Member(TNode *p, const char *key);

   void Print(TNode *p);

   BOOL Remove(BOOL IsRight, TNode *Father, TNode *p, const char *key);
   TNode *RemMax(BOOL IsRight, TNode *Father, TNode *p);

   void Clear(TNode *p);
public:

   TreeClass();
   virtual ~TreeClass();

   void Insert(const char *key, const void *data = NULL, size_t size = 0);
   void Print();
   void *GetData(const TNode *p)
   	{return p->data;}
   void *GetData(const char *key);
   BOOL Remove(const char *key);
   void Clear();
};

class ModBinTree : public TreeClass
{

   virtual TNode *AddTree(TNode *p, const char *key, const void *data, size_t size);

   TNode *GetLower(TNode *t);
   TNode *GoUp(TNode *t);

public:

   ModBinTree() : TreeClass()
	{}
   ~ModBinTree()
   	{}

   TNode *Member(const char *key)
		{return TreeClass::Member(root, key);}

   void ChangeData(TNode *p, void *data, size_t size)
		{memcpy(p->data, data, size);}

   TNode *Insert(char *key, void *data = NULL, size_t size = 0);

   TNode *GetNode(TNode *t);
};

#endif
