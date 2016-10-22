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
#include "pointLight.h"
#include "shape.h"
#include "sphere.h"
#include "plane.h"


using namespace std;


// GLOBAL VARIABLES ////////////////////////////////////////////////////////////


GLint leftMouseButton, rightMouseButton;    //status of the mouse buttons
int mouseX = 0, mouseY = 0;                 //last known X and Y of the mouse
bool forwardKeyHeldDown = false;
bool backwardKeyHeldDown = false;


GLuint raytracedTexHandle;
bool displayingTracedImage = false;


FreeCamera mainCamera;
vector<Shape*> shapes;
vector<PointLight*> lights;


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

    //transfer lighting parameters to OpenGL -- only relevant in preview mode.
    for(unsigned int i = 0; i < lights.size(); i++)
        lights[i]->enableLightAndUpdateParameters();


    //clear the render buffer
    glClearColor(0,0,0,1);
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

        for(unsigned int i = 0; i < lights.size(); i++)  //send the light positions to OpenGL
            lights[i]->setLightPosition();

        //draw a grid, just to keep us from going crazy / getting lost.
        glPushMatrix();
            drawGrid();
        glPopMatrix();

        //render the spheres! 
        for(unsigned int i = 0; i < shapes.size(); i++)
            shapes[i]->draw();


        //and render our light, for debug purposes...
        for(unsigned int i = 0; i < lights.size(); i++)
            lights[i]->draw();
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
        if(displayingTracedImage)
        {
            //go to town! do the raytracing.
            unsigned char *raytracedImage;

            int start = glutGet(GLUT_ELAPSED_TIME);
            raytraceImage(mainCamera, 
                            shapes, 
                            lights, 
                            mainCamera.imageWidth, 
                            mainCamera.imageHeight, 
                            raytracedImage);

            int end = glutGet(GLUT_ELAPSED_TIME);
            printf("Render took %.2f seconds.\n", (end - start) / 1000.0f);

            //use this to set the value of the global texture handle for the image,
            //raytracedTexHandle; this will get bound and used to draw a quad over 
            //the screen that is textured with the raytraced image in renderScene().
            registerOpenGLTexture(raytracedImage, 
                                    mainCamera.imageWidth, 
                                    mainCamera.imageHeight, 
                                    raytracedTexHandle);


            //also, save the image to file.
            writeToPPM("output.ppm", mainCamera.imageWidth, mainCamera.imageHeight, raytracedImage);


            //delete the raw image data to clean up after ourselves.
            delete [] raytracedImage;
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

    //for completeness: set the overall ambient light to 0 
    float black[4] = {0, 0, 0, 0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);

    //tells OpenGL to blend colors across triangles. Once lighting is
    //enabled, this means that objects will appear smoother - if your object
    //is rounded or has a smooth surface, this is probably a good idea;
    //if your object has a blocky surface, you probably want to disable this.
    glShadeModel(GL_SMOOTH);

    glDisable(GL_CULL_FACE);
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
                                        Point(0,0,0),
                                        Point(0,0,0),
                                        0.0f,
                                        0.0f,
                                        1.0f,
                                        1.45f);

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

            float dr, dg, db, ar, ag, ab, sr, sg, sb, s, ref, alpha, ior;
            dr = atof( tokens[1].c_str() );
            dg = atof( tokens[2].c_str() );
            db = atof( tokens[3].c_str() );
            ar = atof( tokens[4].c_str() );
            ag = atof( tokens[5].c_str() );
            ab = atof( tokens[6].c_str() );
            sr = atof( tokens[7].c_str() );
            sg = atof( tokens[8].c_str() );
            sb = atof( tokens[9].c_str() );
            s = atof( tokens[10].c_str() );
            ref = atof( tokens[11].c_str() );
            alpha = atof( tokens[12].c_str() );
            ior = 1.45;
            if (alpha < 0) {
                ior = atof( tokens[13].c_str() );
            }

            currentMaterial = Material(Point(dr, dg, db),
                                        Point(ar, ag, ab),
                                        Point(sr, sg, sb),
                                        s,
                                        ref,
                                        alpha,
                                        ior);

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

        } else if(!tokens[0].compare("light")) {
            float x, y, z, dr, dg, db, ar, ag, ab, sr, sg, sb, rad;
            x = atof( tokens[1].c_str() );
            y = atof( tokens[2].c_str() );
            z = atof( tokens[3].c_str() );
            dr = atof( tokens[4].c_str() );
            dg = atof( tokens[5].c_str() );
            db = atof( tokens[6].c_str() );
            ar = atof( tokens[7].c_str() );
            ag = atof( tokens[8].c_str() );
            ab = atof( tokens[9].c_str() );
            sr = atof( tokens[10].c_str() );
            sg = atof( tokens[11].c_str() );
            sb = atof( tokens[12].c_str() );
            rad = atof( tokens[13].c_str() );

            PointLight *pl = new PointLight(Point(x, y, z),
                                            Point(dr, dg, db),
                                            Point(ar, ag, ab),
                                            Point(sr, sg, sb),
                                            rad,
                                            lights.size());
            lights.push_back(pl);
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
