#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

const long double PI = 3.141592653589793238L;

class Point {

public:

    Point(float px, float py) : x(px), y(py) { };
    ~Point(void) { };

    float distance2(Point p) {
        return (this->x - p.x) * (this->x - p.x) + (this->x - p.y) * (this->y - p.y);
    }

    float distance(Point p) {
        return sqrt(this->distance2(p));
    }

    Point* closest(Point a, Point b) {
        float da = b.y - a.y;
        float db = a.x - b.x;
        float c1 = db * a.x + db * a.y;
        float c2 = -db * this->x + da * this->y;
        float det = da * da + db * db;
        float cx = 0;
        float cy = 0;

        if (det != 0) {
            cx = (da * c1 - db * c2) / det;
            cy = (da * c2 + db * c1) / det;
        } else {
            // Le point est déjà sur la droite
            cx = this->x;
            cy = this->y;
        }

        return new Point(cx, cy);
    }

    float x;
    float y;

};

class Pod : Point{

public:
    float getAngle(Point p) {
        float d = this->distance(p);
        float dx = (p.x - this->x) / d;
        float dy = (p.y - this->y) / d;

        // Trigonométrie simple. On multiplie par 180.0 / PI pour convertir en degré.
        float a = acos(dx) * 180.0 / PI;

        // Si le point qu'on veut est en dessus de nous, il faut décaler l'angle pour qu'il soit correct.
        if (dy < 0) {
            a = 360.0 - a;
        }

        return a;
    }

    float diffAngle(Point p) {
        float a = this->getAngle(p);

        // Pour connaitre le sens le plus proche, il suffit de regarder dans les 2 sens et on garde le plus petit
        // Les opérateurs ternaires sont la uniquement pour éviter l'utilisation d'un operateur % qui serait plus lent
        float right = this->angle <= a ? a - this->angle : 360.0 - this->angle + a;
        float left = this->angle >= a ? this->angle - a : this->angle + 360.0 - a;

        if (right < left) {
            return right;
        } else {
            // On donne un angle négatif s'il faut tourner à gauche
            return -left;
        }
    }

    void rotate(Point p) {
        float a = this->diffAngle(p);

        // On ne peut pas tourner de plus de 18° en un seul tour
        if (a > 18.0) {
            a = 18.0;
        } else if (a < -18.0) {
            a = -18.0;
        }

        this->angle += a;

        // L'opérateur % est lent. Si on peut l'éviter, c'est mieux.
        if (this->angle >= 360.0) {
            this->angle = this->angle - 360.0;
        } else if (this->angle < 0.0) {
            this->angle += 360.0;
        }
    }

    void boost(int thrust) {
        // N'oubliez pas qu'un pod qui a activé un shield ne peut pas accélérer pendant 3 tours
        if (this->shield) {
            return;
        }

        // Conversion de l'angle en radian
        float ra = this->angle * PI / 180.0;

        // Trigonométrie
        this->vx += cos(ra) * thrust;
        this->vy += sin(ra) * thrust;
    }

    void move(float t) {
        this->x += this->vx * t;
        this->y += this->vy * t;
    }

    void end() {
        this->x = round(this->x);
        this->y = round(this->y);
        this->vx = (int)(this->vx * 0.85);
        this->vy = (int)(this->vy * 0.85);

        // N'oubliez pas que le timeout descend de 1 chaque tour. Il revient à 100 quand on passe par un checkpoint
        this->timeout -= 1;
    }

    void play(Point p, int thrust) {
        this->rotate(p);
        this->boost(thrust);
        this->move(1.0);
        this->end();
    }

    float timeout;
    float angle;
    float vx;
    float vy;
    bool shield;
};

class Collision : Point {

public:

    Collision(Unit pu1, Unit pu2, float pdist) : u1(pu1), u2(pu2), dist(pdist) { };

    Unit u1;
    Unit u2;
    float dist;
};

class Unit : Point {

public:
    Collision* collision(Unit u) {
        // Distance carré
        float dist = this->distance2(u);

        // Somme des rayons au carré
        float sr = (this->r + u.r) * (this->r + u.r);

        // On prend tout au carré pour éviter d'avoir à appeler un sqrt inutilement. C'est mieux pour les performances

        if (dist < sr) {
            // Les objets sont déjà l'un sur l'autre. On a donc une collision immédiate
            return new Collision(this, u, 0.0);
        }

        // Optimisation. Les objets ont la même vitesse ils ne pourront jamais se rentrer dedans
        if (this->vx == u.vx && this->vy == u.vy) {
            return null;
        }

        // On se met dans le référentiel de u. u est donc immobile et se trouve sur le point (0,0) après ça
        float x = this->x - u.x;
        float y = this->y - u.y;
        Point myp = new Point(x, y);
        float vx = this->vx - u.vx;
        float vy = this->vy - u.vy;
        Point up = new Point(0, 0)

        // On cherche le point le plus proche de u (qui est donc en (0,0)) sur la droite décrite par notre vecteur de vitesse
        Point p = up.closest(myp, new Point(x + vx, y + vy));

        // Distance au carré entre u et le point le plus proche sur la droite décrite par notre vecteur de vitesse
        float pdist = up.distance2(p);

        // Distance au carré entre nous et ce point
        float mypdist = this->distance2(p);

        // Si la distance entre u et cette droite est inférieur à la somme des rayons, alors il y a possibilité de collision
        if (pdist < sr) {
            // Notre vitesse sur la droite
            float length = sqrt(vx*vx + vy*vy);

            // On déplace le point sur la droite pour trouver le point d'impact
            float backdist = sqrt(sr - pdist);
            p.x = p.x - backdist * (vx / length);
            p.y = p.y - backdist * (vy / length);

            // Si le point s'est éloigné de nous par rapport à avant, c'est que notre vitesse ne va pas dans le bon sens
            if (myp.distance2(p) > mypdist) {
                return null;
            }

            pdist = p.distance(myp);

            // Le point d'impact est plus loin que ce qu'on peut parcourir en un seul tour
            if (pdist > length) {
                return null;
            }

            // Temps nécessaire pour atteindre le point d'impact
            float t = pdist / length;

            return new Collision(this, u, t);
        }

        return null;
    }

    float r;
};

int main() {

    while (1) {
        int x;
        int y;
        int nextCheckpointX;
        int nextCheckpointY;
        int nextCheckpointDist;
        int nextCheckpointAngle;
        cin >> x >> y >> nextCheckpointX >> nextCheckpointY >> nextCheckpointDist >> nextCheckpointAngle; cin.ignore();
        int opponentX;
        int opponentY;
        cin >> opponentX >> opponentY; cin.ignore();

        cout << nextCheckpointX << " " << nextCheckpointY << " 80" << endl;
    }
}