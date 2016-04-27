#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <string>
#include "glm/glm.hpp"

struct FaceData
{
    int vertexIndex[3];
    int texCoordIndex[3];
    int normalIndex[3];
};

class GeometryData
{
public:
    void loadFromOBJFile(std::string filename);

    int vertexCount();

    void* vertexData();
    void* textureCoordData();
    void* normalData();
    void* tangentData();
    void* bitangentData();

    void rotateObject();
    void scaleObject();

    //glm::mat4 GetWorldMatrix(char axis, float degrees);
    //void applyModifications(glm::mat4 x, glm::mat4 y);

private:
    std::vector<float> vertices;
    std::vector<float> textureCoords;
    std::vector<float> normals;
    std::vector<float> tangents;
    std::vector<float> bitangents;

    std::vector<FaceData> faces;
};

#endif
