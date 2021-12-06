/*
    turtle.cpp

    Simple array-based turtle graphics engine in C. Exports to BMP files.

    Author: Damian Kurpiewski, December 2021

    Based on a work by Mike Lam, James Madison University
    (https://w3.cs.jmu.edu/lam2mo/cs240_2015_08/turtle.html)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Known issues:

        - "filled" polygons are not always entirely filled, especially when
          side angles are very acute; there is probably an incorrect floating-
          point or integer conversion somewhere

*/

#include "yatg.hpp"


Turtle::Turtle(int width, int height)
{
    numPixelsOutOfBounds = 0;

    auto total_size = sizeof(rgb_t) * width * height;

    // free previous image array if necessary
    if (mainTurtleImage != nullptr) {
        free(mainTurtleImage);
        mainTurtleImage = nullptr;
    }

    // allocate new image and initialize it to white
    mainTurtleImage = (rgb_t*)malloc(total_size);
    if (mainTurtleImage == nullptr) {
        fprintf(stderr, "Can't allocate memory for turtle image.\n");
        exit(EXIT_FAILURE);
    }
    memset(mainTurtleImage, 255, total_size);

    // save field size for later
    mainFieldWidth = width;
    mainFieldHeight = height;

    // disable video
    mainFieldSaveFrames = false;

    // reset turtle position and color
    reset();
}

void Turtle::reset()
{
    // move turtle to middle of the field
    mainTurtle.xpos = 0.0;
    mainTurtle.ypos = 0.0;

    // orient to the right (0 deg)
    mainTurtle.heading = 0.0;

    // default draw color is black
    mainTurtle.penColor.red = 0;
    mainTurtle.penColor.green = 0;
    mainTurtle.penColor.blue = 0;

    // default fill color is black
    mainTurtle.fillColor.red = 0;
    mainTurtle.fillColor.green = 255;
    mainTurtle.fillColor.blue = 0;

    // default pen position is down
    mainTurtle.pendown = true;

    // default fill status is off
    mainTurtle.filled = false;
    mainTurtlePolyVertexCount = 0;
}

void Turtle::backup() {
    backupTurtle = mainTurtle;
}

void Turtle::restore() {
    mainTurtle = backupTurtle;
}

void Turtle::forward(int pixels)
{
    // calculate (x,y) movement vector from heading
    double radians = mainTurtle.heading * M_PI / 180.0;
    double dx = cos(radians) * pixels;
    double dy = sin(radians) * pixels;

    // delegate to another method to actually move
    goTo(mainTurtle.xpos + dx, mainTurtle.ypos + dy);
}

void Turtle::backward(int pixels)
{
    // opposite of "forward"
    forward(-pixels);
}

void Turtle::strafeLeft(int pixels) {
    turnLeft(90);
    forward(pixels);
    turnRight(90);
}

void Turtle::strafeRight(int pixels) {
    turnRight(90);
    forward(pixels);
    turnLeft(90);
}

void Turtle::turnLeft(double angle)
{
    // rotate turtle heading
    mainTurtle.heading += angle;

    // constrain heading to range: [0.0, 360.0)
    if (mainTurtle.heading < 0.0) {
        mainTurtle.heading += 360.0;
    } else if (mainTurtle.heading >= 360.0) {
        mainTurtle.heading -= 360.0;
    }
}

void Turtle::turnRight(double angle)
{
    // opposite of "turn left"
    turnLeft(-angle);
}

void Turtle::penUp()
{
    mainTurtle.pendown = false;
}

void Turtle::penDown()
{
    mainTurtle.pendown = true;
}

void Turtle::beginFill()
{
    mainTurtle.filled = true;
    mainTurtlePolyVertexCount = 0;
}

void Turtle::endFill()
{
    // based on public-domain fill algorithm in C by Darel Rex Finley, 2007
    //   from http://alienryderflex.com/polygon_fill/

    double nodeX[MAX_POLYGON_VERTICES];     // x-coords of polygon intercepts
    int nodes;                              // size of nodeX
    int x, y, i, j;                         // current pixel and loop indices
    double temp;                            // temporary variable for sorting

    //  loop through the rows of the image
    for (y = -(mainFieldHeight / 2); y < mainFieldHeight / 2; y++) {

        //  build a list of polygon intercepts on the current line
        nodes = 0;
        j = mainTurtlePolyVertexCount - 1;
        for (i = 0; i < mainTurtlePolyVertexCount; i++) {
            if ((mainTurtlePolyY[i] < (double)y &&
                 mainTurtlePolyY[j] >= (double)y) ||
                (mainTurtlePolyY[j] < (double)y &&
                 mainTurtlePolyY[i] >= (double)y)) {

                // intercept found; record it
                nodeX[nodes++] = (mainTurtlePolyX[i] +
                                  ((double)y - mainTurtlePolyY[i]) /
                                  (mainTurtlePolyY[j] - mainTurtlePolyY[i]) *
                                  (mainTurtlePolyX[j] - mainTurtlePolyX[i]));
            }
            j = i;
            if (nodes >= MAX_POLYGON_VERTICES) {
                fprintf(stderr, "Too many intercepts in fill algorithm!\n");
                exit(EXIT_FAILURE);
            }
        }

        //  sort the nodes via simple insertion sort
        for (i = 1; i < nodes; i++) {
            temp = nodeX[i];
            for (j = i; j > 0 && temp < nodeX[j-1]; j--) {
                nodeX[j] = nodeX[j-1];
            }
            nodeX[j] = temp;
        }

        //  fill the pixels between node pairs
        for (i = 0; i < nodes; i += 2) {
            for (x = (int)floor(nodeX[i])+1; x < (int)ceil(nodeX[i+1]); x++) {
                fillPixel(x, y);
            }
        }
    }

    mainTurtle.filled = false;

    // redraw polygon (filling is imperfect and can occasionally occlude sides)
    for (i = 0; i < mainTurtlePolyVertexCount; i++) {
        int x0 = (int)round(mainTurtlePolyX[i]);
        int y0 = (int)round(mainTurtlePolyY[i]);
        int x1 = (int)round(mainTurtlePolyX[(i + 1) %
                                            mainTurtlePolyVertexCount]);
        int y1 = (int)round(mainTurtlePolyY[(i + 1) %
                                            mainTurtlePolyVertexCount]);
        drawLine(x0, y0, x1, y1);
    }
}

void Turtle::goTo(int x, int y)
{
    goTo((double)x, (double)y);
}

void Turtle::goTo(double x, double y)
{
    // draw line if pen is down
    if (mainTurtle.pendown) {
        drawLine((int)round(mainTurtle.xpos),
                 (int)round(mainTurtle.ypos),
                 (int)round(x),
                 (int)round(y));
    }

    // change current turtle position
    mainTurtle.xpos = (double)x;
    mainTurtle.ypos = (double)y;

    // track coordinates for filling
    if (mainTurtle.filled && mainTurtle.pendown &&
        mainTurtlePolyVertexCount < MAX_POLYGON_VERTICES) {
        mainTurtlePolyX[mainTurtlePolyVertexCount] = x;
        mainTurtlePolyY[mainTurtlePolyVertexCount] = y;
        mainTurtlePolyVertexCount++;
    }
}

void Turtle::setHeading(double angle)
{
    mainTurtle.heading = angle;
}

void Turtle::setPenColor(int red, int green, int blue)
{
    mainTurtle.penColor.red = red;
    mainTurtle.penColor.green = green;
    mainTurtle.penColor.blue = blue;
}

void Turtle::setFillColor(int red, int green, int blue)
{
    mainTurtle.fillColor.red = red;
    mainTurtle.fillColor.green = green;
    mainTurtle.fillColor.blue = blue;
}

void Turtle::dot()
{
    // draw a pixel at the current location, regardless of pen status
    drawPixel((int)round(mainTurtle.xpos),
              (int)round(mainTurtle.ypos));
}



void Turtle::drawPixel(int x, int y)
{
    if (x < (- mainFieldWidth / 2) || x > (mainFieldWidth / 2) ||
        y < (-mainFieldHeight / 2) || y > (mainFieldHeight / 2)) {

        // only print the first 100 error messages (prevents runaway output)
        if (++numPixelsOutOfBounds < 100) {
            fprintf(stderr, "Pixel out of bounds: (%d,%d)\n", x, y);
        }
        return;
    }

    // calculate pixel offset in image data array
    int idx = mainFieldWidth * (y + mainFieldHeight / 2)
              + (x + mainFieldWidth / 2);

    // "draw" the pixel by setting the color values in the image matrix
    if (idx >= 0 && idx < mainFieldWidth * mainFieldHeight) {
        mainTurtleImage[idx].red   = mainTurtle.penColor.red;
        mainTurtleImage[idx].green = mainTurtle.penColor.green;
        mainTurtleImage[idx].blue  = mainTurtle.penColor.blue;
    }

    // track total pixels drawn and emit video frame if a frame interval has
    // been crossed (and only if video saving is enabled, of course)
    if (mainFieldSaveFrames &&
        mainFieldPixelCount++ % mainFieldFrameInterval == 0) {
        saveFrame();
    }
}

void Turtle::fillPixel(int x, int y)
{
    // calculate pixel offset in image data array
    int idx = mainFieldWidth * (y + mainFieldHeight / 2)
              + (x + mainFieldWidth / 2);

    // check to make sure it's not out of bounds
    if (idx >= 0 && idx < mainFieldWidth * mainFieldHeight) {
        mainTurtleImage[idx].red   = mainTurtle.fillColor.red;
        mainTurtleImage[idx].green = mainTurtle.fillColor.green;
        mainTurtleImage[idx].blue  = mainTurtle.fillColor.blue;
    }
}

void Turtle::drawLine(int x0, int y0, int x1, int y1)
{
    // uses a variant of Bresenham's line algorithm:
    //   https://en.wikipedia.org/wiki/Talk:Bresenham%27s_line_algorithm

    int absX = abs(x1-x0);          // absolute value of coordinate distances
    int absY = abs(y1-y0);
    int offX = x0<x1 ? 1 : -1;      // line-drawing direction offsets
    int offY = y0<y1 ? 1 : -1;
    int x = x0;                     // incremental location
    int y = y0;
    int err;

    drawPixel(x, y);
    if (absX > absY) {

        // line is more horizontal; increment along x-axis
        err = absX / 2;
        while (x != x1) {
            err = err - absY;
            if (err < 0) {
                y   += offY;
                err += absX;
            }
            x += offX;
            drawPixel(x,y);
        }
    } else {

        // line is more vertical; increment along y-axis
        err = absY / 2;
        while (y != y1) {
            err = err - absX;
            if (err < 0) {
                x   += offX;
                err += absY;
            }
            y += offY;
            drawPixel(x,y);
        }
    }
}

void Turtle::drawCircle(int x0, int y0, int radius)
{
    // implementation based on midpoint circle algorithm:
    //   https://en.wikipedia.org/wiki/Midpoint_circle_algorithm

    int x = radius;
    int y = 0;
    int switch_criteria = 1 - x;

    if (mainTurtle.filled) {
        fillCircle(x0, y0, radius);
    }

    while (x >= y) {
        drawPixel( x + x0,  y + y0);
        drawPixel( y + x0,  x + y0);
        drawPixel(-x + x0,  y + y0);
        drawPixel(-y + x0,  x + y0);
        drawPixel(-x + x0, -y + y0);
        drawPixel(-y + x0, -x + y0);
        drawPixel( x + x0, -y + y0);
        drawPixel( y + x0, -x + y0);
        y++;
        if (switch_criteria <= 0) {
            switch_criteria += 2 * y + 1;       // no x-coordinate change
        } else {
            x--;
            switch_criteria += 2 * (y - x) + 1;
        }
    }
}

void Turtle::fillCircle(int x0, int y0, int radius) {

    int rad_sq = radius * radius;

    // Naive algorithm, pretty ugly due to no antialiasing:
    for (int x = x0 - radius; x < x0 + radius; x++) {
        for (int y = y0 - radius; y < y0 + radius; y++) {
            int dx = x - x0;
            int dy = y - y0;
            int dsq = (dx * dx) + (dy * dy);
            if (dsq < rad_sq) fillPixel(x, y);
        }
    }
}

void Turtle::fillCircle(int radius)
{
    fillCircle(mainTurtle.xpos, mainTurtle.ypos, radius);
}

void Turtle::drawTurtle()
{
    // We are going to make our own backup of the turtle, since turtle_backup()
    // only gives us one level of undo.
    turtle_t original_turtle = mainTurtle;

    penUp();

    // Draw the legs
    for (int i = -1; i < 2; i+=2) {
        for (int j = -1; j < 2; j+=2) {
            backup();
            forward(i * 7);
            strafeLeft(j * 7);

            setFillColor(
                    mainTurtle.penColor.red,
                    mainTurtle.penColor.green,
                    mainTurtle.penColor.blue
            );
            fillCircle(5);

            setFillColor(
                    original_turtle.fillColor.red,
                    original_turtle.fillColor.green,
                    original_turtle.fillColor.blue
            );
            fillCircle(3);
            restore();
        }
    }

    // Draw the head
    backup();
    forward(10);
    setFillColor(
            mainTurtle.penColor.red,
            mainTurtle.penColor.green,
            mainTurtle.penColor.blue
    );
    fillCircle(5);

    setFillColor(
            original_turtle.fillColor.red,
            original_turtle.fillColor.green,
            original_turtle.fillColor.blue
    );
    fillCircle(3);
    restore();

    // Draw the body
    for (int i = 9; i >= 0; i-=4) {
        backup();
        setFillColor(
                mainTurtle.penColor.red,
                mainTurtle.penColor.green,
                mainTurtle.penColor.blue
        );
        fillCircle(i+2);

        setFillColor(
                original_turtle.fillColor.red,
                original_turtle.fillColor.green,
                original_turtle.fillColor.blue
        );
        fillCircle(i);
        restore();
    }

    // Restore the original turtle position:
    mainTurtle = original_turtle;
}

void Turtle::beginVideo(int pixels_per_frame)
{
    mainFieldSaveFrames = true;
    mainFieldFrameCount = 0;
    mainFieldFrameInterval = pixels_per_frame;
    mainFieldPixelCount = 0;
}

void Turtle::saveFrame()
{
    char filename[32];
    sprintf(filename, "frame%05d.bmp", ++mainFieldFrameCount);
    saveBMP(filename);
}

void Turtle::endVideo()
{
    mainFieldSaveFrames = false;
}

double Turtle::getX()
{
    return mainTurtle.xpos;
}

double Turtle::getY()
{
    return mainTurtle.ypos;
}

void Turtle::drawInt(int value)
{
    // calculate number of digits to draw
    int ndigits = 1;
    if (value > 9) {
        ndigits = (int)(ceil(log10(value)));
    }

    // draw each digit
    for (int i=ndigits-1; i>=0; i--) {
        int digit = value % 10;
        for (int y=0; y<5; y++) {
            for (int x=0; x<4; x++) {
                if (TURTLE_DIGITS[digit][y*4+x] == 1) {
                    drawPixel(mainTurtle.xpos + i * 5 + x, mainTurtle.ypos - y);
                }
            }
        }
        value = value / 10;
    }
}

void Turtle::cleanup()
{
    // free image array if allocated
    if (mainTurtleImage != nullptr) {
        free(mainTurtleImage);
        mainTurtleImage = nullptr;
    }
}

Turtle::~Turtle() {
    cleanup();
}

void Turtle::saveBMP(const char *filename)
{
    int i, j, ipos;
    int bytesPerLine;
    unsigned char *line;
    FILE *file;
    BMPHeader bmph{};
    int width = mainFieldWidth;
    int height = mainFieldHeight;
    char *rgb = (char*)mainTurtleImage;

    // the length of each line must be a multiple of 4 bytes
    bytesPerLine = (3 * (width + 1) / 4) * 4;

    strncpy(bmph.bfType, "BM", 2);
    bmph.bfOffBits = 54;
    bmph.bfSize = bmph.bfOffBits + bytesPerLine * height;
    bmph.bfReserved = 0;
    bmph.biSize = 40;
    bmph.biWidth = width;
    bmph.biHeight = height;
    bmph.biPlanes = 1;
    bmph.biBitCount = 24;
    bmph.biCompression = 0;
    bmph.biSizeImage = bytesPerLine * height;
    bmph.biXPelsPerMeter = 0;
    bmph.biYPelsPerMeter = 0;
    bmph.biClrUsed = 0;
    bmph.biClrImportant = 0;

    file = fopen (filename, "wb");
    if (file == nullptr) {
        fprintf(stderr, "Could not write to file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fwrite(&bmph.bfType, 2, 1, file);
    fwrite(&bmph.bfSize, 4, 1, file);
    fwrite(&bmph.bfReserved, 4, 1, file);
    fwrite(&bmph.bfOffBits, 4, 1, file);
    fwrite(&bmph.biSize, 4, 1, file);
    fwrite(&bmph.biWidth, 4, 1, file);
    fwrite(&bmph.biHeight, 4, 1, file);
    fwrite(&bmph.biPlanes, 2, 1, file);
    fwrite(&bmph.biBitCount, 2, 1, file);
    fwrite(&bmph.biCompression, 4, 1, file);
    fwrite(&bmph.biSizeImage, 4, 1, file);
    fwrite(&bmph.biXPelsPerMeter, 4, 1, file);
    fwrite(&bmph.biYPelsPerMeter, 4, 1, file);
    fwrite(&bmph.biClrUsed, 4, 1, file);
    fwrite(&bmph.biClrImportant, 4, 1, file);

    line = (unsigned char*)malloc(bytesPerLine);
    memset(line, 0, bytesPerLine);
    if (line == nullptr) {
        fprintf(stderr, "Can't allocate memory for BMP file.\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            ipos = 3 * (width * i + j);
            line[3*j] = rgb[ipos + 2];
            line[3*j+1] = rgb[ipos + 1];
            line[3*j+2] = rgb[ipos];
        }
        fwrite(line, bytesPerLine, 1, file);
    }

    free(line);
    fclose(file);
}