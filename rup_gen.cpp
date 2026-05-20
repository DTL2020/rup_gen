// rup_gen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "stdio.h"
#include <vector>
#include "math.h"

#include "vec3.h"
#include "quat.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define rad2deg(x) (x*180.0/M_PI)
#define deg2rad(x) (x*M_PI/180.0)

#include "matfunc.h"

struct Face {
	int v1;
	int v2;
	int v3;
};

#define NUM_VERTEX_IN_CIRCLE 20 // one for all diameters define for now, better to do depending on diameter
#define NUM_STEPS_IN_RUPOR 55 // length in 10ths of mm (centimeter) ?
float fRadius0 = 0.3f; // start rupor radius. in centimeters.
float fBeta=0.078f; // beta exp param 

// generate circle of vertices around given origin point rotated to given angle
void CircleGen(std::vector<vec3>& vVerts, vec3 vOrigin, float xAng, float yAng, float zAng, float fRadius)
{
    for (int i = 0; i < NUM_VERTEX_IN_CIRCLE; i++)
    {
		vec3 Vert;
		Vert.x = fRadius;
		Vert.y = 0;
		Vert.z = 0;

		float fRot = ((float)i * 360.0f)/ NUM_VERTEX_IN_CIRCLE; // in degrees or 

		// rotate around zero
		Vert = rotatept(Vert, deg2rad(fRot), vec3(0.0, 0.0, 1.0).normalized());

		//move to origin
		Vert.x += vOrigin.x;
		Vert.y += vOrigin.y;
		Vert.z += vOrigin.z;

		vVerts.push_back(Vert);

    }
}

// connect 2 circles with faces
void CircleConnect(std::vector<Face>& vFaces, int iStartFirstCirc, int iStartSecondCirc)
{
	Face F1;
	Face F2;

	int iLastVertNum = iStartSecondCirc + 1;
	int iStartFirst = iStartFirstCirc;
	int iStartSecond = iStartSecondCirc;
	// connect by dual-triangles (quads), num of quads = num of verts in circle
	for (int i = 0; i < NUM_VERTEX_IN_CIRCLE - 1; i++) // all quads except last
	{
		// first triangle of quad

		F1.v1 = iStartFirst;
		F1.v2 = iStartSecond;
		F1.v3 = iLastVertNum;

		// second triangle of quad
		F2.v1 = iStartFirst + 1;
		F2.v2 = iStartFirst;
		F2.v3 = iLastVertNum;

		vFaces.push_back(F1);
		vFaces.push_back(F2);

		iLastVertNum++;
		iStartFirst++;
		iStartSecond++;
	}

	// last quad
	F1.v1 = iStartFirst;
	F1.v2 = iStartSecond;
	F1.v3 = iStartSecondCirc;

	F2.v1 = iStartFirstCirc;
	F2.v2 = iStartFirst;
	F2.v3 = iStartSecondCirc;

	vFaces.push_back(F1);
	vFaces.push_back(F2);
}

int main()
{
    FILE* f_out = fopen("rup.obj", "wb");

	double dSquare0 = M_PI * fRadius0 * fRadius0;

	vec3 vOrigin = vec3(0, 0, 0);

    fprintf(f_out, "# 3ds Max Wavefront OBJ\r\n");

	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# object rupor 01\r\n");
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "\r\n");

	// Create a vector containing vec3 points coords
	std::vector<vec3> Vertices;

	// Create a vector containing Faces data
	std::vector<Face> Faces;

	double fSquareCurrent;

	float fAxisCoord = 0.0f; // in centimeters

	int iStartFirstCirc = 1;
	int iStartSecondCirc = iStartFirstCirc + NUM_VERTEX_IN_CIRCLE;

	// create first circle as base start to connect all next
	fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord);

	// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
	float fRadius = sqrtf(fSquareCurrent / M_PI);

	CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);

	fAxisCoord += 1.0f; // in cm
	vOrigin.z += 1.0f; // 1 cm step in Z axis

	// create a set of straight circles of vertices and connect with faces
	for (int i = 1; i < NUM_STEPS_IN_RUPOR; i++)
	{
		fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord);

		// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
		float fRadius = sqrtf(fSquareCurrent / M_PI);

		CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);

		CircleConnect(Faces, iStartFirstCirc, iStartSecondCirc);

		fAxisCoord += 1.0f; // in cm
		vOrigin.z += 1.0f; // 1 cm step in Z axis

		iStartFirstCirc += NUM_VERTEX_IN_CIRCLE;
		iStartSecondCirc += NUM_VERTEX_IN_CIRCLE;
	}

	// write vertices list
	for (int i = 0; i < Vertices.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", Vertices[i].x, Vertices[i].y, Vertices[i].z);
	}

	fprintf(f_out, "# %d vertices\r\n", (int)Vertices.size());
	fprintf(f_out, "\r\n");
	fprintf(f_out, "g Rup01\r\n");

	// write vertices list
	for (int i = 0; i < Faces.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", Faces[i].v1, Faces[i].v2, Faces[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)Faces.size());


    fclose(f_out);

}
