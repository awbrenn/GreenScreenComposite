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
int GLUT_IMAGE_HEIGHT;
int GLUT_IMAGE_WIDTH;
pixel **GLUT_PIXMAP;
char *OUTPUT_FILE = NULL;

// Image class declaration
class Image {
public:
    int height;
    int width;
    pixel ** pixmap;

    Image(int, int);
};

Image::Image(int x, int y) {
    width = x;
    height = y;
    pixmap = new pixel*[height];
    pixmap[0] = new pixel[width * height];

    for (int i = 1; i < height; i++)
        pixmap[i] = pixmap[i - 1] + width;
}

/* Handles errors
 * input	- message is printed to stdout, if kill is true end program
 * output	- None
 */
void errorHandler (string message, bool kill) {
    cout << "ERROR: " << message << "\n";
    if (kill)
        exit(0);
}

/* Converts pixels from vector to pixel pointers
 * input		- vector of pixels, number of channels
 * output		- None
 * side effect	- saves pixel data from vector to PIXMAP
 */
Image convertVectorToPixelPointers (vector<unsigned char> vector_pixels, int channels, Image image) {

    int i = 0;
    if (channels == 3) {
        for (int row = image.height-1; row >= 0; row--)
            for (int col = 0; col < image.width; col++) {
                image.pixmap[row][col].r = vector_pixels[i++];
                image.pixmap[row][col].g = vector_pixels[i++];
                image.pixmap[row][col].b = vector_pixels[i++];
                image.pixmap[row][col].a = 255;
            }
    }
    else if (channels == 4) {
        for (int row = image.height-1; row >= 0; row--)
            for (int col = 0; col < image.width; col++) {
                image.pixmap[row][col].r = vector_pixels[i++];
                image.pixmap[row][col].g = vector_pixels[i++];
                image.pixmap[row][col].b = vector_pixels[i++];
                image.pixmap[row][col].a = vector_pixels[i++];
            }
    }

    else
        errorHandler ("Could not convert image.. sorry", 1);

    return image;
}

/* Reads image specified in argv[1]
 * input		- the input file name
 * output		- None
 */
Image imageReader (string filename) {
    int xres, yres;
    ImageInput *in = ImageInput::open(filename);
    if (!in)
        errorHandler("Could not open input file", true);
    const ImageSpec &spec = in->spec();
    xres = spec.width;
    yres = spec.height;
    int channels = spec.nchannels;
    if(channels < 3 || channels > 4)
        errorHandler("Application supports 3 or 4 channel images only", 1);
    vector<unsigned char> oiio_pixels (xres*yres*channels);
    in->read_image (TypeDesc::UINT8, &oiio_pixels[0]);
    in->close ();
    delete in;

    Image image(xres, yres);
    return convertVectorToPixelPointers(oiio_pixels, channels, image);
}

unsigned char calculateColor(unsigned char color1, unsigned char color2, unsigned char alpha1, unsigned char alpha2) {
    return (unsigned char) (((alpha1 / 255.0) * (color1 / 255.0) + (1 - (alpha1 / 255.0)) * (alpha2 / 255.0) * (color2 /255.0)) * 255);
}

void overOperation(Image imageA, Image imageB) {
    for (int row = 0; row < GLUT_IMAGE_HEIGHT; row++)
        for (int col = 0; col < GLUT_IMAGE_WIDTH; col++) {
            GLUT_PIXMAP[row][col].r = calculateColor(imageA.pixmap[row][col].r, imageB.pixmap[row][col].r, imageA.pixmap[row][col].a, imageB.pixmap[row][col].a);
            GLUT_PIXMAP[row][col].g = calculateColor(imageA.pixmap[row][col].g, imageB.pixmap[row][col].g, imageA.pixmap[row][col].a, imageB.pixmap[row][col].a);
            GLUT_PIXMAP[row][col].b = calculateColor(imageA.pixmap[row][col].b, imageB.pixmap[row][col].b, imageA.pixmap[row][col].a, imageB.pixmap[row][col].a);
            GLUT_PIXMAP[row][col].a = calculateColor(imageA.pixmap[row][col].a, imageB.pixmap[row][col].a, imageA.pixmap[row][col].a, imageB.pixmap[row][col].a);
        }
}

/* Draw Image to opengl display
 * input		- None
 * output		- None
 * side effect	- draws image to opengl display window
 */
void drawImage() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glRasterPos2i(0,0);
    glDrawPixels(GLUT_IMAGE_WIDTH, GLUT_IMAGE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, GLUT_PIXMAP[0]);
    glFlush();
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
        errorHandler("Could not create output file", false);
        return;
    }
    ImageSpec spec (xres, yres, channels, TypeDesc::UINT8);
    out->open (filename, spec);
    out->write_image (TypeDesc::UINT8, glut_display_map+(window_height-1)*scanlinesize, AutoStride, -scanlinesize, AutoStride);
    out->close ();
    delete out;
    cout << "SUCCESS: Image successfully written to " << OUTPUT_FILE << "\n";
}

/* Key press handler
 * input	- Handled by opengl, because this is a callback function.
 * output	- None
 */
void handleKey(unsigned char key, int x, int y) {

    if (key == 'w') {
        if(OUTPUT_FILE != NULL) {
            int window_width = glutGet(GLUT_WINDOW_WIDTH), window_height = glutGet(GLUT_WINDOW_HEIGHT);
            unsigned char glut_display_map[window_width*window_height*4];
            glReadPixels(0,0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE, glut_display_map);
            writeImage(glut_display_map, window_width, window_height);
        }
        else
            errorHandler("Cannot write to file. Specify filename in third argument to write to file.", false);
    }
    else if (key == 'q' || key == 'Q') {
        printf("\nProgram Terminated.\n");
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
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(GLUT_IMAGE_WIDTH, GLUT_IMAGE_HEIGHT);
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
    gluOrtho2D(0, GLUT_IMAGE_WIDTH, 0, GLUT_IMAGE_HEIGHT);

    // specify window clear (background) color to be opaque white
    glClearColor(1, 1, 1, 0);

    // Routine that loops forever looking for events. It calls the registered
    // callback routine to handle each event that is detected
    glutMainLoop();
}

int main (int argc, char* argv[]) {
    if (argc < 3 || argc > 4)
        errorHandler("Proper use: $> composite A.img B.img"
                " OR $> composite A.img B.img ouput.img", 1);

    Image imageA = imageReader(argv[1]);
    Image imageB = imageReader(argv[2]);
    OUTPUT_FILE = argv[3];

    // set initial opengl window dimensions to the image A's dimensions
    GLUT_IMAGE_HEIGHT = imageA.height;
    GLUT_IMAGE_WIDTH = imageA.width;
    GLUT_PIXMAP = new pixel*[GLUT_IMAGE_HEIGHT];
    GLUT_PIXMAP[0] = new pixel[GLUT_IMAGE_WIDTH * GLUT_IMAGE_HEIGHT];

    for (int i = 1; i < GLUT_IMAGE_HEIGHT; i++)
        GLUT_PIXMAP[i] = GLUT_PIXMAP[i - 1] + GLUT_IMAGE_WIDTH;

    overOperation(imageA, imageB);
    openGlInit(argc, argv);

    return 0;
}