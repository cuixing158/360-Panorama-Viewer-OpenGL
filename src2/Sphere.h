#ifndef SPHERE_DATA_H
#define SPHERE_DATA_H

#include <GL/glew.h>
//#include <GLES3/gl3.h>

#define PI 3.14159265358979323846
class SphereData {
   public:
    SphereData(float radius, unsigned int rings, unsigned int sectors);
    ~SphereData();

    const GLfloat* getVertices() const;
    const GLfloat* getTexCoords() const;
    const GLushort* getIndices() const;
    int getNumVertices() const;
    int getNumIndices() const;
    int getNumTexs() const;

    int getRings() const;
    int getSectors() const;

   private:
    GLfloat* vertices;
    GLfloat* texCoords;
    GLushort* indices;
    int numVertices;
    int numIndices;
    int numTexs;

    GLuint m_rings;
    GLuint m_sectors;
};

#endif  // SPHERE_DATA_H
