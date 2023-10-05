
#ifndef SHAPES_H
#define SHAPES_H

#include <glm/glm.hpp>
#include <vector>
#include <array> 
#include <SDL.h>
#include "draw.h"
#include "color.h"

struct Fragment {
    glm::ivec2 position; // X and Y coordinates of the pixel (in screen space)
    Color color;
    float z;
    glm::vec3 original;
    float intensity;
    glm::vec3 normal;
};


struct Uniforms {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewport;
    int index;
};


Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms);
std::vector<std::vector<Vertex>> primitiveAssembly(const std::vector<Vertex>& transformedVertices);
Fragment fragmentShaderStellarSpace(Fragment& fragment);
Fragment moonFragmentShader(Fragment& fragment);
Fragment planetFragmentShader(Fragment& fragment);
Fragment spaceshipFragmentShader(Fragment& fragment);
Fragment gasPlanetV1(Fragment& fragment);
Fragment fragmentShaderMars(Fragment& fragment);
Fragment fragmentShaderSun(Fragment& fragment);
Fragment fragmentShaderRock(Fragment& fragment);
// Declaración de la función rasterize



#endif // SHAPES_H
