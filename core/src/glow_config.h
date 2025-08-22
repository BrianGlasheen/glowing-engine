#pragma once

#define BINDLESS 1			// bindless textures, if disabled glMultiDraw*Indirect replaced with for(cmd) glDraw*InstancedBaseVertexBaseInstance(cmd)
//#define IMGUI 1			//
//#define PHYSICS 1			// 
//#define GPU_ANIMATION 1	// compute skeletal transforms on gpu