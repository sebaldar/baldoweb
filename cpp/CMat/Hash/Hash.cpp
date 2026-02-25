#ifndef _hash_
    #include "Hash.h"
#endif

/*
22/03/01
	procedura Hash::Clear
   	sostituito
				delete np->data;
				delete np->key;
      con
				delete [] np->data;
				delete [] np->key;

*/

/***********************************/
/*** Dichiarazione lista linkata ***/
/***********************************/

LINKLIST::LINKLIST (void *the_data, size_t size)
{
	data = (void*) new char[size];
	memcpy(data, the_data, size);
}

LINKLIST::~LINKLIST()
{
// distrugge tutta la lista
	if (next)
		delete next;
	delete [] (char *) data;
}

LINKLIST * LINKLIST::Add(void *the_data, size_t size)
{
// inserisce sempre in seconda posizione
	LINKLIST *new_list = new LINKLIST(the_data, size);
	new_list->next = next;
	return next = new_list;
}

/*********************************************/
/*** Dichiarazione iteratore lista linkata ***/
/*********************************************/

LISTITERATOR::LISTITERATOR(LINKLIST *lst) :
	ref(lst)
{
}

BOOL LISTITERATOR::AtEnd()
{
	return ref == NULL;
}

LISTITERATOR::operator void * ()
{
	return ref->data;
}

LISTITERATOR *LISTITERATOR::operator ++()
{
	ref = ref->next;
	return this;
}

/*********************************************/
/*** Dichiarazione lista linkata di interi ***/
/*********************************************/

INTLIST::INTLIST(int i) :
	LINKLIST(&i, sizeof(int))
{
}

INTLIST * INTLIST::Add(int i)
{
	return (INTLIST *) LINKLIST::Add(&i, sizeof(int));
}

/*******************************************************/
/*** Dichiarazione iteratore lista linkata di interi ***/
/*******************************************************/

INTITERATOR::INTITERATOR(INTLIST *lst) :
	LISTITERATOR(lst)
{
}

INTITERATOR::operator int ()
{
	int *value = (int *)ref->data;;
	return *value;
}

INTITERATOR *INTITERATOR::operator ++()
{
	ref = ref->next;
	return this;
}

/*********************************/
/*** Dichiarazione classe hash ***/
/*********************************/

HashClass::HashClass (int hashsize) :
	HASHSIZE(hashsize)
{

	for (int i = 0; i < HASHSIZE; i++)
		HashTab[i] = NULL;
}

HashClass::~HashClass()
{
	Clear();
}

// calcola il codice Hash
unsigned HashClass::Hash(const char *s) const
{
	unsigned ashvalue = 0;
	for (; *s != '\0'; s++)
    	ashvalue = *s + 31 * ashvalue;
	return ashvalue % HASHSIZE;
}

bool HashClass::Exist(const char *key) const
{
    if (LookUp(key)) return true;
    else return false;
}


// LookUp cerca la chiave in HashTab
// ritorna un puntatore alla struttura o NULL se non trovato
Tab *HashClass::LookUp(const char *s) const
{
	Tab *np;

	for (np = HashTab[Hash(s)]; np != NULL; np = np->next)
		if (strcmp(s, np->key) == 0)
			return np;	// trovato
	return NULL;
}

// rimuove un elemento dalla tabella
void *HashClass::Cancel(const char *key)
{
	unsigned hashvalue = Hash(key);

	Tab *np;
	Tab *previous;	// elemento precedente

	previous = HashTab[hashvalue];

	for (np = HashTab[hashvalue]; np != NULL; np = np->next) {

		if (strcmp(key, np->key) == 0) {

			if (previous == np)
				HashTab[hashvalue] = np->next;
			else
				previous->next = np->next;
			delete (char *) np->data;
			delete np->key;
			delete np;
			return previous;
		}

		previous = np;

	}

	return NULL;

}

void HashClass::Print() const
{
}

// install inserisce data in HashTab
void *HashClass::Insert(const char *key, const void *data, size_t size)
{
	Tab *np = LookUp(key);
	if (!np)	// non trovato
	{
		np = new Tab;
		np->key = new char[strlen(key) + 1];
		strcpy(np->key, key);
		unsigned hashvalue = Hash(key);

		np->next = HashTab[hashvalue];
		HashTab[hashvalue] = np;
	}
	else	// il text é giá in HashTab
		  delete (char *) np->data;

	np->data = (void *) new char [size];

	memcpy(np->data, data, size);

	return np->data;
}

void *HashClass::GetData(const char *s) const
{
	Tab *np = LookUp(s);

	if (np) return np->data;
	else return NULL;
}

// rimuove tutti gli elementi
void HashClass::Clear()
{

	Tab *np, *tb;

	for (int i = 0; i < HASHSIZE; i++) {
		if (HashTab[i]) {
			for (np = HashTab[i]; np != NULL; np = tb) {
				delete [] (char *) np->data;
				delete [] np->key;
				tb = np->next;
				delete np;
			}
			HashTab[i] = NULL;
		}
	}
}

/**********************************************
**** Implementazione della classe HashIterator ***
***********************************************/

HashIterator::HashIterator (HashClass *hash) :
    tbl(hash),
    count(0),
    tab(NULL),
    eof(true)
{
}

void HashIterator::First()
{
    for (count = 0; count < tbl->HASHSIZE; count++) {
//        #pragma warn -pia // -w-pia	Possibly incorrect assignment (Default ON)
		if ((tab = tbl->HashTab[count])) {//        #pragma warn +pia // -w-pia	Possibly incorrect assignment (Default ON)
			for (; tab != NULL; tab = tab->next) {
                if (tab) {
                    eof = false;
                    return;
                }
			}
		}
	}
    eof = true;
}

void HashIterator::Next()
{
    if (tab->next) {
        tab = tab->next;
        return;
    }
    for (count += 1; count < tbl->HASHSIZE; count++) {
//        #pragma warn -pia // -w-pia	Possibly incorrect assignment (Default ON)
		if ((tab = tbl->HashTab[count])) {
//        #pragma warn +pia // ripristina warning
            eof = false;
            return;
         }
	}
    eof = true;
}

void *HashIterator::GetData () const
{
    return tab->data ;
}

char * HashIterator::GetKey () const
{
    return tab->key ;
}

bool HashIterator::Eof() const
{
    return eof;
}

void *HashIterator::Clear ()
{
	tbl->Clear () ;
    return NULL;
}

