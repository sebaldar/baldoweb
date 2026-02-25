#include <sstream>

#include <vsop.hpp>

#include <celestial_body.hpp>
#include <angolo.hpp>
#include <line.hpp>
#include <plane.hpp>

std::vector< double > elp82b( double js );

// Nel file .cpp, inizializziamo DATADIR usando la funzione
const std::string CelestialBody::DATADIR = std::getenv("VSOP_DIR");


const double CelestialBody::pi = atan(1) * 4;
const double CelestialBody::G = 6.67408e-11;	// costante gravitazionale m3 kg s2
const double CelestialBody::LIGHT_SPEED = 299792.458 ; // km / s
const double CelestialBody::AU = 149597870.700; // km

const VSOP MainBody::venus_data ( DATADIR + "vsop87/VSOP87C.ven" );
const VSOP MainBody::venus_LBR_D_data ( DATADIR + "vsop87/VSOP87D.ven" );

const VSOP MainBody::uranus_data ( DATADIR + "vsop87/VSOP87C.ura" );
const VSOP MainBody::uranus_LBR_D_data ( DATADIR + "vsop87/VSOP87D.ura" );

const VSOP MainBody::saturn_data ( DATADIR + "vsop87/VSOP87C.sat" );
const VSOP MainBody::saturn_LBR_D_data ( DATADIR + "vsop87/VSOP87D.sat" );

const VSOP MainBody::mercury_data ( DATADIR + "vsop87/VSOP87C.mer" );
const VSOP MainBody::mercury_LBR_D_data ( DATADIR + "vsop87/VSOP87D.mer" );

const VSOP MainBody::neptune_data ( DATADIR + "vsop87/VSOP87C.nep" );
const VSOP MainBody::neptune_LBR_D_data ( DATADIR + "vsop87/VSOP87D.nep" );

const VSOP MainBody::mars_data ( DATADIR + "vsop87/VSOP87C.mar" );
const VSOP MainBody::mars_LBR_D_data ( DATADIR + "vsop87/VSOP87D.mar" );

const VSOP MainBody::jupiter_data ( DATADIR + "vsop87/VSOP87C.jup" );
const VSOP MainBody::jupiter_LBR_D_data ( DATADIR + "vsop87/VSOP87D.jup" );

const VSOP MainBody::earth_data ( DATADIR + "vsop87/VSOP87C.ear" );
const VSOP MainBody::earth_LBR_A_data ( DATADIR + "vsop87/VSOP87A.ear" );
const VSOP MainBody::earth_LBR_B_data ( DATADIR + "vsop87/VSOP87B.ear" );
const VSOP MainBody::earth_LBR_D_data ( DATADIR + "vsop87/VSOP87D.ear" );

ELP_GROUP1 Moon::elp1 ( DATADIR + "elp/ELP1" );
ELP_GROUP1 Moon::elp2 ( DATADIR + "elp/ELP2" );
ELP_GROUP1 Moon::elp3 ( DATADIR + "elp/ELP3" );

ELP_GROUP2 Moon::elp4 ( DATADIR + "elp/ELP4" );
ELP_GROUP2 Moon::elp5 ( DATADIR + "elp/ELP5" );
ELP_GROUP2 Moon::elp6 ( DATADIR + "elp/ELP6" );

ELP_GROUP2 Moon::elp7 ( DATADIR + "elp/ELP7" );
ELP_GROUP2 Moon::elp8 ( DATADIR + "elp/ELP8" );
ELP_GROUP2 Moon::elp9 ( DATADIR + "elp/ELP9" );

ELP_GROUP3 Moon::elp10 ( DATADIR + "elp/ELP10" );
ELP_GROUP3 Moon::elp11 ( DATADIR + "elp/ELP11" );
ELP_GROUP3 Moon::elp12 ( DATADIR + "elp/ELP12" );


ELP_GROUP3 Moon::elp13 ( DATADIR + "elp/ELP13" );
ELP_GROUP3 Moon::elp14 ( DATADIR + "elp/ELP14" );
ELP_GROUP3 Moon::elp15 ( DATADIR + "elp/ELP15" );

ELP_GROUP4 Moon::elp16 ( DATADIR + "elp/ELP16" );
ELP_GROUP4 Moon::elp17 ( DATADIR + "elp/ELP17" );
ELP_GROUP4 Moon::elp18 ( DATADIR + "elp/ELP18" );

ELP_GROUP4 Moon::elp19 ( DATADIR + "elp/ELP19" );
ELP_GROUP4 Moon::elp20 ( DATADIR + "elp/ELP20" );
ELP_GROUP4 Moon::elp21 ( DATADIR + "elp/ELP21" );

ELP_GROUP5 Moon::elp22 ( DATADIR + "elp/ELP22" );
ELP_GROUP5 Moon::elp23 ( DATADIR + "elp/ELP23" );
ELP_GROUP5 Moon::elp24 ( DATADIR + "elp/ELP24" );

ELP_GROUP5 Moon::elp25 ( DATADIR + "elp/ELP25" );
ELP_GROUP5 Moon::elp26 ( DATADIR + "elp/ELP26" );
ELP_GROUP5 Moon::elp27 ( DATADIR + "elp/ELP27" );

ELP_GROUP5 Moon::elp28 ( DATADIR + "elp/ELP28" );
ELP_GROUP5 Moon::elp29 ( DATADIR + "elp/ELP29" );
ELP_GROUP5 Moon::elp30 ( DATADIR + "elp/ELP30" );

ELP_GROUP5 Moon::elp31 ( DATADIR + "elp/ELP31" );
ELP_GROUP5 Moon::elp32 ( DATADIR + "elp/ELP32" );
ELP_GROUP5 Moon::elp33 ( DATADIR + "elp/ELP33" );

ELP_GROUP5 Moon::elp34 ( DATADIR + "elp/ELP34" );
ELP_GROUP5 Moon::elp35 ( DATADIR + "elp/ELP35" );
ELP_GROUP5 Moon::elp36 ( DATADIR + "elp/ELP36" );

//const NutationCoefficient g_NutationCoefficients[] =
const double Earth::tab22A [n22A][9] =
{
  {  0,  0,  0,  0,  1, -171996,  -174.2,  92025,     8.9    },
  { -2,  0,  0,  2,  2,  -13187,    -1.6,   5736,    -3.1    },
  {  0,  0,  0,  2,  2,   -2274,    -0.2,    977,    -0.5    },
  {  0,  0,  0,  0,  2,    2062,     0.2,   -895,     0.5    },
  {  0,  1,  0,  0,  0,    1426,    -3.4,     54,    -0.1    },
  {  0,  0,  1,  0,  0,     712,     0.1,     -7,       0    },
  { -2,  1,  0,  2,  2,    -517,     1.2,    224,    -0.6    },
  {  0,  0,  0,  2,  1,    -386,    -0.4,    200,       0    },
  {  0,  0,  1,  2,  2,    -301,       0,    129,    -0.1    },
  { -2, -1,  0,  2,  2,     217,    -0.5,    -95,     0.3    },
  { -2,  0,  1,  0,  0,    -158,       0,      0,       0    },
  { -2,  0,  0,  2,  1,     129,     0.1,    -70,       0    },
  {  0,  0, -1,  2,  2,     123,       0,    -53,       0    },
  {  2,  0,  0,  0,  0,      63,       0,      0,       0    },
  {  0,  0,  1,  0,  1,      63,     0.1,    -33,       0    },
  {  2,  0, -1,  2,  2,     -59,       0,     26,       0    },
  {  0,  0, -1,  0,  1,     -58,    -0.1,     32,       0    },
  {  0,  0,  1,  2,  1,     -51,       0,     27,       0    },
  { -2,  0,  2,  0,  0,      48,       0,      0,       0    },
  {  0,  0, -2,  2,  1,      46,       0,    -24,       0    },
  {  2,  0,  0,  2,  2,     -38,       0,     16,       0    },
  {  0,  0,  2,  2,  2,     -31,       0,     13,       0    },
  {  0,  0,  2,  0,  0,      29,       0,      0,       0    },
  { -2,  0,  1,  2,  2,      29,       0,    -12,       0    },
  {  0,  0,  0,  2,  0,      26,       0,      0,       0    },
  { -2,  0,  0,  2,  0,     -22,       0,      0,       0    },
  {  0,  0, -1,  2,  1,      21,       0,    -10,       0    },
  {  0,  2,  0,  0,  0,      17,    -0.1,      0,       0    },
  {  2,  0, -1,  0,  1,      16,       0,     -8,       0    },
  { -2,  2,  0,  2,  2,     -16,     0.1,      7,       0    },
  {  0,  1,  0,  0,  1,     -15,       0,      9,       0    },

  { -2,  0,  1,  0,  1,     -13,       0,      7,       0    },
  {  0, -1,  0,  0,  1,     -12,       0,      6,       0    },
  {  0,  0,  2, -2,  0,      11,       0,      0,       0    },
  {  2,  0, -1,  2,  1,     -10,       0,      5,       0    },
  {  2,  0,  1,  2,  2,     -8,        0,      3,       0    },
  {  0,  1,  0,  2,  2,      7,        0,     -3,       0    },
  { -2,  1,  1,  0,  0,     -7,        0,      0,       0    },
  {  0, -1,  0,  2,  2,     -7,        0,      3,       0    },
  {  2,  0,  0,  2,  1,     -7,        0,      3,       0    },
  {  2,  0,  1,  0,  0,      6,        0,      0,       0    },
  { -2,  0,  2,  2,  2,      6,        0,     -3,       0    },
  { -2,  0,  1,  2,  1,      6,        0,     -3,       0    },
  {  2,  0, -2,  0,  1,     -6,        0,      3,       0    },
  {  2,  0,  0,  0,  1,     -6,        0,      3,       0    },
  {  0, -1,  1,  0,  0,      5,        0,      0,       0    },
  { -2, -1,  0,  2,  1,     -5,        0,      3,       0    },
  { -2,  0,  0,  0,  1,     -5,        0,      3,       0    },
  {  0,  0,  2,  2,  1,     -5,        0,      3,       0    },
  { -2,  0,  2,  0,  1,      4,        0,      0,       0    },
  { -2,  1,  0,  2,  1,      4,        0,      0,       0    },
  {  0,  0,  1, -2,  0,      4,        0,      0,       0    },
  { -1,  0,  1,  0,  0,     -4,        0,      0,       0    },
  { -2,  1,  0,  0,  0,     -4,        0,      0,       0    },
  {  1,  0,  0,  0,  0,     -4,        0,      0,       0    },
  {  0,  0,  1,  2,  0,      3,        0,      0,       0    },
  {  0,  0, -2,  2,  2,     -3,        0,      0,       0    },
  { -1, -1,  1,  0,  0,     -3,        0,      0,       0    },
  {  0,  1,  1,  0,  0,     -3,        0,      0,       0    },
  {  0, -1,  1,  2,  2,     -3,        0,      0,       0    },
  {  2, -1, -1,  2,  2,     -3,        0,      0,       0    },
  {  0,  0,  3,  2,  2,     -3,        0,      0,       0    },
  {  2, -1,  0,  2,  2,     -3,        0,      0,       0    },
};

const double Moon::tab47A [n47][6]=
		{

			{0,	0,	1,	0,	6288774,	-20905355	},
			{2,	0,	-1,	0,	1274027,	-3699111	},
			{2,	0,	0,	0,	658314,		-2955968	},
			{0,	0,	2,	0,	213618,		-569925		},
			{0,	1,	0,	0,	-185116,	48888		},
			{0,	0,	0,	2,	-114332,	-3149	},
			{2,	0,	-2,	0,	58793,	246158	},
			{2,	-1,	-1,	0,	57066,	-152138	},
			{2,	0,	1,	0,	53322,	-170733	},
			{2,	-1,	0,	0,	45758,	-204586	},
			{0,	1,	-1,	0,	-40923,	-129620	},
			{1,	0,	0,	0,	-34720,	108743	},
			{0,	1,	1,	0,	-30383,	104755	},
			{2,	0,	0,	-2,	15327,	10321	},
			{0,	0,	1,	2,	-12528,	0	},
			{0,	0,	1,	-2,	10980,	79661	},
			{4,	0,	-1,	0,	10675,	-34782	},
			{0,	0,	3,	0,	10034,	-23210	},
			{4,	0,	-2,	0,	8548,	-21636	},
			{2,	1,	-1,	0,	-7888,	24208	},
			{2,	1,	0,	0,	-6766,	30824	},
			{1,	0,	-1,	0,	-5163,	-8379	},
			{1,	1,	0,	0,	4987,	-16675	},
			{2,	-1,	1,	0,	4036,	-12831	},
			{2,	0,	2,	0,	3994,	-10445	},
			{4,	0,	0,	0,	3861,	-11650	},
			{2,	0,	-3,	0,	3665,	14403	},
			{0,	1,	-2,	0,	-2689,	-7003	},
			{2,	0,	-1,	2,	-2602,	0	},
			{2,	-1,	-2,	0,	2390,	10056	},
			{1,	0,	1,	0,	-2348,	6322	},
			{2,	-2,	0,	0,	2236,	-9884	},

			{0,	1,	2,	0,	-2120,	5751	},
			{0,	2,	0,	0,	-2069,	0	},
			{2,	-2,	-1,	0,	2048,	-4950	},
			{2,	0,	1,	-2,	-1773,	4130	},
			{2,	0,	0,	2,	-1595,	0	},
			{4,	-1,	-1,	0,	1215,	-3958	},
			{0,	0,	2,	2,	-1110,	0	},
			{3,	0,	-1,	0,	-892,	3258	},
			{2,	1,	1,	0,	-810,	2616	},
			{4,	-1,	-2,	0,	759,	-1897	},
			{0,	2,	-1,	0,	-713,	-2117	},
			{2,	2,	-1,	0,	-700,	2354	},
			{2,	1,	-2,	0,	691,	0	},
			{2,	-1,	0,	-2,	596,	0	},
			{4,	0,	1,	0,	549,	-1423	},
			{0,	0,	4,	0,	537,	-1117	},
			{4,	-1,	0,	0,	520,	-1571	},
			{1,	0,	-2,	0,	-487,	-1739	},
			{2,	1,	0,	-2,	-399,	0	},
			{0,	0,	2,	-2,	-381,	-4421	},
			{1,	1,	1,	0,	351,	0	},
			{3,	0,	-2,	0,	-340,	0	},
			{4,	0,	-3,	0,	330,	0	},
			{2,	-1,	2,	0,	327,	0	},
			{0,	2,	1,	0,	-323,	1165	},
			{1,	1,	-1,	0,	299,	0	},
			{2,	0,	3,	0,	294,	0	},
			{2,	0,	-1,	-2,	0,	8752	}

		};

const double Moon::tab47B [n47][5]=
		{

			{0,	0,	0,	1,	5128122	},
			{0,	0,	1,	1,	280602	},
			{0,	0,	1,	-1,	277693	},
			{2,	0,	0,	-1,	173237	},
			{2,	0,	-1,	1,	55413	},
			{2,	0,	-1,	-1,	46271	},
			{2,	0,	0,	1,	32573	},
			{0,	0,	2,	1,	17198	},
			{2,	0,	1,	-1,	9266	},
			{0,	0,	2,	-1,	8822	},
			{2,	-1,	0,	-1,	8216	},
			{2,	0,	-2,	-1,	4324	},
			{2,	0,	1,	1,	4200	},
			{2,	1,	0,	-1,	-3359	},
			{2,	-1,	-1,	1,	2463	},
			{2,	-1,	0,	1,	2211	},
			{2,	-1,	-1,	-1,	2065	},
			{0,	1,	-1,	-1,	-1870	},
			{4,	0,	-1,	-1,	1828	},
			{0,	1,	0,	1,	-1794	},
			{0,	0,	0,	3,	-1749	},
			{0,	1,	-1,	1,	-1565	},
			{1,	0,	0,	1,	-1491	},
			{0,	1,	1,	1,	-1475	},
			{0,	1,	1,	-1,	-1410	},
			{0,	1,	0,	-1,	-1344	},
			{1,	0,	0,	-1,	-1335	},
			{0,	0,	3,	1,	1107	},
			{4,	0,	0,	-1,	1021	},
			{4,	0,	-1,	1,	833	},

			{0,	0,	1,	-3,	777	},
			{4,	0,	-2,	1,	671	},
			{2,	0,	0,	-3,	607	},
			{2,	0,	2,	-1,	596	},
			{2,	-1,	1,	-1,	491	},
			{2,	0,	-2,	1,	-451	},
			{0,	0,	3,	-1,	439	},
			{2,	0,	2,	1,	422	},
			{2,	0,	-3,	-1,	421	},
			{2,	1,	-1,	1,	-366	},
			{2,	1,	0,	1,	-351	},
			{4,	0,	0,	1,	331	},
			{2,	-1,	1,	1,	315	},
			{2,	-2,	0,	-1,	302	},
			{0,	0,	1,	3,	-283	},
			{2,	1,	1,	-1,	-229	},
			{1,	1,	0,	-1,	223	},
			{1,	1,	0,	1,	223	},
			{0,	1,	-2,	-1,	-220	},
			{2,	1,	-1,	-1,	-220	},
			{1,	0,	1,	1,	-185	},
			{2,	-1,	-2,	-1,	181	},
			{0,	1,	2,	1,	-177	},
			{4,	0,	-2,	-1,	176	},
			{4,	-1,	-1,	-1,	166	},
			{1,	0,	1,	-1,	-164	},
			{4,	0,	1,	-1,	132	},
			{1,	0,	-1,	-1,	-119	},
			{4,	-1,	0,	-1,	115	},
			{2,	-2,	0,	1,	107	}

		};


const double Moon::tab47B_copia [n47][5]=
{
  { 0, 0,  0,  1,5128122  },
  { 0, 0,  1,  1,280602  },
  { 0, 0,  1,  -1,277693   },
  { 2, 0,  0,  -1 ,173237  },
  { 2, 0,  -1, 1 ,55413 },
  { 2, 0,  -1, -1,46271 },
  { 2, 0,  0,  1,32573   },
  { 0, 0,  2,  1, 17198 },
  { 2, 0,  1,  -1,9266  },
  { 0, 0,  2,  -1,8822  },
  { 2, -1, 0,  -1, 8216 },
  { 2, 0,  -2, -1,4324   },
  { 2, 0,  1,  1,4200   },
  { 2, 1,  0,  -1,-3359  },
  { 2, -1, -1, 1,2463  },
  { 2, -1, 0,  1,2211   },
  { 2, -1, -1, -1,  2065    },
  { 0, 1,  -1, -1, -1870 },
  { 4, 0,  -1, -1,1828   } ,
  { 0, 1,  0,  1,-1794  },
  { 0, 0,  0,  3,-1749 },
  { 0, 1,  -1, 1, -1565   },
  { 1, 0,  0,  1 ,-1491 },
  { 0, 1,  1,  1,-1475   },
  { 0, 1,  1,  -1,-1410    },
  { 0, 1,  0,  -1 ,-1344  },
  { 1, 0,  0,  -1,-1335  },
  { 0, 0,  3,  1 ,1107 },
  { 4, 0,  0,  -1 ,1021 },
  { 4, 0,  -1, 1, 833},
  { 0, 0,  1,  -3 ,777},
  { 4, 0,  -2, 1 ,671 },
  { 2, 0,  0,  -3 ,607},
  { 2, 0,  2,  -1, 596},
  { 2, -1, 1,  -1 ,491},
  { 2, 0,  -2, 1 ,-451 },
  { 0, 0,  3,  -1, 439},
  { 2, 0,  2,  1,  422},
  { 2, 0,  -3, -1, 421},
  { 2, 1,  -1, 1, -366 },
  { 2, 1,  0,  1, -351 },
  { 4, 0,  0,  1, 331 },
  { 2, -1, 1,  1 ,315 },
  { 2, -2, 0,  -1, 302},
  { 0, 0,  1,  3,  -283},
  { 2, 1,  1,  -1, -229},
  { 1, 1,  0,  -1, 223},
  { 1, 1,  0,  1,  223},
  { 0, 1,  -2, -1,-220 },
  { 2, 1,  -1, -1, -220},
  { 1, 0,  1,  1,  -185},
  { 2, -1, -2, -1, 181},
  { 0, 1,  2,  1, -177 },
  { 4, 0,  -2, -1 ,176},
  { 4, -1, -1, -1, 166},
  { 1, 0,  1,  -1, -164},
  { 4, 0,  1,  -1, 132},
  { 1, 0,  -1, -1, -119},
  { 4, -1, 0,  -1, 115},
  { 2, -2, 0,  1,  107}
};

CelestialBody::CelestialBody ( const std::string & _name, CelestialBody * _parent ) :
	parent( _parent ), name( _name )
{
}

CelestialBody::~CelestialBody() {
}

double CelestialBody::sin_deg ( Grade g )
{
	return sin ( Radiant( g ) );
}
	
double CelestialBody::cos_deg ( Grade g )
{
	return cos ( Radiant( g ) );
}
/*	
bool CelestialBody::manageBody ( const	XMLElement * el )
{

	for ( XMLNode * n = el->firstChild(); n; n = n->nextSibling() ) {

		XMLElement * el = dynamic_cast<XMLElement *>(n);

		std::string tag = el->value();
		if ( tag == "body" ) {

			std::string nome = el->getAttribute("name");
			std::string type = el->getAttribute("type");
			if ( nome == name ) {

				radius = ( float ) el->getAttribute( "radius" ).AsFloat;
				gl.shader = el->getAttribute( "shader" );
				gl.map = el->getAttribute( "map" );
				mass = ( float ) el->getAttribute( "mass" ).AsFloat;

				return true;

			}

			if ( manageBody ( el ) ) return true ;

		}

	}

	return false ;


}
*/
std::string CelestialBody::showOrbitalData ( double T )
{

	OrbitXYZ o = orbit ( T );

	return showOrbitalData ( o );

}

std::string CelestialBody::primitive ( double T ) const 
{

	OrbitXYZ o = orbit ( T );

	return primitive ( o );


}

std::string CelestialBody::primitiveJSON ( double T ) const 
{

	OrbitXYZ o = orbit ( T );

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;

	double vx = o.vx ;
	double vy = o.vy ;
	double vz = o.vz ;

	double r = pow( x * x + y * y + z * z, 0.5 );
	double v = pow( vx * vx + vy * vy + vz * vz, 0.5 );

	std::stringstream result;
  
  return result.str();
}

std::string CelestialBody::showOrbitalData ( OrbitXYZ o )
{
/*
	double x = o.x ;
	double y = o.y ;
	double z = o.z ;

	double vx = o.vx ;
	double vy = o.vy ;
	double vz = o.vz ;

	double r = pow( x * x + y * y + z * z, 0.5 );
	double v = pow( vx * vx + vy * vy + vz * vz, 0.5 );

	double r_au = radius  / CelestialBody::AU ; 
 
 	std::stringstream result;
	result.precision(10);
  
  result = "{" ;
  result << 
    
    "\"type\":\"sphere\"," <<
    
    "\"name\":\"" << name << "\"," <<
    
    "\"radius\":" << gl.FACT * r_au << "," <<
    
    "\"widthSegments\":" << gl.widthSegments  << "," <<
    
    "\"heightSegments:\"" << gl.heightSegments  << "," <<
    
    "\"clickable:\"" << "true"  << "," <<
    
    "\"rotate:\"" << "{"  <<
      "\"x:\"" << o.rot_x  << "," <<
      "\"y:\"" << o.rot_y  << "," <<
      "\"z:\"" << o.rot_z  << "" <<
    "},"  <<
    
    "\"scale:\"" << "{"  <<
      "\"x:\"" << gl.scale  << "," <<
      "\"y:\"" << gl.scale << "," <<
      "\"z:\"" << gl.scale << "" <<
    "},"  <<
    
    "\"position:\"" << "{"  <<
      "\"x:\"" << gl.FACT * x  << "," <<
      "\"y:\"" << gl.FACT * y << "," <<
      "\"z:\"" << gl.FACT * z << "" <<
    "},"  <<
    
    "\"material:\"" << "{"  <<
      "\"type\":\"" << gl.shader << "\"," <<
      "\"side\":\"" << gl.side << "\"," <<
      "\"shininess\":\"" << 12.8 << "\"," <<
      "\"map\":\"" << gl.map << "\"," <<
      "\"ambient:\"" << "{"  <<
        "\"r:\"" << 1  << "," <<
        "\"g:\"" << 1 << "," <<
        "\"b:\"" << 1 << "," <<
        "\"a:\"" << 1 << "" <<
      "},"  <<
       "\"diffuse:\"" << "{"  <<
        "\"r:\"" << 1  << "," <<
        "\"g:\"" << 1 << "," <<
        "\"b:\"" << 1 << "," <<
        "\"a:\"" << 1 << "" <<
      "},"  <<
       "\"specular:\"" << "{"  <<
        "\"r:\"" << 1  << "," <<
        "\"g:\"" << 1 << "," <<
        "\"b:\"" << 1 << "," <<
        "\"a:\"" << 1 << "" <<
      "},"  <<

    "\"shadow:\"" << "{"  <<
      "\"mode:\"" << gl.shadow  << "\"" <<
    "},"  <<
  
  "},"  <<
std::cout << result.str() << std::endl;
*/	
  return "";


}

std::string CelestialBody::primitive ( OrbitXYZ & o, bool recreate ) const 
{

	// se recreate è true ricrea l'oggetto

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;
	

	double r = radius  / CelestialBody::AU ; 

	std:: string create;
	if ( recreate )
		create="<recreate />";
	else
		create = "";

//	x = 10, y = -10, z = 0;
	std::stringstream result;
	
	result.precision(20); 
	result << 
		"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << gl.FACT * r <<
			"\" widthSegments=\""<< gl.widthSegments <<
			"\" heightSegments=\"" << gl.heightSegments << "\">" <<
	create <<
	"	<clickable />"	<< std::endl <<
	"	<rotate x=\"" << o.rot_x << "\" y=\"" << o.rot_y << "\" z=\"" << o.rot_z << "\" />"	<< std::endl <<
	"	<scale x=\"" << gl.scale << "\" y=\"" << gl.scale << "\" z=\"" << gl.scale << "\" />"	<< std::endl <<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<< std::endl <<
	"	<material type=\"" << gl.shader << "\" side=\"" << gl.side << "\" shininess=\"12.8\" map=\"" << gl.map << "\">" << std::endl <<
	"		<ambient r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<< std::endl <<
	"		<diffuse r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<< std::endl <<
	"		<specular r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<< std::endl <<
	"	</material>"	<<std::endl <<
	"	<shadow mode=\"" << gl.shadow << "\" />"	<< std::endl <<
	"</primitive>" << std::endl 	;
	
/*
	for ( auto it = childs.begin(); it != childs.end(); ++it ) {
		*it->primitive();
	}
*/
	return result.str();

}



std::string CelestialBody::primitive_pos () const 
{
	OrbitXYZ o = position;
	return primitive_pos ( o );
}

std::string CelestialBody::primitive ( bool recreate ) const 
{
	OrbitXYZ o = position;
	return primitive ( o, recreate );
}

std::string CelestialBody::primitive_pos ( OrbitXYZ & o ) const 
{

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;

	std::stringstream result;
	
	result.precision(20); 
	result << 
	"<primitive type=\"sphere\" name=\"" << name << "\">" <<std::endl <<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<std::endl <<
	"</primitive>" << std::endl ;
	
	return result.str();

}

void CelestialBody::setMap ( const std::string & map_dir, const std::string & map ) 
{

	if ( map != "" )
		gl.map = map_dir + "/" + map;

}


Point CelestialBody::bodyCoord(double latitude, double longitude, double H) const
{
    // H = altezza in metri sopra la superficie
    // Output in chilometri (coerente con earth.a in km)
    
    const double PI = atan(1.0) * 4.0;           // definizione di π
    const double DEG_TO_RAD = PI / 180.0;

    const double R = a;                          // raggio in km (assumiamo sferico)
    const double lat_rad = latitude * DEG_TO_RAD;
    const double lon_rad = longitude * DEG_TO_RAD;

    double cos_lat = cos(lat_rad);
    double sin_lat = sin(lat_rad);

    // Raggio effettivo (distanza dal centro) = R + H/1000 km
    double effective_radius = R + H / 1000.0;

    double x = effective_radius * cos_lat * cos(lon_rad);
    double y = effective_radius * cos_lat * sin(lon_rad);
    double z = effective_radius * sin_lat;

    return Point(x, y, z);
}

Sky::Sky ( ) :
	CelestialBody ( "sky", nullptr )
{

	D = 40 ; // UA
	radius =  D / 2 ;
	
	
	gl.shader = "basic";
	gl.map = "/my_image/default/TychoSkymapII.t3_08192x04096.jpg";

}

Sky::~Sky() 
{
}



void Sky::setMap ( const std::string & map_dir, const std::string & map ) 
{

	if ( map == "" ) 
		gl.map = "mag:4"  ;
	else if ( map.substr(0,4) != "mag:" )
		gl.map = map_dir + "/" + map;
	else {
		gl.map = map;
	}

}

std::string Sky::primitive ( OrbitXYZ & o, bool recreate ) const 
{

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;
	
	double r = radius  ; 
		
	std:: string create;
	if ( recreate )
		create="<recreate />";
	else
		create = "";

	std::stringstream result;

	// se map inizia / allora è una direttori e bisogna caricare la texture
	// altrimenti è una sfera nera
	bool is_texture = gl.map.substr(0,1) == "/" ? true : false;
	
	if ( is_texture ) {
	
		result << 
		"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << gl.FACT * r <<
			"\" widthSegments=\""<< gl.widthSegments <<
			"\" heightSegments=\"" << gl.heightSegments << "\">" <<
		create <<
		"	<clickable />"	<< 
		"	<rotate x=\"90\" y=\"0\" z=\"0\" />"	<<
		"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<
		"		<material type=\"basic\" shininess=\"12.8\" side=\"back\" map=\"" << gl.map << "\">"	<<
		"		<ambient r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
		"		<diffuse r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
		"		<specular r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
		"	</material>"	<<
		"</primitive>"	;
	
	}
	else {


		result << 
		"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << gl.FACT * r <<
			"\" widthSegments=\""<< gl.widthSegments <<
			"\" heightSegments=\"" << gl.heightSegments << "\">" <<
		create <<
		"	<clickable />"	<< 
		"	<rotate x=\"90\" y=\"0\" z=\"0\" />"	<<
		"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<
		"		<material type=\"basic\" shininess=\"12.8\" side=\"back\" color=\"" << color << "\">"	<<
		"	</material>"	<<
		"</primitive>"	;

	}

	return result.str();

}

MainBody::MainBody ( const std::string & name, CelestialBody * parent, const VSOP * _vsop ) :
	CelestialBody( name, parent ), vsop( _vsop )
{
}

MainBody::~MainBody() {
}

OrbitXYZ MainBody::orbit ( double JD ) const
{

	return orbit ( *vsop, JD );

}

OrbitXYZ MainBody::orbit ( const VSOP & vsop_data, double JD ) const 
{

	return vsop_data.getOrbit( JD );

}

OrbitXYZ MainBody::orbit_eq ( const VSOP & vsop_data, double JD ) const 
{


	Sun sun ;
	OrbitXYZ orbit_sun_xyz = sun.orbit ( MainBody::earth_data, JD ) ;
		
	Earth earth;
	Grade epsilon = earth.obliquity ( JD );
	Radiant eps = epsilon ;
	
	return orbit_eq ( vsop_data, orbit_sun_xyz, epsilon, JD ) ;

}

OrbitXYZ MainBody::orbit_eq ( const VSOP & vsop_data, const OrbitXYZ sun_pos, double epsilon, double JD ) const 
{


	double x_s = sun_pos.x ;
	double y_s = sun_pos.y ;
	double z_s = sun_pos.z ;
		
	OrbitXYZ o = orbit ( vsop_data, JD );

	double x_ecli = o.x - x_s;
	double y_ecli = o.y - y_s ;
	double z_ecli = o.z - z_s ;
		
	Radiant eps = Grade ( epsilon ) ;
	
		
	double x_eq = x_ecli;
	double y_eq = y_ecli * cos( eps ) - z_ecli * sin( eps );
	double z_eq = y_ecli * sin( eps ) + z_ecli * cos( eps );
	
	o.x = x_eq;
	o.y = y_eq;
	o.z = z_eq;

	return o; 

}

OrbitEcli MainBody::orbitLBR  ( double JD ) const
{

	return vsop->getOrbitLBR( JD );

}

Earth::Earth (  ) :
	Earth( ( VSOP *) nullptr )

{
}

Earth::Earth ( CelestialBody * _parent  ) :
	Earth( & MainBody::earth_data )
{
	parent = _parent;
}

Earth::Earth ( const VSOP * _vsop   ) :
	MainBody( "earth", nullptr, _vsop )
{

    D = 2 * 6371.00507612; // km
    M = 5.972e+24; // kg massa terra 
    radius =  D / 2 ;
    R = 1000 * radius;
    
	a = 6378.14 ;	// raggio all'equatore km
	a = radius;
	b = 6356.755 ;	// raggio al polo km
	
	gl.shader = "lambert";
	gl.map = "/my_image/default/earth.jpg";
	gl.side = "double";
	gl.shadow = "receive";
	
	gl.widthSegments = 400;
	gl.heightSegments = 400;
	

}

Earth::~Earth() {
}


std::string Earth::primitive_pos ( OrbitXYZ & o ) const 
{

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;

	double r = radius  / CelestialBody::AU ; 

	std::stringstream result;
	
	result.precision(20); 
	result << 
	"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << size_fact * gl.FACT * r << "\">" <<std::endl <<
	"	<rotate x=\"" << o.rot_x << "\" y=\"" << o.rot_y << "\" z=\"" << o.rot_z << "\" />"	<<std::endl <<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<std::endl <<
	"</primitive>" << std::endl ;
	
	return result.str();

}

double Earth::sunAngleOverOrizont  ( double JD, double lat, double lng ) const 
{
				
	Sun sun;
	OrbitEcli orbit_sun = sun.orbitLBR( JD );
		
	OrbitEqu   o_sun ( JD );
	// converte le coordinate eclittiche in equatoriali
	o_sun = orbit_sun;
		
	// converte in coordinate rettangolari
	OrbitXYZ o_sun_xyz = o_sun;
	double x_sun = o_sun_xyz.x ;
	double y_sun = o_sun_xyz.y ;
	double z_sun = o_sun_xyz.z ;
	
	double rot = rotation( JD ) ;
	Point coord = bodyCoord ( lat, lng + rot );

	double xc = coord.x / CelestialBody::AU;
	double yc = coord.y / CelestialBody::AU;
	double zc = coord.z / CelestialBody::AU;

	// retta ortogonale al punto di vista
	StraightLine pdv ( Point( 0, 0, 0 ), Point( xc, yc, zc ) );
	// intersezione piano tangente a coord e asse z
	double zint = ( xc * xc + yc * yc + zc * zc ) / zc;
	StraightLine int_z (  Point( xc, yc, zc ), Point( 0, 0, zint) );
		
	// retta ortogonale
	StraightLine orto ( Direction( pdv.orto( int_z ) ), Point( xc, yc, zc ) );
	
	StraightLine to_sun ( Point( xc, yc, zc ), Point( x_sun,  y_sun, z_sun) );
			
	return 90 - Grade( to_sun.angle( pdv ) ) ;

}

double Earth::sunRise  ( const Date & d, double lat, double lng, double simulation ) const
{
	
	double JD = d.now( simulation ) ;

	struct F1 : public Funzione {

		Variabile JD ;
		double lat, lng;

		
		double eval() {

			Earth earth;
			double angle = earth.sunAngleOverOrizont( JD, lat, lng ) ;
			
			return ( angle + 0.83 ) ;

		}
		F1( Variabile & jd, double l, double lg  ) :
			JD( jd ), lat(l), lng(lg) {
		}

	};

	Variabile jd = JD;
	jd.setMin( JD-0.01 );
	jd.setMax( JD+0.01 );
	
    F1 f1 ( jd, lat, lng );

	Math < Funzione, Variabile, double > m;
	m.setLog ( [] ( const std::string & log_str) {

		std::stringstream ss ;
		ss << getenv("_") << ".log" ;
		std::fstream srvlog ( ss.str(),  std::fstream::in | std::fstream::out | std::fstream::app );
		
		srvlog << log_str << std::endl ;

		} );
	m.setPrecisione ( 10e-6, 10e-10, 10e-10 ) ;
	m.risolveEquazione ( f1, f1.JD ) ;
	
	return f1.JD ;

}

double Earth::rotation ( const Date & d, double simulation ) const
{

	double JD = d.now( simulation ) ;
	return rotation ( JD );
	
}

double Earth::rotation ( double JD ) const
{
	
	Sun sun;
	OrbitEcli orbit_sun = sun.orbitLBR( JD );
		
	OrbitEqu   o_sun ( JD );
	o_sun = orbit_sun;
		
	double alfa_sun = Grade( o_sun.alfa );

	UT uut = JD;
	double hours = uut.hours( ) - 12  ; 
	double g = 360 * hours / 24;
		
	Grade rot = g + alfa_sun ;
	
	return rot;

}

std::string Earth::Easter ( short year )
{

    // la domenica successiva alla luna piena seguente
    // l'equinozio di marzo
    int je = floor( marchEquinox( year ) ); // julian day

    Moon moon;
    // 180 gradi luna e sole sono opposti
    int jm = floor ( moon.phases ( je, 180 ) ) ;   // luna piena prossima a je
	while ( jm < je )
		jm = floor ( moon.phases( jm + 29, 180 ));


    Date d ( jm );

    short week = ( jm + 1 ) % 7 ;   // 0 è domenica
    short day_to_sunday = 7 - week ;
    jm += day_to_sunday ;

/*
    // la pasqua deve cadere tra il 22 marzo e il 25 aprile
    if ( d.day(jm) == 26 )
        jm -= 7;
*/
    // la data in forma di stringa
    return d.toDate( jm );

}

std::string Earth::EasterEcle ( short year )
{

	short a = year % 19;
	short b = floor( year / 100 );
	short c = year % 100;
	short d = floor( b / 4 );
	short e = b % 4 ;
	short f = floor ( ( b + 8 ) / 25 );
	short g = floor ( ( b - f + 1 ) / 3 );
	short h = ( 19 * a + b -d - g + 15 ) % 30 ;
	short i = floor ( c / 4 );
	short k = c % 4 ;
	short l = ( 32 + 2 * e + 2 * i - h - k ) % 7 ;
	short m = floor ( ( a + 11 * h + 22 * l ) / 451 );
	short n = floor ( ( h + l - 7 * m + 114 ) / 31 );
	short p = ( h + l - 7 * m + 114 ) % 31 ;

	short month = n;
	short day = p + 1;

	std::stringstream ss ;
	ss << day << "-" << month << "-" << year ;

	Date dt ( ss.str() );

	return dt.toDate();

}

double  Earth::equinox ( short year, short month )
{

	std::stringstream ss ;
	ss << "21-" << month << "-" << year << " 00:00:00" ;

    Date date ( ss.str());

	double JDE0 = date.julianDay();


    FDeclination declinazione ( 0 );
	declinazione.JD = JDE0 ;

	Math < Funzione, Variabile, double > m;

	m.setPrecisione ( 1e-6, 1e-8, 1e-6 ) ;
	if ( m.risolveEquazione ( declinazione, declinazione.JD ) ) {

		return declinazione.JD;
	
	}
	else 
		throw ( "Unable to calculate equinox" );

}

double  Earth::marchEquinox ( short year )
{

	std::stringstream ss ;
	ss << "21-03-" << year << " 00:00:00" ;

    Date date ( ss.str());

	double JDE0 = date.julianDay();

	try {
		return calculateEquiSosti ( JDE0, 0 ) ;
	}
	catch ( const std::string & e ) {
		throw ( e );
	}

}

double  Earth::juneSolstice ( short year )
{

	std::stringstream ss ;
	ss << "21-06-" << year << " 00:00:00" ;

    Date date ( ss.str());

	double JDE0 = date.julianDay();

	return calculateEquiSosti ( JDE0, 90 ) ;

}

double  Earth::septemberEquinox ( short year )
{

	std::stringstream ss ;
	ss << "21-09-" << year << " 00:00:00" ;

    Date date ( ss.str());

	double JDE0 = date.julianDay();

	try {
		return calculateEquiSosti ( JDE0, 180 ) ;
	}
	catch ( const std::string & e ) {
		throw ( e );
	}

}

double  Earth::decemberSolstice ( short year )
{


	std::stringstream ss ;
	ss << "21-12-" << year << " 00:00:00" ;

    Date date ( ss.str());

	double JDE0 = date.julianDay();

	return calculateEquiSosti ( JDE0, 270 ) ;


}

double Earth::calculateEquiSosti ( double JDE0, double long_degree )
{

	FLongitudine longitudine ( long_degree );
	longitudine.JD = JDE0 ;

	Math < Funzione, Variabile, double > m;

	Date d ;
	short year = d.year(JDE0);

//	m.setPrecisione ( 1e-7, 1e-7, 1e-7 ) ;
	while ( true ) {

        try {

            if ( m.risolveEquazione ( longitudine, longitudine.JD ) ) {

                short y = d.year( longitudine.JD );
                if ( year == y )
                    return longitudine.JD;
                else if ( y < year )
                    JDE0 += 100.0;
                else if ( y > year )
                    JDE0 -= 100.0;

                longitudine.JD = JDE0;

            }
            else
                throw ( "Unable to calculate equinox or solstice" );

        }
        catch ( const std::string & str ) {

            throw ( str );

        }

	}

}

Earth::Precession Earth::precession ( double alfa, double delta, double JD  )
{

	Grade
		alfa0 = alfa,
		delta0 = delta;
		
	double t = 	( JD - JCentury::JD2000 ) / 36525.0;
	
	// in secondi
	Grade chi = 
		(2306.2181 * t + 0.30188 * t * t + 0.017998 * t * t * t) / 3600 ;
	Grade zeta =
		(2306.2181 * t + 1.09468 * t * t + 0.018203 * t * t * t) / 3600 ;
	Grade teta =
		(2004.3109 * t - 0.42665 * t * t - 0.041833 * t * t * t) / 3600 ;
		
	

	double A = 
		cos( Radiant(delta0) ) * sin( Radiant( Grade(alfa0 + chi)) ) ;
	double B = 
		cos( Radiant(teta) ) * cos(Radiant(delta0)) * cos( Radiant(Grade(alfa0 + chi)) ) - 
		sin(Radiant(teta)) * sin(Radiant(delta0));
	double C = 
		sin( Radiant(teta) ) * cos(Radiant(delta0)) * cos( Radiant(Grade(alfa0 + chi)) ) +
		cos(Radiant(teta)) * sin(Radiant(delta0));

/*se il denominatore di A / B è minore di 0 (B < 0) 
* al risultato di A / B si aggiungono 180°; 
* se invece esso è maggiore di 0 (B > 0) non si aggiunge nulla 
* e il risultato è già la tangente nell’angolo corretto;
* */

	double tan_alfa_meno_zeta = A / B;
	double alfa_meno_zeta ;
	if ( A/B < 0 ) 
		alfa_meno_zeta = Grade( Radiant( atan( tan_alfa_meno_zeta ) ) ) + 180;
	else
		alfa_meno_zeta = Grade( Radiant( atan( tan_alfa_meno_zeta ) ) ) ;
	
	alfa_meno_zeta = Grade( Radiant( atan( tan_alfa_meno_zeta ) ) ) ;
	
	double sin_delta = C;
	
	alfa = alfa_meno_zeta + zeta ;
	delta = Grade( Radiant( asin( sin_delta ) ) );
	
	return Precession( alfa, delta );

}

Earth::Nutation Earth::nutation ( double JD )
{

	JCentury T = JDay( JD ) ;

	Nutation n ;
	double D = 297.85036 + 445267.111480 * T - 0.0019142 * pow( T, 2 ) +
                pow( T, 3 ) / 189474  ;
	double M = 357.52772 + 35999.050340 * T - 0.0001603 * pow( T, 2 ) +
                pow( T, 3 ) / 300000  ;
	double M1 = 134.96298 + 477198.867398 * T + 0.0086972 * pow( T, 2 ) +
                pow( T, 3 ) / 56250  ;
	double F = 93.27191 + 483202.017538 * T - 0.0036825 * pow( T, 2 ) -
                pow( T, 3 ) / 327270  ;

    double O = 125.04452 - 1934.136261 * T + 0.0020708 * pow( T, 2 ) +
                pow( T, 3 ) / 450000  ;


	for ( size_t i = 0; i < n22A; ++i ) {

		short d = tab22A[i][0];
		short m = tab22A[i][1] ;
		short m1 = tab22A[i][2] ;
		short f = tab22A[i][3] ;
		short o = tab22A[i][4] ;

		double cp0 = tab22A[i][5] ;
		double cp1 = tab22A[i][6] ;

		double ce0 = tab22A[i][7] ;
		double ce1 = tab22A[i][8] ;

		double arg = ( d * D + m * M + m1 * M1 + f * F + o * O ) ;
		double coeff_sin = ( cp0 + cp1 * T ) ;
		double coeff_cos = ( ce0 + ce1 * T ) ;

		n.Dpsi += coeff_sin * sin ( arg * pi / 180 );
		n.Depsilon += coeff_cos * cos ( arg * pi / 180 ) ;

	}

    n.Dpsi /= 10000;
	n.Depsilon /= 10000 ;

    // son espressi in secondi, per passare ai gradi devo dividere per 3600
    n.Dpsi /= 3600;
	n.Depsilon /= 3600 ;

    return n;

}

double Earth::obliquity ( double JD )
{

	JCentury T = JDay( JD ) ;
    double U = T / 100;

	Angle a;
	a.setGradeGPS( 23, 26, 21.448 );

	double g = a.toGrade();
	double s = 4680.93 * U -
                1.55 *      pow ( U, 2 ) +
                1999.25 *   pow ( U, 3 ) -
                51.38 *     pow ( U, 4 ) -
                249.67 *    pow ( U, 5 ) -
                39.05 *     pow ( U, 6 ) +
                7.12 *      pow ( U, 7 ) +
                27.87 *     pow ( U, 8 ) +
                5.79 *      pow ( U, 9 ) +
                2.45 *      pow ( U, 10 ) ;

    g = g + s / 3600 ;

	return g ;

}

Sun::Sun ( ) :
	MainBody( "sun", nullptr, & MainBody::earth_LBR_D_data )
{

	D = 1392684 ; // km
	radius =  D / 2 ;
    R = 1000 * radius;

	M = 1.989e+30;	// kg
	
	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km

	gl.shadow = "cast";
	gl.shader = "basic";
	gl.map = "/my_image/default/sun.jpg";

}


OrbitEcli Sun::orbitLBR  ( double JD ) const
{

	OrbitEcli orbit = vsop->getOrbitLBR( JD );

	double L_earth = orbit.L;  // longitudine in radianti
	Angle l (L_earth);
	Grade g = l ;
	g = g - 180;   // riferiamolo alla terra e non al sole

	orbit.L = g;
	orbit.B = -orbit.B;

	return orbit;

}

Venus::Venus ( ) :
	MainBody( "venus", nullptr, & MainBody::venus_data )
{

    D = 12104 ; // km
    radius =  D / 2 ;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	gl.map = "/my_image/default/venus.jpg";
	gl.shader = "lambert";

}

Mars::Mars ( ) :
	MainBody( "mars", nullptr, & MainBody::mars_data )
{

    D = 6792 ; // km
    radius =  D / 2 ;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	gl.map = "/my_image/default/Mars_Geolocalisation.jpg";
	gl.shader = "lambert";

}


Jupiter::Jupiter ( ) :
	MainBody( "jupiter", nullptr, & MainBody::jupiter_data )
{

    D = 142984 ; // km
    M = 1.8986e+27; // kg 

    radius =  D / 2 ;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	gl.map = "/my_image/default/jupiter.jpg";
	gl.shader = "lambert";
	
}

double Jupiter::obliquity ( double JD )
{

	const double J1900 = 2451545.0 - 36525.0 ;
	double T = (JD - J1900) / 36525.0;
    double U = T / 100;

	return 3.120262 + 0.0006 * U ;

}

Saturn::Saturn ( ) :
	MainBody( "saturn", nullptr, & MainBody::saturn_data )
{

    D = 120536 ; // km
    M = 8.27e+23;	// kg
    radius =  D / 2 ;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	ringInnerR = 66900 ;	// km
	ringOuterR = 175000 ;	// km

	gl.shader = "lambert";
	gl.map = "/my_image/default/saturn.jpg";

}

std::string Saturn::primitive ( OrbitXYZ & o, bool recreate ) const 
{

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;
	
	double r = radius  / CelestialBody::AU ; 

	std:: string create;
	if ( recreate )
		create="<recreate />";
	else
		create = "";

	double rInnerR = ringInnerR / CelestialBody::AU;
	double rOuterR = ringOuterR / CelestialBody::AU;
		
//	x = 10, y = -10, z = 0;
	std::stringstream result;
	
	result << 
		"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << gl.FACT * r <<
			"\" widthSegments=\""<< gl.widthSegments <<
			"\" heightSegments=\"" << gl.heightSegments << "\">" <<
	create <<
	"	<clickable />"	<< 
"		<rotate x=\"" << o.rot_x << "\" y=\"" << o.rot_y << "\" z=\"" << o.rot_z << "\" />"	<<
"		<scale x=\"" << gl.scale << "\" y=\"" << gl.scale << "\" z=\"" << gl.scale << "\" />"	<<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<
"		<material type=\"" << gl.shader << "\" side=\"" << gl.side << "\" shininess=\"12.8\" map=\"" << gl.map << "\">" <<
	"		<ambient r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"		<diffuse r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"		<specular r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"	</material>"	<<
	"	<shadow mode=\"cast\" />"	<<
	"</primitive>"	<<
	
	
	"<primitive type=\"ring\" name=\"saturnRing\" innerRadius=\"" << gl.FACT * rInnerR << 
			 "\" outerRadius=\"" << gl.FACT * rOuterR <<"\" thetaSegments=\"30\">" <<
	"	<clickable />"	<< 
"		<rotate x=\"0\" y=\"0\" z=\"0\" />"	<<
"		<scale x=\"" << gl.scale << "\" y=\"" << gl.scale << "\" z=\"" << gl.scale << "\" />"	<<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<
	"	<shadow mode=\"cast\" />"	<<
	"</primitive>"	;


	return result.str();

}

Uranus::Uranus ( ) :
	MainBody( "uranus", nullptr, & MainBody::uranus_data )
{

	D = 51118 ; // km
	radius =  D / 2 ;
    R = 1000 * radius;
	
	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	ringInnerR = 26840 ;	// km
	ringOuterR = 69900 ;	// km

	
	gl.map = "/my_image/default/uranus.jpg";
	
}

std::string Uranus::primitive ( OrbitXYZ & o, bool recreate ) const 
{

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;
	
	double r = radius  / CelestialBody::AU ; 

	std:: string create;
	if ( recreate )
		create="<recreate />";
	else
		create = "";

	double rInnerR = ringInnerR / CelestialBody::AU;
	double rOuterR = ringOuterR / CelestialBody::AU;
		
//	x = 10, y = -10, z = 0;
	std::stringstream result;
	
	result << 
		"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << gl.FACT * r <<
			"\" widthSegments=\""<< gl.widthSegments <<
			"\" heightSegments=\"" << gl.heightSegments << "\">" <<
	create <<
	"	<clickable />"	<< 
"		<rotate x=\"" << o.rot_x << "\" y=\"" << o.rot_y << "\" z=\"" << o.rot_z << "\" />"	<<
"		<scale x=\"" << gl.scale << "\" y=\"" << gl.scale << "\" z=\"" << gl.scale << "\" />"	<<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<
"		<material type=\"" << gl.shader << "\" side=\"" << gl.side << "\" shininess=\"12.8\" map=\"" << gl.map << "\">" <<
	"		<ambient r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"		<diffuse r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"		<specular r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"	</material>"	<<
	"	<shadow mode=\"cast\" />"	<<
	"</primitive>"	<<
	
	
	"<primitive type=\"ring\" name=\"uranusRing\" innerRadius=\"" << gl.FACT * rInnerR << 
			 "\" outerRadius=\"" << gl.FACT * rOuterR <<"\" thetaSegments=\"30\">" <<
	"	<clickable />"	<< 
"		<rotate x=\"0\" y=\"0\" z=\"0\" />"	<<
"		<scale x=\"" << gl.scale << "\" y=\"" << gl.scale << "\" z=\"" << gl.scale << "\" />"	<<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<
	"	<material size=\"4\" />"	<<
	"</primitive>"	;


	return result.str();

}

Mercury::Mercury ( ) :
	MainBody( "mercury", nullptr, & MainBody::mercury_data )
{

	D = 4878 ; // km
	radius =  D / 2 ;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km

	gl.map = "/my_image/default/mercury.jpg";
	gl.shader = "lambert";

}



Neptune::Neptune ( ) :
	MainBody( "neptune", nullptr, & MainBody::neptune_data )
{

    D = 49244 ; // km
    radius =  D / 2 ;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km
	
	gl.map = "/my_image/default/neptune.jpg";
	gl.shader = "lambert";

}

Moon::Moon (  ) :
	CelestialBody( "moon", nullptr )
{

    D = 3474.2; // km
    M = 7.342e+22; // kg massa  
    radius = D / 2;
    R = 1000 * radius;

	a = radius ;	// raggio all'equatore km
	b = radius ;	// raggio al polo km

	gl.shader = "pong";
	gl.side = "double";
	gl.shadow = "receive";

	
	gl.map = "/my_image/default/moonmap1k.jpg";

}

Moon::~Moon() {
}


std::string Moon::primitive_pos ( OrbitXYZ & o ) const 
{

	double x = o.x ;
	double y = o.y ;
	double z = o.z ;

	double r = radius  / CelestialBody::AU ; 

	std::stringstream result;
	
	result.precision(20); 
	result << 
	"<primitive type=\"sphere\" name=\"" << name << "\" radius=\"" << size_fact * gl.FACT * r << "\">" <<std::endl <<
	"	<rotate x=\"" << o.rot_x << "\" y=\"" << o.rot_y << "\" z=\"" << o.rot_z << "\" />"	<<std::endl <<
	"	<position x=\"" << gl.FACT * x << "\" y=\"" << gl.FACT * y << "\" z=\"" << gl.FACT * z << "\" />"	<<std::endl <<
	"</primitive>"	<< std::endl ;
	
	return result.str();

}
Moon::PeriodicTerm Moon::periodicTerm ( double D, double M, double M1, double F, double E)
{

	double longitude = 0, distance = 0 ;
	for ( short i = 0; i <  n47; ++i ) {

		short d     = tab47A[i][0];
		short m     = tab47A[i][1];
		short m1    = tab47A[i][2];
		short f     = tab47A[i][3];

		double l    = tab47A[i][4];
		double r    = tab47A[i][5];

		double ang = ( d * D + m * M + m1 * M1 + f * F ) * pi / 180;
/*
		double e = 1;
		switch ( abs(m) ) {
            case 1 :
                e = E;
            case 2 :
                e = E * E;
		}
*/
		double e = 1;
		if ( m != 0 ) {
            m = abs( m );
            e = pow( E, m );
		}

		longitude +=
			l * e * sin ( ang );

		distance +=
			r * e * cos ( ang );

	}

	return Moon::PeriodicTerm ( longitude, distance );

}

double Moon::latitude ( double D, double M, double M1, double F, double /* E */ )
{

	double latit = 0 ;
	for ( short i = 0; i <  n47; ++i ) {

		short d     = tab47B[i][0];
		short m     = tab47B[i][1];
		short m1    = tab47B[i][2];
		short f     = tab47B[i][3];

		double l    = tab47B[i][4];

		double e = 1;
/*
		if ( m != 0 ) {
            m = abs( m );
            e = pow( E, m );
		}
*/

		double ang = ( d * D + m * M + m1 * M1 + f * F ) * pi / 180;
		latit +=
			l * e * sin ( ang );

	}

	return latit;

}

OrbitXYZ Moon::orbit ( double JD )
{

	JCentury T = JDay( JD ) ;

	std::vector< double > xyz;
	xyz = elp82b ( T ) ;

    OrbitXYZ orbit;
    orbit.x = xyz[0];
    orbit.y = xyz[1];
    orbit.z = xyz[2];

	return orbit;

}

OrbitEcli Moon::orbitAlfa ( double JD )
{

	JCentury T = JDay( JD ) ;

	double L1 = 218.3164477 + 481267.88123421 * T - 0.0015786 * pow( T, 2 ) + pow( T, 3 ) / 538841 - pow(T, 4) / 65194000 ;
	double D = 297.8501921 + 445267.1114034 * T - 0.0018819 * pow( T, 2 ) + pow( T, 3 ) / 545868 - pow(T, 4) / 113065000 ;
	double M = 357.5291092 + 35999.0502909 * T - 0.0001536 * pow( T, 2 ) + pow( T, 3 ) / 24490000  ;
	double M1 = 134.9633964 + 477198.8675055 * T + 0.0087414 * pow( T, 2 ) + pow( T, 3 ) / 69699 - pow(T, 4) / 14712000 ;
	double F = 93.2720950 + 483202.0175233 * T - 0.0036539 * pow( T, 2 ) - pow( T, 3 ) / 3526000 + pow(T, 4) / 863310000 ;

	double A1 = ( 119.75 + 131.849 * T ) ;
	double A2 = ( 53.09 + 479264.290 * T) ;
	double A3 = ( 313.45 + 481266.484 * T) ;

	double E = 1 - 0.002516 * T - 0.0000074 * pow( T, 2 )  ;


	PeriodicTerm	pT = periodicTerm ( D, M, M1, F, E );

	double addLongitude =
		3958 * sin( A1 * pi / 180 ) + 1962 * sin ( (L1 - F) * pi / 180  ) +
            318 * sin ( A2 * pi / 180  ) ;
	double addLatitude =
		-2235 * sin( L1 * pi / 180  ) + 382 * sin ( A3 * pi / 180  ) +
		175 * sin ( (A1 - F) * pi / 180  ) + 175 * sin ( (A1 + F) * pi / 180  ) +
		127 * sin ( (L1 - M1) * pi / 180  ) - 115 * sin ( (L1 + M1) * pi / 180  ) ;

	double	longitude = pT.longitude + addLongitude;
	double	distance = pT.distance ;
	double	latit = latitude ( D, M, M1, F, E ) + addLatitude;

	double lambda = L1 + longitude / 1000000;	// degree
	double beta = latit / 1000000;	// degree
	double delta = 385000.56 + distance / 1000;	// km

	OrbitEcli orbit ( lambda, beta, delta );
/*
    Angle l1 ;
	std::cout.precision(12);
    std::cout << "****" << std::endl;

    std::cout << "T=" << T << std::endl;
    l1.setGrade(L1);
    std::cout << "L1=" << l1.toGrade() << std::endl;
    l1.setGrade(D);
    std::cout << "D=" << l1.toGrade() << std::endl;
    l1.setGrade(M);
    std::cout << "M=" << l1.toGrade() << std::endl;
    l1.setGrade(M1);
    std::cout << "M1=" << l1.toGrade() << std::endl;
    l1.setGrade(F);
    std::cout << "F=" << l1.toGrade() << std::endl;
    l1.setGrade(A1);
    std::cout << "A1=" << l1.toGrade() << std::endl;
    l1.setGrade(A2);
    std::cout << "A2=" << l1.toGrade() << std::endl;
    l1.setGrade(A3);
    std::cout << "A3=" << l1.toGrade() << std::endl;
    l1.setGrade(E);
    std::cout << "E=" << l1.toGrade() << std::endl;

    std::cout << "suml=" << longitude << std::endl;
    std::cout << "sumb=" <<  latit << std::endl;
    std::cout << "sumr=" <<  distance << std::endl;

    l1.setGrade(lambda);
    std::cout << "lambda=" << l1.toGrade() << std::endl;
    l1.setGrade(delta);
    std::cout << "delta=" << delta << std::endl;
    std::cout << "****" << std::endl;
*/
	return orbit;

}


OrbitEcli Moon::orbitAlfaELP ( double JD )
{


	JCentury T = JDay ( JD ) ;

    double longitude = 0;
    double latitude = 0;
    double distance = 0;

    try {


        longitude =  (

                elp1.Longitude( T ) +

                elp4.Longitude( T ) +

                T * elp7.Longitude( T ) +

                elp10.Longitude( T ) +

                T * elp13.Longitude( T ) +

                elp16.Longitude( T ) +

                T * elp19.Longitude( T ) +

                elp22.Longitude( T ) +

                T * elp25.Longitude( T ) +

                elp28.Longitude( T ) +

                elp31.Longitude( T ) +

                T * T * elp34.Longitude(T)

            ) / 3600;


        DelaunayComplete d ( T );

        longitude += d.W1() + d.pA( );


        latitude  = (

                elp2.Latitude( T ) +

                elp5.Latitude( T ) +

                T * elp8.Latitude( T ) +

                elp11.Latitude( T ) +

                T * elp14.Latitude( T ) +

                elp17.Latitude( T ) +

                T * elp20.Latitude( T ) +

                elp23.Latitude( T ) +

                T * elp26.Latitude( T ) +

                elp29.Latitude( T ) +

                elp32.Latitude( T ) +

                T * T * elp35.Latitude( T )

            ) / 3600;

        distance =

                elp3.Distance( T ) +

                elp6.Distance( T ) +

                T * elp9.Distance( T ) +

                elp12.Distance( T ) +

                T * elp15.Distance( T ) +

                elp18.Distance( T ) +

                T * elp21.Distance( T ) +

                elp24.Distance( T ) +

                T * elp27.Distance( T ) +

                elp30.Distance( T ) +

                elp33.Distance( T ) +

                T * T * elp36.Distance( T );

    }

    catch ( const std::string & str ) {
        std::cout << str << std::endl;
    }



    OrbitEcli orbit ( longitude, latitude, distance  / CelestialBody::AU );

    return orbit;

}

std::string Moon::showOrbitalAlfaData ( double JD )
{


	OrbitEcli o = orbitAlfa ( JD );

	double lambda = o.L ;
	double beta = o.B ;
	double delta = o.R ;

	std::stringstream result;

	result.precision(10);
	result << "moon " << JD << " " << " lambda=" << lambda << " beta=" << beta << " delta=" << delta << std::endl;

	return result.str();


}

double Moon::angleME ( double JD, Earth & earth )
{

	OrbitEcli orbitEarth = earth.orbitLBR ( JD ) ;
	OrbitEcli orbitMoon = orbitAlfa ( JD ) ;

    double L_earth = orbitEarth.L;  // longitudine in radianti

    Angle l (L_earth);
    double l_earth = l.toGrade() - 180;   // riferiamolo alla terra e non al sole

    if ( l_earth < 0 ) l_earth += 360;

    double l_moon = orbitMoon.L;   // longitudine in gradi

    Angle l1 ;
    l1.setGrade(l_moon);    // normalizza i gradi
    l_moon = l1.toGrade();

    // inutile aggiungere nutazione e aberrazione
    // chè calcoliamo la differenza

    // 0 gradi luna nuova
    // 90 gradi primo quarto
    // 180 gradi è la luna piena
    // 270 gradi ultimo quarto
	return fabs( l_earth - l_moon )  ;

}

double Moon::angleME ( double JD )
{

    Moon moon;
    Earth earth ( & MainBody::earth_LBR_D_data );

    return angleME ( JD, earth );

}

double Moon::phases ( double JD, double phase )
{

    struct MoonPhase : public Funzione {

		double grade ;
		Variabile JD;

		Earth earth ;
		Moon moon;

		double eval () {

			double angle = moon.angleME( JD, earth );
			return angle - grade ;

		}

		MoonPhase ( double _grade ) :
            grade ( _grade ), earth ( & MainBody::earth_LBR_D_data ) {
		}

    } moonPhase (phase);

	moonPhase.JD = JD ;


	Math < Funzione, Variabile, double > m;
	m.setPrecisione ( 1e-8, 1e-8, 1e-7 ) ;
	while ( true ) {

        try {

            if ( m.risolveEquazione ( moonPhase, moonPhase.JD ) ) {

				return moonPhase.JD;

            }
            else
                throw ( "Unable to calculate moon phase" );

        }
        catch ( const std::string & str ) {

            throw ( str );


        }

	}

}

std::vector< double > Moon::nodes ( short year )
{

    struct MoonNodes : public Funzione {

		Variabile JD;

		Moon moon;

		double eval () {

			OrbitEcli orbitMoon = moon.orbitAlfa ( JD ) ;

            double l_moon = orbitMoon.B;   // latitudine in gradi

            // è sulla eclittica se la latidunine è zero
			return fabs( l_moon )  ;

		}

		MoonNodes ( )  {
		}

    } moonNodes ;

	std::stringstream ss ;
	ss << "01-01-" << year << " 00:00:00" ;

    Date date ( ss.str());

	double JD0 = date.julianDay();

    std::vector< double > v;
    moonNodes.JD = JD0;

	Math < Funzione, Variabile, double > m;
	m.setPrecisione ( 1e-8, 1e-8, 1e-6 ) ;
    short y = year;
	while ( y <= year ) {

        try {

            if ( m.risolveEquazione ( moonNodes, moonNodes.JD ) ) {

                double j = moonNodes.JD;
                Date d ( j );
                y = d.year();
                if ( y == year ) {
                    // è un nodo di quest'anno
                    v.push_back( moonNodes.JD );
                }
                // passa al prossimo nodo
                moonNodes.JD += 14;

            }
            else
                throw ( "Unable to calculate moon nodes" );

        }
        catch ( const std::string & str ) {

            throw ( str );


        }

    }

    return v;

}


