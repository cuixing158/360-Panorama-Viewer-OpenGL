#include "Sphere.h"
#include <cmath>

SphereData::SphereData(float radius, unsigned int rings, unsigned int sectors) {
    m_rings = rings;
    m_sectors = sectors;
    numVertices = rings * sectors * 3;
    numTexs = rings * sectors * 2;
    numIndices = (rings - 1) * (sectors - 1) * 6;

    vertices = new GLfloat[numVertices];
    texCoords = new GLfloat[numTexs];
    indices = new GLushort[numIndices];

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);
    int v = 0, t = 0, i = 0;
    for (unsigned int r = 0; r < rings; r++) {
        for (unsigned int s = 0; s < sectors; s++) {
            float y = sin(-2 * PI * +PI * r * R);
            float x = cos(2 * PI * s * S) * sin(PI * r * R);
            float z = sin(2 * PI * s * S) * sin(PI * r * R);

            //            float z = sin(-M_PI_2 + M_PI * r * R);
            //            float y = cos(-M_PI_2 + M_PI * r * R)*sin(2 * M_PI * s * S) ;
            //            float x = cos(-M_PI_2 + M_PI * r * R)*cos(2 * M_PI * s * S);

            texCoords[t++] = s * S;
            texCoords[t++] = r * R;

            vertices[v++] = x * radius;
            vertices[v++] = y * radius;
            vertices[v++] = z * radius;
        }
    }

    for (unsigned int r = 0; r < rings - 1; r++) {
        for (unsigned int s = 0; s < sectors - 1; s++) {
            indices[i++] = r * sectors + s;
            indices[i++] = r * sectors + (s + 1);
            indices[i++] = (r + 1) * sectors + (s + 1);
            indices[i++] = r * sectors + s;
            indices[i++] = (r + 1) * sectors + (s + 1);
            indices[i++] = (r + 1) * sectors + s;
        }
    }
}

SphereData::~SphereData() {
    delete[] vertices;
    delete[] texCoords;
    delete[] indices;
}

const GLfloat* SphereData::getVertices() const {
    return vertices;
}

const GLfloat* SphereData::getTexCoords() const {
    return texCoords;
}

const GLushort* SphereData::getIndices() const {
    return indices;
}

int SphereData::getNumVertices() const {
    return numVertices;
}

int SphereData::getNumTexs() const {
    return numTexs;
}

int SphereData::getNumIndices() const {
    return numIndices;
}

int SphereData::getRings() const {
    return m_rings;
}

int SphereData::getSectors() const {
    return m_sectors;
}
