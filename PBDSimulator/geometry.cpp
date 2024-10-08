#include "geometry.h"
#include "constraints.cpp"
#include <float.h>
// unsigned int control_word;
// unsigned int fp_control_state = _controlfp_s(&control_word, _EM_INEXACT, _MCW_EM);

float constexpr DEFAULT_MASS = 1.f;

SoftGeometry::SoftGeometry() {
	float radius = 1.0;

	float weft_stretch = 1.0;
	float warp_stretch = 1.0; 

	float selvage_direction = 0.0;
	float MU = 1.0f;
}

void SoftGeometry::Draw(Shader& shader) {

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &this->vertices[0], GL_DYNAMIC_DRAW);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
	// unbind 
	glBindVertexArray(0);
}

void SoftGeometry::setupSoftMesh() {
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	// normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// velocity
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO); 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_DYNAMIC_DRAW);
}

void SoftGeometry::applyTransformation(const glm::mat4 &matrix) {
	for (auto &v : this->vertices) {
		glm::vec4 pos = glm::vec4(v.Position, 1.0f);
		pos = matrix * pos;
		v.Position = glm::vec3(pos.x, pos.y, pos.z);
		v.lPosition = v.Position;
	}
}

void SoftGeometry::applyTransToRange(int start, int end, const glm::mat4& matrix) {
	for (int i = start; i < end; ++i) {
		glm::vec4 pos = glm::vec4(vertices[i].Position, 1.0f);
		pos = matrix * pos;
		vertices[i].Position = glm::vec3(pos.x, pos.y, pos.z);
		vertices[i].lPosition = vertices[i].Position;
	}
}

void Surface::updatePoints(float deltaTime, glm::vec3 f ) {
	std::vector<Vertex>::iterator Vtx;
	int n = 4;

	for (Vtx = vertices.begin(); Vtx != vertices.end(); ++Vtx) {
		glm::vec3 x0 = (*Vtx).Position;
		glm::vec3 v0 = (*Vtx).Velocity;
		(*Vtx).lPosition = x0;

		v0 = v0 + deltaTime * f;
		x0 = x0 + v0 * deltaTime;

		(*Vtx).Velocity = v0;
		(*Vtx).Position = x0;
	}

	for (int i = 0; i < n; ++i) {
		this->solveConstraints(deltaTime / n);
	}

	for (int i = 0; i < width; ++i) {
		vertices[i].Position = vertices[i].lPosition;
	}

	for (Vtx = vertices.begin(); Vtx != vertices.end(); ++Vtx) {
		glm::vec3 x0 = (*Vtx).Position;
		glm::vec3 v0 = (*Vtx).Velocity;
		v0 = (x0 - (*Vtx).lPosition) / deltaTime;
		(*Vtx).Velocity = v0 * 0.999f; // damp the velocity to force convergence
	}

	this->normalizeMesh();
}

void Surface::solveConstraints(float dt) {
	std::vector<std::pair<int, int>>::iterator constraint;

	for (constraint = constraints.begin(); constraint != constraints.end(); ++constraint) {
		int index1 = (*constraint).first;
		int index2 = (*constraint).second;

		glm::vec3 dx1 = callDistanceSolver(index1, index2, dt);
		vertices[index1].Position += dx1;
		vertices[index2].Position -= dx1;
	}
}

void Surface::setConstraints() {
	for (int y = 0; y < this->length; ++y) {
		for (int x = 0; x < this->width; ++x) {
			if (x < this->width - 1) {
				constraints.push_back(std::make_pair(getIndexAt(x, y), getIndexAt(x + 1, y)));
			}
			if (y < this->length - 1) {
				constraints.push_back(std::make_pair(getIndexAt(x, y), getIndexAt(x, y + 1)));
			}
		}
	}
}

int Surface::getIndexAt(int x, int y) {
	return y * this->width + x;
}

Vertex& Surface::getVertexAt(int x, int y) {
	return vertices[getIndexAt(x, y)];
}

Vertex& Surface::getVertexAt(int i) {
	return vertices[i];
}

glm::vec3 Surface::callDistanceSolver(int index1, int index2, float dt) {
	int vsize = this->vertices.size();

	// ensure the indices are within bounds
	if (vsize > index2 && 
		index2 >= 0 && 
		index1 >= 0 &&
		index1 != index2) {
		Vertex& vtx1 = this->vertices[index1];
		Vertex& vtx2 = this->vertices[index2];
		if (vtx1.Position != vtx2.Position) {
			return solveDistanceConstraint(vtx1.Position, vtx2.Position, vtx1.Mass, vtx2.Mass, dt);
		}
		
	}

	return glm::vec3(0.f);
}

void SoftGeometry::debug() {
	std::cout << "VTX start: " << vertices[0].Position.x
		<< " " << vertices[0].Position.y
		<< " " << vertices[0].Position.z
		<< std::endl;
}

Surface::Surface(int wid, int len, int res, float general_mass=1.f) : res{ res } {

	this->width = wid * res;
	this->length = len * res;

	for (int y = 0; y < length; y++) {
		for (int x = 0; x < width; x++) {
			glm::vec3 vtx = glm::vec3(0.0f - x + width / 2, 0.0f - y + length / 2, 0.0f);
			// Position and mass initialization
			Vertex vertex = Vertex{ vtx, glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f), general_mass };
			vertices.push_back(vertex);
		}
	}

	for (int i = 0; i < width * length; i++) {
		if (i % width != 0 && i > width) {
			this->indices.push_back(i - width - 1);
			this->indices.push_back(i - width);
			this->indices.push_back(i - 1);

			this->faces.push_back({ i - width - 1, i - width, i - 1 });

			this->indices.push_back(i - 1);
			this->indices.push_back(i - width);
			this->indices.push_back(i);

			this->faces.push_back({ i - 1, i - width, i });
		}
	}

	this->setConstraints();
	this->setupSoftMesh();
	this->normalizeMesh();
}

// interpretation of IQ's smooth Mesh_normalize from iquilezles.org/articles/normals
void SoftGeometry::normalizeMesh() {
	std::vector<Vertex>::iterator vtx;
	for (vtx = vertices.begin(); vtx != vertices.end(); ++vtx) {
		(*vtx).Normal = glm::vec3(0.0f);
	}

	std::vector<Face>::iterator face;

	for (face = faces.begin(); face != faces.end(); ++face) {
		const int ia = (*face).a;
		const int ib = (*face).b;
		const int ic = (*face).c;

		const glm::vec3 edge1 = vertices[ia].Position - vertices[ib].Position;
		const glm::vec3 edge2 = vertices[ic].Position - vertices[ib].Position;
		const glm::vec3 normal = glm::cross(edge1, edge2);

		vertices[ia].Normal += normal;
		vertices[ib].Normal += normal;
		vertices[ic].Normal += normal;
	}

	for (vtx = vertices.begin(); vtx != vertices.end(); ++vtx) {
		(*vtx).Normal = glm::normalize((*vtx).Normal);
	}
}

// debug function for testing constraint solver
void Surface::addVelocityToIndex(int index, glm::vec3 v) {
	this->vertices[index].Velocity = v;
}

Plane::Plane(const glm::vec3 origin) {
	glm::vec3 v1 = origin + glm::vec3(-0.5f, 0.0f, -0.5f);
	glm::vec3 v2 = origin + glm::vec3(0.5f, 0.0f, -0.5f);
	glm::vec3 v3 = origin + glm::vec3(-0.5f, 0.0f, 0.5f);
	glm::vec3 v4 = origin + glm::vec3(0.5f, 0.0f, 0.5f);

	vertices.push_back(Vertex{ v1 });
	vertices.push_back(Vertex{ v2 });
	vertices.push_back(Vertex{ v3 });
	vertices.push_back(Vertex{ v4 });

	this->indices.push_back(0);
	this->indices.push_back(1);
	this->indices.push_back(2);

	this->indices.push_back(1);
	this->indices.push_back(3);
	this->indices.push_back(2);

	this->Normal = glm::cross(v1, v2);

	this->setupSoftMesh();
}

glm::vec3 Plane::getCenter() const {
	return vertices[0].Position;
}
