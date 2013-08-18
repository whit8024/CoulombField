#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <Windows.h>

#include <HD/hd.h>

#include "helper.h"

#include <HDU/hduError.h>
#include <HDU/hduVector.h>

static double sphereSmall = 12.0;
static double sphereBig = 50;
/* Charge (positive/negative) */
int charge = 1;
int width, height;
static HHD ghHD = HD_INVALID_HANDLE;
static HDSchedulerHandle gSchedulerCallback = HD_INVALID_HANDLE;
float initX = 50;
/* Glut callback functions used by helper.cpp */
void displayFunction(void);
void handleIdle(void);

hduVector3Dd forceField(hduVector3Dd pos);
char label1[50];
char label2[50];
char label3[50];
char label4[50];
/* Haptic device record. */
struct DeviceDisplayState
{
    HHD m_hHD;
    hduVector3Dd position;
    hduVector3Dd force;
	hduVector3Dd velocity;
	
};
static DeviceDisplayState state;
HDdouble timer = 0;

hduVector3Dd project(const hduVector3Dd &a, const hduVector3Dd &b)
{
	hduVector3Dd tmp= (a.dotProduct(b)*b / b.dotProduct(b));
	return tmp;
	
}
/*******************************************************************************
 Graphics main loop function.
*******************************************************************************/
hduVector3Dd getVirtual(hduVector3Dd velocity)
{
	hduVector3Dd velo=project(velocity , hduVector3Dd (1,0,0));
	initX+= (velo[0]>0 ? 1 :-1 ) *
	state.velocity.magnitude()*0.001;    
	return hduVector3Dd (initX, 0,0);
}
void displayFunction(void)
{
    // Setup model transformations.
    glMatrixMode(GL_MODELVIEW); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    setupGraphicsState();
	
	GLfloat mau[3]={1,1,1};
    glEnable(GL_COLOR_MATERIAL);
  
	// Draw the fixed sphere.
    
	static const float fixedSphereColor[4] = {.2, .8, .8, .8};
    GLUquadricObj* pQuadObj = gluNewQuadric();
    drawSphere(pQuadObj, hduVector3Dd (0,0,0), fixedSphereColor, sphereBig );
	
	// Do code here to grab position

    // hdScheduleSynchronous(DeviceStateCallback, &state,
    //                    HD_MIN_SCHEDULER_PRIORITY);
	static const float dynamicSphereColor[4] = { .8, .2, .2, .8 };
	//hduVector3Dd currentPos= getVirtual(state.velocity );

	drawSphere(pQuadObj,
               state.position,
               dynamicSphereColor,
               sphereSmall);	
	
	char chuoi[50];
	sprintf(chuoi, "Current velocity: %.4f", state.velocity.magnitude());
	
	drawHapticsString(label1, hduVector3Dd(width *-0.9,height*0.9,0), 0.1, mau);
    drawHapticsString(label2, hduVector3Dd(width *-0.9,height*0.8,0), 0.1, mau);
    drawHapticsString(label3, hduVector3Dd(width *-0.9,height*0.7,0), 0.1, mau);
    drawHapticsString(label4, hduVector3Dd(width *-0.9,height*0.6,0), 0.1, mau);
    
	hduVector3Dd forceVector = forceField(state.position);
    

    drawForceVector(pQuadObj,
					state.position,
                    forceVector,
                    sphereSmall*.1);

    gluDeleteQuadric(pQuadObj);
  
    glPopMatrix();
    glutSwapBuffers();                      
}
                                
/*******************************************************************************
 Called periodically by the GLUT framework.
*******************************************************************************/
void handleIdle(void)
{
    glutPostRedisplay();

    //if (!hdWaitForCompletion(gSchedulerCallback, HD_WAIT_CHECK_STATUS))
    //{
    //    printf("The main scheduler callback has exited\n");
    //    printf("Press any key to quit.\n");
    //    getchar();
    //    exit(-1);
    //}
}

/******************************************************************************
 Popup menu handler
******************************************************************************/
void handleMenu(int ID)
{
    switch(ID) 
    {
        case 0:
            exit(0);
            break;
        case 1:
            charge *= -1;
            break;
    }
}


/*******************************************************************************
 Given the position is space, calculates the (modified) coulomb force.
*******************************************************************************/
hduVector3Dd forceField(hduVector3Dd pos)
{
    double dist = pos.magnitude();
    
    hduVector3Dd forceVec(0,0,0);
    
        double loi = pos[0]-sphereBig;
        if (loi<0)
		{
			forceVec = hduVector3Dd (-10.0*loi,0,0);
			
		}
    return forceVec;
}



/*******************************************************************************
 Main callback that calculates and sets the force.
*******************************************************************************/
double am=0.01;
double freg=500;
double B=10;
double K=1;
hduVector3Dd calculateF(hduVector3Dd nguon, hduVector3Dd &origin, double radi)
{
	if ((nguon-origin).magnitude()<radi)
		return (radi-(nguon-origin).magnitude())* normalize(nguon-origin)
				;
	else return hduVector3Dd(0,0,0);
}


hduVector3Dd remVec(0,0,0);
hduVector3Dd CoulombCallback(hduVector3Dd pos, hduVector3Dd remVec)
{
	HDdouble instRate;
    hduVector3Dd force(0,0,0);
		
	timer += 1.0 / 1000;
    
	hduVector3Dd layforce=calculateF(pos,
			hduVector3Dd(0,0.0,0),
			sphereBig );
	if (layforce.magnitude()>0.01*sphereBig 
		&& timer>0.5
		) //penetrate more than 1% and 5 seconds has passed
	{
		timer=0;
	}	
	force=am * remVec *exp(-B*timer) * sin(timer * freg);
	

	sprintf(label1, "Current Force: %.4f",force.magnitude());
	//sprintf(label3, "Current exponential: %.4f",exp(-B*timer));
	sprintf(label4, "Current timer: %.4f", timer);
	sprintf(label2, "Current Hook Force: %.4f",layforce.magnitude());
	
	force=force+ K* layforce	;
	
	if (force.magnitude()>1)
	force= force/force.magnitude();

	return force;
}

DWORD WINAPI ThreadedLoop( LPVOID lpParam ) 
{
	glutMainLoop();
	return 0; // Never happens
}

/*******************************************************************************
 Schedules the coulomb force callback.
*******************************************************************************/
HANDLE CoulombForceField()
{
    std::cout << "haptics callback" << std::endl;
    //gSchedulerCallback = hdScheduleAsynchronous(
    //    CoulombCallback, 0, HD_DEFAULT_SCHEDULER_PRIORITY);

    //HDErrorInfo error;
    //if (HD_DEVICE_ERROR(error = hdGetError()))
    //{
    //    hduPrintError(stderr, &error, "Failed to initialize haptic device");
    //    fprintf(stderr, "\nPress any key to quit.\n");
    //    getchar();
    //    exit(-1);
    //}

    std::cout << "graphics callback" << std::endl;
	glutMainLoop();
    return CreateThread(NULL, 0, ThreadedLoop, NULL, 0, NULL);
	//glutMainLoop(); // Enter GLUT main loop.
}



/******************************************************************************
 This handler gets called when the process is exiting. Ensures that HDAPI is
 properly shutdown
******************************************************************************/
void exitHandler()
{
    //hdStopScheduler();
    //hdUnschedule(gSchedulerCallback);

    //if (ghHD != HD_INVALID_HANDLE)
    //{
    //    hdDisableDevice(ghHD);
    //    ghHD = HD_INVALID_HANDLE;
    //}
}
void keyboard(unsigned char bam, int x, int y)
{
	if (bam=='x')
	{
		timer=0;
		
	}

}
/******************************************************************************
 Main function.
******************************************************************************/
int initGraphics(int argc, char* argv[])
{
    HDErrorInfo error;

    printf("Starting application\n");
    timer=0;
    atexit(exitHandler);

     //Initialize the device.  This needs to be called before any other
     //actions on the device are performed.
    //ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
    //if (HD_DEVICE_ERROR(error = hdGetError()))
    //{
    //    hduPrintError(stderr, &error, "Failed to initialize haptic device");
    //    fprintf(stderr, "\nPress any key to quit.\n");
    //    getchar();
    //    exit(-1);
    //}

    //printf("Found device %s\n",hdGetString(HD_DEVICE_MODEL_TYPE));
    //
    //hdEnable(HD_FORCE_OUTPUT);
    //hdEnable(HD_MAX_FORCE_CLAMPING);

    //hdStartScheduler();
    //if (HD_DEVICE_ERROR(error = hdGetError()))
    //{
    //    hduPrintError(stderr, &error, "Failed to start scheduler");
    //    fprintf(stderr, "\nPress any key to quit.\n");
    //    getchar();
    //    exit(-1);
    //}
	
    initGlut(argc, argv);

    // Get the workspace dimensions.
    //HDdouble maxWorkspace[6];
    //hdGetDoublev(HD_MAX_WORKSPACE_DIMENSIONS, maxWorkspace);
	HDdouble maxWorkspace[6] = {-210, -110, -85, 210, 205, 130};
	 

    // Low/left/back point of device workspace.
    hduVector3Dd LLB(maxWorkspace[0], maxWorkspace[1], maxWorkspace[2]);
    // Top/right/front point of device workspace.
    hduVector3Dd TRF(maxWorkspace[3], maxWorkspace[4], maxWorkspace[5]);
    initGraphics(LLB, TRF, width, height);

    // Application loop.
    CoulombForceField();
	return 0;
}

char* GetForce(float x, float y, float z, float vx, float vy, float vz)
{
	hduVector3Dd pos, velocity, force;

	pos = hduVector3Dd(x,y,z);
	velocity = hduVector3Dd(vx, vy, vz);
	force = calculateF(pos, hduVector3Dd(0,0,0), sphereBig) * -1;

	char buf[80];
	sprintf(buf, "f(%f,%f,%f)", force[0], force[1], force[2]);
	return buf;
}

void DefineState(float x, float y, float z, float vx, float vy, float vz)
{
	hduVector3Dd pos, velocity, force;

	pos = hduVector3Dd(x,y,z);
	velocity = hduVector3Dd(vx, vy, vz);
	force = calculateF(pos, hduVector3Dd(0,0,0), sphereBig);

	state.force = force;
	state.position = pos;
	state.velocity = velocity;
}


/******************************************************************************/
