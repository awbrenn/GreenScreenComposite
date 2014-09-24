/*
 * Program description: Creates an alphamask of an image
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

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

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
 * side effect - prints error message and kills program if kill flag is set
 */
void handleError (string message, bool kill) {
    cout << "ERROR: " << message << "\n";
    if (kill)
        exit(0);
}

/* Converts pixels from vector to pixel pointers
 * input		- vector of pixels, number of channels
 * output		- None
 * side effect	- saves pixel data from vector to PIXMAP
 */
void convertVectorToPixelPointers (vector<unsigned char> vector_pixels, int channels) {
    PIXMAP = new pixel*[IMAGE_HEIGHT];
    PIXMAP[0] = new pixel[IMAGE_WIDTH * IMAGE_HEIGHT];

    for (int i = 1; i < IMAGE_HEIGHT; i++)
        PIXMAP[i] = PIXMAP[i - 1] + IMAGE_WIDTH;

    int i = 0;
    if (channels == 3) {
        for (int row = IMAGE_HEIGHT-1; row >= 0; row--)
            for (int col = 0; col < IMAGE_WIDTH; col++) {
                PIXMAP[row][col].r = vector_pixels[i++];
                PIXMAP[row][col].g = vector_pixels[i++];
                PIXMAP[row][col].b = vector_pixels[i++];
                PIXMAP[row][col].a = 255;
            }
    }
    else if (channels == 4) {
        for (int row = IMAGE_HEIGHT-1; row >= 0; row--)
            for (int col = 0; col < IMAGE_WIDTH; col++) {
                PIXMAP[row][col].r = vector_pixels[i++];
                PIXMAP[row][col].g = vector_pixels[i++];
                PIXMAP[row][col].b = vector_pixels[i++];
                PIXMAP[row][col].a = vector_pixels[i++];
            }
    }

    else
        handleError ("Could not convert image.. sorry", 1);
}

/* Reads image specified in argv[1]
 * input		- the input file name
 * output		- None
 */
void readImage (string filename) {
    ImageInput *in = ImageInput::open(filename);
    if (!in)
        handleError("Could not open input file", true);
    const ImageSpec &spec = in->spec();
    IMAGE_WIDTH = spec.width;
    IMAGE_HEIGHT = spec.height;
    int channels = spec.nchannels;
    if(channels < 3 || channels > 4)
        handleError("Application supports 3 or 4 channel images only", 1);
    vector<unsigned char> pixels (IMAGE_WIDTH*IMAGE_HEIGHT*channels);
    in->read_image (TypeDesc::UINT8, &pixels[0]);
    in->close ();
    delete in;
    convertVectorToPixelPointers(pixels, channels);
}

/* Write image to specified file
 * input		- pixel array, width of display window, height of display window
 * output		- None
 * side effect	- writes image to a file
 */
void writeImage(unsigned char *glut_display_map, int window_width, int window_height) {
    const char *filename = OUTPUT_FILE;
    const int xres = window_width, yres = window_height;
    const int channels = 4; // RGBA
    int scanlinesize = xres * channels;
    ImageOutput *out = ImageOutput::create (filename);
    if (! out) {
        handleError("Could not create output file", false);
        return;
    }
    ImageSpec spec (xres, yres, channels, TypeDesc::UINT8);
    out->open (filename, spec);
    out->write_image (TypeDesc::UINT8, glut_display_map+(window_height-1)*scanlinesize, AutoStride, -scanlinesize, AutoStride);
    out->close ();
    delete out;
    cout << "SUCCESS: Image successfully written to " << OUTPUT_FILE << "\n";
}

/* Draw Image to opengl display
 * input		- None
 * output		- None
 * side effect	- draws image to opengl display window
 */
void drawImage() {
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(0,0);
    glDrawPixels(IMAGE_WIDTH, IMAGE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, PIXMAP[0]);
    glutSwapBuffers();
}

/* Key press handler
 * input	- Handled by opengl, because this is a callback function.
 * output	- None
 */
void handleKey(unsigned char key, int x, int y) {

    if (key == 'w') {
        int window_width = glutGet(GLUT_WINDOW_WIDTH), window_height = glutGet(GLUT_WINDOW_HEIGHT);
        unsigned char glut_display_map[window_width*window_height*4];
        glReadPixels(0,0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE, glut_display_map);
        writeImage(glut_display_map, window_width, window_height);
    }
    else if (key == 'q' || key == 'Q') {
        cerr << "\nProgram Terminated." << endl;
        exit(0);
    }
}

/* Initialize opengl
 * input	- command line arguments
 * output	- none
 */
void openGlInit(int argc, char* argv[]) {
    // start up the glut utilities
    glutInit(&argc, argv);

    // create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(IMAGE_WIDTH, IMAGE_HEIGHT);
    glutCreateWindow("A Display for an Image");

    // set up the callback routines to be called when glutMainLoop() detects
    // an event
    glutDisplayFunc(drawImage);		  		// display callback
    glutKeyboardFunc(handleKey);	  		// keyboard callback
    // glutMouseFunc(handleClick);		// click callback

    // define the drawing coordinate system on the viewport
    // lower left is (0, 0), upper right is (WIDTH, HEIGHT)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, IMAGE_WIDTH, 0, IMAGE_HEIGHT);

    // specify window clear (background) color to be opaque white
    glClearColor(0, 0, 0, 0);

    // Routine that loops forever looking for events. It calls the registered
    // callback routine to handle each event that is detected
    glutMainLoop();
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        handleError("Proper use: $> alphamask input.img output.img", 1);
    readImage(argv[1]);
    OUTPUT_FILE = argv[2];
    openGlInit(argc, argv);
}