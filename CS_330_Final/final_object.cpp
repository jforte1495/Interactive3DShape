//header inclusions
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

//SOIL image loader inclusion
#include "SOIL2/SOIL2.h"


//GLM Inclusions
#include <GL/glm/glm.hpp>
#include <GL/glm/gtc/matrix_transform.hpp>
#include <GL/glm/gtc/type_ptr.hpp>

using namespace std; //standard namespace

#define WINDOW_TITLE "Jeffrey Forte Final Project" //Window Title Macro

//Shader program macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

//Variable declaration for shader, window size initialization, buffer & array objects
GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, VAO, texture;
GLfloat degrees = glm::radians(45.0f); 		//converts float to radians


GLfloat cameraSpeed = 0.0005f;				//Movement speed per frame

GLfloat scale_by_y = 2.0f;
GLfloat scale_by_x = 2.0f;
GLfloat scale_by_z = 2.0f;

GLchar currentKey;							//will be store key pressed
int keymod; 		//check for alt key

GLfloat lastMouseX = 400, lastMouseY = 300;		//Locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch =0.0f;		//mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.5f;				//used for mouse/camera rotation sensitivity
bool mouseDetected = false;				//Initially true when mouse movement is detected

bool rotate = false;
bool checkMotion = false;
bool checkZoom = false;

//Global vector Declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);				//Initial camera Position. Placed 5 units in Z
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);					//Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f);				//Temporary z unit vector
glm::vec3 front;			//Temporary z unit vector for mouse


//Object and light color
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 secondLightColor(0.2f, 0.2f, 0.2f);

//Light position and scale
glm::vec3 lightPosition(1.0f, 0.5f, -3.0f);
glm::vec3 lightScale(0.3f);
                      //ambient   specular    highlight
glm::vec3 lightStrength(0.1f,     1.0f,       0.5f);


//camera rotation
float cameraRotation = glm::radians(-45.0f);

//function prototypes
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UGenerateTexture(void);
void UMouseMotion(int x, int y);
void UMouseMove(int x, int y);
void UMouseClicks(int button, int state, int x , int y);
void UKeyboard(unsigned char key, int x, int y);
void UKeyReleased(unsigned char key, int x, int y);

//Vertex Shader source code
const GLchar * vertexShaderSource = GLSL(330,
	layout (location = 0) in vec3 position; //vertex data from vertex attrib pointer 0
	layout (location = 2) in vec2 textureCoordinate;

	out vec2 mobileTextureCoordinate;

	//global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main(){
		gl_Position = projection * view * model * vec4(position, 1.0f); //transforms vertices to clip coordinates
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips the texture horizontal
	}
);

//fragment shader source code
const GLchar * fragmentShaderSource = GLSL(330,
	in vec2 mobileTextureCoordinate;

	out vec4 gpuTexture; //variable to pass color data to the GPU

	uniform sampler2D uTexture; //Useful when working with multiple textures

	void main(){
		gpuTexture = texture(uTexture, mobileTextureCoordinate);
	}
);

const GLchar * lightVertexShaderSource = GLSL(330,
	layout (location = 0) in vec3 position; //VAP position 0 for vertex position data
	layout (location = 1) in vec3 normal; //VAP position 1 for normals
	layout (location = 2) in vec2 textureCoordinate;

	out vec3 Normal; //for outgoing normals to fragment shader
	out vec3 FragmentPos; // for outgoing color / pixels to fragment shader
	out vec2 mobileTextureCoordinate; // uv coords for texture

	//uniform / global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

    void main(){
        gl_Position = projection * view * model * vec4(position, 1.0f);
        Normal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation
        FragmentPos = vec3(model * vec4(position, 1.0f)); //Gets fragment / pixel position in world space only
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips the texture horizontal
	}
);

const GLchar * lightFragmentShaderSource = GLSL(330,
	in vec3 Normal; 								//For incoming normals
	in vec3 FragmentPos; 							//for incoming fragment position
	in vec2 mobileTextureCoordinate;

	out vec4 result; 								//sends light color to the GPU

	//Uniform / Global variables for object color, light color, light position and camera/view position
	uniform vec3 lightColor;
	uniform vec3 secondLightColor;
	uniform vec3 lightPos;
	uniform vec3 viewPosition;
    uniform vec3 lightStrength;
	uniform sampler2D uTexture; 	//Useful when working with multiple textures

    void main(){
    	vec3 norm = normalize(Normal); 								//Normalize vectors to 1 unit
    	vec3 ambient = lightStrength.x * lightColor; 				//Generate ambient light color
    	vec3 ambientTwo = lightStrength.x * secondLightColor;		//Generate second ambient light color
    	vec3 lightDirection = normalize(lightPos - FragmentPos); 	//Calculate distance (light direction) between light source and fragments/pixels on
    	float impact = max(dot(norm, lightDirection), 0.0); 		//Calculate diffuse impact
    	vec3 diffuse = impact * lightColor; 						//Generate diffuse light color
    	vec3 viewDir = normalize(viewPosition - FragmentPos); 		//Calculate view direction
    	vec3 reflectDir = reflect(-lightDirection, norm); 			//Calculate reflection vector
    	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), lightStrength.z);
    	vec3 specular = lightStrength.y * specularComponent * lightColor;

    	//Calculate phong result
    	vec3 phongOne = (ambient + diffuse + specular) * vec3(texture(uTexture, mobileTextureCoordinate));

    	//second light position
    	lightDirection = normalize(vec3(6.0f, 0.0f, -3.0f)- FragmentPos);
    	impact = max(dot(norm, lightDirection), 0.0);						 //Calculate diffuse impact
    	diffuse = impact * secondLightColor; 								 //Generate diffuse light color
    	viewDir = normalize(viewPosition - FragmentPos); 					//Calculate view direction
    	reflectDir = reflect(-lightDirection, norm); 						//Calculate reflection vector
    	specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), lightStrength.z);


    	//second light specular
    	vec3 specularTwo = 0.1f * specularComponent * secondLightColor;

    	vec3 phongTwo = (ambientTwo + diffuse + specularTwo) * vec3(texture(uTexture, mobileTextureCoordinate));

    	result = vec4(phongOne + phongTwo, 1.0f); 							//Send lighting results to GPU
	}
);

//*******MAIN PROGRAM*******
int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);

	glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

	UCreateShader();

	UCreateBuffers();

	UGenerateTexture();

	//Use the Shader Program
	glUseProgram(shaderProgram);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 		//Set background color

	glutDisplayFunc(URenderGraphics);

	glutPassiveMotionFunc(UMouseMove);			//Detects mouse movement

	glutMotionFunc(UMouseMotion);				//tracks mouse motion

	glutMouseFunc(UMouseClicks);				//detects mouse clicks

	glutKeyboardFunc(UKeyboard);							//Detects Key press

	glutKeyboardUpFunc(UKeyReleased);						//Detects Key Released


	glutMainLoop();

	//Destroy buffer objects once used
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}

//Resize the window
void UResizeWindow(int w, int h) {
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

//Renders Graphics
void URenderGraphics(void) {
	glEnable(GL_DEPTH_TEST); 					//Enable z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clears the screen

	glBindVertexArray(VAO); 					//Activate the vertex array object before rendering and transforming them

	//Camera Movement Logic
		if(currentKey == 'w')
			cameraPosition += cameraSpeed * CameraForwardZ;

		if(currentKey == 's')
			cameraPosition -= cameraSpeed * CameraForwardZ;

		if(currentKey == 'a')
			cameraPosition -= glm::normalize(glm::cross(CameraForwardZ, CameraUpY)) * cameraSpeed;

		if(currentKey == 'd')
			cameraPosition += glm::normalize(glm::cross(CameraForwardZ, CameraUpY)) * cameraSpeed;

		CameraForwardZ = front;				//replaces camera forward vector with Radiands normalized as a unit vector
	//Transforms the object
	glm::mat4 model;
	model = glm::scale(model, glm::vec3(scale_by_x, scale_by_y, scale_by_z)); 			//Increase the by a variable scale
	model = glm::rotate(model, degrees, glm::vec3(0.0, 1.0f, 0.0f)); 	//Rotate the object y -45 degrees
	model = glm::translate(model, glm::vec3(0.0, 0.0f, 0.0f)); 			//Place the object at the center of the viewport

	//Transforms the camera
	glm::mat4 view;
	view = glm::lookAt(cameraPosition - CameraForwardZ, cameraPosition, CameraUpY);
	//view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));
	//view = glm::translate(view, cameraPosition);

	//creates a perspective projection
	glm::mat4 projection;
	projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

	//Retrieves and passes transform matrices to the shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint secondLightColorLoc, lightColorLoc, lightPositionLoc, lightStrengthLoc, viewPositionLoc;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
    lightStrengthLoc = glGetUniformLocation(shaderProgram, "lightStrength");
    secondLightColorLoc = glGetUniformLocation(shaderProgram, "secondLightColor");
	viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");

	//pass color, light, and camera data to the cube shader programs corresponding uniforms
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(secondLightColorLoc, secondLightColor.r, secondLightColor.g, secondLightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(lightStrengthLoc, lightStrength.x, lightStrength.y, lightStrength.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glutPostRedisplay();

	glBindTexture(GL_TEXTURE_2D, texture);

	//Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, 300);

	glBindVertexArray(0); 						//Deactivates the vertex array object

	glutSwapBuffers(); 							// Flips the back buffer with the front buffer every frame
}

//Creates the shader program
void UCreateShader() {

	//Vertex shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER); 				// creates the vertex shader
	glShaderSource(vertexShader, 1, &lightVertexShaderSource, NULL); 	//Attaches the vertex shader to the source code
	glCompileShader(vertexShader); 										//compiles the vertex shader

	//Fragment shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); 				//Creates the fragment shader
	glShaderSource(fragmentShader, 1, &lightFragmentShaderSource, NULL); 	//Attaches the fragment shader to the source code
	glCompileShader(fragmentShader); 										//compiles the fragment shader

	//Shader Program
	shaderProgram = glCreateProgram(); 					//Creates the shader program and returns an id
	glAttachShader(shaderProgram, vertexShader); 		//Attach vertex shader to the shader program
	glAttachShader(shaderProgram, fragmentShader); 		//Attach fragment shader to the shader program
	glLinkProgram(shaderProgram); 						//link vertex and fragment shader to shader program

	//delete the vertex and fragment shaders once linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

//Creates the buffer and array objects
void UCreateBuffers() {

	//position and texture coordinate data for 36 triangles
	GLfloat vertices[] ={
										//Positions 		//Normals			//Texture Coordinates

									//FRONT FACE
									 0.0f, 0.80f, 0.0f, 	0.0f,  0.0f,  1.0f,     0.75f, 1.0f,							//Left Top Triangle 1
									-0.20f, 0.75f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.95f,
									 0.0f, 0.75f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.75f, 0.95f,

									 0.0f, 0.80f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.75f, 1.0f,							//Top Right Triangle
								     0.20f, 0.75f, 0.0f,	0.0f,  0.0f,  1.0f,		0.95f, 0.95f,
								     0.0f, 0.75f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.75f, 0.95f,

								     -0.20f, 0.75f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.95f,		//	Top Square
								      0.20f, 0.75f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.95f,
								     -0.25f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.0f, 0.72f,
								     -0.25f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.0f, 0.72f,
								      0.20f, 0.75f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.95f,
								      0.25f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		1.0f, 0.72f,

								      0.25f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		1.0f, 0.72f,		//mid left indent(in)
								      0.20f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.72f,
								      0.20f, 0.45f, 0.0f,   0.0f,  0.0f,  1.0f,		0.95f, 0.67f,

								     -0.25f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.0f,  0.72f,		//mid right indent(in)
								     -0.20f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.72f,
								     -0.20f, 0.45f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.67f,

								     -0.20f, 0.50f, 0.0f,	0.0f,  0.0f,  1.0f,		0.05f, 0.72f,		//Rectangle body
								      0.20f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.72f,
								     -0.20f, 0.0f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.06f,
								      0.20f, 0.50f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.72f,
								     -0.20f, 0.0f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.06f,
								      0.20f, 0.0f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.06f,

								     -0.20f, 0.0f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.05f, 0.06f,		//bottom left indent(out)
								     -0.25f, -0.07f, 0.0f,  0.0f,  0.0f,  1.0f,		0.0f, 0.0f,
								     -0.20f, -0.07f, 0.0f,  0.0f,  0.0f,  1.0f,		0.05f, 0.0f,

								      0.20f, 0.0f, 0.0f, 	0.0f,  0.0f,  1.0f,		0.95f, 0.06f,		//bottom right indent(out)
								      0.25f, -0.07f, 0.0f,  0.0f,  0.0f,  1.0f,		1.0f, 0.0f,
								      0.20f, -0.07f, 0.0f,  0.0f,  0.0f,  1.0f,		0.95f, 0.0f,

								     -0.20f, 0.0f, 0.0f, 	 0.0f,  0.0f,  1.0f,	0.05f, 0.06f,			//	Bottom Rectangle
								      0.20f, -0.07f, 0.0f, 	 0.0f,  0.0f,  1.0f,	0.95f, 0.0f,
								     -0.20f, -0.07f, 0.0f, 	 0.0f,  0.0f,  1.0f,	0.05f, 0.0f,
								      0.20f, 0.0f, 0.0f, 	 0.0f,  0.0f,  1.0f,	0.95f, 0.06f,
							         -0.20f, 0.0f, 0.0f, 	 0.0f,  0.0f,  1.0f,	0.05f, 0.06f,
							          0.20f, -0.07f, 0.0f, 	 0.0f,  0.0f,  1.0f,	0.95f, 0.0f,


							          //BACK FACE
									  0.0f, 0.80f, -0.07f, 	0.0f,  0.0f,  -1.0f,	0.75f, 1.0f,			//Left Top triangle(back)
									 -0.20f, 0.75f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.05f, 0.95f,
									  0.0f, 0.75f, -0.07f, 	0.0f,  0.0f,  -1.0f,	0.75f, 0.95f,

								      0.0f, 0.80f, -0.07f, 	0.0f,  0.0f,  -1.0f,	0.75f, 1.0f,			//Top Right Triangle (back)
								      0.20f, 0.75f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.95f,
								      0.0f, 0.75f, -0.07f, 	0.0f,  0.0f,  -1.0f,	0.75f, 0.95f,

								     -0.20f, 0.75f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.05f, 0.95f,			//	Top Square(back)
								      0.20f, 0.75f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.95f,
								     -0.25f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.0f, 0.72f,
								     -0.25f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.0f, 0.72f,
								      0.20f, 0.75f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.95f,
								      0.25f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	1.0f, 0.72f,

								      0.25f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	1.0f, 0.72f,			//mid left indent(in) (back)
								      0.20f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.72f,
								      0.20f, 0.45f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.67f,

								     -0.25f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.0f, 0.72f,			//mid right indent(in) (back)
								     -0.20f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.05f, 0.72f,
								     -0.20f, 0.45f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.05f, 0.67f,

								     -0.20f, 0.50f, -0.07f,	0.0f,  0.0f,  -1.0f,	0.05f, 0.72f,			//Rectangle body (back)
								      0.20f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.72f,
								     -0.20f, 0.0f, -0.07f,  0.0f,  0.0f,  -1.0f,	0.05f, 0.06f,
								      0.20f, 0.50f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.72f,
								     -0.20f, 0.0f, -0.07f,  0.0f,  0.0f,  -1.0f,	0.05f, 0.06f,
								      0.20f, 0.0f, -0.07f,  0.0f,  0.0f,  -1.0f,	0.95f, 0.06f,

								     -0.20f, 0.0f, -0.07f, 	 0.0f,  0.0f,  -1.0f,	0.05f, 0.06f,			//bottom left indent(out)(back)
								     -0.25f, -0.07f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.0f, 0.0f,
								     -0.20f, -0.07f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.05f, 0.0f,

								      0.20f, 0.0f, -0.07f, 	 0.0f,  0.0f,  -1.0f,	0.95f, 0.06f,			//bottom right indent(out)(back)
								      0.25f, -0.07f, -0.07f, 0.0f,  0.0f,  -1.0f,	1.0f, 0.0f,
								      0.20f, -0.07f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.0f,

								     -0.20f, 0.0f, -0.07f, 	 0.0f,  0.0f,  -1.0f,	0.05f, 0.06f,			//	Bottom Rectangle (back)
								      0.20f, -0.07f, -0.07f,  0.0f,  0.0f,  -1.0f,	0.95f, 0.0f,
								     -0.20f, -0.07f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.05f, 0.0f,
								      0.20f, 0.0f, -0.07f, 	 0.0f,  0.0f,  -1.0f,	0.95f, 0.06f,
								     -0.20f, 0.0f, -0.07f, 	 0.0f,  0.0f,  -1.0f,	0.05f, 0.06f,
								      0.20f, -0.07f, -0.07f, 0.0f,  0.0f,  -1.0f,	0.95f, 0.0f,


								    //LEFT SIDE PROFILE SHAPES(LEFT)
								     -0.20f, 0.75f, 0.0f, 	 -1.0f,  0.0f, 0.0f,	0.0f, 0.95f,			//	Top Square (left profile)
							         -0.20f, 0.75f, -0.07f,  -1.0f,  0.0f, 0.0f,	0.06f, 0.95f,
					                 -0.25f, 0.50f, 0.0f,    -1.0f,  0.0f, 0.0f,	0.0f, 0.72f,
					                 -0.20f, 0.75f, -0.07f,  -1.0f,  0.0f, 0.0f,	0.06f, 0.95f,
								     -0.25f, 0.50f, -0.07f,  -1.0f,  0.0f, 0.0f,	0.06f, 0.72f,
						             -0.25f, 0.50f, 0.0f, 	 -1.0f,  0.0f, 0.0f,	0.0f, 0.72f,

								     -0.25f, 0.50f, 0.0f, 	 -1.0f,  0.0f, 0.0f,	0.0f, 0.72f,		//Left mid indent(left)
							         -0.25f, 0.50f, -0.07f,  -1.0f,  0.0f, 0.0f,	0.06f, 0.72f,
					                 -0.20f, 0.45f, 0.0f, 	 -1.0f,  0.0f, 0.0f,	0.0f, 0.67f,
				        	         -0.25f, 0.50f, -0.07f,  -1.0f,  0.0f, 0.0f,	0.06f, 0.72f,
								     -0.20f, 0.45f, -0.07f,  -1.0f,  0.0f, 0.0f,	0.06f, 0.67f,
								     -0.20f, 0.45f, 0.0f, 	 -1.0f,  0.0f, 0.0f,	0.0f, 0.67f,



								     -0.20f, 0.45f, 0.0f, 	-1.0f,  0.0f, 0.0f,		0.0f, 0.67f,		//body profile (left)
							         -0.20f, 0.45f, -0.07f, -1.0f,  0.0f, 0.0f,		0.06f, 0.67f,
					                 -0.20f, 0.0f, 0.0f, 	-1.0f,  0.0f, 0.0f,		0.0f, 0.06f,
					                 -0.20f, 0.45f, -0.07f, -1.0f,  0.0f, 0.0f,		0.06f, 0.67f,
					                 -0.20f, 0.0f, 0.0f, 	-1.0f,  0.0f, 0.0f,		0.0f, 0.06f,
					                 -0.20f, 0.0f, -0.07f,   -1.0f,  0.0f, 0.0f,	0.06f, 0.06f,

					                 -0.20f, 0.0f, 0.0f, 	 -1.0f,  0.0f, 0.0f,	0.0f, 0.06f,			//bottom left indent(out) (profile)
					                 -0.20f, 0.0f, -0.07f,   -1.0f,  0.0f, 0.0f,	0.06f, 0.06f,
					                 -0.25f, -0.07f, 0.0f,   -1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
					                 -0.25f,-0.07f, 0.0f,    -1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
					                 -0.20f, 0.0f, -0.07f,   -1.0f,  0.0f, 0.0f,	0.06f, 0.06f,
					                 -0.25f, -0.07f, -0.07f, -1.0f,  0.0f, 0.0f,	0.06f, 0.0f,

					                 //RIGHT SIDE PROFILE
								      0.20f, 0.75f, 0.0f, 	1.0f,  0.0f,  0.0f,		0.0f, 0.95f,		//	Top Square (right profile)
							          0.20f, 0.75f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.95f,
					                  0.25f, 0.50f, 0.0f, 	1.0f,  0.0f,  0.0f,		0.0f, 0.72f,
					                  0.20f, 0.75f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.95f,
							          0.25f, 0.50f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.72f,
							          0.25f, 0.50f, 0.0f, 	1.0f,  0.0f,  0.0f,		0.0f, 0.72f,

							          0.25f, 0.50f, 0.0f,  	1.0f,  0.0f,  0.0f,		0.0f, 0.72f,		//Left mid indent(right)
							          0.25f, 0.50f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.72f,
							          0.20f, 0.45f, 0.0f,   1.0f,  0.0f,  0.0f,		0.0f, 0.67f,
							          0.25f, 0.50f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.72f,
							          0.20f, 0.45f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.67f,
							          0.20f, 0.45f, 0.0f,   1.0f,  0.0f,  0.0f,		0.0f, 0.67f,



							          0.20f, 0.45f, 0.0f, 	1.0f,  0.0f,  0.0f,		0.0f, 0.67f,		//body profile (right)
							          0.20f, 0.45f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.67f,
							          0.20f, 0.0f, 0.0f, 	1.0f,  0.0f,  0.0f,		0.0f, 0.06f,
							          0.20f, 0.45f, -0.07f, 1.0f,  0.0f,  0.0f,		0.06f, 0.67f,
							          0.20f, 0.0f, 0.0f, 	1.0f,  0.0f,  0.0f,		0.0f, 0.06f,
							          0.20f, 0.0f, -0.07f,   1.0f,  0.0f,  0.0f,	0.06f, 0.06f,

							          0.20f, 0.0f, 0.0f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.06f,			//bottom right indent(out) (profile)
							          0.20f, 0.0f, -0.07f,   1.0f,  0.0f,  0.0f,	0.06f, 0.06f,
							          0.25f, -0.07f, 0.0f,   1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
							          0.25f,-0.07f, 0.0f,    1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
							          0.20f, 0.0f, -0.07f,   1.0f,  0.0f,  0.0f,	0.06f, 0.06f,
								      0.25f, -0.07f, -0.07f, 1.0f,  0.0f,  0.0f,	0.06f, 0.0f,


								     //TOP FACES
								      0.0f, 0.80f, 0.0f, 	0.0f, 1.0f,  0.0f,      0.75f, 0.0f,	 		//Left Top Face
								      0.0f, 0.80f, -0.07f,  0.0f, 1.0f,  0.0f,		0.75f, 0.06f,
								     -0.20f, 0.75f, 0.0f, 	0.0f, 1.0f,  0.0f,		0.05f, 0.0f,
								     -0.20f, 0.75f, 0.0f, 	0.0f, 1.0f,  0.0f,		0.05f, 0.0f,
								      0.0f, 0.80f, -0.07f,  0.0f, 1.0f,  0.0f,		0.75f, 0.06,
								     -0.20f, 0.75f, -0.07f, 0.0f, 1.0f,  0.0f,		0.05f, 0.06,

								      0.0f, 0.80f, 0.0f, 	0.0f, 1.0f,  0.0f,     	0.75f, 0.0f,		//Right Top Face
								      0.0f, 0.80f, -0.07f,  0.0f, 1.0f,  0.0f,		0.75f, 0.06f,
								      0.20f, 0.75f, 0.0f, 	0.0f, 1.0f,  0.0f,		0.95f, 0.0f,
								      0.20f, 0.75f, 0.0f, 	0.0f, 1.0f,  0.0f,		0.95f, 0.0f,
								      0.0f, 0.80f, -0.07f,  0.0f, 1.0f,  0.0f,		0.75f, 0.06f,
								      0.20f, 0.75f, -0.07f, 0.0f, 1.0f,  0.0f,		0.95f, 0.06f,

							       //BOTTOM FACE (BASE)
								     -0.25f, -0.07f, 0.0f, 	 0.0f, -1.0f,  0.0f,	0.0f, 0.0f,			//BASE FACE
								     -0.25f, -0.07f, -0.07f, 0.0f, -1.0f,  0.0f,	0.0f, 0.06f,
								      0.25f, -0.07f, 0.0f,   0.0f, -1.0f,  0.0f,	1.0f, 0.0f,
								      0.25f, -0.07f, 0.0f, 	 0.0f, -1.0f,  0.0f,	1.0f, 0.0f,
								      0.25f, -0.07f, -0.07f, 0.0f, -1.0f,  0.0f,	1.0f, 0.06f,
								     -0.25f, -0.07f, -0.07f, 0.0f, -1.0f,  0.0f,	0.0f, 0.06f,


					};

	//Generate buffer ids
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	//Activate the VAO before binding and setting VBOs and VAPs
	glBindVertexArray(VAO);

	//Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Copy vertices to VBO

	//set attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0); 					//Enables vertex attribute

	//Set attribute pointer 1 to hold Normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Set attribute pointer 2 to hold Texture coordinate data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);					 //Unbind the VAO
}

//Generate and load the texture
void UGenerateTexture(){
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height;

	unsigned char* image = SOIL_load_image("black_plastic.jpg", &width, &height, 0, SOIL_LOAD_RGB);						//loads texture file

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); 				//Unbind the texture
}


//Implements Mouse Click Functions
void UMouseClicks (int button, int state, int x, int y) {

	keymod = glutGetModifiers();				//checks for modifier(alt, shift, ctrl, etc.)

	checkMotion = false; 				//sets checkMotion to false

	//checks if button is left, if alt state is down
	if(button == GLUT_LEFT_BUTTON && keymod == GLUT_ACTIVE_ALT && state == GLUT_DOWN) {

		//if true then set motion true
		checkMotion = true;

		//set zooming to false
		checkZoom = false;

	}else if(button == GLUT_RIGHT_BUTTON && keymod == GLUT_ACTIVE_ALT && state == GLUT_DOWN) { 							///checks right button

			checkMotion = false;
			checkZoom = true;
		}
}

//Implements UKeyReleased Function
void UKeyReleased(unsigned char key, GLint x, GLint y) {
	cout<<"Key Released!"<<endl;
	currentKey = '0';
}

void UMouseMove(int x, int y){

        front.x = 10.0f * cos(yaw);
        front.y = 10.0f * sin(pitch);
        front.z = sin(yaw) * cos(pitch) * 10.0f;

}

void UMouseMotion(int x_curr, int y_curr) {

	//if left alt and mouse are pressed

	if (checkMotion) {

		//gets the direction mouse was moved
		mouseXOffset = x_curr - lastMouseX;
		mouseYOffset = lastMouseY - y_curr;

		//updates new mouse coordinates
		lastMouseX = x_curr;
		lastMouseY = y_curr;

		//Applies Sensitivity to mouse direction
		mouseXOffset *= sensitivity;
		mouseYOffset *= sensitivity;



		//directs change in yaw to X axis
		if  (yaw!= yaw+mouseXOffset && pitch == pitch+mouseYOffset) {

			yaw += mouseXOffset; 				//increments yaw

		}else if(pitch != pitch+mouseYOffset && yaw == yaw+mouseXOffset) {

			pitch += mouseYOffset; 				//increments y to move vertically
		}

		front.x = 10.0f * cos(yaw);
		front.y = 10.0f * sin(pitch);
		front.z = sin(yaw) * cos(pitch) * 10.0f;

	}

	if(checkZoom) {

		//determine the direction of the y
		if(lastMouseY > y_curr) {

			//increment scale values
				scale_by_y += 0.1f;
				scale_by_x += 0.1f;
				scale_by_z += 0.1f;

				//redisplay
				glutPostRedisplay();
		}else {
			//decrement scale values (zooms in)
			scale_by_y -= 0.1f;
			scale_by_x -= 0.1f;
			scale_by_z -= 0.1f;

			//control zoom size
			if(scale_by_y < 0.2f) {
				 scale_by_y = 0.2f;
			     scale_by_x = 0.2f;
			     scale_by_z = 0.2f;
			}

			glutPostRedisplay();
		}
		//update x and y
		lastMouseY = y_curr;
		lastMouseX = x_curr;
	}
}

//Implements UKeyboard Function
void UKeyboard(unsigned char key, GLint x, GLint y) {
	switch(key){
		case 'w':
			currentKey = key;
			cout<<"You pressed W!"<<endl;
			break;
		case 's':
			currentKey = key;
			cout<<"You pressed S!"<<endl;
			break;
		case 'a':
			currentKey = key;
			cout<<"You pressed A!"<<endl;
			break;
		case 'd':
			currentKey = key;
			cout<<"You pressed D!"<<endl;
			break;

		default:
			cout<<"Press a key!"<<endl;
	}
}
