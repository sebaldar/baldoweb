#ifndef __xml__
#define __xml__

#include <iostream>
#include <vector>

#ifndef _field_h
	#include <field.hpp>
#endif




class XMLNode ;
class XMLDocument ;
class XMLDeclaration ;
class XMLDocType ;
class XMLScript ;
class XMLElement ;
class XMLText ;

//typedef std::vector<XMLElement *> NODES ;
using NODES = std::vector<XMLElement*>;

class XMLNode {

	friend class PARSER ;

public :

	enum TYPE
	{
		DOCUMENT,
		ELEMENT,
		COMMENT,
		UNKNOWN,
		TEXT,
		DECLARATION,
		DOCTYPE,
		SCRIPT
	} ;

private:



	std::string printXML ( const XMLNode * curEl, int tab = 0 ) const ;
	XMLElement * searchId ( const std::string &id ) const ;
	XMLNode * searchTagName ( const std::string &tag, NODES & v) ;

protected :
		
	TYPE fType ;

	/** The meaning of 'value' changes for the specific type of
		XMLNode.
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
	*/
	std::string fValue ;

	/* in nodo genitore */
	XMLNode * fParent ;
	/* il nodo figlio */
	XMLNode * fChild ;
	/* il nodo fratello */
	XMLNode * fSibling ;

	// clona node copiando il nodo stesso
	// e i suoi figli
	// non clona i nodi sibling
	// ritorna un puntatore al nodo clonato
	static XMLNode * clone ( XMLNode * node ) ;

public :


	XMLNode ( TYPE type, const std::string & value ) ;
	virtual ~XMLNode () ;

	XMLElement * createElement ( std::string tag_name) ;
	XMLText * createTextNode ( std::string value ) ;

	std::string printXML ( int tab = 0 ) const ;

	TYPE type () const { return fType ; }
	TYPE nodeType () const { return fType ; }

	std::string value () const { return fValue ; }
	std::string tagName () const { return fValue ; }

	void setValue ( std::string a_value ) { fValue = a_value ; }
	
	bool hasChilds () const { return fChild ? true : false ; }
	XMLNode * firstChild () const { return fChild ; }
	
	XMLElement * getElementById ( const std::string &id ) ;
	NODES  getElementsByTagName ( const std::string & tag ) ;
	XMLElement * getFirstElementByTagName ( const std::string & tag ) ;
	
	XMLNode * nextSibling () const { return fSibling ; }
	
	
	/*
		inserisce node come ultimo sibling.
		se node ha nodi sibling questi rimangono tali e quali
	*/
	XMLNode * appendChild ( XMLNode * node ) ;

	/* questa versione di appendChild fa una copia 
	di tutta la gerarchia di node ed usa questa */
	XMLNode * appendChild ( XMLNode &node ) ;

	XMLNode * insertBefore( XMLNode &new_node, 	XMLNode * existing_node ) ;
	XMLNode * removeChild ( XMLNode * to_be_removed ) ;
	XMLNode * removeNode ( );
	
	XMLNode * clone ( ) { return clone ( this ); } ;
//	XMLNode * parent () const { return fParent ; }
	XMLNode * parentNode () const { return fParent ; }
	XMLNode * childNode () const { return fChild ; }
	XMLNode * siblingNode () const { return fSibling ; }
	
	void setChild ( XMLNode * node ) { fChild = node; };
	void setSibling ( XMLNode * node ) { fSibling = node; };
	void setParent ( XMLNode * node ) { fParent = node; };

} ;

class XMLDocument : public XMLNode {


public :

	XMLDocument ( ) ;
	XMLDocument ( const std::string & file_name ) ;
	~XMLDocument () ;

	void loadFromString ( const std::string & buffer ) ;
	void loadTagFromPos ( const std::string & buffer, size_t pos ) ;

	void loadFromFile ( const std::string & file_name ) ;
	void saveToFile ( const std::string & file_name ) ;

	void parse ( const std::string & src ) ;
	
	XMLElement * rootElement () const ; 
	XMLNode * rootNode () const ; 
	
	class FileName {
			
			XMLDocument * doc;
		
		public :
			
			FileName ( XMLDocument * _doc) : doc (_doc) {}
			
			operator std::string () { return doc->fValue; } 
			operator const char * () { return doc->fValue.c_str(); } 
			
			FileName & operator = ( std::string file ) {
				doc->fValue = file ;
				return ( *this ) ;
			}
			
	} fileName ;

} ;

class XMLAttribute {

friend class XMLElement ;

	XMLElement * parent ;

	std::string fName ;
	std::string fValue ;


public :

	XMLAttribute ( XMLElement *parent, const std::string &name, const std::string &value ) ;
	~XMLAttribute ( ) ;

	XMLAttribute * next ;
	std::string name () { return fName ; }
	std::string value () { return fValue ; }

};

class XMLElement : public XMLNode {


	XMLAttribute * attribute ;

public :

	XMLElement ( const std::string & tag_value ) ;
	~XMLElement () ;

	XMLAttribute * setAttribute ( const std::string &name, const std::string &value ) ;
	TFIELD getAttribute ( const std::string &name ) const ;
	// cerca nei figli l'elemento con tag name tag e ritorna
	// il valore dell'attributo name, se esiste, altrimenti la stringa vuota
	TFIELD getAttributeFromTagName ( const std::string &tag, const std::string &name ) ;
	
	XMLAttribute * getAttributes () { return attribute ; }
	std::string text () const ;
	
	
	bool hasAttributes () { return attribute ? true : false ;}
	
	std::string innerHTML () ;

} ;

class XMLText : public XMLNode {


public :

	XMLText ( const std::string & text_value ) ;
	~XMLText () ;

} ;

class XMLComment : public XMLNode {


public :

	XMLComment ( const std::string & text_value ) ;
	~XMLComment () ;

} ;

class XMLDeclaration : public XMLNode {


public :

	XMLDeclaration ( const std::string & declaration ) ;
	~XMLDeclaration () ;

} ;

class XMLDocType : public XMLNode {


public :

	XMLDocType ( const std::string & doc_type ) ;
	~XMLDocType () ;

} ;

class XMLScript : public XMLNode {


public :

	XMLScript ( const std::string & script ) ;
	~XMLScript () ;

} ;


#endif

