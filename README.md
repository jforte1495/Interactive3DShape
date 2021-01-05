# Interactive3DShape
A demonstration of OpenGL combined with C++ functions to create an interactive environment for 3-D Object building

Dependencies: 
This project requires the following DLL's 
  opengl 
  freeglut 
  glu32 
  glew32 
  
Follow the instructions in this link to get started with this: https://www3.ntu.edu.sg/home/ehchua/programming/opengl/HowTo_OpenGL_C.html

This project also makes use of Simple OpenGL Image Loader(SOIL) for image texturing. This has been included in the project

This is an interactive C++ program that demonstrates the creation of an environment where one can develop and work on building 3-D objects in OpenGL

To use, follow these instructions:

  To ZOOM in/out on the object: Press LFT-ALT + RGHT MOUSE BUTTON + Move mouse forward/back

  To ROTATE the object: Press LFT-ALT + LFT MOUSE BUTTON + Move mouse left/right OR up/down
          the Vertical/Horizontal rotation(PITCH) is limited to 90 degrees in either direction.
          
  
  To MOVE the object within the window: Utilize WASD keys respectively.
  
  
  
  
  Bugs:
  After moving the object using WASD keys, the object is not re-centered respectively to the camera view. This will cause any rotation to rotate with respect to the ACTUAL center 
  axes and NOT the object itself.
