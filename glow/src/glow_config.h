#pragma once

#define BINDLESS 1	 		// bindless textures, if disabled glMultiDraw*Indirect replaced with for(cmd) glDraw*InstancedBaseVertexBaseInstance(cmd)
//#define IMGUI 1			//
//#define PHYSICS 1			// 
#define GPU_ANIMATION 0	// compute skeletal transforms on gpu
#define DEBUG_SKELETON 1 // if using gpu animation then we have buffers needed to draw skeleton on gpu, if not doing gpu animation and want to visualize bones then need to set this
//#define FMOD                        // with cmake
// #define USE_FMOD 0
// define max draw commands?

//#define PSS_CSM 1
