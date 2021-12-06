/*
    turtle.hpp

    Simple array-based turtle graphics engine in CPP.
    Exports to BMP files.

    (header info only; see turtle.c for implementation)

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

// pixel data (red, green, blue triplet)
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} rgb_t;


/**  TURTLE STATE  **/

typedef struct {
    double xpos;       // current position and heading
    double ypos;       // (uses floating-point numbers for
    double heading;    //  increased accuracy)

    rgb_t penColor;   // current pen color
    rgb_t fillColor;  // current fill color
    bool pendown;     // currently drawing?
    bool filled;      // currently filling?
} turtle_t;

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
    turtle_t mainTurtle{};
    turtle_t backupTurtle{};

    rgb_t *mainTurtleImage = nullptr;        // 2d pixel data field

    int mainFieldWidth = 0;           // size in pixels
    int mainFieldHeight = 0;

    bool mainFieldSaveFrames = false;  // currently saving video frames?
    int mainFieldFrameCount = 0;   // current video frame counter
    int mainFieldFrameInterval = 10;  // pixels per frame
    int mainFieldPixelCount = 0;   // total pixels drawn by turtle since

    // beginning of video
    int mainTurtlePolyVertexCount = 0;       // polygon vertex count
    double mainTurtlePolyX[MAX_POLYGON_VERTICES]{}; // polygon vertex x-coords
    double mainTurtlePolyY[MAX_POLYGON_VERTICES]{}; // polygon vertex y-coords

    size_t numPixelsOutOfBounds;

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
    /*
    Initialize the 2d field that the turtle moves on.
    */
    Turtle(int width, int height);

    ~Turtle();

    /*
        Reset the turtle's location, orientation, color, and pen status to the
        default values: center of the field (0,0), facing right (0 degrees), black,
        and down, respectively).
    */
    void reset();


    /*
        Creates a backup of the current turtle. Once you have a backup you can
        restore from a backup using turtle_restore(); This is useful in complex
        drawing situations.
    */
    void backup();


    /*
        Restores the turtle from the backup. Note that the behavior is undefined
        if you have not first called turtle_backup().
    */
    void restore();


    /*
        Move the turtle forward, drawing a straight line if the pen is down.
    */
    void forward(int pixels);


    /*
        Move the turtle backward, drawing a straight line if the pen is down.
    */
    void backward(int pixels);

    void strafeLeft(int pixels);

    void strafeRight(int pixels);

    /*
        Turn the turtle to the left by the specified number of degrees.
    */
    void turnLeft(double angle);


    /*
        Turn the turtle to the right by the specified number of degrees.
    */
    void turnRight(double angle);


    /*
        Set the pen status to "up" (do not draw).
    */
    void penUp();


    /*
        Set the pen status to "down" (draw).
    */
    void penDown();


    /*
        Start filling. Call this before drawing a polygon to activate the
        bookkeeping required to run the filling algorithm later.
    */
    void beginFill();


    /*
        End filling. Call this after drawing a polygon to trigger the fill
        algorithm. The filled polygon may have up to 128 sides.
    */
    void endFill();


    /*
        Move the turtle to the specified location, drawing a straight line if the
        pen is down. Takes integer coordinate parameters.
    */
    void goTo(int x, int y);


    /*
        Move the turtle to the specified location, drawing a straight line if the
        pen is down. Takes real-numbered coordinate parameters, and is also used
        internally to implement forward and backward motion.
    */
    void goTo(double x, double y);


    /*
        Rotate the turtle to the given heading (in degrees). 0 degrees means
        facing to the right; 90 degrees means facing straight up.
    */
    void setHeading(double angle);


    /*
        Set the current drawing color. Each component (red, green, and blue) may
        be any value between 0 and 255 (inclusive). Black is (0,0,0) and white is
        (255,255,255).
    */
    void setPenColor(int red, int green, int blue);


    /*
        Set the current filling color. Each component (red, green, and blue) may
        be any value between 0 and 255 (inclusive). Black is (0,0,0) and white is
        (255,255,255).
    */
    void setFillColor(int red, int green, int blue);


    /*
        Draw a 1-pixel dot at the current location, regardless of pen status.
    */
    void dot();


    /*
        Draw a 1-pixel dot at the given location using the current draw color,
        regardless of current turtle location or pen status.
    */
    void drawPixel(int x, int y);


    /*
        Draw a 1-pixel dot at the given location using the current fill color,
        regardless of current turtle location or pen status.
    */
    void fillPixel(int x, int y);


    /*
        Draw a straight line between the given coordinates, regardless of current
        turtle location or pen status.
    */
    void drawLine(int x0, int y0, int x1, int y1);


    /*
        Draw a circle at the given coordinates with the given radius, regardless of
        current turtle location or pen status.
    */
    void drawCircle(int x, int y, int radius);


    /*
        Fill a circle at the given coordinates with the given radius, regardless of
        current turtle location or pen status.
    */
    void fillCircle(int x0, int y0, int radius);

    /*
        Fill a circle at the current coordinates with the given radius.
    */
    void fillCircle(int radius);


    /*
        Draw a turtle at the current pen location.
    */
    void drawTurtle();


    /*
        Save current field to a .bmp file.
    */
    void saveBMP(const char *filename);


    /*
        Enable video output. When enabled, periodic frame bitmaps will be saved
        with sequentially-ordered filenames matching the following pattern:
        "frameXXXX.bmp" (X is a digit). Frames are emitted after a regular number of
        pixels have been drawn; this number is set by the parameter to this
        function. Some experimentation may be required to find a optimal values for
        different shapes.
    */
    void beginVideo(int pixels_per_frame);


    /*
        Emit a single video frame containing the current field image.
    */
    void saveFrame();


    /*
        Disable video output.
    */
    void endVideo();


    /*
        Returns the current x-coordinate.
    */
    double getX();


    /*
        Returns the current x-coordinate.
    */
    double getY();


    /*
        Draw an integer at the current location.
    */
    void drawInt(int value);


    /*
        Clean up any memory used by the turtle graphics system. Call this at the
        end of the program to ensure there are no memory leaks.
    */
    void cleanup();
};


#endif //TURTLEGRAPHICS_YATG_HPP
