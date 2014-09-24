/*
 * Program description: Displays an image in opengl window, writes what is displayed
 * to specified output image when 'w' key is pressed, and gives pixel information when
 * the opengl window display is clicked with left or right mouse click.
 *
 * Author: Austin Brennan - awbrenn@g.clemson.edu
 *
 * Date: October 2, 2014
 *
 * Other information: Check README for more information
 *
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include <OpenImageIO/imageio.h>

using namespace std;
OIIO_NAMESPACE_USING

// Struct Declaration
struct pixel {
    unsigned char r, g, b, a;
};

// Global Variables
int IMAGE_HEIGHT;
int IMAGE_WIDTH;
pixel **PIXMAP;
char *OUTPUT_FILE = NULL;

/* Handles errors
 * input	- message is printed to stdout, if kill is true end program
 * output	- None
 */
void errorHandler (string message, bool kill) {
    cout << "ERROR: " << message << "\n";
    if (kill)
        exit(0);
}

int main(int argc, char *argv[]) {
    cout << "Hello alphamask World" << endl;
}