#ifndef _point_
#define _point_

#include <iostream>
#include <vector>
#include <cmath>     // per std::sqrt, std::abs, sin, cos, atan
#include <algorithm> // per std::abs (opzionale)

class Point  {


    friend class F1;
    friend class F2;
    friend class F3;

    friend class Line;

protected:

public:


	// calcola il punto medio tra due punti
	static Point middle ( const Point &, const Point & );
	// calcola il punto p% intermedio tra due punti
	static Point intermediate ( const Point &, const Point &, double p );
	// calcola la distanza tra due punti
	static double distance ( const Point &, const Point & ) ;
	// ritorna la radice della distanza tra due punti
	static double distance2 ( const Point &, const Point & ) ;
	
	
	double x, y, z;
	Point ( ) ;
	Point ( double , double , double  ) ;
	Point ( const Point & point ) ;
	Point ( const std::vector< double > & v ) ;

	// calcola la distanza tra this e p
	double distance ( const Point & p ) const ;
	
	Point &operator = ( const Point & p );


 };


class Direction {
public:
    double x, y, z;

    Direction() : x(0.0), y(0.0), z(0.0) {}

    // direzione che unisce p all'origine
    explicit Direction(const Point& p)
        : x(p.x), y(p.y), z(p.z) {}

    // Costruttore da tre double → NON delegare per evitare problemi
    Direction(double xx, double yy, double zz)
        : x(xx), y(yy), z(zz) {}

    // Copy constructor
    Direction(const Direction& d)
        : x(d.x), y(d.y), z(d.z) {}

    // la direzione che unisce il punto p1 al punto p2
    Direction(const Point& p1, const Point& p2)
        : x(p2.x - p1.x),
          y(p2.y - p1.y),
          z(p2.z - p1.z) {}          // ← CORRETTO: era p2.z - p2.z !

    Direction& operator=(const Direction& d) {
        x = d.x;
        y = d.y;
        z = d.z;
        return *this;
    }

    Direction& operator=(const Point& p) {
        x = p.x;
        y = p.y;
        z = p.z;
        return *this;
    }

    // direzione opposta
    Direction oppost() const {           // meglio const
        return Direction(-x, -y, -z);
    }

    // NEGAZIONE (molto utile e usata spesso)
    Direction operator-() const {
        return Direction(-x, -y, -z);
    }

    // ───────────────────────────────────────────────
    // Metodi NECESSARI per far funzionare Horizont::track()
    // ───────────────────────────────────────────────

    double length() const {
        return std::sqrt(x*x + y*y + z*z);
    }

    Direction normalized() const {
        double len = length();
        if (len < 1e-10) {              // protezione contro vettore nullo
            return Direction(0.0, 0.0, 1.0);
        }
        return Direction(x / len, y / len, z / len);
    }

    // Prodotto vettoriale (cross product)
    Direction cross(const Direction& other) const {
        return Direction(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // Moltiplicazione per scalare  (es: est * 5.0)
    Direction operator*(double s) const {
        return Direction(x * s, y * s, z * s);
    }

    // Somma (utile in futuro)
    Direction operator+(const Direction& other) const {
        return Direction(x + other.x, y + other.y, z + other.z);
    }
};


#endif	// if _point_
