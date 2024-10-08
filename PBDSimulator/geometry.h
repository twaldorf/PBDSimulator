#ifndef SOFT_GEOMETRY_H
#define SOFT_GEOMETRY_H

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "shader.h"
#include <vector>

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 Normal;
	float Mass;
	glm::vec3 lPosition;
};

struct Face {
	int a, b, c;
};

class SoftGeometry {
protected:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Face> faces;

	unsigned int VAO;
	unsigned int VBO;
	unsigned int IBO;
	float MU;

	void normalizeMesh();

public:
	SoftGeometry();
	void Draw(Shader& shader);
	void setupSoftMesh();
	void debug();
	void applyTransformation(const glm::mat4 &matrix);
	void applyTransToRange(int, int, const glm::mat4& matrix);

};

class Surface : public SoftGeometry {
	
public:
	Surface(int len, int width, int res, float);
	void solveConstraints(float);
	void updatePoints(float deltaTime, glm::vec3 f);
	void addVelocityToIndex(int i, glm::vec3 v);
	int length, width, res;
private:
	glm::vec3 callDistanceSolver(int index1, int index2, float);
	std::vector<std::pair<int, int>> constraints;
	void setConstraints();
	int getIndexAt(int, int);
	Vertex& getVertexAt(int, int);
	Vertex& getVertexAt(int);
	
};

class Plane : public SoftGeometry {
	glm::vec3 Normal;
public:
	Plane(const glm::vec3 origin);
	glm::vec3 getCenter() const;
};

#endif