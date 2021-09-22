#include <stdio.h>
#include "log/log.h"

#include <stdlib.h>
#include "collections/vector.h"
#include "linear_algebra/vec3.h"
#include "linear_algebra/mat4.h"
#include "linear_algebra/utils.h"

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "gl/manager.h"
#include "gl/camera.h"
#include "voxelengine/chunk.h"

#include "tests/testground.h"

#include "voxelengine/data.h"
#include "voxelengine/debug.h"
EngineData* data;
#include "linear_algebra/perlinnoise.h"

#include <SDL_image.h>

bool viewMode = false;
bool firstMouse = true;
bool mouseEnabled = true;
float lastX = 400;
float lastY = 300;
float yaw;
float pitch;
float mouseSpeed = 20.0f;

void processInput(float deltaTime){
	EngineData* data = getEngineData();
	// Camera
	float cameraSpeed = data->camera->cameraSpeed * deltaTime;
	if (glfwGetKey(data->window, GLFW_KEY_W) == GLFW_PRESS)
		data->camera->cameraPos = vec3_add(data->camera->cameraPos, vec3_mul_val(data->camera->cameraFront, cameraSpeed));
	if (glfwGetKey(data->window, GLFW_KEY_S) == GLFW_PRESS)
		data->camera->cameraPos = vec3_sub(data->camera->cameraPos, vec3_mul_val(data->camera->cameraFront, cameraSpeed));
	if (glfwGetKey(data->window, GLFW_KEY_A) == GLFW_PRESS)
		data->camera->cameraPos = vec3_sub(data->camera->cameraPos, vec3_mul_val(vec3_unit(vec3_cross(data->camera->cameraFront, data->camera->cameraUp)), cameraSpeed));
	if (glfwGetKey(data->window, GLFW_KEY_D) == GLFW_PRESS)
		data->camera->cameraPos = vec3_add(data->camera->cameraPos, vec3_mul_val(vec3_unit(vec3_cross(data->camera->cameraFront, data->camera->cameraUp)), cameraSpeed));
	if (glfwGetKey(data->window, GLFW_KEY_SPACE) == GLFW_PRESS)
		data->camera->cameraPos = vec3_add(data->camera->cameraPos, vec3_mul_val(data->camera->cameraUp, cameraSpeed));
	if (glfwGetKey(data->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		data->camera->cameraPos = vec3_sub(data->camera->cameraPos, vec3_mul_val(data->camera->cameraUp, cameraSpeed));
	if (glfwGetKey(data->window, GLFW_KEY_ESCAPE)){
		glfwSetInputMode(data->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		mouseEnabled = false;
	}
	if (glfwGetMouseButton(data->window, GLFW_MOUSE_BUTTON_LEFT)){
		if(mouseEnabled){
			Ray r;
			r.origin = vec3$(data->camera->cameraPos.x,data->camera->cameraPos.y,data->camera->cameraPos.z);
			r.lenght = 10.0f;
			r.dir = vec3$(data->camera->cameraFront.x,data->camera->cameraFront.y,data->camera->cameraFront.z);
			RaycastHit hit;
			if(RayCast(r, &hit)){
				LOG_INFO("HIT voxel x: %f, y: %f, z: %f", hit.point.x, hit.point.y, hit.point.z)
				place_voxel_to_hit(hit, 0);
			}
		}else
		{
			glfwSetInputMode(data->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			mouseEnabled = true;
			glfwSetCursorPos(data->window , data->width/2.0f, data->height/2.0f);
		}
		
	}
	if (glfwGetKey(data->window, GLFW_KEY_P) == GLFW_PRESS){
		if (viewMode) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			viewMode = !viewMode;
		}
		else{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			viewMode = !viewMode;
		}
	}
	// Cursor Pos
	if(mouseEnabled){
		double xpos, ypos;
		glfwGetCursorPos(data->window, &xpos, &ypos);

		yaw   += mouseSpeed * deltaTime * (float)( data->width/2 - xpos );
		pitch += mouseSpeed * deltaTime * (float)( data->height/2 - ypos );

		if(pitch > 89.0f)
			pitch =  89.0f;
		if(pitch < -89.0f)
			pitch = -89.0f;

		Vec3 front = vec3$(cos(to_radians(pitch)) * sin(to_radians(yaw)), sin(to_radians(pitch)), cos(to_radians(pitch)) * cos(to_radians(yaw)));
		data->camera->cameraFront = vec3_unit(front);

		// Set cursor middle of the screen	
		glfwSetCursorPos(data->window, data->width/2.0f, data->height/2.0f);
	}
}

void drawLoop(){
	EngineData* data = getEngineData();

	float deltaTime = 0.0f; // Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame

	int viewLoc = glGetUniformLocation(data->shaderProgram, "view");
	int projectionLoc = glGetUniformLocation(data->shaderProgram, "projection");
	int lightPos = glGetUniformLocation(data->shaderProgram, "skyLightDir");
	while (!glfwWindowShouldClose(data->window)){
		Vec3 light = vec3$(-0.2f, -1.0f, -0.3f);
        glClearColor(0.0353f, 0.6941f, 0.9254f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		glUseProgram(data->shaderProgram);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//ProcessInput
  		processInput(deltaTime);
		

		// Le cube
        Mat4 view = mat4_lookAt(data->camera->cameraPos, vec3_sub(data->camera->cameraPos, data->camera->cameraFront), data->camera->cameraUp);
		Mat4 projection;
		projection = mat4_perspective(to_radians(45.0f), (float)data->width / (float)data->height, 0.1f, 10000.0f);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data);
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection.data);
		glUniform3f(lightPos, light.x, light.y, light.z);

		tests(data->window);
		pthread_rwlock_rdlock(&data->chunkM->chunkslock);
		for (struct chunk_list* ch = data->chunkM->chunks; ch; ch = ch->next)
		{
			if(ch->chunk->is_air)
				continue;
			drawChunk(ch->chunk);
		}
		pthread_rwlock_unlock(&data->chunkM->chunkslock);
		drawDebug(deltaTime, view, projection);
		updateChunks(data->camera->cameraPos);

		glfwSwapBuffers(data->window);
		glfwPollEvents();
	}
}

int main() {
    data = malloc(sizeof(EngineData));
    data->atlas = malloc(sizeof(struct Atlas));
    data->atlas->next = NULL;
	data->width = 1280;
	data->height = 720;
    glinit();
    data->camera = initCamera(vec3$(5.0f, 5.0f,  5.0f), vec3$(0.0f, 0.0f, -1.0f), vec3$(0.0f, 1.0f,  0.0f));
	data->shaderProgram = bindShader();
    
    // Texture
    unsigned int atlasnb = createAtlas("res/terrain.png", 16, 16, 0, 0, 3);
    setVoxelTileByIndex(atlasnb, 1, 3, FACE_SIDES);
    setVoxelTileByIndex(atlasnb, 1, 191, FACE_UP);
    setVoxelTileByIndex(atlasnb, 1, 2, FACE_DOWN);
    setVoxelTileByIndex(atlasnb, 2, 2, FACE_ALL);
    setVoxelTileByIndex(atlasnb, 3, 1, FACE_ALL);

    data->chunkM = initChunkManager();

    initDebug();
	drawLoop();


    glDeleteTextures(1, &data->atlas->next->texture->m_texture);

	LOG_PANIC("End of program.");
	glend();
	return 0;
}