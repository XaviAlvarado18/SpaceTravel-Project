#include "SDL2/SDL.h"
#include <ctime>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "color.h"
#include <vector>
#include "draw.h"
#include "shapes.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "FatNoiseLite.h"
#include <cmath>
#include <map>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
float T = 0.5f;

Color clearColor = {0, 0, 0, 255};
Color currentColor = {255, 255, 255, 255};
Color colorA(253, 221, 202, 255); 
Color colorB(0, 255, 255, 255); 
Color colorC(0, 24, 255, 255); 
glm::vec3 L = glm::vec3(0, 0, 200.0f); 

Uniforms uniforms;
//Uniforms uniforms2;
SDL_Renderer* renderer;

std::array<double, WINDOW_WIDTH * WINDOW_HEIGHT> zbuffer;

struct Face {
    std::array<int, 3> vertexIndices;
    std::array<int, 3> normalIndices;
};

Fragment fragmentShaderSun(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Define los colores base de los aros de lava
    Color lavaColor1 = Color(255, 255, 0); // Rojo para el primer anillo
    Color lavaColor2 = Color(64, 11, 11); // Rojo oscuro-anaranjado para el segundo anillo

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite:: NoiseType_Cellular);
    noise.SetSeed(1337);
    noise.SetFrequency(0.010f);
    noise.SetFractalType(FastNoiseLite:: FractalType_PingPong);
    noise.SetFractalOctaves(4);
    noise.SetFractalLacunarity(2 + T);
    noise.SetFractalGain(0.90f);
    noise.SetFractalWeightedStrength(0.70f);
    noise.SetFractalPingPongStrength(3 );
    noise.SetCellularDistanceFunction(FastNoiseLite:: CellularDistanceFunction_Euclidean);
    noise.SetCellularReturnType(FastNoiseLite:: CellularReturnType_Distance2Add);
    noise.SetCellularJitter(1);

    float ox = 3000.0f;
    float oy =3000.0f;
    float zoom = 5000.0f;

    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom));

    Color tmpColor = (noiseValue < 0.1f) ? lavaColor1 : lavaColor2;


    fragment.color = tmpColor * fragment.z;
    return fragment;
}


Color interpolateColor(const glm::vec3& barycentricCoord, const Color& colorA, const Color& colorB, const Color& colorC) {
    float u = barycentricCoord.x;
    float v = barycentricCoord.y;
    float w = barycentricCoord.z;

    // Realiza una interpolación lineal para cada componente del color
    uint8_t r = static_cast<uint8_t>(u * colorA.r + v * colorB.r + w * colorC.r);
    uint8_t g = static_cast<uint8_t>(u * colorA.g + v * colorB.g + w * colorC.g);
    uint8_t b = static_cast<uint8_t>(u * colorA.b + v * colorB.b + w * colorC.b);
    uint8_t a = static_cast<uint8_t>(u * colorA.a + v * colorB.a + w * colorC.a);

    return Color(r, g, b, a);
}

bool isBarycentricCoordInsideTriangle(const glm::vec3& barycentricCoord) {
    return barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 &&
           barycentricCoord.x <= 1 && barycentricCoord.y <= 1 && barycentricCoord.z <= 1 &&
           glm::abs(1 - (barycentricCoord.x + barycentricCoord.y + barycentricCoord.z)) < 0.00001f;
}

glm::vec3 calculateBarycentricCoord(const glm::vec2& A, const glm::vec2& B, const glm::vec2& C, const glm::vec2& P) {
    float denominator = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    float u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denominator;
    float v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denominator;
    float w = 1 - u - v;
    return glm::vec3(u, v, w);
}



void render(const std::vector<Vertex>& vertexArray,  const Uniforms& uniforms) {
    std::vector<Vertex> transformedVertexArray;
    for (const auto& vertex : vertexArray) {
        auto transformedVertex = vertexShader(vertex, uniforms);
        transformedVertexArray.push_back(transformedVertex);
    }
    

    std::fill(zbuffer.begin(), zbuffer.end(), std::numeric_limits<double>::max());
    Fragment processedFragment;
    

    for (size_t i = 0; i < transformedVertexArray.size(); i += 3) {
        const Vertex& a = transformedVertexArray[i];
        const Vertex& b = transformedVertexArray[i + 1];
        const Vertex& c = transformedVertexArray[i + 2];
        

        glm::vec3 A = a.position;
        glm::vec3 B = b.position;
        glm::vec3 C = c.position;

        int minX = static_cast<int>(std::min({A.x, B.x, C.x}));
        int minY = static_cast<int>(std::min({A.y, B.y, C.y}));
        int maxX = static_cast<int>(std::max({A.x, B.x, C.x}));
        int maxY = static_cast<int>(std::max({A.y, B.y, C.y}));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                glm::vec2 pixelPosition2(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel

                
                glm::vec3 barycentricCoord = calculateBarycentricCoord(A, B, C, pixelPosition2);
                glm::vec3 interpolatedOriginal = barycentricCoord.x * a.original + barycentricCoord.y * b.original + barycentricCoord.z * c.original;
               

                if (isBarycentricCoordInsideTriangle(barycentricCoord)) {
                    Color g {200,0,0};
                    Color interpolatedColor = interpolateColor(barycentricCoord, g, g, g);

                    float depth = barycentricCoord.x * A.z + barycentricCoord.y * B.z + barycentricCoord.z * C.z;


                    glm::vec3 normal = a.normal * barycentricCoord.x + b.normal * barycentricCoord.y+ c.normal * barycentricCoord.z;

                    float fragmentIntensity = glm::dot(normal, glm::vec3 (0,0,1.0f));

                    Color finalColor = interpolatedColor * fragmentIntensity;
                    

                    Fragment fragment;
                    fragment.position = glm::ivec2(x, y);
                    fragment.color = finalColor;
                    fragment.z = depth;  // Set the depth of the fragment
                    fragment.original = interpolatedOriginal;
                    fragment.intensity = fragmentIntensity;

                    int index = y * WINDOW_WIDTH + x;
                    if (depth < zbuffer[index]) {
                        //Color fragmentShaderf = fragmentShader(fragment);
                        
                        switch(uniforms.index){
                            case 1: 
                                processedFragment = fragmentShaderStellarSpace(fragment); // Procesar el fragmento
                                break;
                            
                            case 2:
                                processedFragment = fragmentShaderSun(fragment); // Procesar el fragmento
                                break;

                            case 3:
                                processedFragment = planetFragmentShader(fragment);
                                break;

                            case 4:
                                processedFragment = spaceshipFragmentShader(fragment);
                                break;

                            case 5:
                                processedFragment = gasPlanetV1(fragment);
                                break;

                            case 6:
                                processedFragment = fragmentShaderMars(fragment);
                                break;
                        }
                        
                        SDL_SetRenderDrawColor(renderer, processedFragment.color.r, processedFragment.color.g, processedFragment.color.b, processedFragment.color.a);
                        //SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                        SDL_RenderDrawPoint(renderer, x, y);

                        // Update the z-buffer value for this pixel
                        zbuffer[index] = depth;
                    }
                }
            }
        }
    }
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

float a = 3.14f / 3.0f;

//STELLAR SPACE
glm::mat4 createModelMatrixStellarSpace() {
    glm::mat4 transtation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 4.7f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    float fixedAngle = 0.0f;  // Establece el ángulo a un valor fijo (en radianes)
    glm::mat4 rotation = glm::rotate(glm::mat4(1), fixedAngle, glm::vec3(0, 1.0f, 0.0f));
    return transtation * scale * rotation;
}

glm::mat4 createModelMatrix() {
    glm::mat4 transtation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((a++)), glm::vec3(0, 1.0f, 0.0f));
    return transtation * scale * rotation;
}

glm::mat4 createModelMatrixGasPlanet() {
    glm::mat4 transtation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, -15.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((a++)), glm::vec3(0, 1.0f, 0.0f));
    return transtation * scale * rotation;
}

glm::mat4 createModelMatrixMarsPlanet() {
    glm::mat4 transtation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 15.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((a++)), glm::vec3(0, 1.0f, 0.0f));
    return transtation * scale * rotation;
}


std::vector<Vertex> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<Face>& faces)
{
    std::vector<Vertex> vertexArray;

    float scale = 1.0f;

    for (const auto& face : faces)
    {
        glm::vec3 vertexPosition1 = vertices[face.vertexIndices[0]];
        glm::vec3 vertexPosition2 = vertices[face.vertexIndices[1]];
        glm::vec3 vertexPosition3 = vertices[face.vertexIndices[2]];

        glm::vec3 normalPosition1 = normals[face.normalIndices[0]];
        glm::vec3 normalPosition2 = normals[face.normalIndices[1]];
        glm::vec3 normalPosition3 = normals[face.normalIndices[2]];

        vertexArray.push_back(Vertex {vertexPosition1, normalPosition1});
        vertexArray.push_back(Vertex {vertexPosition2, normalPosition2});
        vertexArray.push_back(Vertex {vertexPosition3, normalPosition3});


    }

    return vertexArray;
}

bool loadOBJ(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_normals, std::vector<Face>& out_faces)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "Failed to open the file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string lineHeader;
        iss >> lineHeader;

        if (lineHeader == "v")
        {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            out_vertices.push_back(vertex);
        }
        else if (lineHeader == "vn")
        {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            out_normals.push_back(normal);
        }
        else if (lineHeader == "f")
        {
            Face face;
            for (int i = 0; i < 3; ++i)
            {
                std::string faceData;
                iss >> faceData;

                std::replace(faceData.begin(), faceData.end(), '/', ' ');

                std::istringstream faceDataIss(faceData);
                int temp; // for discarding texture indices
                faceDataIss >> face.vertexIndices[i] >> temp >> face.normalIndices[i];

                face.vertexIndices[i]--;
                face.normalIndices[i]--;
            }
            out_faces.push_back(face);
        }
    }

    return true;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    SDL_Init(SDL_INIT_EVERYTHING);


    Uniforms uniforms;
    Uniforms uniforms2; //SUN
    Uniforms uniforms3; //Earth
    Uniforms uniforms4; //Starship
    Uniforms uniforms5; //GasPlanet
    Uniforms uniforms6; //RockPlanet

    glm::vec3 translation(0.0f, 0.0f, 0.0f); 
    glm::vec3 rotationAngles(0.0f, 0.0f, 0.0f); 
    glm::vec3 scale(1.0f, 1.0f, 1.0f);


    glm::vec3 cameraPosition(0.0f, 0.0f, 6.33189f); 
    glm::vec3 targetPosition(0.0f, 0.0f, 0.0f);   
    glm::vec3 upVector(0.0f, 1.0f, 0.0f);

    uniforms.view = glm::lookAt(cameraPosition, targetPosition, upVector);
    uniforms.index = 1;
    uniforms2.view = glm::lookAt(cameraPosition, targetPosition, upVector);
    uniforms2.index = 2;
    uniforms3.view = glm::lookAt(cameraPosition, targetPosition, upVector);
    uniforms3.index = 3;
    uniforms4.view = glm::lookAt(cameraPosition, targetPosition, upVector);
    uniforms4.index = 4;
    uniforms5.view = glm::lookAt(cameraPosition, targetPosition, upVector);
    uniforms5.index = 5;
    uniforms6.view = glm::lookAt(cameraPosition, targetPosition, upVector);
    uniforms6.index = 6;

    srand(time(nullptr));

    SDL_Window* window = SDL_CreateWindow("Planetario", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    //STELLAR STARS
    std::vector<glm::vec3> verticesStellar;
    std::vector<glm::vec3> normalStellar;
    std::vector<Face> facesStellar;

    //SUN
    std::vector<glm::vec3> verticesK;
    std::vector<glm::vec3> normalK;
    std::vector<Face> facesK;

    //Moon
    std::vector<glm::vec3> verticesMoon;
    std::vector<glm::vec3> normalMoon;
    std::vector<Face> facesMoon;

    //PLANET
    std::vector<glm::vec3> verticesEarth;
    std::vector<glm::vec3> normalEarth;
    std::vector<Face> facesEarth;

    //STARSHIP
    std::vector<glm::vec3> verticesShip;
    std::vector<glm::vec3> normalShip;
    std::vector<Face> facesShip;
    
    //MARS
    std::vector<glm::vec3> verticesMars;
    std::vector<glm::vec3> normalMars;
    std::vector<Face> facesMars;
    


    //LOAD SUN
    bool success2 = loadOBJ("../sphere.obj", verticesK, normalK, facesK);
    if (!success2) {
        return 1;
    }

    //PLANET
    bool success3 = loadOBJ("../sphere.obj", verticesEarth, normalEarth, facesEarth);
    if(!success3){
        return 1;
    }

    //STARS
    bool success1 = loadOBJ("../sphere.obj", verticesStellar, normalStellar, facesStellar);
    if(!success1){
        return 1;
    }

    //STARSHIP
    bool success4 = loadOBJ("../navecita.obj", verticesShip, normalShip, facesShip);
    if(!success4){
        return 1;
    }

    //MOON
    bool success5 = loadOBJ("../sphere.obj", verticesMoon, normalMoon, facesMoon);
    if(!success5){
        return 1;
    }

    //MARS
    bool success6 = loadOBJ("../sphere.obj", verticesMars, normalMars, facesMars);
    if(!success6){
        return 1;
    }



    glm::mat4 additionalRotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));


    for (glm::vec3& vertex : verticesStellar) {
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    for (glm::vec3& vertex : verticesK) {
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    for (glm::vec3& vertex : verticesEarth){
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    for (glm::vec3& vertex : verticesShip){
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    for (glm::vec3& vertex : verticesMoon){
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    for (glm::vec3& vertex : verticesMars){
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    //STARS
    std::vector<Vertex> vertexArrayStars = setupVertexArray(verticesStellar, normalStellar, facesStellar);

    //SUN
    std::vector<Vertex> vertexArrayWolf = setupVertexArray(verticesK, normalK, facesK);

    //Earth
    std::vector<Vertex> vertexArrayEarth= setupVertexArray(verticesEarth, normalEarth, facesEarth);

    //STARSHIP
    std::vector<Vertex> vertexArrayShip = setupVertexArray(verticesShip, normalShip, facesShip);

    //MOON
    std::vector<Vertex> vertexArrayMoon= setupVertexArray(verticesMoon, normalMoon, facesMoon);

    //MARS
    std::vector<Vertex> vertexArrayMars = setupVertexArray(verticesMars, normalMars, facesMars);


    bool running = true;
    SDL_Event event;
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 traslateMatrix = glm::mat4(1.0f); 
    glm::mat4 sunTranslationMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 2.0f));
    glm::mat4 translationMatrixEarth = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, -18.0f));  // El número 5.0f es solo un ejemplo.
    glm::mat4 translationSpaceShip = glm::translate(glm::mat4(1), glm::vec3(0.0f, 140.0f, 560.0f));
    
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f, 4.0f, 4.0f));
    glm::mat4 scaleMatrixSun = glm::scale(glm::mat4(1.0f), glm::vec3(0.08f, 0.08f, 0.08f));
    glm::mat4 scaleMatrixMoon = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
    glm::mat4 scaleMatrixEarth = glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.04f, 0.04f));
    glm::mat4 scaleMatrixAux= glm::scale(glm::mat4(1.0f), glm::vec3(0.006f, 0.006f, 0.006f));
    glm::mat4 scaleMatrixMars = glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.04f, 0.04f));


    float rotationAngle = 0.0f;

    
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    glm::vec3 previousCameraPosition = cameraPosition;
    glm::vec3 currentTranslation = glm::vec3(translationSpaceShip[3]);


    while (running) {

    
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            
            switch(event.type) {
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym){
                        case SDLK_w:
                            cameraPosition += glm::vec3(0.0f, 0.0f, -0.1f);
                            currentTranslation = glm::vec3(translationSpaceShip[3]);
                            // Mover la nave espacial hacia adelante (ajusta la cantidad según tus necesidades)
                            currentTranslation += glm::vec3(0.0f, 0.0f, -17.0f);
                            // Actualizar la parte de traslación de translationSpaceShip
                            translationSpaceShip = glm::translate(glm::mat4(1.0f), currentTranslation);                
                            break;  

                        case SDLK_s:
                            cameraPosition += glm::vec3(0.0f, 0.0f, 0.1f);
                            currentTranslation = glm::vec3(translationSpaceShip[3]);
                            // Mover la nave espacial hacia adelante (ajusta la cantidad según tus necesidades)
                            currentTranslation += glm::vec3(0.0f, 0.0f, 17.0f);
                            // Actualizar la parte de traslación de translationSpaceShip
                            translationSpaceShip = glm::translate(glm::mat4(1.0f), currentTranslation);                        
                            break;

                        case SDLK_a:
                            cameraPosition += glm::vec3(-0.1f, 0.0f, 0.0f);
                            currentTranslation = glm::vec3(translationSpaceShip[3]);
                            // Mover la nave espacial hacia adelante (ajusta la cantidad según tus necesidades)
                            currentTranslation += glm::vec3(-1.0f, 0.0f, 0.0f);
                            // Actualizar la parte de traslación de translationSpaceShip
                            translationSpaceShip = glm::translate(glm::mat4(1.0f), currentTranslation);  
                            break;

                        case SDLK_d:
                            cameraPosition += glm::vec3(0.1f, 0.0f, 0.0f);
                            currentTranslation = glm::vec3(translationSpaceShip[3]);
                            // Mover la nave espacial hacia adelante (ajusta la cantidad según tus necesidades)
                            currentTranslation += glm::vec3(1.0f, 0.0f, 0.0f);
                            // Actualizar la parte de traslación de translationSpaceShip
                            translationSpaceShip = glm::translate(glm::mat4(1.0f), currentTranslation);  
                            break;
                    }
                    break;
                case SDL_QUIT:
                    running = false;
                    break;
            }
             uniforms.view = glm::lookAt(cameraPosition, targetPosition, upVector);
             uniforms2.view = glm::lookAt(cameraPosition, targetPosition, upVector);
             uniforms3.view = glm::lookAt(cameraPosition, targetPosition, upVector);
             uniforms4.view = glm::lookAt(cameraPosition, targetPosition, upVector);
             uniforms5.view = glm::lookAt(cameraPosition, targetPosition, upVector);
             uniforms6.view = glm::lookAt(cameraPosition, targetPosition, upVector);
            
        }

        float orbitRadius = 4.0f;  // You can adjust as needed
        float orbitInclination = 0.0f;  // inclination angle in degrees
        float xOrbitOffset = orbitRadius * cos(glm::radians(rotationAngle));
        float yOrbitOffset = orbitRadius * sin(glm::radians(rotationAngle)) * sin(glm::radians(orbitInclination));
        float zOrbitOffset = orbitRadius * sin(glm::radians(rotationAngle)) * cos(glm::radians(orbitInclination));

        float orbitRadius2 = 8.0f;  // You can adjust as needed
        float xOrbitOffset2 = orbitRadius2 * cos(glm::radians(rotationAngle));
        float yOrbitOffset2 = orbitRadius2 * sin(glm::radians(rotationAngle)) * sin(glm::radians(orbitInclination));
        float zOrbitOffset2 = orbitRadius2 * sin(glm::radians(rotationAngle)) * cos(glm::radians(orbitInclination));


        float orbitRadius3 = 12.0f;  // You can adjust as needed
        float xOrbitOffset3 = orbitRadius3 * cos(glm::radians(rotationAngle));
        float yOrbitOffset3 = orbitRadius3 * sin(glm::radians(rotationAngle)) * sin(glm::radians(orbitInclination));
        float zOrbitOffset3 = orbitRadius3 * sin(glm::radians(rotationAngle)) * cos(glm::radians(orbitInclination));


        glm::mat4 earthOrbitTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(xOrbitOffset, yOrbitOffset, zOrbitOffset));
        glm::mat4 marsOrbitTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(xOrbitOffset2, yOrbitOffset2, zOrbitOffset2));
        glm::mat4 gasOrbitTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(xOrbitOffset3, yOrbitOffset3, zOrbitOffset3));
        


        rotationAngle += 1.0f; // Puedes ajustar la velocidad de rotación cambiando el valor 1.0f.

        // Extraer la parte de traslación de translationSpaceShip
        //glm::vec3 translationSpaceShipTranslation = glm::vec3(translationSpaceShip[3]);

        // Calcular la nueva posición de la nave espacial
        //glm::vec3 newSpaceShipPosition = translationSpaceShipTranslation + (cameraPosition + translation) - previousCameraPosition;


        //translationSpaceShip = glm::translate(glm::mat4(1.0f), newSpaceShipPosition);


        uniforms.model = scaleMatrix * createModelMatrixStellarSpace() * rotationMatrix;    
        uniforms.projection = createProjectionMatrix();
        uniforms.viewport = createViewportMatrix();

        uniforms2.model = scaleMatrixSun  * createModelMatrix();    
        uniforms2.projection = createProjectionMatrix();
        uniforms2.viewport = createViewportMatrix();

        uniforms3.model = scaleMatrixEarth * earthOrbitTranslate * createModelMatrix();
        uniforms3.projection = createProjectionMatrix();
        uniforms3.viewport = createViewportMatrix();

        uniforms4.model = scaleMatrixAux  * createModelMatrixStellarSpace() * translationSpaceShip;
        uniforms4.projection = createProjectionMatrix();
        uniforms4.viewport = createViewportMatrix();

        uniforms5.model = scaleMatrixMoon * gasOrbitTranslate * createModelMatrixGasPlanet();
        uniforms5.projection = createProjectionMatrix();
        uniforms5.viewport = createViewportMatrix();

        uniforms6.model = scaleMatrixMars * marsOrbitTranslate * createModelMatrixMarsPlanet();
        uniforms6.projection = createProjectionMatrix();
        uniforms6.viewport = createViewportMatrix();


        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        SDL_RenderClear(renderer);

        glm::vec4 transformedLight = glm::inverse(createModelMatrixStellarSpace()) * glm::vec4(L, 0.0f);
        glm::vec3 transformedLightDirection = glm::normalize(glm::vec3(transformedLight));

        glm::vec4 camPositionEarth = uniforms3.model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 camPositionSun = uniforms2.model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 camPositionMoon = uniforms5.model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 camPositionMars = uniforms6.model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        if(camPositionMoon.z > camPositionEarth.z > camPositionSun.z > camPositionMars.z)
        {
            // La tierra está más cerca de la cámara, renderiza primero el sol:
            //render(vertexArrayStars,uniforms);
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);  //Nave
            render(vertexArrayMars, uniforms6);  //MARS
            render(vertexArrayWolf, uniforms2);  // Sun
            render(vertexArrayEarth, uniforms3);  // Earth
            render(vertexArrayMoon, uniforms5);  //Moon
        }
        else if(camPositionMoon.z  > camPositionSun.z > camPositionEarth.z){
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);  //Nave
            render(vertexArrayEarth, uniforms3);  // Earth
            render(vertexArrayMoon, uniforms5);  //Moon
            render(vertexArrayWolf, uniforms2);  // Sun
            render(vertexArrayMars, uniforms6);
            
        }
        else if(camPositionEarth.z > camPositionMoon.z > camPositionSun.z)
        {
            // El sol está más cerca de la cámara, renderiza primero la tierra:
            //render(vertexArrayStars,uniforms);
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);   //Nave
            render(vertexArrayWolf, uniforms2);  // Sun
            render(vertexArrayEarth, uniforms3);  // Earth
            render(vertexArrayMoon, uniforms5); //Moon
            render(vertexArrayMars, uniforms6);
        }
        else if(camPositionEarth.z  > camPositionSun.z > camPositionMoon.z){
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);   //Nave
            render(vertexArrayMoon, uniforms5); //Moon
            render(vertexArrayWolf, uniforms2);  // Sun
            render(vertexArrayEarth, uniforms3);  // Earth
            render(vertexArrayMars, uniforms6);
        }
        else if(camPositionSun.z > camPositionMoon.z > camPositionEarth.z) {
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);   //Nave
            
            render(vertexArrayEarth, uniforms3);  // Earth
            render(vertexArrayMoon, uniforms5); //Moon
            render(vertexArrayWolf, uniforms2);  // Sun
            render(vertexArrayMars, uniforms6);
        }else if(camPositionSun.z > camPositionEarth.z > camPositionMoon.z ){
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);   //Nave        
            render(vertexArrayMoon, uniforms5); //Moon
            render(vertexArrayEarth, uniforms3);  // Earth
            render(vertexArrayWolf, uniforms2);  // Sun
            render(vertexArrayMars, uniforms6);
        }else if(camPositionSun.z > camPositionEarth.z > camPositionMars.z > camPositionMoon.z ){
            render(vertexArrayStars, uniforms);
            render(vertexArrayShip, uniforms4);   //Nave   
            render(vertexArrayMoon, uniforms5); //Moon
            render(vertexArrayMars, uniforms6);
            render(vertexArrayEarth, uniforms3);
            render(vertexArrayWolf, uniforms2); 
        }
        

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}