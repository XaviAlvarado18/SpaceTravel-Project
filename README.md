# SpaceTravel-Project
Proyecto #01 para el curso de Gráficos por Computadora en Universidad del Valle de Guatemala

El proyecto consiste en generar una simulación de un sistema solar en C++ utilizando la renderización de modelos .obj en base a triángulos, movimientos de cámara, movimientos de personaje y órbitas de planetas. 

## Bibliotecas
- SDL2 (Simple DirectMedia Layer): SDL2 es una biblioteca multimedia ampliamente utilizada para gestionar ventanas, gráficos y eventos en aplicaciones multimedia. En este proyecto, se usa para inicializar y controlar la ventana de la aplicación, el rendimiento y el manejo de eventos de teclado y ventana.

- GLM: Es una biblioteca de matemáticas que proporciona estructuras y funciones para operaciones matemáticas, especialmente útiles en aplicaciones gráficas y juegos. Se utiliza para manipular matrices, vectores y realizar transformaciones en los objetos.

- windows.h: La biblioteca "windows.h" se usa para definir las funciones y estructuras necesarias para la programación de ventanas en aplicaciones de Windows. Es necesario para configurar una aplicación de Windows y gestionar eventos de ventana.

- fstream: Esta biblioteca se utiliza para operaciones de entrada y salida de archivos. En el proyecto, se usa para cargar archivos de modelos 3D en formato OBJ.

- iostream: La biblioteca "iostream" se utiliza para entrada y salida estándar, permitiendo la comunicación con la consola.
- FatNoiseLite: Es una biblioteca de generación de ruido de código abierto y de código portátil para C++, proporciona una amplia selección de algoritmos de ruido, incluyendo ruido aleatorio, ruido fraccional Browniano, ruido de Perlin, ruido de Voronoi, y más.

## Funciones y métodos
- Color: Esta estructura representa un color RGBA (rojo, verde, azul y alfa) utilizado para definir colores en la aplicación.

- Uniforms: La estructura "Uniforms" es utilizada para almacenar las matrices de transformación y otros datos uniformes utilizados en los sombreadores y en la configuración de la vista.

- Fragment: La estructura "Fragment" es utilizada para representar fragmentos en la representación gráfica, almacenando información como posición, color, profundidad y otros datos.

- Face: La estructura "Face" representa una cara en un modelo 3D con información sobre los índices de vértices y normales.

- Vertex: La estructura "Vertex" almacena información sobre un vértice, incluyendo su posición y normal.

## Funciones y métodos

- render(): Esta función se encarga de realizar la renderización de los objetos 3D en la pantalla, aplicando transformaciones y sombreado según los parámetros especificados.

- Funciones para la creación de matrices de transformación como createViewportMatrix(), createProjectionMatrix(), createModelMatrixStellarSpace(), etc.

- Funciones para cargar modelos 3D en formato OBJ, como loadOBJ() y setupVertexArray().

- El bucle principal de la aplicación se encuentra en WinMain(), donde se gestionan eventos, se actualiza la vista de la cámara y se realiza la renderización.
