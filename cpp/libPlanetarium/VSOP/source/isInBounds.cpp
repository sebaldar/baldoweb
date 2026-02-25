#define _USE_MATH_DEFINES  // deve essere prima di <cmath>
#include <cmath>
#include <vector>

// Conversione coordinate celesti -> cartesiane unit sphere
inline void skyToCartesian(double raDeg, double decDeg,
                                  double &x, double &y, double &z)
{
    double raRad  = raDeg  * M_PI / 180.0;
    double decRad = decDeg * M_PI / 180.0;
    x = cos(decRad) * cos(raRad);
    y = cos(decRad) * sin(raRad);
    z = sin(decRad);
}

// Test punto in poligono sferico
bool isInBounds(const std::pair<double,double> &point,
                       const std::vector<std::pair<double,double>> &polygon)
{
    if (polygon.size() < 3) return false;

    double px, py, pz;
    skyToCartesian(point.first, point.second, px, py, pz);

    double totalAngle = 0.0;

    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();

        double x1, y1, z1, x2, y2, z2;
        skyToCartesian(polygon[i].first, polygon[i].second, x1, y1, z1);
        skyToCartesian(polygon[j].first, polygon[j].second, x2, y2, z2);

        // Vettori relativi a P
        double v1x = x1 - px;
        double v1y = y1 - py;
        double v1z = z1 - pz;

        double v2x = x2 - px;
        double v2y = y2 - py;
        double v2z = z2 - pz;

        // Normalizza
        double len1 = sqrt(v1x*v1x + v1y*v1y + v1z*v1z);
        double len2 = sqrt(v2x*v2x + v2y*v2y + v2z*v2z);
        v1x /= len1; v1y /= len1; v1z /= len1;
        v2x /= len2; v2y /= len2; v2z /= len2;

        // Angolo tra i due vettori
        double dot = v1x*v2x + v1y*v2y + v1z*v2z;
        if (dot >  1.0) dot =  1.0;
        if (dot < -1.0) dot = -1.0;
        double angle = acos(dot);

        // Determina segno
        double crossx = v1y*v2z - v1z*v2y;
        double crossy = v1z*v2x - v1x*v2z;
        double crossz = v1x*v2y - v1y*v2x;
        double sign = (px*crossx + py*crossy + pz*crossz) >= 0 ? 1.0 : -1.0;

        totalAngle += sign * angle;
    }

    return fabs(fabs(totalAngle) - 2*M_PI) < 1e-6;
}

// Normalizzazione RA per evitare problemi a RA=0
void normalizeRA(std::vector<std::pair<double,double>> &poly,
                        std::pair<double,double> &point)
{
    // Trova media RA
    double sumRA = 0;
    for (auto &p : poly) sumRA += p.first;
    double avgRA = sumRA / poly.size();

    // Se il poligono è vicino a RA=0/360, shift
    if (avgRA > 270 || avgRA < 90) {
        auto shift = [&](double ra) {
            if (ra > 180) return ra - 360.0;
            return ra;
        };
        for (auto &p : poly) p.first = shift(p.first);
        point.first = shift(point.first);
    }
}
