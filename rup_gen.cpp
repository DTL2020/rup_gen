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

// General model params

#define NUM_VERTEX_IN_CIRCLE 100 // one for all diameters define for now, better to do depending on diameter
#define NUM_STEPS_IN_RUPOR 25 // length in 10ths of mm (centimeter) ?
#define SPECIAL_BEGIN 2
#define SPECIAL_END 2
float fRadius0 = 0.3f; // start rupor radius. in centimeters.
float fSpRadius = 7.0f; // spiral bend radius in centimeters
float fAxisCoordStep = 1.0f; // rupor axis coord step in centimeters
float fBeta=0.078f; // beta exp param 
float fSpLengthAdvInit = 0.021f; // spiral length advance at each step in X-axis (to be adjusted for connecting turns ?)
float fThickness = 0.1f; // spacing between inner and outer surfaces
float fThicknessBegin = 0.2f; // spacing between inner and outer surfaces
float fThicknessEnd = 0.2f; // spacing between inner and outer surfaces
bool bStraight = true;

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

		// rotate around zero, Z-axis
		Vert = rotatept(Vert, deg2rad(fRot), vec3(0.0, 0.0, 1.0).normalized());

		// rotate around zero, X-axis
		Vert = rotatept(Vert, deg2rad(xAng), vec3(1.0, 0.0, 0.0).normalized());

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
	fprintf(f_out, "# General model params\r\n");
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# NUM_VERTEX_IN_CIRCLE %d \r\n", NUM_VERTEX_IN_CIRCLE);
	fprintf(f_out, "# NUM_STEPS_IN_RUPOR %d (in fAxisCoordStep units) \r\n", NUM_STEPS_IN_RUPOR);
	fprintf(f_out, "# fRadius0 %f (in centimeters) start rupor radius\r\n", fRadius0);
	fprintf(f_out, "# fSpRadius %f (in centimeters) spiral bend radius\r\n", fSpRadius);
	fprintf(f_out, "# fAxisCoordStep %f (in centimeters) rupor axis coord step in centimeters\r\n", fAxisCoordStep);
	fprintf(f_out, "# fBeta %f beta exp param\r\n", fBeta);
	fprintf(f_out, "# 0.7 low frequency cutoff about %f Hz (for 34000 cm/sec sound speed)\r\n", fBeta * 34000.0f / (2 * M_PI * 1.41f));
	fprintf(f_out, "# fSpLengthAdvInit %f initial spiral length advance at each step in X-axis\r\n", fSpLengthAdvInit);
	fprintf(f_out, "# fThickness %f (in centimeters) spacing between inner and outer surfaces\r\n", fThickness);
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "\r\n");

	// Create a vector containing vec3 points coords
	std::vector<vec3> Vertices;

	// Create a vector containing vec3 points coords
	std::vector<vec3> VerticesOuter;

	// Create a vector containing Faces data
	std::vector<Face> Faces;

	// Create a vector containing Faces data
	std::vector<Face> FacesOuter;

	double fSquareCurrent;

	float fAxisCoord = 0.0f; // in centimeters

	int iStartFirstCirc = 1;
	int iStartSecondCirc = iStartFirstCirc + NUM_VERTEX_IN_CIRCLE;

	int iNumVerticesInSide = NUM_VERTEX_IN_CIRCLE * NUM_STEPS_IN_RUPOR;

	// global to save to model text max radius
	float fRadius;

	// main spiral circles rotation
	// spiral angle position
	float fSpAng = 0;

	// spiral translation position
	vec3 SpTrans;
	SpTrans.x = 0.0f;

	if (!bStraight)
	{
		SpTrans.y = (-1.0f) * fSpRadius;
	}
	else
		SpTrans.y = 0.0f;

	SpTrans.z = 0.0f;

	// create first circle as base start to connect all next
	fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord);

	// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
	fRadius = sqrtf(fSquareCurrent / M_PI);

	// translate vOrigin of first circle to start pos
	if (!bStraight)
	{
		vOrigin.x = (-1.0f) * (fThicknessBegin - fThickness);
	}
	else
	vOrigin.x = 0.0f;
	vOrigin.y = SpTrans.y;
	vOrigin.z = SpTrans.z;

	CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);
	CircleGen(VerticesOuter, vOrigin, 0, 0, 0, fRadius + fThicknessBegin);

	fAxisCoord += fAxisCoordStep; // in cm

	// calculate fSpAngStep from fAxisCoord of roupor square and current spiral radius
	float fSpAngStep = (fAxisCoordStep * 360.0f) / (2 * M_PI * fSpRadius);

	bool bBegin = true;
	bool bEnd = true;

	// create a set of straight circles of vertices and connect with faces
	for (int i = 1; i < NUM_STEPS_IN_RUPOR; i++)
	{
		fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord);

		// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
		fRadius = sqrtf(fSquareCurrent / M_PI);

		// do not spiral rotate (and not spiral axis translate of DIRECT_BEGIN and DIRECT_END number of rupor steps 
		if (i < SPECIAL_BEGIN) // Z-advance no spiral rot
		{
			if (!bStraight)
			{
				// spiral transforms
				// rotate around zero, X-axis, additive at each length step
				SpTrans = rotatept(SpTrans, deg2rad(fSpAngStep), vec3(1.0, 0.0, 0.0).normalized());


				// calculate X-step for connecting turns with overlap
				float fSpLengthAdv = fSpLengthAdvInit * ((fRadius + fThickness) / (fRadius0 + fThickness)); // attempt to be proportional to current circle radius

				// translate origin
				vOrigin.x += fSpLengthAdv; // to add X-shift in axis of the spiral, step back to additional thickness
				vOrigin.y = SpTrans.y;
				vOrigin.z = SpTrans.z;

				fSpAng += fSpAngStep;
			}
			else // y only advance
			{
				vOrigin.z += fAxisCoordStep;
			}

			CircleGen(Vertices, vOrigin, fSpAng, 0, 0, fRadius);
			CircleGen(VerticesOuter, vOrigin, fSpAng, 0, 0, fRadius + fThicknessBegin);

		}
		else if (i > (NUM_STEPS_IN_RUPOR - (SPECIAL_END + 1))) // Y-advance no spiral rot
		{
			if (!bStraight)
			{
				if (bEnd)
				{
					// adjust step advance at the end once
					vOrigin.x += (fThicknessEnd - fThickness);
					bEnd = false;
				}

				// spiral transforms
				// rotate around zero, X-axis, additive at each length step
				SpTrans = rotatept(SpTrans, deg2rad(fSpAngStep), vec3(1.0, 0.0, 0.0).normalized());

				// calculate X-step for connecting turns with overlap
				float fSpLengthAdv = fSpLengthAdvInit * ((fRadius + fThicknessEnd) / (fRadius0 + fThicknessEnd)); // attempt to be proportional to current circle radius

				// translate origin
				vOrigin.x += fSpLengthAdv; // to add X-shift in axis of the spiral, step back to additional thickness
				vOrigin.y = SpTrans.y;
				vOrigin.z = SpTrans.z;

				fSpAng += fSpAngStep;
			}
			else
			{
				vOrigin.z += fAxisCoordStep;
			}

			CircleGen(Vertices, vOrigin, fSpAng, 0, 0, fRadius);
			CircleGen(VerticesOuter, vOrigin, fSpAng, 0, 0, fRadius + fThicknessEnd);
		}
		else // normal spiral
		{
			if (!bStraight)
			{
				if (bBegin)
				{
					// reset begin x-offset once
					vOrigin.x += (fThicknessBegin - fThickness);
					bBegin = false;
				}

				// spiral transforms
				// rotate around zero, X-axis, additive at each length step
				SpTrans = rotatept(SpTrans, deg2rad(fSpAngStep), vec3(1.0, 0.0, 0.0).normalized());

				// calculate X-step for connecting turns with overlap
				float fSpLengthAdv = fSpLengthAdvInit * ((fRadius + fThickness) / (fRadius0 + fThickness)); // attempt to be proportional to current circle radius

				// translate origin
				vOrigin.x += fSpLengthAdv; // to add X-shift in axis of the spiral
				vOrigin.y = SpTrans.y;
				vOrigin.z = SpTrans.z;

				fSpAng += fSpAngStep;
			}
			else
			{
				vOrigin.z += fAxisCoordStep;
			}

			CircleGen(Vertices, vOrigin, fSpAng, 0, 0, fRadius);
			CircleGen(VerticesOuter, vOrigin, fSpAng, 0, 0, fRadius + fThickness);

		}

		CircleConnect(Faces, iStartFirstCirc, iStartSecondCirc);
		CircleConnect(FacesOuter, iStartFirstCirc + iNumVerticesInSide, iStartSecondCirc + iNumVerticesInSide);

		fAxisCoord += fAxisCoordStep; // in cm

		iStartFirstCirc += NUM_VERTEX_IN_CIRCLE;
		iStartSecondCirc += NUM_VERTEX_IN_CIRCLE;
	}

	// write vertices list
	// inner side
	for (int i = 0; i < Vertices.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", Vertices[i].x, Vertices[i].y, Vertices[i].z);
	}

	// outer side
	for (int i = 0; i < VerticesOuter.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", VerticesOuter[i].x, VerticesOuter[i].y, VerticesOuter[i].z);
	}

	fprintf(f_out, "# %d vertices\r\n", (int)(Vertices.size()+ VerticesOuter.size()));
	fprintf(f_out, "\r\n");
	fprintf(f_out, "g Rup01Inner\r\n");

	// write faces list
	for (int i = 0; i < Faces.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", Faces[i].v1, Faces[i].v2, Faces[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)Faces.size());

	fprintf(f_out, "g Rup01Outer\r\n");

	// write faces list
	for (int i = 0; i < FacesOuter.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", FacesOuter[i].v1, FacesOuter[i].v2, FacesOuter[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)FacesOuter.size());

	fprintf(f_out, "# Max out Diameter %f (in centimeters)\r\n", fRadius * 2.0f);
	fprintf(f_out, "# Max X length %f (in centimeters)\r\n", vOrigin.x + fRadius0 + fRadius + 2* fThickness);
	fprintf(f_out, "# Max Diameter below %f (in centimeters)\r\n", (fSpRadius + fRadius + fThickness)*2.0f);

    fclose(f_out);

}
