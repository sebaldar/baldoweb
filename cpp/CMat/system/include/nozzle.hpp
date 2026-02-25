#ifndef _nozzle_
#define _nozzle_



class NOZZLE
{

private:

public:

	std::string item;
	class LINEA * linea ;

	class COMPONENTE  *parent;

public:

	NOZZLE (  class COMPONENTE * e,  const std::string & name,  LINEA * line = nullptr ) :
		item(name), linea(line), parent(e) {}



};


#endif
