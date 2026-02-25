#include "parser.hpp"

#ifndef _field_h
	#include <field.hpp>
#endif

#ifndef until_h
	#include <Utils.h>
#endif



ENTITY entity [ NUM_ENTITY ] =
{
	{ "&amp;", '&' },
	{ "&lt;", '<' },
	{ "&gt;", '>' },
	{ "&quot;", '\"' },
	{ "&apos;", '\'' },
	{ "&", '&' }
};


void ConvertUTF32ToUTF8(
	unsigned long input,
	char* output,
	int* length )
{
	const unsigned long BYTE_MASK = 0xBF;
	const unsigned long BYTE_MARK = 0x80;
	const unsigned long FIRST_BYTE_MARK[7] =
		{ 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	if (input < 0x80)
		*length = 1;
	else if ( input < 0x800 )
		*length = 2;
	else if ( input < 0x10000 )
		*length = 3;
	else if ( input < 0x200000 )
		*length = 4;
	else
		{ *length = 0; return; }	// This code won't covert
									// this correctly anyway.

	output += *length;

	// Scary scary fall throughs.
	switch (*length)
	{
		case 4:
			--output;
			*output = (char)((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
		case 3:
			--output;
			*output = (char)((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
		case 2:
			--output;
			*output = (char)((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
		case 1:
			--output;
			*output = (char)(input | FIRST_BYTE_MARK[*length]);
	}
}


int IsAlpha( unsigned char anyByte )
{

/*
This will only work for low-ascii,
everything else is assumed to be a valid letter.
I'm not sure this is the best approach, but it is quite tricky trying
to figure out alhabetical vs. not across encoding. So take a very
conservative approach.
* */

//	if ( encoding == TIXML_ENCODING_UTF8 )
//	{
		if ( anyByte < 127 )
			return isalpha( anyByte );
		else
			return 1;	// What else to do?
						// The unicode set is huge...get
						// the english ones right.
//	}
//	else
//	{
//		return isalpha( anyByte );
//	}
}


int IsAlphaNum( unsigned char anyByte )
{

/*

This will only work for low-ascii,
everything else is assumed to be a valid
letter. I'm not sure this is the best approach,
but it is quite tricky trying
to figure out alhabetical vs. not across encoding. So take a very
conservative approach.
* */

//	if ( encoding == TIXML_ENCODING_UTF8 )
//	{
		if ( anyByte < 127 )
			return isalnum( anyByte );
		else
			return 1;	// What else to do? The unicode set is huge...get the english ones right.
//	}
//	else
//	{
//		return isalnum( anyByte );
//	}
}


/*
const char* SkipWhiteSpace ( const char* p, XMLEncoding encoding )
{
	if ( !p || !*p )
	{
		return 0;
	}
	if ( encoding == TIXML_ENCODING_UTF8 )
	{
		while ( *p )
		{
			const unsigned char* pU = (const unsigned char*)p;

			// Skip the stupid Microsoft UTF-8 Byte order marks
			if (	*(pU+0)==TIXML_UTF_LEAD_0
				 && *(pU+1)==TIXML_UTF_LEAD_1
				 && *(pU+2)==TIXML_UTF_LEAD_2 )
			{
				p += 3;
				continue;
			}
			else if(*(pU+0)==TIXML_UTF_LEAD_0
				 && *(pU+1)==0xbfU
				 && *(pU+2)==0xbeU )
			{
				p += 3;
				continue;
			}
			else if(*(pU+0)==TIXML_UTF_LEAD_0
				 && *(pU+1)==0xbfU
				 && *(pU+2)==0xbfU )
			{
				p += 3;
				continue;
			}

			// Still using old rules for white space.
			if ( IsWhiteSpace( *p ) || *p == '\n' || *p =='\r' )
				++p;
			else
				break;
		}
	}
	else
	{
		while ( *p && IsWhiteSpace( *p ) || *p == '\n' || *p =='\r' )
			++p;
	}

	return p;
}
*/

PARSER::PARSER ( const std::string & parse, XMLEncoding _encoding ) :
	encoding ( _encoding )
{

	try {
        fDoc = Identify ( NULL, parse ) ;
	} catch ( std::exception& e ) {
        std::cout << "Standard exception su costruttore PARSER di xml: " << e.what() << std::endl;
		throw ( std::string(e.what()) ) ;
    }
	catch ( ... ) {
		throw ( std::string("il documento non è in un formato xml valido!") ) ;
    }


}

XMLNode* PARSER::Identify ( XMLNode * parent, const std::string & str )
{

	const std::string xmlHeader = "<?xml" ;
	const std::string commentHeader = "<!--" ;
	const std::string dtdHeader = "<!" ;
	const std::string cdataHeader = "<![CDATA[" ;
	const std::string scriptHeader = "<?php" ;
	const std::string script1Header = "<?" ;


	// salti tutti gli spazi
	size_t i = 0 ;
	size_t len = str.length ();
	while ( isspace( str.at(i) ) && i < len ) i++;

	if ( i == std::string::npos ) return NULL ;

	if ( str.at(i) == '<' ) {

		size_t j = i ;
		// si sposta al primo non spazio
		while ( j < len && !isspace( str.at(j) )
					&&	j < len && str.at(j) != '>' ) j++;

		if ( str.at ( j ) == '>' && str.at ( j - 1 ) == '/' )
			j--;

		std::string header = str.substr ( i, j - i );

		if ( header.substr (0, 5 ) == xmlHeader ) {
			// cerca la fine
			i += 4 ;
			size_t j = str.find ( "?>", i );
			if ( j == std::string::npos ) {
				std::string res = str +
					"\n" +
					"Manca il tag di chiusura della dichiarazione";
				throw ( res ) ;
			}

			std::string text = str.substr ( i, j - i );

			XMLDeclaration * declaration_node =
									new XMLDeclaration ( text ) ;
			declaration_node->fParent = parent;

			// cerca inizio nuovo tag
			while ( j < len && str.at(j) != '<' ) j++;

			if ( j == len ) return declaration_node ;
			else {
				std::string str_sibling = utils::trim( str.substr ( j, len ) );
				if ( str_sibling != "" )
					declaration_node->fSibling =
								Identify ( parent, str_sibling );
				return declaration_node ;
			}
		}
		else if ( header.substr (0, 5 ) == scriptHeader ||
						header.substr (0, 2 ) == script1Header ){

			if ( header.substr (0, 5 ) == scriptHeader ) i += 5;
			else i += 2 ;

			size_t j = str.find ( "?>", i );

			if ( j == std::string::npos ) {
				std::string res = str +
							"\n" +
							"Manca il tag di chiusura dello script";

				throw ( res ) ;

			}


			std::string script = str.substr ( i, j - i  );

			XMLScript * script_node = new XMLScript ( script ) ;

			script_node->fParent = parent;

			// cerca inizio nuovo tag

			while ( j < len && str.at(j) != '<' ) j++;


			if ( j == len ) return script_node ;

			else {

				std::string str_sibling = utils::trim( str.substr ( j, len ) );

				if ( str_sibling != "" )

					script_node->fSibling =
								Identify ( parent, str_sibling );

				return script_node ;

			}



		}
		else if ( header.substr (0, 4 ) == commentHeader ){

			// cerca il fine commento

			i += 4 ;

			size_t j = str.find ( "-->", i );

			if ( j == std::string::npos ) {

				std::string res = str +
						"\n" +
						"Manca il tag di chiusura del commento";

				throw ( res ) ;

			}


			std::string text = str.substr ( i, j - i );


			XMLComment * comment_node = new XMLComment ( text ) ;
			comment_node->fParent = parent;

			j += 3;	// si porta oltre la fine tag

            while ( j < len && isblank(str[j]) ) j++;
            std::string str_sibling = utils::trim( str.substr ( j, len ) );

			if ( str_sibling != "" )
                comment_node->fSibling =
							Identify ( parent, str_sibling );

            return comment_node ;

		}
		else if ( header.substr (0, 9 ) == cdataHeader ) {

			// cerca la fine dei dati
			i += 9 ;
			size_t j = str.find ( "]]>", i );
			if ( j == std::string::npos ) {
				std::string res = str +
							"\n" +
							"Manca il tag di chiusura del data header";
				throw ( res ) ;
			}

			std::string text = str.substr ( i, j - i );


			XMLText * text_node = new XMLText ( text ) ;
			text_node->fParent = parent;


			// cerca inizio nuovo tag
			while ( j < len && str.at(j) != '<' ) j++;

			if ( j == len ) return text_node ;
			else {

				std::string str_sibling = utils::trim( str.substr ( j, len ) );
                if ( str_sibling != "" )
					text_node->fSibling =
								Identify ( parent, str_sibling );
                return text_node ;
			}
		}
		else if ( header.substr (0, 2 ) == dtdHeader ) {
           // cerca la fine
           /* TODO - non tratta la dichiarazione inline */
			i += 2 ;

			size_t j = str.find ( ">", i );

            if ( j == std::string::npos ) {
				std::string res = str +
					"\n" +
					"Manca il tag di chiusura della dichiarazione dtd";
				throw ( res ) ;
			}

			std::string text = str.substr ( i, j - i );

			XMLDocType * declaration_node = new XMLDocType ( text ) ;
			declaration_node->fParent = parent;

			// cerca inizio nuovo tag
			while ( j < len && str.at(j) != '<' ) j++;

			if ( j == len ) return declaration_node ;
			else {
				std::string str_sibling = utils::trim( str.substr ( j, len ) );
				if ( str_sibling != "" )
					declaration_node->fSibling =
								Identify ( parent, str_sibling );
				return declaration_node ;
			}
		}
		else if ( header.length() > 1 ) {

			// si tratta di un elemento
			std::string tag = header.substr (1);

			XMLElement * el = new XMLElement ( tag ) ;
			el->fParent = parent;
			size_t iniz = j;
            // cerca il tag di chiusura
            while ( j < len && str.at(j) != '>' ) {
				// verifica che  il tag di chiusura non sia tra apicini
				if ( str.at(j) == '"' ) {
					j++;
					// non fare niente fino ai prossimi apicini
					while ( j < len && str.at(j) != '"' ) j++;
				}
				j++;
			}
            // verifica se l'elemento è finito
            if ( str.at( j - 1 ) == '/' ) --j ;

            std::string attr_list = utils::trim( str.substr ( iniz, j - iniz ) );

            if ( attr_list != "" )
                makeAttribute ( el, attr_list ) ;

            size_t tag_fine ;
            if ( str.at( j ) != '/' ) {

                tag_fine = getEndTag ( str, tag );
                if ( tag_fine == std::string::npos ) {
                    std::string res = str +
							"\n" +
							"Manca il tag di chiusura " +
							str + "==>>" + tag + "<<==";
                    throw ( res ) ;
                }

                std::string str_child =
					utils::trim ( str.substr ( j + 1, tag_fine - j - 1 ) );
                tag_fine += 3 + tag.length ();

	            if ( str_child != "" )
                    el->fChild = Identify ( el, str_child );

            }
            else
                tag_fine = j + 2;

            while ( tag_fine < len &&
					isblank(str[tag_fine]) ) tag_fine++;
            std::string str_sibling = utils::trim( str.substr ( tag_fine, len ) );

			if ( str_sibling != "" )
                el->fSibling = Identify ( parent, str_sibling );

            return el ;

		}
		else
			return NULL ;
	}
	else {
		// allora si trata di un testo
		// cerca l'inizio del prossimo nodo
		size_t j = i ;
		while ( j < len && str.at(j) != '<' ) j++;

		std::string text = str.substr ( i, j );

		XMLText * text_node = new XMLText ( text ) ;
		text_node->fParent = parent;

        // cerca inizio nuovo tag
        while ( j < len && str.at(j) != '<' ) j++;

        if ( j == len ) return text_node ;
        else {
            std::string str_sibling = utils::trim( str.substr ( j, len ) );
			if ( str_sibling != "" )
                text_node->fSibling = Identify ( parent, str_sibling );
            return text_node ;
        }

	}


	return NULL ;

}

size_t PARSER::getEndTag ( std::string str, std::string tag )
{

	// tolgo il primo tag
	size_t iniz = str.find ( "<" + tag );


	// cerca se esiste un successivo elemento con quel tag
	size_t pos_start_tag = str.find ( "<" + tag + ">", iniz + 1 );
	if ( pos_start_tag == std::string::npos ) {
        pos_start_tag = str.find ( "<" + tag + " ", iniz + 1 );
    }
//	size_t pos_start_tag = str.find ( "<" + tag, iniz + 1 );
    // cerca il tag di chiusura successivo
	size_t pos_end_tag  = str.find ( "</" + tag + ">" );
	/*
        se il tag di inizio è precedente a quello di chiusura
        vuol dire che ci sono dei tag annidati
	*/
	while ( pos_start_tag < pos_end_tag ) {
		// c'è un tag annidato
		// posizionati al prossimo tag di chiusura

		/* verifica se è nella forma <tag />*/

        size_t count = pos_start_tag;
		while ( str.at( count ) != '>' && count < str.length() )
			if ( str.substr ( count, 2 ) == "/>" ) break;
			else count++;
		if ( str.substr ( count, 2 ) != "/>" ) {
            // cerca il tag di chiusura successivo
            pos_end_tag  = str.find ( "</" + tag + ">", pos_end_tag + 1 );
		}

		if ( pos_end_tag == std::string::npos ) return std::string::npos;

		pos_start_tag = str.find ( "<" + tag, pos_start_tag + 1 );

	}
	return pos_end_tag;
}

void PARSER::makeAttribute ( XMLElement * el, const std::string & attr_list )
{
    size_t i = 0;
    size_t len = attr_list.length() ;
    for ( i = 0; i < len; i++) {
        if ( isspace ( attr_list.at(i) ) ) {
            std::string res = attr_list +
					":\n" +
					"Il nome di attributo non deve contenere spazi";
            throw ( res ) ;
        }
        if ( attr_list.at(i) == '=' )
            break;
    }

    std::string name = attr_list.substr ( 0, i );

    i++;
    char apice = attr_list.at (i) ;
    if ( apice != '\"' && apice != '\'') {
        std::string res = attr_list +
			":\n" +
			"Il valore di attributo deve essere racchiuso tra apici";
        throw ( res ) ;
    }
    size_t val1 = ++i;

    // cerca l'apice di chiusura

    for ( ; i < len; i++) {
        if ( attr_list.at (i) == apice )
            break;
    }

    if ( i >= len ) {
        std::string res = attr_list +
			":\n" +
			"Il valore di attributo deve essere racchiuso tra apici";
        throw ( res ) ;
    }

    size_t val2 = i ;

    std::string value = attr_list.substr ( val1, val2 - val1 );

    el->setAttribute ( name, value ) ;

    if ( ++i == len )
        return ;
    else {
        std::string remaining = utils::trim ( attr_list.substr ( i, len) );
        makeAttribute ( el, remaining );
    }
}

