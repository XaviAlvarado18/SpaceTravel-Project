
#include "shapes.h"
#include <glm/glm.hpp>
#include "FatNoiseLite.h"
#include <glm/gtc/noise.hpp>
#include "color.h"
#include "draw.h"

glm::vec3 L4 = glm::vec3(0.0f, 0, 1.0f);
glm::vec3 L5 = glm::vec3(0.0f, 0, 1.0f);
glm::vec3 L2 = glm::vec3(0.0f, 0, 1.0f);
glm::vec3 L3 = glm::vec3(0.0f, 0, 1.0f);
float T = 0.5f;

float ox;
float oy;
float zoom;

FastNoiseLite planetContientNoiseGenerator;

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    glm::vec4 transformedVertex = uniforms.viewport * uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);
    glm::vec3 vertexRedux;
    vertexRedux.x = transformedVertex.x / transformedVertex.w;
    vertexRedux.y = transformedVertex.y / transformedVertex.w;
    vertexRedux.z = transformedVertex.z / transformedVertex.w;
    Color fragmentColor(253, 221, 202, 255); 
    glm::vec3 normal = glm::normalize(glm::mat3(uniforms.model) * vertex.normal);
    // Create a fragment and assign its attributes
    Fragment fragment;
    fragment.position = glm::ivec2(transformedVertex.x, transformedVertex.y);
    fragment.color = fragmentColor;
    return Vertex {vertexRedux, normal, vertex.position};
}

std::vector<std::vector<Vertex>> primitiveAssembly(
    const std::vector<Vertex>& transformedVertices
) {
    std::vector<std::vector<Vertex>> groupedVertices;

    for (int i = 0; i < transformedVertices.size(); i += 3) {
        std::vector<Vertex> vertexGroup;
        vertexGroup.push_back(transformedVertices[i]);
        vertexGroup.push_back(transformedVertices[i+1]);
        vertexGroup.push_back(transformedVertices[i+2]);
        
        groupedVertices.push_back(vertexGroup);
    }

    return groupedVertices;
}

uint32_t fnv1aHash(float x, float y, float z) {
    uint32_t hash = 2166136261u; // Valor inicial del hash
    const uint32_t prime = 16777619u; // Factor primo

    // Combina las coordenadas en el hash
    hash ^= *(reinterpret_cast<const uint32_t*>(&x));
    hash ^= *(reinterpret_cast<const uint32_t*>(&y));
    hash ^= *(reinterpret_cast<const uint32_t*>(&z));

    // Aplica la multiplicación por el factor primo
    hash *= prime;

    return hash;
}

const double starDensity = 0.0005; // Mayor densidad = más estrellas

// Función para generar ruido utilizando FastNoise Lite
float generateNoise(float x, float y, float z) {
    FastNoiseLite noise;
    int offsetX = 1000;
    int offsetY = 1000;
    float offsetZ = 0.6f;
    int scale = 1000;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.005f);

    // Ajusta el umbral para menos continentes
    float continentThreshold = 0.4f;

    float normalizedValue = noise.GetNoise(((x * scale) + offsetX) * offsetZ, ((y * scale) + offsetY) * offsetZ, (z * scale)* offsetZ);
    
    // Compara el valor normalizado con el umbral para determinar continente u océano
    return (normalizedValue < continentThreshold) ? 1.0f : 0.0f;
}

// Función para generar nubes volumétricas
float generateCloudDensity(float x, float y, float z) {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.07f);

    float scale = 100.0f; // Ajusta la escala para nubes más grandes
    float cloudDensity = noise.GetNoise(x * scale, y * scale, z * scale);

    // Ajusta la densidad de nubes para hacerlas más difusas
    cloudDensity = (cloudDensity + 1.0f) / 2.0f;

    // Define un umbral para la generación de nubes
    float cloudThreshold = 0.68f; // Umbral para generar nubes

    // Si la densidad de nubes supera el umbral, se generan nubes
    if (cloudDensity > cloudThreshold) {
        return cloudDensity;
    } else {
        return 0.0f; // Sin nubes en otras ubicaciones
    }
}

Fragment fragmentShaderStellarSpace(Fragment& fragment) {

    uint32_t hash = fnv1aHash(fragment.original.x, fragment.original.y, fragment.original.z);
    float normalizedHash = static_cast<float>(hash) / static_cast<float>(UINT32_MAX);

    Color color = (normalizedHash < starDensity) ? Color(244, 244, 244): Color(0,0,0);

    fragment.color = color;

    return fragment;
}

Fragment moonFragmentShader(Fragment& fragment) {

    glm::vec3 groundColor = glm::vec3(176.0f/255.0f,176.0f/255.0f, 176.0f/255.0f);
    glm::vec3 oceanColor = glm::vec3(67.0f/255.0f,75.0f/255.0f, 77.0f/255.0f);
    glm::vec3 cloudColor = glm::vec3(255.0f/255.0f,255.0f/255.0f, 255.0f/255.0f);
    glm::vec3 uv = glm::vec3(fragment.original.x, fragment.original.y, fragment.original.z) ;
    float intensity = glm::dot(fragment.normal, L5);
    float intensity2 = glm::dot(fragment.normal, L4 * 2.5f);
    //float intensity = fragment.intensity;
    float oxE = 3000;
    float oyE = 3000;
    float zoomE = 700;

    float noiseValue = abs(planetContientNoiseGenerator.GetNoise((uv.x + oxE) * zoomE, (uv.y + oyE) * zoomE, uv.z * zoomE)) ;

    intensity = (intensity< 0.05f)? 0.05f: intensity;
    intensity2 = (intensity2> 0.95f)? 0.95f: intensity2;

    glm::vec3 tmpColor =  (noiseValue>0.5f)? oceanColor: groundColor;
    tmpColor =(intensity<=1)? glm::mix(glm::vec3(0,0,0),tmpColor, intensity): tmpColor;
    if(intensity>0.05f){
        if(intensity2<1 && intensity2>0.15f){
            tmpColor = glm::mix(glm::vec3(0,0,0), tmpColor, (1-intensity2));
        }
    }
    else{
        tmpColor = tmpColor;
    }

    fragment.color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    return fragment;
}

Fragment planetFragmentShader(Fragment& fragment){
    Color color;

    glm::vec3 groundColor = glm::vec3(0.44f, 0.51f, 0.33f);
    glm::vec3 groudColor2 = glm::vec3(0.97f, 0.53f, 0.18f);
    glm::vec3 oceanColor = glm::vec3(0.12f, 0.38f, 0.57f);
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::vec2 uv = glm::vec2(fragment.original.x, fragment.original.y);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite noiseGenerator2;
    noiseGenerator2.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float ox = 1200.0f;
    float oy = 3000.0f;
    float zoom = 400.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + ox) * zoom, (uv.y + oy) * zoom);

    float oxg = 5500.0f;
    float oyg = 6900.0f;
    float zoomg = 900.0f;

    float noiseValueG = noiseGenerator2.GetNoise((uv.x + oxg) * zoomg, (uv.y + oyg) * zoomg);

    glm::vec3 tmpColor;
    if (noiseValue < 0.5f) {
        tmpColor = oceanColor;
    } else {
        tmpColor = groundColor;
        if (noiseValueG < 0.1f) {
            float t = (noiseValueG + 1.0f) * 0.5f; // Map [-1, 1] to [0, 1]
            tmpColor = glm::mix(groundColor, groudColor2, t);
        }
    }

    float oxc = 5500.0f;
    float oyc = 6900.0f;
    float zoomc = 300.0f;

    float noiseValueC = noiseGenerator.GetNoise((uv.x + oxc) * zoomc, (uv.y + oyc) * zoomc);

    if (noiseValueC > 0.5f) {
        float t = (noiseValueC - 0.5f) * 2.0f; // Map [-1, 1] to [0, 1]
        tmpColor = glm::mix(tmpColor, cloudColor, t);
    }

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    fragment.color = color;

    return fragment;
}

Fragment spaceshipFragmentShader(Fragment& fragment) {
    Color color;

    glm::vec3 baseColor = glm::vec3(0.4f, 0.4f, 0.4f); // Color base, gris metalizado.
    glm::vec3 highlightColor = glm::vec3(0.8f, 0.8f, 0.8f); // Reflejos, blanco para representar luz reflejada.

    glm::vec2 uv = glm::vec2(fragment.original.x, fragment.original.y);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Value); // Cambio a NoiseType Value para crear un efecto de superficie metálica.

    float ox = 1200.0f;
    float oy = 3000.0f;
    float zoom = 400.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + ox) * zoom, (uv.y + oy) * zoom);

    glm::vec3 tmpColor;
    
    // Aquí establecemos la lógica para decidir el color del píxel basándonos en el valor del ruido.
    if (noiseValue < 0.5f) {
        tmpColor = baseColor; // Usamos el color base para los valores de ruido más bajos.
    } else {
        tmpColor = highlightColor; // Usamos el color de resalte para los valores de ruido más altos.
    }

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    fragment.color = color;

    return fragment;
}


Fragment gasPlanetV1(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Define los colores para el planeta gaseoso
    Color cloudColor1 = Color(100, 100, 200); // Color de las nubes
    Color cloudColor2 = Color(200, 255, 255); // Color más claro para las nubes

    // Configuración del ruido
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetSeed(1337);
    noise.SetFrequency(0.05f); // Ajusta la frecuencia para controlar la apariencia de las nubes
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    noise.SetFractalLacunarity(2.0f);
    noise.SetFractalGain(0.5f);

    float ox = 3000.0f;
    float oy = 3000.0f;
    float zoom = 200.0f;

    // Generar valor de ruido
    float noiseValue = noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom);

    // Mapear el valor de ruido al rango de colores
    float t = (noiseValue + 1.0f) / 2.0f; // Asegura que esté en el rango [0, 1]

    // Mezcla los colores de las nubes basados en el valor de ruido
    Color cloudColor = cloudColor1 * (1.0f - t) + cloudColor2 * t;

    fragment.color = cloudColor;
    return fragment;
}

Fragment fragmentShaderMars(Fragment& fragment) {
// Define el color base para Marte (rojo)
    glm::vec3 marsColor = glm::vec3(0.7f, 0.0f, 0.0f); // Rojo característico de Marte

    // Simula la topografía de Marte con montañas y valles (utilizando ruido)
    float elevation = generateNoise(fragment.original.x, fragment.original.y, fragment.original.z);

    // Añade casquetes polares blancos
    float polarCapElevation = generateNoise(fragment.original.x, fragment.original.y, fragment.original.z + 0.1f); // Cambia la capa polar ligeramente hacia arriba
    if (fragment.original.y > 0.7f + polarCapElevation) {
        // Ajusta la mezcla para el casquete polar
        float polarMix = glm::smoothstep(0.7f, 0.8f, fragment.original.y);
        marsColor = glm::mix(marsColor, glm::vec3(1.0f, 1.0f, 1.0f), polarMix);
    }

    // Simula la atmósfera de Marte y la dispersión de la luz (efecto de "cielo rojo")
    glm::vec3 sunlightColor = glm::vec3(1.0f, 1.0f, 1.0f); // Color de la luz solar

    // Ajusta la dispersión atmosférica en función de la altura
    float atmosphericScattering = glm::smoothstep(0.7f, 1.0f, fragment.original.y);

    // Añade un efecto de "cielo rojo" basado en la dispersión
    marsColor = marsColor + (sunlightColor - marsColor) * atmosphericScattering;

    // Aplica detalles de textura realista utilizando ruido Perlin
    float textureDetail = generateNoise(fragment.original.x * 10.0f, fragment.original.y * 10.0f, fragment.original.z * 10.0f);
    marsColor += glm::vec3(textureDetail * 0.1f);


    // Establece el color del fragmento
    fragment.color = Color(marsColor.r, marsColor.g, marsColor.b);

    return fragment;
}


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



