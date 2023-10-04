#ifndef DRAW_H
#define DRAW_H

#include <vector>
#include "SDL2/SDL.h"
#include <glm/glm.hpp>
#include "color.h"

const int FRAMEBUFFER_WIDTH = 800; // Ancho del framebuffer
const int FRAMEBUFFER_HEIGHT = 600; 


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 original;
};


#endif
