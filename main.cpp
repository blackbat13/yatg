#include <iostream>
#include "turtle.hpp"
using namespace std;

const int SIZE = 900;

Turtle turtle(SIZE, SIZE);

int main() {

    turtle.drawTurtle();

    turtle.saveBMP("image.bmp");

    return 0;
}