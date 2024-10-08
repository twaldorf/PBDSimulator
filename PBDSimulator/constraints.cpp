#include <glm.hpp>
#include <iostream>

glm::vec3 static solveDistanceConstraint(glm::vec3 X_1, glm::vec3 X_2, float mass_1 = 1.f, float mass_2=1.f, float dt=0.00001) {
	if (X_1 == X_2) {
		return glm::vec3(0.f);
	}

	glm::vec3 to = X_2 - X_1;
	float inv_mass_1 = 1.f / mass_1;
	float inv_mass_2 = 1.f / mass_2;

	float omega = inv_mass_1 + inv_mass_2;

	float distance = glm::length(to);
	float rest_distance = 1.f;
	float constraint = distance - rest_distance;

	float stiffness = 0.01f;

	return constraint * stiffness * glm::normalize(to) / omega;
}

