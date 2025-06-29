#include "editor.h"

void Editor::mouse_button_callback(GLFWwindow* glfw_window, int button, int action, int mods) {
	printf("editor mouse button callback!\n");
}

void Editor::mouse_callback(GLFWwindow* glfw_window, double xpos, double ypos) {
	printf("editor mouse callback!\n");
}

void Editor::scroll_callback(GLFWwindow* glfw_window, double xoffset, double yoffset) {
	printf("editor scroll callback!\n");
}

void Editor::key_callback(GLFWwindow* glfw_window, int key, int scancode, int action, int mods) {
	printf("editor key callback!\n");
}

void Editor::char_callback(GLFWwindow* glfw_window, uint32_t key) {
	printf("editor char callback!\n");
}