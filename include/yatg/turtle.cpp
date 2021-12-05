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

#include "turtle.hpp"


Turtle::Turtle(int width, int height)
{
    num_pixels_out_of_bounds = 0;

    int total_size = sizeof(rgb_t) * width * height;

    // free previous image array if necessary
    if (main_turtle_image != NULL) {
        free(main_turtle_image);
        main_turtle_image = NULL;
    }

    // allocate new image and initialize it to white
    main_turtle_image = (rgb_t*)malloc(total_size);
    if (main_turtle_image == NULL) {
        fprintf(stderr, "Can't allocate memory for turtle image.\n");
        exit(EXIT_FAILURE);
    }
    memset(main_turtle_image, 255, total_size);

    // save field size for later
    main_field_width = width;
    main_field_height = height;

    // disable video
    main_field_save_frames = false;

    // reset turtle position and color
    reset();
}

void Turtle::reset()
{
    // move turtle to middle of the field
    main_turtle.xpos = 0.0;
    main_turtle.ypos = 0.0;

    // orient to the right (0 deg)
    main_turtle.heading = 0.0;

    // default draw color is black
    main_turtle.pen_color.red = 0;
    main_turtle.pen_color.green = 0;
    main_turtle.pen_color.blue = 0;

    // default fill color is black
    main_turtle.fill_color.red = 0;
    main_turtle.fill_color.green = 255;
    main_turtle.fill_color.blue = 0;

    // default pen position is down
    main_turtle.pendown = true;

    // default fill status is off
    main_turtle.filled = false;
    main_turtle_poly_vertex_count = 0;
}

void Turtle::backup() {
    backup_turtle = main_turtle;
}

void Turtle::restore() {
    main_turtle = backup_turtle;
}

void Turtle::forward(int pixels)
{
    // calculate (x,y) movement vector from heading
    double radians = main_turtle.heading * PI / 180.0;
    double dx = cos(radians) * pixels;
    double dy = sin(radians) * pixels;

    // delegate to another method to actually move
    goTo(main_turtle.xpos + dx, main_turtle.ypos + dy);
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
    main_turtle.heading += angle;

    // constrain heading to range: [0.0, 360.0)
    if (main_turtle.heading < 0.0) {
        main_turtle.heading += 360.0;
    } else if (main_turtle.heading >= 360.0) {
        main_turtle.heading -= 360.0;
    }
}

void Turtle::turnRight(double angle)
{
    // opposite of "turn left"
    turnLeft(-angle);
}

void Turtle::penUp()
{
    main_turtle.pendown = false;
}

void Turtle::penDown()
{
    main_turtle.pendown = true;
}

void Turtle::beginFill()
{
    main_turtle.filled = true;
    main_turtle_poly_vertex_count = 0;
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
    for (y = -(main_field_height/2); y < main_field_height/2; y++) {

        //  build a list of polygon intercepts on the current line
        nodes = 0;
        j = main_turtle_poly_vertex_count-1;
        for (i = 0; i < main_turtle_poly_vertex_count; i++) {
            if ((main_turtle_polyY[i] <  (double)y &&
                 main_turtle_polyY[j] >= (double)y) ||
                (main_turtle_polyY[j] <  (double)y &&
                 main_turtle_polyY[i] >= (double)y)) {

                // intercept found; record it
                nodeX[nodes++] = (main_turtle_polyX[i] +
                                  ((double)y - main_turtle_polyY[i]) /
                                  (main_turtle_polyY[j] - main_turtle_polyY[i]) *
                                  (main_turtle_polyX[j] - main_turtle_polyX[i]));
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

    main_turtle.filled = false;

    // redraw polygon (filling is imperfect and can occasionally occlude sides)
    for (i = 0; i < main_turtle_poly_vertex_count; i++) {
        int x0 = (int)round(main_turtle_polyX[i]);
        int y0 = (int)round(main_turtle_polyY[i]);
        int x1 = (int)round(main_turtle_polyX[(i+1) %
                                              main_turtle_poly_vertex_count]);
        int y1 = (int)round(main_turtle_polyY[(i+1) %
                                              main_turtle_poly_vertex_count]);
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
    if (main_turtle.pendown) {
        drawLine((int)round(main_turtle.xpos),
                 (int)round(main_turtle.ypos),
                 (int)round(x),
                 (int)round(y));
    }

    // change current turtle position
    main_turtle.xpos = (double)x;
    main_turtle.ypos = (double)y;

    // track coordinates for filling
    if (main_turtle.filled && main_turtle.pendown &&
        main_turtle_poly_vertex_count < MAX_POLYGON_VERTICES) {
        main_turtle_polyX[main_turtle_poly_vertex_count] = x;
        main_turtle_polyY[main_turtle_poly_vertex_count] = y;
        main_turtle_poly_vertex_count++;
    }
}

void Turtle::setHeading(double angle)
{
    main_turtle.heading = angle;
}

void Turtle::setPenColor(int red, int green, int blue)
{
    main_turtle.pen_color.red = red;
    main_turtle.pen_color.green = green;
    main_turtle.pen_color.blue = blue;
}

void Turtle::setFillColor(int red, int green, int blue)
{
    main_turtle.fill_color.red = red;
    main_turtle.fill_color.green = green;
    main_turtle.fill_color.blue = blue;
}

void Turtle::dot()
{
    // draw a pixel at the current location, regardless of pen status
    drawPixel((int)round(main_turtle.xpos),
              (int)round(main_turtle.ypos));
}



void Turtle::drawPixel(int x, int y)
{
    if (x < (- main_field_width/2)  || x > (main_field_width/2) ||
        y < (-main_field_height/2) || y > (main_field_height/2)) {

        // only print the first 100 error messages (prevents runaway output)
        if (++num_pixels_out_of_bounds < 100) {
            fprintf(stderr, "Pixel out of bounds: (%d,%d)\n", x, y);
        }
        return;
    }

    // calculate pixel offset in image data array
    int idx = main_field_width * (y+main_field_height/2)
              + (x+main_field_width/2);

    // "draw" the pixel by setting the color values in the image matrix
    if (idx >= 0 && idx < main_field_width*main_field_height) {
        main_turtle_image[idx].red   = main_turtle.pen_color.red;
        main_turtle_image[idx].green = main_turtle.pen_color.green;
        main_turtle_image[idx].blue  = main_turtle.pen_color.blue;
    }

    // track total pixels drawn and emit video frame if a frame interval has
    // been crossed (and only if video saving is enabled, of course)
    if (main_field_save_frames &&
        main_field_pixel_count++ % main_field_frame_interval == 0) {
        saveFrame();
    }
}

void Turtle::fillPixel(int x, int y)
{
    // calculate pixel offset in image data array
    int idx = main_field_width * (y+main_field_height/2)
              + (x+main_field_width/2);

    // check to make sure it's not out of bounds
    if (idx >= 0 && idx < main_field_width*main_field_height) {
        main_turtle_image[idx].red   = main_turtle.fill_color.red;
        main_turtle_image[idx].green = main_turtle.fill_color.green;
        main_turtle_image[idx].blue  = main_turtle.fill_color.blue;
    }
}

void Turtle::drawLine(int x0, int y0, int x1, int y1)
{
    // uses a variant of Bresenham's line algorithm:
    //   https://en.wikipedia.org/wiki/Talk:Bresenham%27s_line_algorithm

    int absX = ABS(x1-x0);          // absolute value of coordinate distances
    int absY = ABS(y1-y0);
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

    if (main_turtle.filled) {
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
    fillCircle(main_turtle.xpos, main_turtle.ypos, radius);
}

void Turtle::drawTurtle()
{
    // We are going to make our own backup of the turtle, since turtle_backup()
    // only gives us one level of undo.
    turtle_t original_turtle = main_turtle;

    penUp();

    // Draw the legs
    for (int i = -1; i < 2; i+=2) {
        for (int j = -1; j < 2; j+=2) {
            backup();
            forward(i * 7);
            strafeLeft(j * 7);

            setFillColor(
                    main_turtle.pen_color.red,
                    main_turtle.pen_color.green,
                    main_turtle.pen_color.blue
            );
            fillCircle(5);

            setFillColor(
                    original_turtle.fill_color.red,
                    original_turtle.fill_color.green,
                    original_turtle.fill_color.blue
            );
            fillCircle(3);
            restore();
        }
    }

    // Draw the head
    backup();
    forward(10);
    setFillColor(
            main_turtle.pen_color.red,
            main_turtle.pen_color.green,
            main_turtle.pen_color.blue
    );
    fillCircle(5);

    setFillColor(
            original_turtle.fill_color.red,
            original_turtle.fill_color.green,
            original_turtle.fill_color.blue
    );
    fillCircle(3);
    restore();

    // Draw the body
    for (int i = 9; i >= 0; i-=4) {
        backup();
        setFillColor(
                main_turtle.pen_color.red,
                main_turtle.pen_color.green,
                main_turtle.pen_color.blue
        );
        fillCircle(i+2);

        setFillColor(
                original_turtle.fill_color.red,
                original_turtle.fill_color.green,
                original_turtle.fill_color.blue
        );
        fillCircle(i);
        restore();
    }

    // Restore the original turtle position:
    main_turtle = original_turtle;
}

void Turtle::beginVideo(int pixels_per_frame)
{
    main_field_save_frames = true;
    main_field_frame_count = 0;
    main_field_frame_interval = pixels_per_frame;
    main_field_pixel_count = 0;
}

void Turtle::saveFrame()
{
    char filename[32];
    sprintf(filename, "frame%05d.bmp", ++main_field_frame_count);
    saveBMP(filename);
}

void Turtle::endVideo()
{
    main_field_save_frames = false;
}

double Turtle::getX()
{
    return main_turtle.xpos;
}

double Turtle::getY()
{
    return main_turtle.ypos;
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
                    drawPixel(main_turtle.xpos + i*5 + x, main_turtle.ypos - y);
                }
            }
        }
        value = value / 10;
    }
}

void Turtle::cleanup()
{
    // free image array if allocated
    if (main_turtle_image != NULL) {
        free(main_turtle_image);
        main_turtle_image = NULL;
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
    struct BMPHeader bmph;
    int width = main_field_width;
    int height = main_field_height;
    char *rgb = (char*)main_turtle_image;

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
    if (file == NULL) {
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
    if (line == NULL) {
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