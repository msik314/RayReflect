#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>

typedef struct
{
    glm::vec4 position;
    glm::vec4 normal;
    //Tangents go here
    glm::vec4 texCoords;
}
Vertex;

#endif //VERTEX_H
