#include <sstream>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "parser.hpp"

#ifndef __xml__
	#include <xml.hpp>
#endif

/* implementazione classe XMLNode */
XMLNode::XMLNode ( TYPE type, const std::string & value ) :
	fType ( type ), fValue ( value ), fParent (NULL),
	fChild ( NULL ), fSibling ( NULL )
{
}

XMLNode::~XMLNode ()
{
	if ( fChild ) delete fChild ;
	if ( fSibling ) delete fSibling ;
}

std::string XMLNode::printXML ( int tab ) const
{
    return printXML ( this, tab );
}

std::string XMLNode::printXML ( const XMLNode * curEl, int tab ) const
{

	std::string loc_print = "";
	std::string p_tab = "";

	for ( int i = 0; i < tab; ++i )
		p_tab += "\t";

	if ( curEl ) {

		TYPE type = curEl->type () ;

		switch ( type ) {
			case ELEMENT : {

					XMLElement * el = ( XMLElement *) curEl;
					std::string tagName = curEl->value ();

					loc_print += p_tab + "<" + tagName ;

					for ( XMLAttribute * att = el->getAttributes() ; att; att = att->next ) {
						std::string name = att->name ();
						std::string value = att->value ();
						if ( value != "" )
							loc_print += " " + name + "=\"" + value + "\"";
					}

                    if ( curEl->hasChilds() ) {
                        loc_print += ">\n";

						loc_print += printXML ( curEl->fChild, tab + 1 );

                        // chiude il tag
                        if ( tagName != "" ) {
                            loc_print += p_tab + "</" + tagName + ">\n";
                        }

                    }
                    else {
                        if ( tagName == "br" || tagName == "img" || tagName == "link" )
							loc_print += " />\n";
						else
                            loc_print += "></" + tagName + ">\n";
					}

				}

				break;
			case TEXT : {
				size_t p;
				std::string str = curEl->value ();
				while ( (p = str.find( "\r" )) != std::string::npos ) {
					str.replace( p, 1, "\n" );
				}
				loc_print = p_tab + curEl->value () + "\n";
			}
				break;
			case  DOCUMENT :
				if ( curEl->hasChilds() )
					loc_print += printXML ( curEl->fChild, tab );
				break;
			case  COMMENT :
				loc_print = p_tab + "<!-- " + curEl->value () + "-->\n";
				break;
			case  UNKNOWN :
				break;
			case  DECLARATION :
				break;
			case  DOCTYPE :
				loc_print = "<!DOCTYPE " + curEl->value () + ">\n";
				break;
			case  SCRIPT : {
				size_t p;
				std::string str = curEl->value ();
				while ( (p = str.find( "\r" )) != std::string::npos ) {
					str.replace( p, 1, "\n" );
				}
				loc_print = p_tab + "<?php\n" + str + "?>\n";
			}
				break;
			default :
				break;
		}

		if ( curEl->fSibling )
       		loc_print += printXML ( curEl->fSibling, tab );

	}

	return loc_print;

}

XMLElement * XMLNode::createElement ( std::string tag_name )
{
	return ( new XMLElement ( tag_name ) );
}

XMLText * XMLNode::createTextNode ( std::string value )
{
	return ( new XMLText ( value ) );
}


XMLElement * XMLNode::getElementById ( const std::string& id )
{

	switch ( fType ) {
		case DOCUMENT :
			return ( ( XMLDocument * ) this )->rootElement()->getElementById ( id );
		case ELEMENT :
			return searchId( id );
/*
                case COMMENT : UNKNOWN : TEXT :
                                DECLARATION : TYPECOUNT :
                        return NULL;
                        break;
*/
		default :
			return NULL;
	};

}

// Funzione pubblica: restituisce tutti gli elementi con il tag specificato
NODES XMLNode::getElementsByTagName(const std::string &tag) {
    NODES result;
    searchTagName(tag, result);  // chiama la funzione ricorsiva
    return result;
}

// Funzione privata ricorsiva: popola il vettore 'v' con tutti gli elementi che corrispondono al tag
XMLNode* XMLNode::searchTagName(const std::string &tag, NODES &v) {
    if (fType == ELEMENT) {
        if (((XMLElement*)this)->fValue == tag) {
            v.push_back((XMLElement*)this);
        }
    }

    if (fChild) fChild->searchTagName(tag, v);    // esplora i figli
    if (fSibling) fSibling->searchTagName(tag, v); // esplora i fratelli

    return nullptr; // il return non serve realmente
}
XMLElement * XMLNode::getFirstElementByTagName ( const std::string & tag )
{
	NODES  nodes = getElementsByTagName ( tag ) ;
	if ( nodes.size () > 0 )
		return nodes.at( 0 );
	else
		return NULL;
}

XMLElement * XMLNode::searchId ( const std::string & id) const
{


	if ( fType == ELEMENT) {
		std::string val = ( ( XMLElement *) this)->getAttribute( "id" );
		if ( val == id ) {
			return ( XMLElement * ) this;
		}
	}

	XMLNode *nd = fChild ;
	if (nd) {
		XMLElement *t = nd->searchId(id);
		if (t) return ( t );
	}

	nd = fSibling ;
	if (nd) {
		XMLElement *t = nd->searchId(id);
		if (t) return (t);
	}

	return ( NULL );

}

XMLNode * XMLNode::appendChild ( XMLNode * node )
{

	node->fParent = this ;
	if ( !fChild ) return fChild = node ;

	XMLNode * last_node = fChild ;
	while ( last_node->fSibling )
		last_node = last_node->fSibling ;

	return last_node->fSibling = node ;

}

XMLNode * XMLNode::appendChild ( XMLNode &node )
{

	XMLNode * n = clone ( &node ) ;
	n->fParent = this ;
	// modifica il parente dei nodi sibling
	XMLNode * s = n->fSibling;
	while ( s ) {
		s->fParent = this ;
		s = s->fSibling;
	}

	if ( !fChild ) return fChild = n ;

	XMLNode * last_node = fChild ;
	while ( last_node->fSibling )
		last_node = last_node->fSibling ;

	return last_node->fSibling = n ;

}

XMLNode * XMLNode::insertBefore( XMLNode &new_node, 	XMLNode * existing_node )
{

	XMLNode * the_child = fChild ;
	XMLNode * before_node = NULL ;
	while ( the_child && the_child != existing_node ) {
		before_node = the_child ;
		the_child = the_child->fSibling ;
	}

	if ( the_child != existing_node )
		return NULL ;

	XMLNode * n = clone ( &new_node ) ;
	n->fParent = this ;
    if ( before_node ) {
        n->fSibling = before_node->fSibling;
        return before_node->fSibling = n;
    }
    else {
        n->fSibling = fChild;
        return fChild = n ;
    }


}

XMLNode * XMLNode::removeNode ( )
{

	// va al genitore
	XMLNode * p = fParent ;
	XMLNode *previous = NULL ;
	if ( p ) {
		for ( XMLNode * n = p->fChild; n; n = n->fSibling ) {
			if ( this == n ) {
				if ( previous ) {
					previous->fSibling = n->fSibling;
				}
				else
					p->fChild = n->fSibling ;

				// scollego il nodo da cancellare
				n->fSibling = NULL ;
                delete n;

				return previous ;

			}

			previous = n ;

		}

		return NULL;

	}
	else
		return NULL;

}

XMLNode * XMLNode::removeChild ( XMLNode * to_be_removed )
{

	// va al genitore
	if ( to_be_removed ) {
        return to_be_removed->removeNode ();
	}
	else
		return NULL ;

}

/* clona il nodo e la sua parentela */
XMLNode * XMLNode::clone ( XMLNode * node )
{

	XMLNode * cl ;
	switch ( node->fType ) {

		case DOCUMENT :
			cl = new XMLDocument ( node->value () ) ;
			break;
		case ELEMENT : {
			cl = new XMLElement ( node->value () ) ;
			XMLAttribute * attr = ( ( XMLElement * ) node)->getAttributes () ;
			while ( attr ) {
				( ( XMLElement * ) cl)->setAttribute ( attr->name (), attr->value () );
				attr = attr->next ;
			}
		}
			break;
		case COMMENT :
			cl = new XMLComment ( node->value () ) ;
			break;
		case UNKNOWN :
			break;
		case TEXT :
			cl = new XMLText ( node->value () ) ;
			break;
		case DECLARATION :
			break;
		case DOCTYPE :
			break;
		case SCRIPT :
			cl = new XMLScript ( node->value () ) ;
			break;

	}

	if ( node->fChild ) {
		cl->fChild = clone ( node->fChild )  ;
		cl->fChild->fParent = cl ;
		XMLNode * s = cl->fChild;
		for ( XMLNode * n = node->fChild->fSibling; n; n = n->fSibling ) {
			s->fSibling = clone ( n ) ;
			s->fSibling->fParent = cl ;
			s = s->fSibling ;
		}
	}
//	if ( node->fSibling ) cl->fSibling = clone ( node->fSibling )  ;

	return cl ;

}

/* implemantazione classe XMLAttribute */
XMLAttribute::XMLAttribute ( XMLElement *_parent, const std::string &_name, const std::string &_value ) :
	parent ( _parent ), fName ( _name ), fValue ( _value ), next ( NULL )
{
}

XMLAttribute::~XMLAttribute (  )
{
	if ( next ) delete next ;
}

/* implemantazione classe XMLDocument */
XMLDocument::XMLDocument ( ) :
	XMLNode ( DOCUMENT, "" ), fileName ( this )
{
}

XMLDocument::XMLDocument ( const std::string & file_name ) :
	XMLNode ( DOCUMENT, file_name ), fileName ( this )
{

	loadFromFile ( file_name ) ;

}

XMLDocument::~XMLDocument ()
{
}

XMLElement * XMLDocument::rootElement() const {
	for (XMLNode * node = fChild; node; node = node->nextSibling()) {
		if (node->nodeType() == ELEMENT)
			return (XMLElement*) node;
	}
	return NULL;
}

XMLNode * XMLDocument::rootNode () const
{

	// il primo figlio di Document che è ELEMENT,TEXT o SCRIPT
	// può essere root, anche se normalmente è ELEMENT
	for ( XMLNode * node = fChild; node ; node = node->nextSibling () )
		switch ( node->nodeType() ) {
			case ELEMENT :
			case TEXT :
			case SCRIPT :
				return node ;
			default :
				;
		}

	return NULL ;

}

void XMLDocument::loadFromString ( const std::string & buffer )
{
        
	PARSER parse ( buffer ) ;

	fChild = parse.parsed_doc() ;

}

void XMLDocument::loadTagFromPos ( const std::string & buffer, size_t pos ) 
{

	// cerca il primo tag a partire da pos
	while ( buffer.at(pos) != '<' ) pos++;
	size_t fine_tag_pos=pos;
	while ( buffer.at(fine_tag_pos) != '>' && buffer.at(fine_tag_pos) != ' ' ) fine_tag_pos++;

	std::string tag = buffer.substr( pos+1, fine_tag_pos - pos -1);
	std::string s_tag = "<" + tag;
	std::string fine_tag = "</" + tag + ">";

	// cerca il primo tag di chiusura apartire da pos
	size_t p = buffer.find( fine_tag, pos+1 );
    if ( p == std::string::npos) {
        std::stringstream cc ;
        cc << "Non è stato trovato il tag di chiusura " << fine_tag ;
        throw( cc.str() );
    }
    // cerca a ritroso se c'è un tag prima di p
	size_t p1 = buffer.rfind( s_tag, p-1 );
	size_t c = 0;   // profondità del tag
	// se lo trovo allora devo andare al tag di chiusura successivo
	while ( p1 > pos ) {
        c++;
        p = buffer.find( fine_tag, p+1 );
        if ( p == std::string::npos) {
            std::stringstream cc ;
            cc << "Non è stato trovato il tag di chiusura " << fine_tag ;
            throw( cc.str() );
        }
        p1= p;
        for ( size_t i = 0; i <= c; ++i ) {
            p1 = buffer.rfind( s_tag, p1-1 );
        }
    }


	PARSER parse ( buffer.substr( pos, p - pos + fine_tag.size() ) ) ;

	fChild = parse.parsed_doc() ;
	
}

void XMLDocument::loadFromFile ( const std::string & file_name )
{

/*
access(const char *path, int mode);
Valori ammissibili per l’argomento mode:
00 esistenza
02 permessi di scrittura
04 permessi di lettura
06 permessi sia di lettura che scrittura
la funzione restituisce 0 se il file ha la modalità indicata.
La funzione restituisce -1 se il file non esiste o non è accessibile nella modalità indicata.
*/
	std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

  if ( ifs.good() ) {

      // get length of file:
    ifs.seekg (0, std::ios::end) ;
    size_t length = ifs.tellg () ;
    ifs.seekg (0, std::ios::beg) ;

    // allocate memory:
    char * buffer = new char [ length + 1 ];

    // read data as a block:
    ifs.read (buffer, length);
    buffer [ length ] = '\0';

    PARSER parse ( buffer ) ;

    fChild = parse.parsed_doc() ;

    ifs.close();

    delete [] buffer;

  }
  else {
    
    int ret = access( file_name.c_str(), 0 );
    if ( ret == -1 )
      throw ( std::string("Il file " + file_name + " non esiste") );

    ret = access( file_name.c_str(), 4 );
    if ( ret == -1 )
      throw ( std::string("Non si hanno i permessi di lettura del file " + file_name) );
  }

}

void XMLDocument::saveToFile ( const std::string & /* file_name */ )
{
	// todo
}

void XMLDocument::parse ( const std::string & src )
{

	try {
    PARSER parse ( src ) ;
		fChild = parse.parsed_doc() ;
	}
	catch ( ... ) {
		throw ("not xml document") ;
	}


}

/* implemantazione classe XMLElement */
XMLElement::XMLElement ( const std::string & tag_value ) :
	XMLNode ( ELEMENT, tag_value ), attribute ( NULL )
{
}

XMLElement::~XMLElement ()
{
	delete attribute ;
}

std::string XMLElement::text () const 
{

	if ( hasChilds () ) {
		
		XMLNode * nd = firstChild();
		while ( nd ) {
			if ( nd->type() == TEXT )
				return nd->value();
			nd=nd->firstChild();
		}
	
		return "";
	
	}
	
	return "" ;

}

std::string XMLElement::innerHTML ()
{
	if ( hasChilds () )
		return firstChild ()->printXML ();
	return "" ;
}

XMLAttribute * XMLElement::setAttribute ( const std::string &name, const std::string &value )
{

	if ( ! attribute )
		return attribute = new XMLAttribute ( this, name, value );

	// se esisce il name ne sostituisce il value
	XMLAttribute * last_att ;
	for ( XMLAttribute * att = attribute; att; att = att->next ) {
		last_att = att;
		if ( att->fName == name ) {
    		att->fValue = value;
    		return att;
		}
	}

	// se non esiste lo aggiunge alla fine
	return last_att->next = new XMLAttribute ( this, name, value );

}

TFIELD XMLElement::getAttribute ( const std::string &name ) const {


	for ( XMLAttribute* attr = attribute; attr; attr = attr->next )
		if ( attr->fName == name )
			return TFIELD ( attr->fValue );

	return TFIELD ( "" );

}

TFIELD XMLElement::getAttributeFromTagName ( const std::string &tag, const std::string &name )
{
	XMLElement *el = getFirstElementByTagName ( tag ) ;
	if ( el ) return el->getAttribute ( name );
	else return TFIELD( "" ) ;
}

/* implemantazione classe XMLText */
XMLText::XMLText ( const std::string & text_value ) :
	XMLNode ( TEXT, text_value )
{
}

XMLText::~XMLText ()
{
}

/* implemantazione classe XMLComment */
XMLComment::XMLComment ( const std::string & text_value ) :
	XMLNode ( COMMENT, text_value )
{
}

XMLComment::~XMLComment ()
{
}

/* implemantazione classe XMLDeclaration */
XMLDeclaration::XMLDeclaration ( const std::string & declaration ) :
	XMLNode ( DECLARATION, declaration )
{
}

XMLDocType::~XMLDocType ()
{
}

/* implemantazione classe XMLDeclaration */
XMLDocType::XMLDocType ( const std::string & doc_type ) :
	XMLNode ( DOCTYPE, doc_type )
{
}

XMLDeclaration::~XMLDeclaration ()
{
}

/* implemantazione classe XMLScript */
XMLScript::XMLScript ( const std::string & script ) :
	XMLNode ( SCRIPT, script )
{
}

XMLScript::~XMLScript ()
{
}

