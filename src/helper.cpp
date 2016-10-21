#include "helper.h"
#include "intersectionData.h"

#include <math.h>
#include <stdlib.h>

#include "rt.h"

//
//  float max()
//
//  Returns the maximum of two numbers.
//
float max(float a, float b)
{
    return (a > b ? a : b);
}

//
//  float min()
//
//  Returns the minimum of two numbers.
//
float min(float a, float b)
{
    return (a < b ? a : b);
}

//
//  float getRand()
//
//  Simple helper function to return a random number between 0.0f and 1.0f.
//
float getRand()
{
    return rand() / (float)RAND_MAX;
}

//
//  float getRand()
//
//  Simple helper function to return a random number between the requested ranges.
//
float getRand(float min, float max)
{
    return (getRand() * (max - min)) + min;
}


//
//  void drawGrid();
//
//  Draws a grid at the origin, ranging from -10 to 10 in the X and Z axes.
//  This size can be controlled with the appropriate calls to glScale() beforehand.
//
void drawGrid()
{
    //turn off lighting for the grid so that it is just solid white lines
    glDisable(GL_LIGHTING);

    //draw a simple grid under the teapot
    glColor3f(1,1,1);
    for(int dir = 0; dir < 2; dir++)
    {
        for(int i = -5; i < 6; i++)
        {
            glBegin(GL_LINE_STRIP);
            for(int j = -5; j < 6; j++)
                glVertex3f(dir<1?i:j, 0, dir<1?j:i);
            glEnd();
        }
    }

    //and then draw the teapot itself!
    glEnable(GL_LIGHTING);
}

//
//  vector<string> tokenizeString(string input, string delimiters)
//
//      This is a helper function to break a single string into std::vector
//  of strings, based on a given set of delimiter characters.
//
vector<string> tokenizeString(string input, string delimiters)
{
    if(input.size() == 0)
        return vector<string>();

    vector<string> retVec = vector<string>();
    size_t oldR = 0, r = 0; 

    //strip all delimiter characters from the front and end of the input string.
    string strippedInput;
    unsigned int lowerValidIndex = 0, upperValidIndex = input.size() - 1; 
    while(lowerValidIndex < input.size() && delimiters.find_first_of(input.at(lowerValidIndex), 0) != string::npos)
        lowerValidIndex++;

    while(upperValidIndex >= 0 && delimiters.find_first_of(input.at(upperValidIndex), 0) != string::npos)
        upperValidIndex--;

    //if the lowest valid index is higher than the highest valid index, they're all delimiters! return nothing.
    if(lowerValidIndex >= input.size() || upperValidIndex < 0 || lowerValidIndex >= upperValidIndex)
        return vector<string>();
    
    //remove the delimiters from the beginning and end of the string, if any.
    strippedInput = input.substr(lowerValidIndex, upperValidIndex-lowerValidIndex+1);

    //search for each instance of a delimiter character, and create a new token spanning
    //from the last valid character up to the delimiter character.
    while((r = strippedInput.find_first_of(delimiters, oldR)) != string::npos)
    {    
        if(oldR != r)           //but watch out for multiple consecutive delimiters!
            retVec.push_back(strippedInput.substr(oldR, r-oldR));
        oldR = r+1; 
    }    
    if(r != 0)
        retVec.push_back(strippedInput.substr(oldR, r-oldR));

    return retVec;
}


//
//  bool registerOpenGLTexture(unsigned char *imageData, int texWidth, int texHeight, GLuint &texHandle)
//
//  Attempts to register an image buffer with OpenGL. Sets the texture handle
//      appropriately upon success, and uses a number of default texturing
//      values. This function only provides the basics: just sets up this image
//      as an unrepeated 2D texture.
//
bool registerOpenGLTexture(unsigned char *imageData, int texWidth, int texHeight, GLuint &texHandle)
{
    //first off, get a real texture handle.
    glGenTextures(1, &texHandle);

    //make sure that future texture functions affect this handle
    glBindTexture(GL_TEXTURE_2D, texHandle);

    //set this texture's color to be multiplied by the surface colors -- 
    //  GL_MODULATE allows us to take advantage of OpenGL lighting
    //  GL_REPLACE uses only the color from the texture.
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    //set the minification ang magnification functions to be linear; not perfect
    //  but much better than nearest-texel (GL_NEAREST).
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Set the texture to repeat in S and T -- though it doesn't matter here
    //  because our texture coordinates are always in [0,0] to [1,1].
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //actually transfer the texture to the GPU!
    glTexImage2D(GL_TEXTURE_2D,                 //still working with 2D textures, obv.
                    0,                          //not using mipmapping, so this is the highest level.
                    GL_RGBA,                     //we're going to provide OpenGL with R, G, and B components...
                    texWidth,                   //...of this width...
                    texHeight,                  //...and this height.
                    0,                          //give it a border of 0 pixels (none)
                    GL_RGBA,                     //and store it, internally, as RGB (OpenGL will convert to floats between 0.0 and 1.0f)
                    GL_UNSIGNED_BYTE,           //this is the format our data is in, and finally,
                    imageData);                 //there's the data!

    //whoops! i guess this function can't fail. in an ideal world, there would
    //be checks with glGetError() that we could use to determine if this failed.
    return true;
}



//
//  void writeToPPM()
//
//  Saves the (width*height*3) unsigned char buffer specified in img to the file
//      name given in filename, in PPM P3 format. 
//
void writeToPPM(const char *filename, int width, int height, unsigned char *img)
{
    FILE *fp = fopen(filename, "w");
    if(!fp)
    {
        fprintf(stderr, "Could not open output file %s for PPM writing.\n", filename);
        return;
    }
    fprintf(fp, "P3\n");
    fprintf(fp, "%d %d\n255\n", width, height);

    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            int idx = (i*width+j)*4;
            fprintf(fp, "%d %d %d%c", img[idx+0], img[idx+1], img[idx+2], (j+1)%5==0?'\n':'\t');
        }
        fprintf(fp, "\n");
    }

    printf("PPM image successfully written to %s.\n", filename);
    fclose(fp);
} 











//
//  void raytraceImage()
//
//  The raytrace callback - gets invoked when the user presses 'r.' Needs to 
//      allocate and fill the raytracedImage buffer based on the camera, shapes, and lights.
//      You will need to compute camera rays for each pixel in the image, test them
//      against all objects in the scene for intersection, and use the lights and materials
//      to determine the final shaded color of a given pixel.
//
void raytraceImage(FreeCamera camera, 
                    vector<Shape*> shapes, 
                    vector<PointLight*> lights, 
                    unsigned int imageWidth, 
                    unsigned int imageHeight, 
                    unsigned char* &raytracedImage)
{
    raytracedImage = new unsigned char[imageWidth*imageHeight*4];

    IntersectionData hit;

    int shape_count = shapes.size();
    Shape** shape_array = new Shape*[shape_count];
    for (int i = 0; i < shape_count; i++) {
        shape_array[i] = shapes[i];
    }

    for(unsigned int i = 0; i < imageHeight; i++) {
        for(unsigned int j = 0; j < imageWidth; j++) {
            int idx = (i*imageWidth+j)*4;

            if(idx % 1000 == 0) {
                printf("\33[2K\r");
                printf("%.2f%% of the way done...", (i*imageWidth+j) / (float)(imageWidth*imageHeight) * 100.0f);
                fflush(stdout);
            }

            float depth;
            Point color = Point(0, 0, 0);
            Ray prime = camera.getPrimaryRayThroughPixel(j, i, imageWidth, imageHeight);
            traceRay(prime, shape_array, shape_array + shape_count, lights, camera.nearClipPlane, camera.farClipPlane, 1, color, depth, 0);

            color.clamp(0, 1);

            raytracedImage[idx+0] = (unsigned char)(color.x*255);
            raytracedImage[idx+1] = (unsigned char)(color.y*255);
            raytracedImage[idx+2] = (unsigned char)(color.z*255);
            raytracedImage[idx+3] = (unsigned char) depth >= camera.farClipPlane ? 0 : 255;
        }
    } 
    
    printf("\33[2K\r");
    printf("100.00%% of the way done...");
    fflush(stdout);
}



