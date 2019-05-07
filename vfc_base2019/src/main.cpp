// View Frustum Culling base code for CPE 476 VFC workshop
// built off 471 P4 game camera - 2015 revise with glfw and obj and glm - ZJW
// Note data-structure NOT recommended for CPE 476 - 
// object locations in arrays and estimated radii
// use your improved data structures
// 2019 revise with Application and window manag3er, shape and program

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include <ctime>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define numO 200

using namespace std;
using namespace glm;

// to cull make sure that the negagtive distance is larger than the other negative distance it can be culled bc its outside
class Application : public EventCallbacks
{

	public:
		WindowManager * windowManager = nullptr;

		//main geometry for program
		shared_ptr<Shape> mesh;
		shared_ptr<Shape> sphere;
  		shared_ptr<Program> prog;

		//global used to control culling or not for sub-window views
  		bool CULL = false;
  		vec3 g_light = vec3(2, 6, 6);

		//camera control - you can ignore - what matters is eye location and view matrix
  		double g_phi, g_theta;
  		float g_Camtrans = -2.5;
  		vec3 view = vec3(0, 0, 1);
  		vec3 strafe = vec3(1, 0, 0);
  		vec3 g_eye = vec3(0, 1, 0);
  		vec3 g_lookAt = vec3(0, 1, -1);

		//transforms on objects - frankenstein due to code re-use
  		glm::vec3 g_transB[numO];
  		float g_scaleB[numO];
  		float g_rotB[numO];
  		vec3 g_transS[numO];
  		float g_scaleS[numO];
  		float g_rotS[numO];
  		int g_mat_ids[numO];
  		float g_ang[numO];

		//ground plane geomwtry info
  		GLuint posBufObjG = 0;
  		GLuint norBufObjG = 0;

		/* helper function to change material attributes */
  		void SetMaterial(int i) {

    		prog->bind();
    		switch (i) {
    		case 0: //shiny blue plastic
    			glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.04, 0.2);
    			glUniform3f(prog->getUniform("MatDif"), 0.0, 0.16, 0.9);
    			glUniform3f(prog->getUniform("MatSpec"), 0.14, 0.2, 0.8);
    			glUniform1f(prog->getUniform("MatShine"), 120.0);
    		break;
    		case 1: // flat grey
    			glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
    			glUniform3f(prog->getUniform("MatDif"), 0.3, 0.3, 0.4);
    			glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.3, 0.4);
    			glUniform1f(prog->getUniform("MatShine"), 4.0);
    		break;
    		case 2: //brass
    			glUniform3f(prog->getUniform("MatAmb"), 0.3294, 0.2235, 0.02745);
    			glUniform3f(prog->getUniform("MatDif"), 0.7804, 0.5686, 0.11373);
    			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.80784);
    			glUniform1f(prog->getUniform("MatShine"), 27.9);
    		break;
    		case 3: //copper
    			glUniform3f(prog->getUniform("MatAmb"), 0.1913, 0.0735, 0.0225);
    			glUniform3f(prog->getUniform("MatDif"), 0.7038, 0.27048, 0.0828);
    			glUniform3f(prog->getUniform("MatSpec"), 0.257, 0.1376, 0.08601);
    			glUniform1f(prog->getUniform("MatShine"), 12.8);
    		break;
    		case 4: // flat grey
    			glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
    			glUniform3f(prog->getUniform("MatDif"), 0.3, 0.3, 0.4);
    			glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.3, 0.4);
    			glUniform1f(prog->getUniform("MatShine"), 4.0);
    		break;
    		case 5: //shadow
    			glUniform3f(prog->getUniform("MatAmb"), 0.12, 0.12, 0.12);
    			glUniform3f(prog->getUniform("MatDif"), 0.0, 0.0, 0.0);
    			glUniform3f(prog->getUniform("MatSpec"), 0.0, 0.0, 0.0);
    			glUniform1f(prog->getUniform("MatShine"), 0);
    		break;
    		case 6: //gold
    			glUniform3f(prog->getUniform("MatAmb"), 0.09, 0.07, 0.08);
    			glUniform3f(prog->getUniform("MatDif"), 0.91, 0.2, 0.91);
    			glUniform3f(prog->getUniform("MatSpec"), 1.0, 0.7, 1.0);
    			glUniform1f(prog->getUniform("MatShine"), 100.0);
    		break;
    		case 7: //green
     			glUniform3f(prog->getUniform("MatAmb"), 0.0, 0.07, 0.0);
     			glUniform3f(prog->getUniform("MatDif"), 0.1, 0.91, 0.3);
     			glUniform3f(prog->getUniform("MatSpec"), 0, 0, 0);
     			glUniform1f(prog->getUniform("MatShine"), 0.0);
    		break;
  		}
	}

mat4 SetProjectionMatrix(shared_ptr<Program> curShade) {
	int width, height;
	glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
	float aspect = width/(float)height;
	mat4 Projection = perspective(radians(50.0f), aspect, 0.1f, 100.0f);
	glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(Projection));
	return Projection;
}


mat4 SetOrthoMatrix(shared_ptr<Program> curShade) {
	float wS = 2.5;
	mat4 ortho = glm::ortho(-15.0f*wS, 15.0f*wS, -15.0f*wS, 15.0f*wS, 2.1f, 100.f);
	glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(ortho));
	return ortho;
}


/* camera controls - this is the camera for the top down view */
mat4 SetTopView(shared_ptr<Program> curShade) {
	mat4 Cam = lookAt(g_eye + vec3(0, 8, 0), g_eye, g_lookAt-g_eye);
	glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
	return Cam;
}

/*normal game camera */
mat4 SetView(shared_ptr<Program> curShade) {
	mat4 Cam = lookAt(g_eye, g_lookAt, vec3(0, 1, 0));
	glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
	return Cam;
}

/* model transforms - these are insane because they came from p2B and P4*/
mat4 SetModel(shared_ptr<Program> curS, vec3 trans, float rotY, float rotX, vec3 sc) {
	mat4 Trans = translate( glm::mat4(1.0f), trans);
	mat4 RotateY = rotate( glm::mat4(1.0f), rotY, glm::vec3(0.0f, 1, 0));
	mat4 RotateX = rotate( glm::mat4(1.0f), rotX, glm::vec3(1,0 ,0));
	mat4 Sc = scale( glm::mat4(1.0f), sc);
	mat4 ctm = Trans*RotateY*Sc*RotateX;
	glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	return ctm;
}

void SetModel(shared_ptr<Program> curS, mat4 m) {
	glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(m));
}

/* draw a snowman old code re-purposed so ugly*/
void drawSnowman(mat4 moveModel, int i) {

  SetMaterial(5);
	//shadow
  mat4 t = translate(mat4(1.0), vec3(0.2, -1.4, 0.2));
  mat4 s = scale(mat4(1.0), vec3(1, 0.01, 1));
  SetModel( prog, moveModel*t*s);

  sphere->draw(prog);

  if (i%2 ==0)
   SetMaterial(0);
 else
  SetMaterial(1);
	//body?
  t = translate(mat4(1.0), vec3(0, -0.5, 0));
  SetModel( prog, moveModel*t);
  sphere->draw(prog);

  t = translate(mat4(1.0), vec3(0., 0.72, 0));
  s = scale(mat4(1.0), vec3(.75, .75, .75));
  mat4 com = t*s;
  SetModel( prog, moveModel*com);
  sphere->draw(prog);

  t = translate(mat4(1.0), vec3(0, 1.75, 0));
  s = scale(mat4(1.0), vec3(0.55, 0.55, 0.55));
  com = t*s;
  SetModel( prog, moveModel*com);
  sphere->draw(prog);

  //switch the shading to greyscale
  SetMaterial(4);
  //the right arm
  t = translate(mat4(1.0), vec3( .37, 0.75, .5));
  mat4 r = rotate(mat4(1.0), g_ang[i], vec3(0, 0, 1));
  mat4 t1 = translate(mat4(1.0), vec3( .37, 0.0, .0));
  s = scale(mat4(1.0), vec3(0.75, 0.05, 0.05));
  com =t*r*t1*s; 
  SetModel( prog, moveModel*com);
  sphere->draw(prog);

   //update animation on arm
  g_ang[i] = sin(glfwGetTime());

   //the left arm
  t = translate(mat4(1.0), vec3( -.75, 0.75, .5));
  s = scale(mat4(1.0), vec3( 0.75, 0.05, 0.05));
  com = t*s;
  SetModel( prog, moveModel*com);
  sphere->draw(prog);

   //eyes
  t = translate(mat4(1.0), vec3( -.35, 1.75, .38));
  s = scale(mat4(1.0), vec3(  0.05, 0.05, 0.05));
  com = t*s;
  SetModel( prog, moveModel*com);
  sphere->draw(prog);

  t = translate(mat4(1.0), vec3( .35, 1.75, .38));
  s = scale(mat4(1.0), vec3( 0.05, 0.05, 0.05));
  com = t*s;
  SetModel( prog, moveModel*com);
  sphere->draw(prog);
}

void initGround() {

  float G_edge = 60;
  GLfloat g_backgnd_data[] = {
    -G_edge, -1.0f, -G_edge,
    -G_edge,  -1.0f, G_edge,
    G_edge, -1.0f, -G_edge,
    -G_edge,  -1.0f, G_edge,
    G_edge, -1.0f, -G_edge,
    G_edge, -1.0f, G_edge,
  };


  GLfloat nor_Buf_G[] = { 
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
  };

  glGenBuffers(1, &posBufObjG);
  glBindBuffer(GL_ARRAY_BUFFER, posBufObjG);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_backgnd_data), g_backgnd_data, GL_STATIC_DRAW);

  glGenBuffers(1, &norBufObjG);
  glBindBuffer(GL_ARRAY_BUFFER, norBufObjG);
  glBufferData(GL_ARRAY_BUFFER, sizeof(nor_Buf_G), nor_Buf_G, GL_STATIC_DRAW);

}

void initGL(const std::string& resourceDirectory) {
  GLSL::checkVersion();
	// Set the background color
	glClearColor(0.6f, 0.6f, 0.8f, 1.0f);
	// Enable Z-buffer test
	glEnable(GL_DEPTH_TEST);

  float tx, tz, s, r;	
  float Wscale = 18.0*(numO/10.0);
  srand(time(NULL));
  //allocate the transforms for the different models
  for (int i=0; i < numO; i++) {
    if(i < 10) {
		 Wscale = 18.0;
	 } else {
  		Wscale = 18.0*(numO/10.0);
	 }
    tx = 0.2 + Wscale*(static_cast <float> (rand()) / static_cast <float> (RAND_MAX))-Wscale/2.0;
    tz = 0.1 + Wscale*(static_cast <float> (rand()) / static_cast <float> (RAND_MAX))-Wscale/2.0;
    r = 6.28*(static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
    g_transB[i] = vec3(tx, 0, tz);
    g_scaleB[i] = 1.0;
    g_rotB[i] = r;
    tx = 0.1 + Wscale*(static_cast <float> (rand()) / static_cast <float> (RAND_MAX))-Wscale/2.0;
    tz = 0.2 + Wscale*(static_cast <float> (rand()) / static_cast <float> (RAND_MAX))-Wscale/2.0;
    r = 6.28*(static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
    g_transS[i] = vec3(tx, 0, tz);
    g_scaleS[i] = 1.0;
    g_rotS[i] = r;
    g_mat_ids[i] = i%4;
    g_ang[i] = 0;
  }

  g_phi = 0;
  g_theta = -3.14/2.0;
    // Initialize the GLSL program to render the obj
  prog = make_shared<Program>();
  prog->setVerbose(true);
  prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
  prog->init();
  prog->addUniform("P");
  prog->addUniform("V");
  prog->addUniform("M");
  prog->addUniform("MatAmb");
  prog->addUniform("MatDif");
  prog->addUniform("MatSpec");
  prog->addUniform("MatShine");
  prog->addUniform("LPos");
  prog->addAttribute("vertPos");
  prog->addAttribute("vertNor");
}

void initGeom(const std::string& resourceDirectory) {

  initGround();

  vector<tinyobj::shape_t> TOshapes;
  vector<tinyobj::material_t> objMaterials;
  string errStr;
    //load in the mesh and make the shape(s)
  bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/Nefertiti-100K.obj").c_str());
  if (!rc) {
    cerr << errStr << endl;
  } else {
    mesh = make_shared<Shape>();
    mesh->createShape(TOshapes[0]);
    mesh->measure();
    mesh->init();
  }

  vector<tinyobj::shape_t> TOshapes2;
  vector<tinyobj::material_t> objMaterials2;
    //load in the mesh and make the shape(s)
  //rc = tinyobj::LoadObj(TOshapes2, objMaterials2, errStr, (resourceDirectory + "/sphere.obj").c_str());
  rc = tinyobj::LoadObj(TOshapes2, objMaterials2, errStr, (resourceDirectory + "/smoothSphere.obj").c_str());
  if (!rc) {
    cerr << errStr << endl;
  } else {
    sphere = make_shared<Shape>();
    sphere->createShape(TOshapes2[0]);
    sphere->measure();
    sphere->init();
  }



}


/* VFC code starts here TODO - start here and fill in these functions!!!*/
vec4 Left, Right, Bottom, Top, Near, Far; // Planes of the VFC
vec4 planes[6];

/*TODO fill in */
void ExtractVFPlanes(mat4 P, mat4 V) {

	/* composite matrix */
	mat4 comp = P*V;
	vec3 n; //use to pull out normal
	float l; //length of normal for plane normalization

	// The rows and columns for indices will be filped here vs what is on the handout, if you transpose the matrix it will be the same indices
	Left.x = comp[0][3] + comp[0][0]; // see handout to fill in with values from comp (index into comp and use the coefficents for plane eq)
	Left.y = comp[1][3] + comp[1][0]; // see handout to fill in with values from comp
	Left.z = comp[2][3] + comp[2][0]; // see handout to fill in with values from comp
	Left.w = comp[3][3] + comp[3][0]; // see handout to fill in with values from comp
	n = vec3(Left.x, Left.y, Left.z);
	l = length(n);
	Left.x /= l;
	Left.y /= l;
	Left.z /= l;
	Left.w /= l;

	planes[0] = Left;
	cout << "Left' " << Left.x << " " << Left.y << " " <<Left.z << " " << Left.w << endl;
  
	Right.x = comp[0][3] - comp[0][0]; // see handout to fill in with values from comp
	Right.y = comp[1][3] - comp[1][0]; // see handout to fill in with values from comp
	Right.z = comp[2][3] - comp[2][0]; // see handout to fill in with values from comp
	Right.w = comp[3][3] - comp[3][0]; // see handout to fill in with values from comp

	//normalize
	n = vec3(Right.x, Right.y, Right.z);
	l = length(n);
	Right.x /= l;
	Right.y /= l;
	Right.z /= l;
	Right.w /= l;

	planes[1] = Right;
	cout << "Right " << Right.x << " " << Right.y << " " <<Right.z << " " << Right.w << endl;

	Bottom.x = comp[0][3] + comp[0][1]; // see handout to fill in with values from comp
	Bottom.y = comp[1][3] + comp[1][1]; // see handout to fill in with values from comp
	Bottom.z = comp[2][3] + comp[2][1]; // see handout to fill in with values from comp
	Bottom.w = comp[3][3] + comp[3][1]; // see handout to fill in with values from comp
	//normalize
	n = vec3(Bottom.x, Bottom.y, Bottom.z);
	l = length(n);
	Bottom.x /= l;
	Bottom.y /= l;
	Bottom.z /= l;
	Bottom.w /= l;
	planes[2] = Bottom;
	cout << "Bottom " << Bottom.x << " " << Bottom.y << " " <<Bottom.z << " " << Bottom.w << endl;
  
	Top.x = comp[0][3] - comp[0][1]; // see handout to fill in with values from comp
	Top.y = comp[1][3] - comp[1][1]; // see handout to fill in with values from comp
	Top.z = comp[2][3] - comp[2][1]; // see handout to fill in with values from comp
	Top.w = comp[3][3] - comp[3][1]; // see handout to fill in with values from comp
	//normalize
	n = vec3(Top.x, Top.y, Top.z);
	l = length(n);
	Top.x /= l;
	Top.y /= l;
	Top.z /= l;
	Top.w /= l;

	planes[3] = Top;
	cout << "Top " << Top.x << " " << Top.y << " " <<Top.z << " " << Top.w << endl;

	Near.x = comp[0][3] + comp[0][2]; // see handout to fill in with values from comp
	Near.y = comp[1][3] + comp[1][2]; // see handout to fill in with values from comp
	Near.z = comp[2][3] + comp[2][2]; // see handout to fill in with values from comp
	Near.w = comp[3][3] + comp[3][2]; // see handout to fill in with values from comp
	//normalize
	n = vec3(Near.x, Near.y, Near.z);
	l = length(n);
	Near.x /= l;
	Near.y /= l;
	Near.z /= l;
	Near.w /= l;
	planes[4] = Near;
	cout << "Near " << Near.x << " " << Near.y << " " <<Near.z << " " << Near.w << endl;

	Far.x = comp[0][3] - comp[0][2]; // see handout to fill in with values from comp
	Far.y = comp[1][3] - comp[1][2]; // see handout to fill in with values from comp
	Far.z = comp[2][3] - comp[2][2]; // see handout to fill in with values from comp
	Far.w = comp[3][3] - comp[3][2]; // see handout to fill in with values from comp
	//normalize
	n = vec3(Far.x, Far.y, Far.z); // Get the values to compute the normal
	l = length(n); // Get the magnitude and use as divisor to get normalized value
	Far.x /= l;
	Far.y /= l;
	Far.z /= l;
	Far.w /= l;
	planes[5] = Far;
	cout << "Far " << Far.x << " " << Far.y << " " <<Far.z << " " << Far.w << endl;


}


/* helper function to compute distance to the plane */
/* TODO: fill in */
float DistToPlane(float A, float B, float C, float D, vec3 point) {
	float outVal = A * point.x + B * point.y + C * point.z + D;

	return outVal;
}

/* Actual cull on planes */
//returns 1 to CULL
//TODO fill in
int ViewFrustCull(vec3 center, float radius) {

  float dist;

  if (CULL) {
    cout << "testing against all planes" << endl;
    for (int i=0; i < 6; i++) {
      dist = DistToPlane(planes[i].x, planes[i].y, planes[i].z, planes[i].w, center);
      //test against each plane
	  if (dist < radius) // Compare to radius of the object
	  {
		  return 1;
	  }
	  else if (dist > radius)
	  {
		  return 0;
	  }


    }
    return 0; 
  } else {
    return 0;
  }
}


/* code to draw the scene */
/* draw all the shapes and possibly the mirror */
void drawScene(shared_ptr<Program> shader, bool cull) {

 for (int i=0; i < numO; i++) {
      //draw the mesh

  if( !cull || !ViewFrustCull(g_transB[i], -1.25)) {
    if (i%2 ==0) { 
      SetMaterial(2);
    } else {
      SetMaterial(3);
    }
    float meshScale = 1.0/(mesh->max.x-mesh->min.x);
    //groundplane at -1
    SetModel(shader, vec3(g_transB[i].x, -0.85, g_transB[i].z), g_rotB[i], -3.14/2.0, vec3(meshScale));
      //draw the mesh	
    mesh->draw(shader);

    //mesh shadow
    SetMaterial(5);
    SetModel(shader, vec3(g_transB[i].x+0.2, g_transB[i].y-1, g_transB[i].z+0.2), g_rotB[i], -3.14/2.0, vec3(meshScale, 0.1*meshScale, meshScale));
    mesh->draw(shader);
  }

  if( !cull || !ViewFrustCull(g_transS[i], -1.5)) {
      /*now draw the snowmen */
    mat4 Trans = glm::translate( glm::mat4(1.0f), vec3(g_transS[i].x, g_transS[i].y+0.4, g_transS[i].z));
    mat4 RotateY = glm::rotate( glm::mat4(1.0f), g_rotS[i], glm::vec3(0.0f, 1, 0));
    mat4 Sc = glm::scale( glm::mat4(1.0f), vec3(g_scaleS[i]));
    mat4 com = Trans*RotateY*Sc;
    drawSnowman(com, i);
  }
}	

SetMaterial(0);
SetModel(shader, vec3(0), 0, 0, vec3(1));
   //draw the ground
glEnableVertexAttribArray(shader->getAttribute("vertPos"));
glBindBuffer(GL_ARRAY_BUFFER, posBufObjG);
glVertexAttribPointer(shader->getAttribute("vertPos"), 3,  GL_FLOAT, GL_FALSE, 0, (void*)0);
glEnableVertexAttribArray(shader->getAttribute("vertNor"));
glBindBuffer(GL_ARRAY_BUFFER, norBufObjG);
glVertexAttribPointer(shader->getAttribute("vertNor"), 3, GL_FLOAT, GL_FALSE, 0, 0);

glDrawArrays(GL_TRIANGLES, 0, 6);

GLSL::disableVertexAttribArray(shader->getAttribute("vertPos"));
GLSL::disableVertexAttribArray(shader->getAttribute("vertNor"));
glBindBuffer(GL_ARRAY_BUFFER, 0);
assert(glGetError() == GL_NO_ERROR);
}

void render() {
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
	glViewport(0, 0, width, height);

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	prog->bind();
	glUniform3f(prog->getUniform("LPos"), g_light.x, g_light.y, g_light.z);
	mat4 P = SetProjectionMatrix(prog);
	mat4 V = SetView(prog);
	ExtractVFPlanes(P, V);
	drawScene(prog, CULL);

	/* draw the complete scene from a top down camera */
	glClear( GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, 300, 300);
	SetOrthoMatrix(prog);
	SetTopView(prog);
	drawScene(prog, false);

	/* draw the culled scene from a top down camera */
	glClear( GL_DEPTH_BUFFER_BIT);
	glViewport(0, height-300, 300, 300);
	SetOrthoMatrix(prog);
	SetTopView(prog);
	drawScene(prog, CULL);

	prog->unbind();

}


void mouseCallback(GLFWwindow *window, int button, int action, int mods) {

	cout << "use two finger mouse scroll" << endl;
}

/* much of the camera is here */
void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
  vec3 diff, newV;
   //cout << "xDel + yDel " << deltaX << " " << deltaY << endl;
  g_theta += deltaX*0.25;
  g_phi += deltaY*0.25;
  newV.x = cosf(g_phi)*cosf(g_theta);
  newV.y = -1.0*sinf(g_phi);
  newV.z = 1.0*cosf(g_phi)*cosf((3.14/2.0-g_theta));
  diff.x = (g_lookAt.x - g_eye.x) - newV.x;
  diff.y = (g_lookAt.y - g_eye.y) - newV.y;
  diff.z = (g_lookAt.z - g_eye.z) - newV.z;
  g_lookAt.x = g_lookAt.x - diff.x;
  g_lookAt.y = g_lookAt.y - diff.y;
  g_lookAt.z = g_lookAt.z - diff.z;
  view = g_eye - g_lookAt;
  strafe = cross(vec3(0, 1,0), view);
}

  void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

 	float speed = 0.2;

 	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
  		g_eye -= speed*strafe;
  		g_lookAt -= speed*strafe;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS) {
  		g_eye += speed*strafe;
  		g_lookAt += speed*strafe;
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
  		g_eye -= speed*view;
  		g_lookAt -= speed*view;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
  		g_eye += speed*view;
  		g_lookAt += speed*view;
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
 		g_light.x += 0.25; 
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
 		g_light.x -= 0.25; 
	if (key == GLFW_KEY_M && action == GLFW_PRESS)
 		g_Camtrans += 0.25; 
	if (key == GLFW_KEY_N && action == GLFW_PRESS)
 		g_Camtrans -= 0.25; 
 	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
  		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
 	}
 	if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
  		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
 	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		CULL = !CULL;
	}
}

  void resizeCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
  }

}; //end of application

int main(int argc, char **argv)
{
// Where the resources are loaded from
 std::string resourceDir = "../resources";

 if (argc >= 2)
 {
  resourceDir = argv[1];
}

Application *application = new Application();

  // Your main will always include a similar set up to establish your window
  // and GL context, etc.

WindowManager *windowManager = new WindowManager();
windowManager->init(1024, 768);
windowManager->setEventCallbacks(application);
application->windowManager = windowManager;

  // This is the code that will likely change program to program as you
  // may need to initialize or set up different data and state

application->initGL(resourceDir);
application->initGeom(resourceDir);

// Loop until the user closes the window.
while (! glfwWindowShouldClose(windowManager->getHandle())) {
    // Render scene.
  application->render();

    // Swap front and back buffers.
  glfwSwapBuffers(windowManager->getHandle());
    // Poll for and process events.
  glfwPollEvents();
}

  // Quit program.
windowManager->shutdown();
return 0;
}
