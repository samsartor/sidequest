/*
 *  CSCI 441, Computer Graphics, Fall 2016
 *
 *  Project: SQ2
 *  File: main.cpp
 *
 *  Description:
 *      Basecode for the third Side Quest assignment (raytracer).
 *
 *  Author:
 *      Jeffrey Paone, Colorado School of Mines
 *      Based on code originally written by
 *          Alexandri Zavodny, University of Notre Dame.
 */


#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>

#include "point.h"
#include "helper.h"
#include "freeCamera.h"
#include "shape.h"
#include "sphere.h"
#include "plane.h"
#include "rt.h"

using namespace std;


// GLOBAL VARIABLES ////////////////////////////////////////////////////////////


GLint leftMouseButton, rightMouseButton;    //status of the mouse buttons
int mouseX = 0, mouseY = 0;                 //last known X and Y of the mouse
bool forwardKeyHeldDown = false;
bool backwardKeyHeldDown = false;


GLuint raytracedTexHandle;
bool displayingTracedImage = false;
bool tracing = false;

FreeCamera rt_camera;
RTData rt_data;
float *rt_dest;
int rt_samples;
int rt_width;
int rt_height;
int rt_start;
Point rt_sky;


FreeCamera mainCamera;
vector<Shape*> shapes;
Point skyColor;


// END GLOBAL VARIABLES ///////////////////////////////////////////////////////


//
//  void resizeWindow(int w, int h)
//
//      We will register this function as GLUT's reshape callback.
//   When the window is resized, the OS will tell GLUT and GLUT will tell
//   us by calling this function - GLUT will give us the new window width
//   and height as 'w' and 'h,' respectively - so save those to our global vars.
//
void resizeWindow(int newWidth, int newHeight)
{
    mainCamera.imageWidth = newWidth;
    mainCamera.imageHeight = newHeight;

    //update the viewport to fill the window
    glViewport(0, 0, mainCamera.imageWidth, mainCamera.imageHeight);

    //update the projection matrix with the new window properties
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    mainCamera.sendPerspectiveProjection();
}



//
//  void mouseCallback(int button, int state, int thisX, int thisY)
//
//  GLUT callback for mouse clicks. We save the state of the mouse button
//      when this is called so that we can check the status of the mouse
//      buttons inside the motion callback (whether they are up or down).
//
void mouseCallback(int button, int state, int thisX, int thisY)
{
    //update the left and right mouse button states, if applicable
    if(button == GLUT_LEFT_BUTTON)
        leftMouseButton = state;
    else if(button == GLUT_RIGHT_BUTTON)
        rightMouseButton = state;
    
    //and update the last seen X and Y coordinates of the mouse
    mouseX = thisX;
    mouseY = thisY;
}

//
//  void mouseMotion(int x, int y)
//
//  GLUT callback for mouse movement. We update cameraPhi, cameraTheta, and/or
//      cameraRadius based on how much the user has moved the mouse in the
//      X or Y directions (in screen space) and whether they have held down
//      the left or right mouse buttons. If the user hasn't held down any
//      buttons, the function just updates the last seen mouse X and Y coords.
//
void mouseMotion(int x, int y)
{
    if(leftMouseButton == GLUT_DOWN)
        mainCamera.updateDirection( (x - mouseX)*0.005, (mouseY - y)*0.005 );

    mouseX = x;
    mouseY = y;
}



//
//  void updateScene()
//
//  Perform any updating on objects in the scene. This could range from animating
//      their positions or doing physics or intersection testing, etc... it's useful
//      to have this separated from the draw() function to help keep code organized.
//
void updateScene()
{
    //update the position of the camera if the user's been holding down 
    //the movement keys; scale the movement based on this constant.
    float movementConstant = 0.3f;

    //move forward!
    if(forwardKeyHeldDown)
        mainCamera.moveAlongDirection(movementConstant);

    //move backwards!
    if(backwardKeyHeldDown)
        mainCamera.moveAlongDirection(-movementConstant);
}




//
//  void renderScene()
//
//  GLUT callback for scene rendering. Sets up the modelview matrix, renders
//      a teapot to the back buffer, and switches the back buffer with the
//      front buffer (what the user sees).
//
void renderScene(void) 
{
    //update any internal parameters in the scene.
    updateScene();

    //clear the render buffer
    glClearColor(skyColor.x,skyColor.y,skyColor.z,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(displayingTracedImage)
    {
        //render a quad that covers the scene. disable lighting, and texture
        //the quad with the result from the raytracing (which should have been
        //registered as an OpenGL texture using the global texture handle).
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(-1, 1, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        //the texture on this quad will have been set to the raytraced image.
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, raytracedTexHandle);
        glDisable(GL_LIGHTING);

        glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(0, 0); glVertex2f(-1,  1);
            glTexCoord2f(1, 0); glVertex2f( 1,  1);
            glTexCoord2f(1, 1); glVertex2f( 1, -1);
        glEnd();

    
        glDisable(GL_TEXTURE_2D);
        //after we're done, restore that projection matrix for the perspective mode.
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    } else {
        //PREVIEW MODE: render the scene like from hw05.

        //update the modelview matrix based on the camera's position
        glMatrixMode(GL_MODELVIEW);             //make sure we aren't changing the projection matrix!
        glLoadIdentity();
        mainCamera.sendLookAtMatrix();          //send the lookat matrix to OpenGL

        float l0Position[4] = { 1, .75, 1, 0 };
        glLightfv( GL_LIGHT0, GL_POSITION, l0Position );

        //draw a grid, just to keep us from going crazy / getting lost.
        glPushMatrix();
            drawGrid();
        glPopMatrix();

        //render the spheres! 
        for(unsigned int i = 0; i < shapes.size(); i++)
            shapes[i]->draw();
    }

    //push the back buffer to the screen
    glutSwapBuffers();
}






//
//  void normalKeysUp(unsigned char key, int x, int y)
//
//      GLUT keyboard callback for when a regular key is released.
//
void normalKeysUp(unsigned char key, int x, int y) 
{
    if(key == 'w' || key == 'W')
        forwardKeyHeldDown = false;

    if(key == 's' || key == 'S')
        backwardKeyHeldDown = false;
}

unsigned char *uploadRT() {
    unsigned char *raytracedImage = makeImageData(rt_data.dest, rt_data.samples, 1.0, rt_data.width, rt_data.height);

    registerOpenGLTexture(raytracedImage, 
                          rt_data.width, 
                          rt_data.height, 
                          raytracedTexHandle);

    return raytracedImage;
}

void stepRT(int timercrap) {
    if (tracing) {
        for(unsigned int i = 0; i < rt_data.height; i++) {
            for(unsigned int j = 0; j < rt_data.width; j++) {
                int idx = (i*rt_data.width+j);

                if(idx % 1000 == 0) {
                    printf("\33[2K\r");
                    printf("%.2f%% of the way done...", (i*rt_data.width+j) / (float)(rt_data.width*rt_data.height) * 100.0f);
                    fflush(stdout);
                }

                Ray prime = rt_camera.getPrimaryRayThroughPixel(j, i, rt_data.width, rt_data.height);
                rt_samplePath(prime, rt_data, idx, rt_camera.nearClipPlane, rt_camera.farClipPlane, 5);
            }
        }
        rt_data.samples += 5;

        delete[] uploadRT();
        glutTimerFunc(0, stepRT, 0);
    }
}

//
//  void normalKeysDown(unsigned char key, int x, int y)
//
//      GLUT keyboard callback for when a regular key is pressed.
//
void normalKeysDown(unsigned char key, int x, int y) 
{
    if(key == 'q' || key == 'Q')
        exit(0);

    if(key == 'w' || key == 'W')
        forwardKeyHeldDown = true;

    if(key == 's' || key == 'S')
        backwardKeyHeldDown = true;

    if(key == 'r' || key == 'R')
    {
        displayingTracedImage = !displayingTracedImage;
        if(displayingTracedImage && !tracing)
        {
            // Initilize data
            rt_camera = mainCamera;
            rt_data.skyEmission = skyColor;
            rt_start = glutGet(GLUT_ELAPSED_TIME);
            rt_data.width = mainCamera.imageWidth;
            rt_data.height = mainCamera.imageHeight;
            int dest_size = rt_data.width * rt_data.height;
            if (rt_data.dest != NULL) delete[] rt_data.dest;
            rt_data.dest = new Point[dest_size];
            for (int i = 0; i < dest_size; i++) {
                rt_data.dest[i] = Point(0, 0, 0);
            }
            rt_data.shapeCount = shapes.size();
            if (rt_data.shapes != NULL) delete[] rt_data.shapes;
            rt_data.shapes = new Shape*[rt_data.shapeCount];
            for (int i = 0; i < rt_data.shapeCount; i++) {
                rt_data.shapes[i] = shapes[i];
            }

            rt_data.samples = 1;
            delete[] uploadRT();
            rt_data.samples = 0;

            // start
            tracing = true;

            glutTimerFunc(0, stepRT, 0);
        }
    }

    if(key == 'e' || key == 'E')
    {
        if (tracing) {
            tracing = false;

            int end = glutGet(GLUT_ELAPSED_TIME);
            printf("Rendered %d samples in %.2f seconds.\n", rt_data.samples, (end - rt_start) / 1000.0f);

            unsigned char *img = uploadRT();
            writeToPPM("output.ppm", rt_data.width, rt_data.height, img);
            delete[] img;
        }
    }

    if(key == 't' || key == 'T')
    {
        displayingTracedImage = !displayingTracedImage;
    }
}




//
// void myTimer(int value)
//
//  We have to take an integer as input per GLUT's timer function prototype;
//  but we don't use that value so just ignore it. We'll register this function
//  once at the start of the program, and then every time it gets called it
//  will perpetually re-register itself and tell GLUT that the screen needs
//  be redrawn. yes, I said "needs be."
//
void myTimer(int value)
{
    glutPostRedisplay();

    glutTimerFunc((unsigned int)(1000.0 / 60.0), myTimer, 0);
}



//
//  void initScene()
//
//  A basic scene initialization function; should be called once after the
//      OpenGL context has been created. Doesn't need to be called further.
//      Just sets up a few function calls so that our main() is cleaner.
//
void initScene(unsigned int windowWidth, unsigned int windowHeight, float verticalFieldOfView) 
{
    //give the camera a scenic starting point.
    Point pos = Point(30, 20, 15);
    Point up = Point(0,1,0);
    float phi = M_PI/2.8f;
    float theta = -M_PI/3.0f;

    mainCamera = FreeCamera( pos, 
                                up, 
                                windowWidth, 
                                windowHeight, 
                                verticalFieldOfView, 
                                0.1f, 
                                10000.0f, 
                                phi, 
                                theta);


    //do some basic OpenGL setup
    glEnable(GL_DEPTH_TEST);

    float black[4] = {0, 0, 0, 0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);

    //tells OpenGL to blend colors across triangles. Once lighting is
    //enabled, this means that objects will appear smoother - if your object
    //is rounded or has a smooth surface, this is probably a good idea;
    //if your object has a blocky surface, you probably want to disable this.
    glShadeModel(GL_SMOOTH);

    glDisable(GL_CULL_FACE);

    float lightCol[4] = { .8, .8, .8, 1};
    float ambientCol[4] = { 1, 1, 1, 1.0 };
    glLightfv( GL_LIGHT0, GL_DIFFUSE,lightCol );
    glLightfv( GL_LIGHT0, GL_AMBIENT, ambientCol );
    glEnable( GL_LIGHT0 );
}

//
//  void loadSceneFromFile()
//
//  Loads a scene file from disk and creates the relevant lights, objects, and materials.
//
void loadSceneFromFile(const char *filename)
{
    string line;

    Material currentMaterial = Material(Point(1,1,1),
                                        Point(0,0,0));

    ifstream in(filename);
    while(getline(in, line))
    {   
        vector<string> tokens = tokenizeString(line, " ");
        if(tokens.size() < 2) continue;
    
        if(!tokens[0].compare("sphere")) 
        {
            float x, y, z, r;
            x = atof( tokens[1].c_str() );
            y = atof( tokens[2].c_str() );
            z = atof( tokens[3].c_str() );
            r = atof( tokens[4].c_str() );

            Sphere *s = new Sphere(Point(x,y,z), r);
            s->material = currentMaterial;

            shapes.push_back(s);
        } else if(!tokens[0].compare("material")) {

            float dr, dg, db, er, eg, eb;
            dr = atof( tokens[1].c_str() );
            dg = atof( tokens[2].c_str() );
            db = atof( tokens[3].c_str() );
            er = atof( tokens[4].c_str() );
            eg = atof( tokens[5].c_str() );
            eb = atof( tokens[6].c_str() );

            currentMaterial = Material(Point(dr, dg, db),
                                       Point(er, eg, eb));

        } else if(!tokens[0].compare("plane")) {

            float x, y, z, nx, ny, nz;
            x = atof( tokens[1].c_str() );
            y = atof( tokens[2].c_str() );
            z = atof( tokens[3].c_str() );
            nx = atof( tokens[4].c_str() );
            ny = atof( tokens[5].c_str() );
            nz = atof( tokens[6].c_str() );

            Plane *p = new Plane(Point(x, y, z), Point(nx, ny, nz));
            p->material = currentMaterial;

            shapes.push_back(p);
        } else if(!tokens[0].compare("sky")) {

            float r, g, b;
            r = atof( tokens[1].c_str() );
            g = atof( tokens[2].c_str() );
            b = atof( tokens[3].c_str() );

            skyColor = Point(r, g, b);
        }
    }

    in.close();
}


// main() //////////////////////////////////////////////////////////////////////
//
//  Program entry point. Does not process command line arguments.
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    string sceneFilename = "scene.txt";
    if(argc != 2)
    {
        printf("No scene file specified; using scene.txt.\n");
    } else {
        sceneFilename = string(argv[1]);
    }

    int windowWidth = 512;
    int windowHeight = 512;

    //create a double-buffered GLUT window at (50,50) with predefined windowsize
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(50,50);
    glutInitWindowSize(windowWidth,windowHeight);
    glutCreateWindow("SideQuest 3: Raytracing");


    //create the camera, light, and set some initial OpenGL parameters...
    initScene(windowWidth, windowHeight, 55.0f);


    //register callback functions...
    glutKeyboardFunc(normalKeysDown);
    glutKeyboardUpFunc(normalKeysUp);
    glutDisplayFunc(renderScene);
    glutReshapeFunc(resizeWindow);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(mouseMotion);


    //load the scene from file...
    loadSceneFromFile(sceneFilename.c_str());



    //setup our timer so that we can animate and stuff.
    glutTimerFunc((unsigned int)(1000.0 / 60.0), myTimer, 0);


    //and enter the GLUT loop, never to exit.
    glutMainLoop();

    return(0);
}
