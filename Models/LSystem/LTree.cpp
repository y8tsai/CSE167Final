#include "LTree.h"
#include <stack>
#include <vector>

LTree::LTree(unsigned int randseed, LSysParam properties) {
	prop = properties;
	memo = NULL;
	reset(randseed);
	if( prop.rules.size() ){
		memo = new RuleMap[prop.rules.size()];
	}
}

void LTree::reset(unsigned int randseed) {
	seed = randseed;
	position = vec3();
	heading = vec3(0.0f, 1.0f, 0.0f);
	numDraws = 0;
	currentLv = 0; //supports 254 levels
	if( memo != NULL ){
		delete[] memo;
	}
}

std::vector<DrawData*>* LTree::generate() {
	std::string rule = prop.rules[prop.startRule];

	rule = evalRule(rule, prop.iterations - 1);
	for (std::size_t i = 0; i < rule.length(); ++i) {
		if (rule[i] == Grammar::DRAW) ++numDraws;
	}

	return draw(rule);
}

std::string LTree::evalRule(std::string &rule, int iteration) {
	if (iteration > 0) {
		for (std::size_t i = 0; i < rule.length(); ++i) {
			if (prop.rules.count(rule[i])) {
				char key = rule[i];
				rule.replace(i, 1, prop.rules[key]);
				int len = prop.rules[key].length() - 1;
				i += len;
			}
		}
		rule = evalRule(rule, iteration - 1);
	} 
	return rule;
}

std::vector<DrawData*>* LTree::draw(std::string tape) {
	std::vector<DrawData*> *mesh = new std::vector<DrawData*>();
	std::stack<vec3> states;

	int vertIdx = 0;
	for (std::size_t i = 0; i < tape.length(); ++i) {
		if (tape[i] == Grammar::DRAW) {
			mesh->push_back(drawForward());
		} else if (tape[i] == Grammar::TURNLEFT) {
			yawLeft(prop.yawMin);
		} else if (tape[i] == Grammar::TURNRIGHT) {
			yawRight(-prop.yawMin);
		} else if (tape[i] == Grammar::SAVE) {
			states.push(position);
			states.push(heading);
		} else if (tape[i] == Grammar::RESTORE) {
			heading = states.top();
			states.pop();
			position = states.top();
			states.pop();
		}
	}
	return mesh;
}

DrawData* LTree::drawForward() {
	return parametricCylinder(1.f, 0.2f, 12, 3);
}



DrawData* LTree::parametricCylinder(GLfloat height, GLfloat radius, GLint slices, GLint stacks) {
	stacks++;
	int count = stacks * slices * (3 + 2); // vertex components + uv texcoord + normal components + color
	GLfloat *vertices = new float[count];

	// +2 indices for degenerate triangles inbetween each stack
	// for every stack +2 indices to close mesh
	int indexCount = (count/5) + slices * (stacks - 2) + (4 * stacks - 6);
	
	GLuint *indices = new GLuint[indexCount];
	int curridx = -1;

	for (int v = 0; v < stacks; ++v) {
		float y = (float) v / stacks * height;
		if( v >= 2 ) curridx++;
		for (int u = 0; u < slices; ++u) {
			curridx += 2;
			double rotation = PI * 2 * (float)u / slices;
			float x = radius * cos(rotation);
			float z = radius * sin(rotation);
			int idx = (v * slices + u);
			indices[curridx] = idx;
			if( stacks > 1 && v > 0 && v != stacks - 1) {
				indices[ v*slices*2  + u*2 + v*4 + 1] = idx;
			}

			idx *= 5;
			// vertex xyz
			vertices[idx] = x;
			vertices[idx + 1] = y; 
			vertices[idx + 2] = z;
			// texcoords
			vertices[idx + 3] = (float)u / (slices-1); //u latitude
			vertices[idx + 4] = (float)v / (stacks -1); //v longitude


		}
		// if v is odd, then copy top 2 of that stack, to close walls
		// if v is even, then go back
		if( v > 0) { 
			curridx += 2;
			indices[curridx] = indices[ curridx - slices * 2];
			indices[++curridx] = indices[ curridx - slices * 2];
		} else if( v < 2) {
			curridx -= (slices * 2 + 1);	
		}
	}

	// Fill in degenerate triangles
	for( curridx = 2*slices+4; curridx < indexCount; curridx += 2*slices+4 ) {
		indices[curridx - 1] = indices[curridx];
		indices[curridx - 2] = indices[curridx-3];
	}

	//std::cout<< indexCount;
	GLuint ibo;
	//Make vertex array buffer object
	DrawData *cyl = new DrawData();
	cyl->type = GL_TRIANGLE_STRIP;
	cyl->indexStart = 0;
	cyl->indexCount = indexCount;
	
	cyl->shaders = Program::LoadShaders("Models/Shaders/tree.vs", "Models/Shaders/tree.fs");

	//Make vertex attribute object
	glGenVertexArrays(1, &cyl->vao);
	glBindVertexArray(cyl->vao);
	// Bind ibo to VAO
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indexCount, indices, GL_STATIC_DRAW);
	
	// Bind VBO to VAO saying
	glGenBuffers(1, &cyl->vbo);	//Make vertex index buffer object
	glBindBuffer(GL_ARRAY_BUFFER, cyl->vbo); // this is the vertex array, use  it
	glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0); //store vertex array in position 0
		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // this is how it's formatted
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), 0);
	// 1st arg: index of generic vertex attribute
	// 2nd arg: # of components per vertex
	// 3rd: data type
	// 4th: auto-normalize?
	// 5th: offset inbetween each vertex <# of components + offset>
	// 6th: pointer to first component of starting vertex

	// uv Texture Coordinate Attribute

	GLint attrib = glGetAttribLocation(cyl->shaders->getHandle(), "turd");
	glEnableVertexAttribArray(attrib);
	glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_TRUE, 5*sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glBindVertexArray(0);

	delete[] vertices;
	delete[] indices;
	
	cyl->texture = new Texture();
	GLuint woodtex;
	glGenTextures(1, &woodtex);
	glBindTexture(GL_TEXTURE_2D, woodtex);
	int w, h;
	unsigned char *img = SOIL_load_image("Resources\\Textures\\treeTrunk.bmp", &w, &h, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	cyl->texture->setID(woodtex);

	return cyl;
}

void LTree::yawLeft(GLfloat turnRadian) {
	vec3 axis = heading.cross(vec3(-heading.v[1], heading.v[0], 0.0f)).normalize();
	quat rot = quat::rotate(turnRadian, axis);
	heading = quat::tovec3(rot * quat(heading) * rot.conjugate());
}

void LTree::yawRight(GLfloat turnRadian) {
	yawLeft(turnRadian);
}