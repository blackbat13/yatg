/*
    turtle.hpp

    Simple array-based turtle graphics engine in CPP.
    Exports to BMP files.

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
*/

#ifndef TURTLEGRAPHICS_YATG_HPP
#define TURTLEGRAPHICS_YATG_HPP


#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#define MAX_POLYGON_VERTICES 128

struct rgb {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct turtleState {
    double xpos;       // current position and heading
    double ypos;       // (uses floating-point numbers for
    double heading;    //  increased accuracy)

    rgb strokeColor;   // current pen color
    rgb fillColor;  // current fill color
    bool pendown;     // currently drawing?
    bool filled;      // currently filling?
};

struct BMPHeader {
    char bfType[2];       // "BM"
    int bfSize;           // size of file in bytes
    int bfReserved;       // set to 0
    int bfOffBits;        // byte offset to actual bitmap data (= 54)
    int biSize;           // size of BITMAPINFOHEADER, in bytes (= 40)
    int biWidth;          // width of image, in pixels
    int biHeight;         // height of images, in pixels
    short biPlanes;       // number of planes in target device (set to 1)
    short biBitCount;     // bits per pixel (24 in this case)
    int biCompression;    // type of compression (0 if no compression)
    int biSizeImage;      // image size, in bytes (0 if no compression)
    int biXPelsPerMeter;  // resolution in pixels/meter of display device
    int biYPelsPerMeter;  // resolution in pixels/meter of display device
    int biClrUsed;        // number of colors in the color table (if 0, use maximum allowed by biBitCount)
    int biClrImportant;   // number of important colors.  If 0, all colors are important
};

class Turtle {
    turtleState mainTurtle{};
    turtleState backupTurtle{};

    rgb *mainTurtleImage = nullptr;        // 2d pixel data field

    unsigned int mainFieldWidth = 0;           // size in pixels
    unsigned int mainFieldHeight = 0;

    bool mainFieldSaveFrames = false;  // currently saving video frames?
    int mainFieldFrameCount = 0;   // current video frame counter
    int mainFieldFrameInterval = 10;  // pixels per frame
    int mainFieldPixelCount = 0;   // total pixels drawn by turtle since

    // beginning of video
    int mainTurtlePolyVertexCount = 0;       // polygon vertex count
    double mainTurtlePolyX[MAX_POLYGON_VERTICES]{}; // polygon vertex x-coords
    double mainTurtlePolyY[MAX_POLYGON_VERTICES]{}; // polygon vertex y-coords

    unsigned long long int numPixelsOutOfBounds;

    const int TURTLE_DIGITS[10][20] = {

            {0, 1, 1, 0,       // 0
                1, 0, 0, 1,
                1, 0, 0, 1,
                1, 0, 0, 1,
                0, 1, 1, 0},

            {0, 1, 1, 0,       // 1
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 1, 1, 1},

            {1, 1, 1, 0,       // 2
                0, 0, 0, 1,
                0, 1, 1, 0,
                1, 0, 0, 0,
                1, 1, 1, 1},

            {1, 1, 1, 0,       // 3
                0, 0, 0, 1,
                0, 1, 1, 0,
                0, 0, 0, 1,
                1, 1, 1, 0},

            {0, 1, 0, 1,       // 4
                0, 1, 0, 1,
                0, 1, 1, 1,
                0, 0, 0, 1,
                0, 0, 0, 1},

            {1, 1, 1, 1,       // 5
                1, 0, 0, 0,
                1, 1, 1, 0,
                0, 0, 0, 1,
                1, 1, 1, 0},

            {0, 1, 1, 0,       // 6
                1, 0, 0, 0,
                1, 1, 1, 0,
                1, 0, 0, 1,
                0, 1, 1, 0},

            {1, 1, 1, 1,       // 7
                0, 0, 0, 1,
                0, 0, 1, 0,
                0, 1, 0, 0,
                0, 1, 0, 0},

            {0, 1, 1, 0,       // 8
                1, 0, 0, 1,
                0, 1, 1, 0,
                1, 0, 0, 1,
                0, 1, 1, 0},

            {0, 1, 1, 0,       // 9
                1, 0, 0, 1,
                0, 1, 1, 1,
                0, 0, 0, 1,
                0, 1, 1, 0},

    };
public:

    /**
     * Initializes the 2d field that the turtle moves on.
     * @param width field width
     * @param height field height
     */
    Turtle(unsigned int width, unsigned int height) {
        numPixelsOutOfBounds = 0;

        auto totalSize = sizeof(rgb) * width * height;

        // allocate new image and initialize it to white
        mainTurtleImage = (rgb *) malloc(totalSize);
        if (mainTurtleImage == nullptr) {
            fprintf(stderr, "Can't allocate memory for turtle image.\n");
            exit(EXIT_FAILURE);
        }
        memset(mainTurtleImage, 255, totalSize);

        // save field size for later
        mainFieldWidth = width;
        mainFieldHeight = height;

        // disable video
        mainFieldSaveFrames = false;

        // reset turtle position and color
        reset();

        // create backup at the initial position
        backup();
    }

    ~Turtle() {
        cleanup();
    }

    /**
     * Resets the turtle's location, orientation, color, and pen status to the default values:
     * center of the field (0,0), facing right (0 degrees), black stroke and green fill color, pen down.
     */
    void reset() {
        // move turtle to middle of the field
        mainTurtle.xpos = 0.0;
        mainTurtle.ypos = 0.0;

        // orient to the right (0 deg)
        mainTurtle.heading = 0.0;

        // default stroke color is black
        mainTurtle.strokeColor.red = 0;
        mainTurtle.strokeColor.green = 0;
        mainTurtle.strokeColor.blue = 0;

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


    /**
     * Creates a backup of the current turtle.
     * The backup can be restored by using restore().
     * Useful in complex drawing situations.
     */
    void backup() {
        backupTurtle = mainTurtle;
    }


    /**
     * Restores the turtle from the backup.
     */
    void restore() {
        mainTurtle = backupTurtle;
    }


    /**
     * Moves the turtle forward, drawing a straight line if the pen is down.
     * @param pixels movement distance
     */
    void forward(int pixels) {
        // calculate (x,y) movement vector from heading
        double radians = mainTurtle.heading * M_PI / 180.0;
        double dx = cos(radians) * pixels;
        double dy = sin(radians) * pixels;

        // delegate to another method to actually move
        goTo(mainTurtle.xpos + dx, mainTurtle.ypos + dy);
    }


    /**
     * Moves the turtle backward, drawing a straight line if the pen is down.
     * @param pixels movement distance
     */
    void backward(int pixels) {
        // opposite of "forward"
        forward(-pixels);
    }

    /**
     * Moves the turtle left.
     * @param pixels movement distance
     */
    void strafeLeft(int pixels) {
        turnLeft(90);
        forward(pixels);
        turnRight(90);
    }

    /**
     * Moves the turtle right.
     * @param pixels movement distance
     */
    void strafeRight(int pixels) {
        turnRight(90);
        forward(pixels);
        turnLeft(90);
    }


    /**
     * Turns the turtle to the left by the specified number of degrees.
     * @param angle
     */
    void turnLeft(double angle) {
        // rotate turtle heading
        mainTurtle.heading += angle;

        // constrain heading to range: [0.0, 360.0)
        if (mainTurtle.heading < 0.0) {
            mainTurtle.heading += 360.0;
        } else if (mainTurtle.heading >= 360.0) {
            mainTurtle.heading -= 360.0;
        }
    }


    /**
     * Turns the turtle to the right by the specified number of degrees.
     * @param angle
     */
    void turnRight(double angle) {
        // opposite of "turn left"
        turnLeft(-angle);
    }


    /**
     * Sets the pen status to "up" (do not draw).
     */
    void penUp() {
        mainTurtle.pendown = false;
    }


    /**
     * Sets the pen status to "down" (draw).
     */
    void penDown() {
        mainTurtle.pendown = true;
    }


    /**
     * Starts filling.
     * Call this before drawing a polygon to activate the bookkeeping required to run the filling algorithm later.
     */
    void beginFill() {
        mainTurtle.filled = true;
        mainTurtlePolyVertexCount = 0;
    }


    /**
     * Ends filling.
     * Call this after drawing a polygon to trigger the fill algorithm.
     * The filled polygon may have up to 128 sides.
     */
    void endFill() {
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
                if ((mainTurtlePolyY[i] < (double) y &&
                     mainTurtlePolyY[j] >= (double) y) ||
                    (mainTurtlePolyY[j] < (double) y &&
                     mainTurtlePolyY[i] >= (double) y)) {

                    // intercept found; record it
                    nodeX[nodes++] = (mainTurtlePolyX[i] +
                                      ((double) y - mainTurtlePolyY[i]) /
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
                for (j = i; j > 0 && temp < nodeX[j - 1]; j--) {
                    nodeX[j] = nodeX[j - 1];
                }
                nodeX[j] = temp;
            }

            //  fill the pixels between node pairs
            for (i = 0; i < nodes; i += 2) {
                for (x = (int) floor(nodeX[i]) + 1; x < (int) ceil(nodeX[i + 1]); x++) {
                    fillPixel(x, y);
                }
            }
        }

        mainTurtle.filled = false;

        // redraw polygon (filling is imperfect and can occasionally occlude sides)
        for (i = 0; i < mainTurtlePolyVertexCount; i++) {
            int x0 = (int) round(mainTurtlePolyX[i]);
            int y0 = (int) round(mainTurtlePolyY[i]);
            int x1 = (int) round(mainTurtlePolyX[(i + 1) %
                                                 mainTurtlePolyVertexCount]);
            int y1 = (int) round(mainTurtlePolyY[(i + 1) %
                                                 mainTurtlePolyVertexCount]);
            drawLine(x0, y0, x1, y1);
        }
    }


    /**
     * Moves the turtle to the specified location, drawing a straight line if the pen is down.
     * Takes integer coordinate parameters.
     * @param x
     * @param y
     */
    void goTo(int x, int y) {
        goTo((double) x, (double) y);
    }


    /**
     * Moves the turtle to the specified location, drawing a straight line if the pen is down.
     * Takes real-numbered coordinate parameters, and is also used internally to implement forward and backward motion.
     * @param x
     * @param y
     */
    void goTo(double x, double y) {
        // draw line if pen is down
        if (mainTurtle.pendown) {
            drawLine((int) round(mainTurtle.xpos),
                     (int) round(mainTurtle.ypos),
                     (int) round(x),
                     (int) round(y));
        }

        // change current turtle position
        mainTurtle.xpos = (double) x;
        mainTurtle.ypos = (double) y;

        // track coordinates for filling
        if (mainTurtle.filled && mainTurtle.pendown &&
            mainTurtlePolyVertexCount < MAX_POLYGON_VERTICES) {
            mainTurtlePolyX[mainTurtlePolyVertexCount] = x;
            mainTurtlePolyY[mainTurtlePolyVertexCount] = y;
            mainTurtlePolyVertexCount++;
        }
    }


    /**
     * Rotates the turtle to the given heading (in degrees).
     * 0 degrees means facing to the right.
     * 90 degrees means facing straight up.
     * @param angle
     */
    void setHeading(double angle) {
        mainTurtle.heading = angle;
    }


    /**
     * Set the current drawing color.
     * Each component (red, green, and blue) may be any value between 0 and 255 (inclusive).
     * Black is (0,0,0) and white is (255,255,255).
     * @param red
     * @param green
     * @param blue
     */
    void setPenColor(int red, int green, int blue) {
        mainTurtle.strokeColor.red = red;
        mainTurtle.strokeColor.green = green;
        mainTurtle.strokeColor.blue = blue;
    }


    /**
     * Set the current filling color.
     * Each component (red, green, and blue) may be any value between 0 and 255 (inclusive).
     * Black is (0,0,0) and white is (255,255,255).
     * @param red
     * @param green
     * @param blue
     */
    void setFillColor(int red, int green, int blue) {
        mainTurtle.fillColor.red = red;
        mainTurtle.fillColor.green = green;
        mainTurtle.fillColor.blue = blue;
    }


    /**
     * Draws a 1-pixel dot at the current location, regardless of pen status.
     */
    void dot() {
        // draw a pixel at the current location, regardless of pen status
        drawPixel((int) round(mainTurtle.xpos),
                  (int) round(mainTurtle.ypos));
    }


    /**
     * Draw a 1-pixel dot at the given location using the current draw color,
     * regardless of current turtle location or pen status.
     * @param x
     * @param y
     */
    void drawPixel(int x, int y) {
        if (x < (-mainFieldWidth / 2) || x > (mainFieldWidth / 2) ||
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
            mainTurtleImage[idx].red = mainTurtle.strokeColor.red;
            mainTurtleImage[idx].green = mainTurtle.strokeColor.green;
            mainTurtleImage[idx].blue = mainTurtle.strokeColor.blue;
        }

        // track total pixels drawn and emit video frame if a frame interval has
        // been crossed (and only if video saving is enabled, of course)
        if (mainFieldSaveFrames &&
            mainFieldPixelCount++ % mainFieldFrameInterval == 0) {
            saveFrame();
        }
    }


    /**
     * Draws a 1-pixel dot at the given location using the current fill color,
     * regardless of current turtle location or pen status.
     * @param x
     * @param y
     */
    void fillPixel(int x, int y) {
        // calculate pixel offset in image data array
        int idx = mainFieldWidth * (y + mainFieldHeight / 2)
                  + (x + mainFieldWidth / 2);

        // check to make sure it's not out of bounds
        if (idx >= 0 && idx < mainFieldWidth * mainFieldHeight) {
            mainTurtleImage[idx].red = mainTurtle.fillColor.red;
            mainTurtleImage[idx].green = mainTurtle.fillColor.green;
            mainTurtleImage[idx].blue = mainTurtle.fillColor.blue;
        }
    }


    /**
     * Draw a straight line between the given coordinates, regardless of current turtle location or pen status.
     * @param x0
     * @param y0
     * @param x1
     * @param y1
     */
    void drawLine(int x0, int y0, int x1, int y1) {
        // uses a variant of Bresenham's line algorithm:
        //   https://en.wikipedia.org/wiki/Talk:Bresenham%27s_line_algorithm

        int absX = abs(x1 - x0);          // absolute value of coordinate distances
        int absY = abs(y1 - y0);
        int offX = x0 < x1 ? 1 : -1;      // line-drawing direction offsets
        int offY = y0 < y1 ? 1 : -1;
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
                    y += offY;
                    err += absX;
                }
                x += offX;
                drawPixel(x, y);
            }
        } else {

            // line is more vertical; increment along y-axis
            err = absY / 2;
            while (y != y1) {
                err = err - absX;
                if (err < 0) {
                    x += offX;
                    err += absY;
                }
                y += offY;
                drawPixel(x, y);
            }
        }
    }


    /**
     * Draws a circle at the given coordinates with the given radius, regardless of current turtle location or pen status.
     * @param x0
     * @param y0
     * @param radius
     */
    void drawCircle(int x0, int y0, int radius) {
        // implementation based on midpoint circle algorithm:
        //   https://en.wikipedia.org/wiki/Midpoint_circle_algorithm

        int x = radius;
        int y = 0;
        int switch_criteria = 1 - x;

        if (mainTurtle.filled) {
            fillCircle(x0, y0, radius);
        }

        while (x >= y) {
            drawPixel(x + x0, y + y0);
            drawPixel(y + x0, x + y0);
            drawPixel(-x + x0, y + y0);
            drawPixel(-y + x0, x + y0);
            drawPixel(-x + x0, -y + y0);
            drawPixel(-y + x0, -x + y0);
            drawPixel(x + x0, -y + y0);
            drawPixel(y + x0, -x + y0);
            y++;
            if (switch_criteria <= 0) {
                switch_criteria += 2 * y + 1;       // no x-coordinate change
            } else {
                x--;
                switch_criteria += 2 * (y - x) + 1;
            }
        }
    }


    /**
     * Fill a circle at the given coordinates with the given radius, regardless of current turtle location or pen status.
     * @param x0
     * @param y0
     * @param radius
     */
    void fillCircle(int x0, int y0, int radius) {
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


    /**
     * Fills a circle at the current coordinates with the given radius.
     * @param radius
     */
    void fillCircle(int radius) {
        fillCircle(mainTurtle.xpos, mainTurtle.ypos, radius);
    }


    /**
     * Draws a turtle at the current location.
     */
    void drawTurtle() {
        // We are going to make our own backup of the turtle, since turtle_backup()
        // only gives us one level of undo.
        turtleState original_turtle = mainTurtle;

        penUp();

        // Draw the legs
        for (int i = -1; i < 2; i += 2) {
            for (int j = -1; j < 2; j += 2) {
                backup();
                forward(i * 7);
                strafeLeft(j * 7);

                setFillColor(
                        mainTurtle.strokeColor.red,
                        mainTurtle.strokeColor.green,
                        mainTurtle.strokeColor.blue
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
                mainTurtle.strokeColor.red,
                mainTurtle.strokeColor.green,
                mainTurtle.strokeColor.blue
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
        for (int i = 9; i >= 0; i -= 4) {
            backup();
            setFillColor(
                    mainTurtle.strokeColor.red,
                    mainTurtle.strokeColor.green,
                    mainTurtle.strokeColor.blue
            );
            fillCircle(i + 2);

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


    /**
     * Saves current field to a .bmp file.
     * @param filename
     */
    void saveBMP(const char *filename) {
        int i, j, ipos;
        unsigned int bytesPerLine;
        unsigned char *line;
        FILE *file;
        BMPHeader bmph{};
        auto width = mainFieldWidth;
        auto height = mainFieldHeight;
        char *rgb = (char *) mainTurtleImage;

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

        file = fopen(filename, "wb");
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

        line = (unsigned char *) malloc(bytesPerLine);
        memset(line, 0, bytesPerLine);
        if (line == nullptr) {
            fprintf(stderr, "Can't allocate memory for BMP file.\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                ipos = 3 * (width * i + j);
                line[3 * j] = rgb[ipos + 2];
                line[3 * j + 1] = rgb[ipos + 1];
                line[3 * j + 2] = rgb[ipos];
            }
            fwrite(line, bytesPerLine, 1, file);
        }

        free(line);
        fclose(file);
    }


    /**
     * Enables the video output.
     * When enabled, periodic frame bitmaps will be saved with sequentially-ordered filenames matching the following pattern:
     * "frameXXXX.bmp" (X is a digit).
     * Frames are emitted after a regular number of pixels have been drawn.
     * This number is set by the parameter to this function.
     * Some experimentation may be required to find a optimal values for different shapes.
     * @param pixelsPerFrame
     */
    void beginVideo(int pixelsPerFrame) {
        mainFieldSaveFrames = true;
        mainFieldFrameCount = 0;
        mainFieldFrameInterval = pixelsPerFrame;
        mainFieldPixelCount = 0;
    }


    /**
     * Emits a single video frame containing the current field image.
     */
    void saveFrame() {
        char filename[32];
        sprintf(filename, "frame%05d.bmp", ++mainFieldFrameCount);
        saveBMP(filename);
    }


    /**
     * Disables the video output.
     */
    void endVideo() {
        mainFieldSaveFrames = false;
    }


    /**
     * Returns the current x-coordinate.
     * @return current x-coordinate
     */
    double getX() {
        return mainTurtle.xpos;
    }


    /**
     * Returns the current y-coordinate.
     * @return current y-coordinate
     */
    double getY() {
        return mainTurtle.ypos;
    }

    /**
     * Draws an integer at the current location.
     * @param number number to draw
     */
    void drawInt(int number) {
        int digitsCount = countDigits(number);

        for (int i = digitsCount - 1; i >= 0; i--) {
            drawDigit(number % 10, i);
            number = number / 10;
        }
    }

private:
    /**
     * Cleans up any memory used by the turtle graphics system.
     */
    void cleanup() {
        if (mainTurtleImage != nullptr) {
            free(mainTurtleImage);
            mainTurtleImage = nullptr;
        }
    }

    /**
     * Draws single digit depending on it location in the number
     * @param digit digit to draw
     * @param digitIndex digit location in the number
     */
    void drawDigit(unsigned char digit, int digitIndex) {
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 4; x++) {
                if (TURTLE_DIGITS[digit][y * 4 + x] == 1) {
                    drawPixel(mainTurtle.xpos + digitIndex * 5 + x, mainTurtle.ypos - y);
                }
            }
        }
    }

    /**
     * Counts the number of digits in the given integer number.
     * @param number
     * @return Number of digits in number
     */
    static unsigned short countDigits(int number) {
        if (number > 9) {
            (int) (ceil(log10(number)));
        } else {
            return 1;
        }
    }
};


#endif //TURTLEGRAPHICS_YATG_HPP
