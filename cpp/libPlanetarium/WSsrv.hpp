#ifndef _WSsrv_
#define _WSsrv_

#include <iostream>
#include <vector>
#include <map>



class WSsrv {


	friend class SkyGrid;
	

	class Map * moonGrid ;

	std::map < int, class Client * > clients;
	
	class TCPClient & geoLoc;

	void do_disconect ( Client * client ) ;
	void do_before_sendLoop ( Client * client ) ;
	
	void set_horizont ( class clientData * data ) ;
	long sendRFC6455 ( int fd, const std::string &type, const std::string &message, bool masked ) const ;
	std::string timestamp() const ;	// returns a timestamp

public :

	class clientData  * client_data ( int sockfd );

	virtual std::string handleClient (  int sockfd, const std::string &buffer   ) ;
  
  
	void initDb () ;
	void print_log ( const std::string & ) const ;
	std::string registerClient ( int socket, const std::string &query );
	void unregisterClient ( int socket );
	
	std::string render ( int id ) ;
	std::string  do_sendLoop ( int id ) ;
 	std::string  setCamera ( int id ) ;
 	
	  std::string setLight ( int id ) ;
	  std::string  executeLoop ( int id  ); // nuova versione di do_sendLoop
	  std::string strActualDate( int ud );
  
	Client * getClient( int id )  {
    // cerca nei clients
    auto it =  clients.find( id ) ;
    return it->second;
  }

	WSsrv ( );
	WSsrv ( std::string & web_dir );
	WSsrv ( unsigned int port, std::string & web_dir );
	virtual ~WSsrv ( );


};


class clientData {
		
	
	public :
		
		const double DEF_FACT = 1 ;
		const short DEF_REFRESH = 0;
		const double DEF_SCALE = 1,
			DEF_SIMULATION = 1,
			DEF_UP_X = 0,
			DEF_UP_Y =0,
			DEF_UP_Z = 1,
			DEF_FOV = 45,
			DEF_ZOOM = 1,
			DEF_NEAR = 0.000003,
			DEF_FAR = 50,
			DEF_MOVE_LAT = 0,
			DEF_MOVE_LNG = 0;
		
		WSsrv * parent ;
		class SolarSystem * system;
		int sockfd ;
		
		class Timer & timer;
		class Timer & timer_result ;
		
		class Date & ref_date;
 		short	refresh ;	// secondi;
 		double scale ;
 		double rotation ;
		double moon_rotation = 0;
  	double simulation ;
  		
		class SkyGrid * skyGrid ;
 		std::string tracciato;
 		std::string asterismi;
 		std::string ecliptic;
 		bool horizont = false;
    bool showEcliptic = false;
 		
		struct  {
			bool pressed = false;
			double altezza = 0;
			double azimut = 0;
		} mouse ;
		
		struct lookAt {
			
			enum VIEW {
				NONE,
				NORTH,
				NORTH_EAST,
				NORTH_WEST,
				EAST,
				SOUTH,
				SOUTH_EAST,
				SOUTH_WEST,
				WEST,
				ZENIT,
				AZIMUT
			} view = NONE;

			std::string where = "earth";
			double altezza = 0;
			double azimut = 0;
		
		} lookat;

		struct Camera {
			
			double lookat_x, lookat_y, lookat_z;
			double up_x, up_y, up_z;
			double pos_x, pos_y, pos_z;
			
			double fov;
			double zoom;
			
			double near;
			double far;
			
			double old_fov = 0;
			double old_zoom = 0;
			double old_near = 0;
			double old_far = 0;
		
		} camera ;
		
		bool orbitControls = false;
		bool ambient_light = false;
		bool executeLoop = false;
		
		double FACT ;

		struct lookFrom {
			
			double latitude = 51.516998;
			double longitude = 0;
			double move_lat;
			double move_lng;
			
			double dstOffset = 0;
			
			std::string where = "undefined";
			bool isFromPlace = false;
		
		} lookfrom;
		
		struct Constellation {
			bool asterism = false;
			bool tracciato = false;
		} ;
		std::map< std::string, Constellation > the_constellation;
		
		clientData (  WSsrv * p, SolarSystem * _system, int socked_id ) ;
		~clientData () ;
			
		void set_default () {
			
			refresh = DEF_REFRESH ;	// secondi;
			scale = DEF_SCALE;
			simulation = DEF_SIMULATION ;
		
			lookfrom.isFromPlace = false;
			lookfrom.move_lat = DEF_MOVE_LAT;
			lookfrom.move_lng = DEF_MOVE_LNG;
			
			camera.up_x = DEF_UP_X;
			camera.up_y = DEF_UP_Y;
			camera.up_z = DEF_UP_Z;
			
			camera.fov = DEF_FOV;
			camera.zoom = DEF_ZOOM;
			
			camera.near = DEF_NEAR;
			camera.far = DEF_FAR;
			
			FACT = DEF_FACT;
		
		}
		
	
} ;
	

#endif	// if _WSsrv_
