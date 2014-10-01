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

// Default settings
int HUE_RANGE[2] = {131,145};
int HUE_BOUNDS_BUFFER[2] = {15, 5}; // partially transparent pixels that are just outside of HUE_RANGE
int HUE_BOUNDS_BUFFER_ALPHA_VALUE = 50; // value of alpha for pixels in the bounds buffer
int BRIGHTNESS_RANGE[2] = {20, 90};

// float VALUE_RANGE[2] = {}

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

void convertRGBtoHSV (unsigned char r, unsigned char g, unsigned char b, float &hue, float &saturation, float &value) {

    float red, green, blue;
    float maxc, minc, delta;

    // r, g, b to 0 - 1 scale
    red = r / 255.0; green = g / 255.0; blue = b / 255.0;

    maxc = max(max(red, green), blue);
    minc = min(min(red, green), blue);

    value = maxc;        // value is maximum of r, g, b

    if(maxc == 0){    // saturation and hue 0 if value is 0
        saturation = 0;
        hue = 0;
    } else {
        saturation = (maxc - minc) / maxc; 	// saturation is color purity on scale 0 - 1

        delta = maxc - minc;
        if(delta == 0)           // hue doesn't matter if saturation is 0
            hue = 0;
        else{
            if(red == maxc)       // otherwise, determine hue on scale 0 - 360
                hue = (green - blue) / delta;
            else if(green == maxc)
                hue = 2.0 + (blue - red) / delta;
            else // (blue == maxc)
                hue = 4.0 + (red - green) / delta;
            hue = hue * 60.0;
            if(hue < 0)
                hue = hue + 360.0;
        }
    }
}

unsigned char calculateAlpha(unsigned char r, unsigned char g, unsigned char b) {
    float hue , saturation, value;
    unsigned char alpha = 255;

    convertRGBtoHSV(r, g, b, hue, saturation, value);
    if (hue > HUE_RANGE[0] and hue < HUE_RANGE[1]) alpha = 0;
//    else if ((hue >= HUE_RANGE[0] - HUE_RANGE_BUFFER and hue < HUE_RANGE[0]) or
//             (hue <= HUE_RANGE[1] + HUE_RANGE_BUFFER and hue > HUE_RANGE[1]) and
//             (value > 10 and value < 80)) {
//        alpha = 20;
//    }

    return alpha;
}

void createAlphamask() {
    for (int row = IMAGE_HEIGHT-1; row >= 0; row--)
        for (int col = 0; col < IMAGE_WIDTH; col++) {
            PIXMAP[row][col].a = calculateAlpha(PIXMAP[row][col].r, PIXMAP[row][col].g, PIXMAP[row][col].b);
        }
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

pixel ** flipImageVertical(pixel **pixmap_vertical_flip) {
    for (int row = IMAGE_HEIGHT-1; row >= 0; row--)
        for (int col = 0; col < IMAGE_WIDTH; col++) {
            pixmap_vertical_flip[(IMAGE_HEIGHT-1)-row][col] = PIXMAP[row][col];
        }

    return pixmap_vertical_flip;
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
void writeImage() {
    const char *filename = OUTPUT_FILE;
    const int channels = 4; // RGBA
    // initialize pixmap_vertical_flip
    pixel **pixmap_vertical_flip;
    pixmap_vertical_flip = new pixel *[IMAGE_HEIGHT];
    pixmap_vertical_flip[0] = new pixel[IMAGE_WIDTH * IMAGE_HEIGHT];
    for (int i = 1; i < IMAGE_HEIGHT; i++)
        pixmap_vertical_flip[i] = pixmap_vertical_flip[i - 1] + IMAGE_WIDTH;

    pixmap_vertical_flip = flipImageVertical(pixmap_vertical_flip);
    ImageOutput *out = ImageOutput::create (filename);
    if (! out) {
        handleError("Could not create output file", false);
        return;
    }
    ImageSpec spec (IMAGE_WIDTH, IMAGE_HEIGHT, channels, TypeDesc::UINT8);
    out->open (filename, spec);

    out->write_image(TypeDesc::UINT8, pixmap_vertical_flip[0]);
    out->close ();
    delete out;
    delete pixmap_vertical_flip;
    cout << "SUCCESS: Image successfully written to " << OUTPUT_FILE << "\n";
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
    glDrawPixels(IMAGE_WIDTH, IMAGE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, PIXMAP[0]);
    glFlush();
}

/* Key press handler
 * input	- Handled by opengl, because this is a callback function.
 * output	- None
 */
void handleKey(unsigned char key, int x, int y) {

    if (key == 'w') {
        writeImage();
    }
    else if (key == 'q' || key == 'Q') {
        cout << "\nProgram Terminated." << endl;
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
    glClearColor(1, 1, 1, 0);

    // Routine that loops forever looking for events. It calls the registered
    // callback routine to handle each event that is detected
    glutMainLoop();
}

void readSettings(char *filename) {
    FILE * pFile;
    pFile = fopen (filename, "r");
    int hue_lower_bound, hue_upper_bound, hue_lower_bound_buffer, hue_upper_bound_buffer;

    fscanf(pFile, "%d %d", &hue_lower_bound, &hue_upper_bound);
    HUE_RANGE[0] = hue_lower_bound;
    HUE_RANGE[1] = hue_upper_bound;
    fscanf(pFile, "%d %d", &hue_lower_bound_buffer, &hue_upper_bound_buffer);


    fclose(pFile);

}

int main(int argc, char *argv[]) {
    if (argc != 3 and argc != 4)
        handleError("Proper use:\n$> alphamask input.img output.img\n"
                    "$> alphamask input.img output.img settings.txt", 1);

    readSettings(argv[3]);
    readImage(argv[1]);
    createAlphamask();
    OUTPUT_FILE = argv[2];
    openGlInit(argc, argv);
}