// rup_gen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "stdio.h"

int main()
{
    FILE* f_out = fopen("rup.obj", "wb");

    fprintf(f_out, "# 3ds Max Wavefront OBJ");

    fclose(f_out);
}
