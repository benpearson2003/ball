#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "glut.h"

#define PI 3.14159265358979324

#define ROWS 10		// Number of rows of Blocks.
#define COLUMNS 25	// Number of columns of Blocks.
#define SLICES 10	// Number of slices of Blocks.
#define FILL_PROBABILITY 5 // Percentage probability that a particular row-column slot will be 
// filled with a Block. It should be an integer between 0 and 100.

using namespace std;

static unsigned int texture[3]; // Array of texture indices.
static float theta = PI/2; // Angle of the sun with the ground.

// Struct of bitmap file.
struct BitMapFile
{
	int sizeX;
	int sizeY;
	unsigned char *data;
};

// Routine to read a bitmap file.
// Works only for uncompressed bmp files of 24-bit color.
BitMapFile *getBMPData(string filename)
{
	BitMapFile *bmp = new BitMapFile;
	unsigned int size, offset, headerSize;

	// Read input file name.
	ifstream infile(filename.c_str(), ios::binary);

	// Get the starting point of the image data.
	infile.seekg(10);
	infile.read((char *) &offset, 4); 

	// Get the header size of the bitmap.
	infile.read((char *) &headerSize,4);

	// Get width and height values in the bitmap header.
	infile.seekg(18);
	infile.read( (char *) &bmp->sizeX, 4);
	infile.read( (char *) &bmp->sizeY, 4);

	// Allocate buffer for the image.
	size = bmp->sizeX * bmp->sizeY * 24;
	bmp->data = new unsigned char[size];

	// Read bitmap data.
	infile.seekg(offset);
	infile.read((char *) bmp->data , size);

	// Reverse color from bgr to rgb.
	int temp;
	for (unsigned int i = 0; i < size; i += 3)
	{ 
		temp = bmp->data[i];
		bmp->data[i] = bmp->data[i+2];
		bmp->data[i+2] = temp;
	}

	return bmp;
}

void loadExternalTextures()			
{
	// Local storage for bmp image data.
	BitMapFile  *image[4]; 

	// Load the textures.
	image[0] = getBMPData("grass.bmp");
	image[1] = getBMPData("dirt.bmp");   
	image[2] = getBMPData("sky.bmp");  
	image[3] = getBMPData("grass2.bmp");

	// Bind grass image to texture index[0]. 
	glBindTexture(GL_TEXTURE_2D, texture[0]); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[0]->sizeX, image[0]->sizeY, 0, 
		GL_RGB, GL_UNSIGNED_BYTE, image[0]->data);

	// Bind sky image to texture index[1]
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[1]->sizeX, image[1]->sizeY, 0, 
		GL_RGB, GL_UNSIGNED_BYTE, image[1]->data);

	// Bind dirt image to texture index[2]
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[2]->sizeX, image[2]->sizeY, 0, 
		GL_RGB, GL_UNSIGNED_BYTE, image[2]->data);
}

//Begin globals. 
static int width, height; // Size of the OpenGL window.

//		BEGIN angle of rotation
float xpos = 0, ypos = 0, zpos = 0, xrot = 0, yrot = 0, angle=0.0;
float lastx, lasty;
//		END angle of rotation

static float xVal = 0.0, yVal = 2.0, zVal = 0.0; // Co-ordinates of the character.
static bool isCollision = false; // Is there collision between objects?
float val = (rand()%5)*0.1;	// Random value used for textures.

float blockWidth = 4.0;

//		BEGIN Movement Vector
float moveAngle = PI/2;
float moveDistance = 4.0;
//		END Movement Vector

//END globals.

void camera (void) {
	glRotatef(xrot,1.0,0.0,0.0);  //rotate our camera on the x-axis (left and right)
	glRotatef(yrot,0.0,1.0,0.0);  //rotate our camera on the y-axis (up and down)
	glTranslated(-xpos,-ypos,-zpos); //translate the screen to the position of our camera
}

class Ball
{
public:
	Ball() 
	{
		centerX = 0.0;
		centerY = 0.0;
		centerZ = 0.0; 
		width = 0.0; // Indicates no Ball exists in the position.
	}
	Ball(float x, float y, float z, float w) 
	{
		centerX = x;
		centerY = y;
		centerZ = z; 
		width = w;
	}
	float getCenterX() { return centerX; }
	float getCenterY() { return centerY; }
	float getCenterZ() { return centerZ; }
	float getWidth()  { return width; }
	void draw() 
	{
		if (width > 0.0) // If Ball exists.
		{
			glPushMatrix();
			glTranslatef(centerX, centerY, centerZ);
			glutSolidSphere(width,10,10);
			glPopMatrix();
		}
	}

private:
	float centerX, centerY, centerZ, width;
};

class Block
{

private:
	float centerX, centerY, centerZ, width;
	int type;

public:
	Block() {centerX = 0.0;
	centerY = 0.0;
	centerZ = 0.0; 
	width = 4.0;	
	type = 0;		/*Indicates type as AIR*/
	}

	Block(float x, float y, float z, float w, int t) {
	centerX = x;
	centerY = y;
	centerZ = z; 
	width = w;
	type = t;
}

	float getCenterX() { return centerX; }
	float getCenterY() { return centerY; }
	float getCenterZ() { return centerZ; }
	float getWidth()  { return width; }
	float getType() { return type; }
	void setType(int t) { type = t; }
	void draw(int row, int column, int slice);
};
Block arrayBlocks[ROWS][COLUMNS][SLICES];	// Global array of Blocks.
// Function to draw Block.
void Block::draw(int row, int column, int slice)
{
	if (type != 0) // If Block exists.
	{
		//glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
		//glEnable(GL_TEXTURE_GEN_T);
		//	glBindTexture(GL_TEXTURE_2D, texture[1]);
		//		glutSolidCube(width);
		//glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
		//glDisable(GL_TEXTURE_GEN_T);
	

		glBlendFunc(GL_ONE, GL_ZERO); // Specify blending parameters to overwrite background.
		glBindTexture(GL_TEXTURE_2D, texture[1]);

			if(arrayBlocks[row-1][column][slice].getType()==0 || row==0)
			{
		glBegin(GL_QUADS);		//front face ccw from +Z
			glNormal3f(0.0, 1.0, 0.0); 
			glTexCoord2d(0.0, 0.0); glVertex3f(centerX-width/2, centerY-width/2, centerZ+width/2);
			glTexCoord2d(0.2, 0.0); glVertex3f(centerX+width/2, centerY-width/2, centerZ+width/2);
			glTexCoord2d(0.2, 0.2); glVertex3f(centerX+width/2, centerY+width/2, centerZ+width/2);
			glTexCoord2d(0.0, 0.2); glVertex3f(centerX-width/2, centerY+width/2, centerZ+width/2);
		glEnd();
			}

			if(arrayBlocks[row][column][slice-1].getType()==0 || slice==0)
			{
		glBegin(GL_QUADS);		//bottom face ccw from -Y
			glNormal3f(0.0, 1.0, 0.0);
			glTexCoord2d(0.0, 0.0); glVertex3f(centerX-width/2, centerY-width/2, centerZ+width/2);
			glTexCoord2d(0.5, 0.0); glVertex3f(centerX-width/2, centerY-width/2, centerZ-width/2);
			glTexCoord2d(0.5, 0.5); glVertex3f(centerX+width/2, centerY-width/2, centerZ-width/2);
			glTexCoord2d(0.0, 0.5); glVertex3f(centerX+width/2, centerY-width/2, centerZ+width/2);
		glEnd();
			}

			if(arrayBlocks[row+1][column][slice].getType()==0 || row == ROWS-1)
			{
		glBegin(GL_QUADS);		//back face ccw from -Z
			glNormal3f(0.0, 1.0, 0.0);
			glTexCoord2d(0.0, 0.0); glVertex3f(centerX-width/2, centerY-width/2, centerZ-width/2);
			glTexCoord2d(0.2, 0.0); glVertex3f(centerX-width/2, centerY+width/2, centerZ-width/2);
			glTexCoord2d(0.2, 0.2); glVertex3f(centerX+width/2, centerY+width/2, centerZ-width/2);
			glTexCoord2d(0.0, 0.2); glVertex3f(centerX+width/2, centerY-width/2, centerZ-width/2);
		glEnd();
			}

			if(arrayBlocks[row][column+1][slice].getType()==0 || column == COLUMNS-1)
			{
		glBegin(GL_QUADS);		//right face ccw from +X
			glNormal3f(0.0, 1.0, 0.0);
			glTexCoord2d(0.0, 0.0); glVertex3f(centerX+width/2, centerY-width/2, centerZ+width/2);
			glTexCoord2d(0.5, 0.0); glVertex3f(centerX+width/2, centerY-width/2, centerZ-width/2);
			glTexCoord2d(0.5, 0.5); glVertex3f(centerX+width/2, centerY+width/2, centerZ-width/2);
			glTexCoord2d(0.0, 0.5); glVertex3f(centerX+width/2, centerY+width/2, centerZ+width/2);
		glEnd();
			}

		if(arrayBlocks[row][column-1][slice].getType()==0 || column == 0)
			{
		glBegin(GL_QUADS);		//left face ccw from -X
			glNormal3f(0.0, 1.0, 0.0);
			glTexCoord2d(0.0, 0.0); glVertex3f(centerX-width/2, centerY-width/2, centerZ-width/2);
			glTexCoord2d(0.5, 0.0); glVertex3f(centerX-width/2, centerY-width/2, centerZ+width/2);
			glTexCoord2d(0.5, 0.5); glVertex3f(centerX-width/2, centerY+width/2, centerZ+width/2);
			glTexCoord2d(0.0, 0.5); glVertex3f(centerX-width/2, centerY+width/2, centerZ-width/2);
		glEnd();
		}

		glBlendFunc(GL_ONE, GL_ZERO); // Specify blending parameters to overwrite background.
		glBindTexture(GL_TEXTURE_2D, texture[0]);

		if(arrayBlocks[row][column][slice+1].getType()==0 || slice == SLICES-1)
			{
		glBegin(GL_QUADS);		//top face ccw from +Y
			glNormal3f(0.0, 1.0, 0.0);
			glTexCoord2d(0.0, 0.0); glVertex3f(centerX-width/2, centerY+width/2, centerZ+width/2);
			glTexCoord2d(val, 0.0); glVertex3f(centerX+width/2, centerY+width/2, centerZ+width/2);
			glTexCoord2d(val, val); glVertex3f(centerX+width/2, centerY+width/2, centerZ-width/2);
			glTexCoord2d(0.0, val); glVertex3f(centerX-width/2, centerY+width/2, centerZ-width/2);
		glEnd();
		}
	}
}



bool canBeSolid[ROWS][COLUMNS][SLICES];		//Determines if block should be designated AIR.

// Initialization routine.

int checkBlocksIntersection(float ballX, float ballY, float ballZ, float ballRadius, float BlockX, float BlockY, float BlockZ, float BlockWidth)
{
	float xMin = BlockX-BlockWidth; float xMax = BlockX+BlockWidth; float yMin = BlockY-BlockWidth; float yMax = BlockY+BlockWidth; float zMin = BlockZ-BlockWidth; float zMax = BlockZ+BlockWidth;
	if((zMin < ballZ-ballRadius && ballZ+ballRadius < zMax) && (yMin < ballY-ballRadius && ballY+ballRadius < yMax+1.0) && (xMin < ballX-ballRadius && ballX+ballRadius < xMax))
	{
		return 1;
	}
	return 0;
}

bool collision(float ballX, float ballY, float ballZ)
{
	int i,j,k;

	float ballRadius = 1.0;

	// Check for collision with each Block.
	for (j=0; j<COLUMNS; j++)
		for (i=0; i<ROWS; i++)
			for (k=0; k<SLICES; k++)
			if (arrayBlocks[i][j][k].getWidth() > 0 ) // If Block exists.
				if (checkBlocksIntersection(ballX, ballY, ballZ, ballRadius,
					arrayBlocks[i][j][k].getCenterX(), 
					arrayBlocks[i][j][k].getCenterY(), 
					arrayBlocks[i][j][k].getCenterZ(), arrayBlocks[i][j][k].getWidth()) && arrayBlocks[i][j][k].getType()!= 0)
					return true;
	return false;
}

void setup(void)
{
	int i=0, j=0, k=0, t=0;

	for (j=0; j<COLUMNS; j++)
	{
		for (i=0; i<ROWS; i++)
		{
			canBeSolid[i][j][k] = true;
		}
	}

	// Initialize global arrayBlocks.
	for (j=0; j<COLUMNS; j++)
	{
		for (i=0; i<ROWS; i++)
		{
			for (k=0; k<SLICES; k++)
			{
				if(rand()%100 < FILL_PROBABILITY*(SLICES-k) && canBeSolid[i][j][k]) /*Lower slices have higher chance of being solid*/ 
				{
					t = 2;		//type is dirt

					if((i+1)<ROWS)
					{
						canBeSolid[i+1][j][k] = true;
					}

					if((j+1)<COLUMNS)
					{
						canBeSolid[i][j+1][k] = true;
					}

					if((k+1)<SLICES)
					{
						canBeSolid[i][j][k+1] = true;
					}
				}

				else
					t = 0;		//type is air

				arrayBlocks[i][j][k] = Block( blockWidth*(-COLUMNS/2+j), blockWidth*k+2.0, -40.0 - blockWidth*i, blockWidth, t);
			}
		}
	}

	glEnable(GL_LIGHTING);
	glClearColor (0.0, 0.0, 0.0, 0.0);

	// Light property vectors that are constant.
	float lightAmb[] = { 0.0, 0.0, 0.0, 1.0 };
	float lightDifAndSpec[] = { 1.0, 1.0, 1.0, 1.0 };
	float globAmb[] = { 0.2, 0.2, 0.2, 1.0 };

	// Light properties.
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDifAndSpec);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightDifAndSpec);

	glEnable(GL_LIGHT0); // Enable particular light source.
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globAmb); // Global ambient light.

	// Material property vectors that are constant.
	float matAmbAndDif[] = {1.0, 1.0, 1.0, 1.0};
	float matSpec[] = { 1.0, 1.0, 1.0, 1.0 };
	float matShine[] = { 50.0 };

	// Material properties.
	//glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matAmbAndDif);
	//glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
	//glMaterialfv(GL_FRONT, GL_SHININESS, matShine);

	// Create texture index array and load external textures.
	glGenTextures(4, texture);
	loadExternalTextures();

	// Turn on OpenGL texturing.
	glEnable(GL_TEXTURE_2D);   

	// Specify how texture values combine with current surface color values.
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

	glEnable(GL_BLEND); // Enable blending.

	// Cull back faces.
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
}

void drawGroundAndSky(){
	glPushMatrix();
	// Blend the grass texture onto a rectangle lying along the xz-plane.
	glBlendFunc(GL_ONE, GL_ZERO); // Specify blending parameters to overwrite background.
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glBegin(GL_POLYGON);
		glNormal3f(0.0, 1.0, 0.0);
		glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, -1.0, +100.0);
		glTexCoord2f(8.0, 0.0); glVertex3f(+100.0, -1.0, +100.0);
		glTexCoord2f(8.0, 8.0); glVertex3f(+100.0, -1.0, -100.0);
		glTexCoord2f(0.0, 8.0); glVertex3f(-100.0, -1.0, -100.0);
	glEnd();

	// Blend the night sky texture onto a rectangle parallel to the xy-plane.
	glBlendFunc(GL_ONE, GL_ZERO); // Specify blending parameters to overwrite background.
	glBindTexture(GL_TEXTURE_2D, texture[2]);        
	glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 0.0); glVertex3f(xVal-1000.0, yVal-50.0, zVal-200.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(xVal+1000.0, yVal-50.0, zVal-200.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(xVal+1000.0, yVal+600.0, zVal-200.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(xVal-1000.0, yVal+600.0, zVal-200.0);
	glEnd();
	glPopMatrix();
}

void drawScene(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	float alpha; // Blending parameter.

	// Light property vectors that change during runtime.
	float lightPos[] = { cos(theta), sin(theta), 0.0, 0.0 };

	// Light properties.
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	int i,j,k = 0;
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport (0, 0, width,  height);

	glEnable(GL_LIGHTING);

	glLoadIdentity();	

	// The blending parameter varies with the angle of the sun.
	if (theta <= 90.0) alpha = theta/90.0; 
	else alpha = (180.0 - theta)/90.0;    
	glColor4f(1.0, 1.0, 1.0, alpha); 

	camera();

	glColor3f(0.0, 0.0, 0.0);

	drawGroundAndSky();

	glColor3f(0.0, 0.0, 0.0);

	Ball myBall(xVal,yVal,zVal,2.0);
	myBall.draw();

	glColor3f(0.0, 0.0, 0.0);

	for (j=0; j<COLUMNS; j++)
		for (i=0; i<ROWS; i++)
			for(k=0; k<SLICES; k++)
				arrayBlocks[i][j][k].draw(i,j,k);

	glutSwapBuffers();
}

void specialKeyInput(int key, int x, int y)
{
	/*if (key == GLUT_KEY_LEFT)
	{
		horizAngle += moveAngle;
	}
	if (key == GLUT_KEY_RIGHT)
	{
		horizAngle -= moveAngle;
	}
	if( key == GLUT_KEY_UP)
	{
		verticalAngle -= moveAngle;
	}
	if( key == GLUT_KEY_DOWN)
	{
		verticalAngle += moveAngle;
	}

	if(horizAngle>2*PI)
		horizAngle-=2*PI;
	else if(horizAngle<0.0)
		horizAngle+=2*PI;
	if(verticalAngle>PI+.05)
		verticalAngle=PI;
	else if(verticalAngle<0.0)
		verticalAngle=0.0;*/

	glutPostRedisplay();
}

void keyInput(unsigned char key, int x, int y)
{
	//float tempxVal = xVal, tempyVal = yVal, tempzVal = zVal;

	switch (key) 
	{
	//case 'a':
	//	tempxVal = xVal + moveDistance*sin(horizAngle-PI/2);
	//	tempzVal = zVal + moveDistance*cos(horizAngle-PI/2);
	//	break;
	//case 'A':
	//	tempxVal = xVal + moveDistance*sin(horizAngle-PI/2);
	//	tempzVal = zVal + moveDistance*cos(horizAngle-PI/2);
	//	break;
	//case 'd':
	//	tempxVal = xVal + moveDistance*sin(horizAngle+PI/2);
	//	tempzVal = zVal + moveDistance*cos(horizAngle+PI/2);
	//	break;
	//case 'D':
	//	tempxVal = xVal + moveDistance*sin(horizAngle+PI/2);
	//	tempzVal = zVal + moveDistance*cos(horizAngle+PI/2);
	//	break;
	//case 'w':
	//	tempxVal = xVal - moveDistance*sin(horizAngle);
	//	tempzVal = zVal - moveDistance*cos(horizAngle);
	//	break;
	//case 'W':
	//	tempxVal = xVal - moveDistance*sin(horizAngle);
	//	tempzVal = zVal - moveDistance*cos(horizAngle);
	//	break;
	//case 's':
	//	tempxVal = xVal + moveDistance*sin(horizAngle);
	//	tempzVal = zVal + moveDistance*cos(horizAngle);
	//	break;
	//case 'S':
	//	tempxVal = xVal + moveDistance*sin(horizAngle);
	//	tempzVal = zVal + moveDistance*cos(horizAngle);
	//	break;
	//case ' ':
	//	xVal = 0.0;
	//	yVal = 2.0;
	//	zVal = 0.0;
	//	horizAngle = 0.0;
	//	verticalAngle = 0.0;
	//	glutPostRedisplay();
	//	break;
	case 27:
		exit(0);
		break;
	default:
		break;
	}

	//// If ball collides with Block, climb Block of less than 5 units.
	//while (collision(tempxVal, tempyVal, tempzVal) && tempyVal < yVal+5.0 )
	//{
	//	tempyVal += 1.0;
	//}

	////if collision still occurs, don't allow ball to move forward
	//if (collision(tempxVal, tempyVal, tempzVal))
	//{
	//	tempxVal = xVal;
	//	tempyVal = yVal;
	//	tempzVal = zVal;
	//}

	////if no collision beneath ball, allow it to fall to ground level
	//while(!collision(tempxVal,tempyVal,tempzVal) && tempyVal>2.0)
	//{
	//	tempyVal -= 1.0;
	//}

	//isCollision = false;
	//xVal = tempxVal;
	//yVal = tempyVal;
	//zVal = tempzVal;

	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{

}

void mouseMotion(int x, int y)
{
	int diffx=x-lastx;		//check the difference between the current x and the last x position
	int diffy=y-lasty;		//check the difference between the current y and the last y position
	lastx=x;				//set lastx to the current x position
	lasty=y;				//set lasty to the current y position
	xrot += (float) diffy;	//set the xrot to xrot with the addition of the difference in the y position
	yrot += (float) diffx;	// set the xrot to yrot with the addition of the difference in the x position

	glutPostRedisplay();
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
	glViewport (0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glFrustum(-8.0, 8.0, -4.5, 4.5, 5.0, 250.0);
	gluPerspective(45.0, 2.0, 1.0, 250.0);
	glMatrixMode(GL_MODELVIEW);

	// Pass the size of the OpenGL window.
	width = w;
	height = h;
}

// Main routine.
int main(int argc, char **argv) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 
	glutInitWindowSize(1050, 500);
	glutInitWindowPosition(0, 0); 
	glutCreateWindow("ball.cpp");
	setup(); 
	glutDisplayFunc(drawScene); 
	glutReshapeFunc(resize);  
	glutKeyboardFunc(keyInput);
	glutSpecialFunc(specialKeyInput);
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(mouseMotion);
	glutMainLoop(); 

	return 0;  
}
