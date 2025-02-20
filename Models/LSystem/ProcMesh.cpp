#include "ProcMesh.h"


CylinderMesh::CylinderMesh() {
	raw_mesh = new Mesh();
	texcoord = new Mesh();
	m2w.makeIdentity();
	rot = quat::rotate(0.0f, vec3(0.0f,1.0f,0.0f));
}

CylinderMesh::~CylinderMesh() {
	raw_mesh->clear();
	delete raw_mesh;
	texcoord->clear();
	delete texcoord;
}

/* FUNCTION: CreateCylinder
 * Creates a parametric cylinder and generated vertices are multiplied by the current
 * model2world matrix. Should not be used to draw directly since vertices have no meaning
 * other than location. Although, can be used with GL_Points
 * @reference ExportGLTriangleStrip
 *
 * PARAMS:
 * h: Height
 * r: Radius
 * stacks: columns
 * slices: faces
 */
void CylinderMesh::CreateCylinder(float h, float r, int stacks, int slices) {
	for(int v = stacks; v >= 0; --v) {
		Verts *raw = new Verts();
		Verts *tex = new Verts();
		float y = h * ((float) v / stacks);
		for(int u = 0; u < slices; ++u) {
			double rotation = PI * 2 * (float)u/slices;
			float x = r * cos(rotation);
			float z = r * sin(rotation);

			// rotate then translate
			vec3 k = quat::tovec3( rot * quat(vec3(x,y,z)) * rot.conjugate());
			raw->push_back( (m2w * vec4(k.v[0],k.v[1],k.v[2], 1.0)).toVec3() );
			float tex_s = (float)u / (slices-1); //u latitude
			float tex_t = (float)v / (stacks); //v longitude
			tex->push_back( vec3(tex_s, tex_t, 0.f) );
		}
		raw_mesh->push_back(raw);
		texcoord->push_back(tex);
	}
}

void CylinderMesh::ClearOrientation() {
	m2w.makeIdentity();
}

void CylinderMesh::Translate(vec3 translation) {
	m2w.setTranslate(translation);
}

void CylinderMesh::Rotate(float deg, vec3 norm_axis) {
	rot = quat::rotate(DEG2RAD(deg), norm_axis);
}

//This is broke don't use
CylinderMesh* CylinderMesh::Stitch(CylinderMesh *fst, CylinderMesh *snd) {
	CylinderMesh *linedUp = new CylinderMesh();
	size_t fstSize = fst->raw_mesh->size();
	size_t sndSize = snd->raw_mesh->size();

	linedUp->raw_mesh->reserve(fstSize + sndSize);
	for(int i = 0; i < fstSize; ++i) {
		Verts *ptr = (*fst->raw_mesh)[i];
		(*fst->raw_mesh)[i] = nullptr;
		linedUp->raw_mesh->push_back(ptr);
	}

	for(int i = 0; i < sndSize; ++i) {
		Verts *ptr = (*snd->raw_mesh)[i];
		(*snd->raw_mesh)[i] = nullptr;
		linedUp->raw_mesh->push_back(ptr);
	}

	return linedUp;
}

/* FUNCTION: ExportGLTriangle
 * Vertices changed to be in ordering for GL_TRIANGLE_STRIP
 */
Verts* CylinderMesh::ExportGLTriangleStrip() {
	Verts *exp = new Verts();
	
	Mesh::iterator it = raw_mesh->begin();
	Mesh::iterator tx = texcoord->begin();

	Verts * prev = *it++;
	Verts * tprev = *tx++;
	while(it != raw_mesh->end()) {
		Verts *curr = *it;
		Verts *tcurr = *tx;

		Verts* submesh = Interleave(prev, curr);
		Verts* subtex = Interleave( tcurr, tprev);
		for(std::size_t i = 0; i < submesh->size(); ++i) {
			exp->push_back( (*submesh)[i] );
			exp->push_back( (*subtex)[i] );
		}
		prev = *it++;
		tprev = *tx++;

		delete submesh;
		delete subtex;
	}
	return exp;
}


Verts* CylinderMesh::Interleave(Verts *bottom, Verts *top) {
	if( bottom == nullptr && top != nullptr) {
		return top;
	} else if( top == nullptr && bottom != nullptr) {
		return bottom;
	}
	Verts *interleaved = new Verts();
	// +2 is to close the mesh
	interleaved->reserve(bottom->size() + top->size() + 2);
	for(std::size_t i = 0; i < bottom->size(); ++i) {
		interleaved->push_back((*top)[i]);
		interleaved->push_back((*bottom)[i]);
	}
	interleaved->push_back((*top)[0]);
	interleaved->push_back((*bottom)[0]);
	

	return interleaved;
}

