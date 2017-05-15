
#include <stdio.h>
#include <stdlib.h>

#define MD_FILE     "hugechoc.conf"
#define ATOM_RADIUS 0.2f
//#define DIM         20
#define DIM  160

#define DELTA       (ATOM_RADIUS*4.0f)


#define error(...) do { fprintf(stderr, "Error: " __VA_ARGS__); exit(EXIT_FAILURE); } while(0)

float min_ext[3] = { 0.0, 0.0, 0.0 };
float max_ext[3] = { DIM*DELTA, DIM*DELTA, DIM*DELTA };

FILE *fp;

unsigned nb_atoms = 0;

void add_atom(unsigned x, unsigned y, unsigned z,
	      float xs, float ys, float zs,
	      unsigned xmin, unsigned ymin, unsigned zmin,
	      unsigned xmax, unsigned ymax, unsigned zmax)
{
  float xpos, ypos, zpos;

  if(x >= xmin && y >= ymin && z >= zmin &&
     x <= xmax && y <= ymax && z <= zmax) {

    xpos = x * DELTA / 2.0;
    ypos = y * DELTA / 2.0;
    zpos = z * DELTA / 2.0;

    fprintf(fp, "%f %f %f\n", xpos, ypos, zpos);
    fprintf(fp, "%f %f %f\n", xs, ys, zs);

    // ROTATION -> balade en Z
    //fprintf(fp, "%f %f %f\n", xpos, zpos, ypos);
    //fprintf(fp, "%f %f %f\n", xs, zs, ys);

    nb_atoms++;

  }
}

void generate_bloc(unsigned xmin, unsigned ymin, unsigned zmin, 
		   unsigned xmax, unsigned ymax, unsigned zmax,
		   float yspeed)
{
  for(int z = zmin*2; z <= zmax*2; z += 2) {
    for(int y = ymin*2; y <= ymax*2; y += 2) { // Change to < for thin borders
      for(int x = xmin*2; x <= xmax*2; x += 2) {

	// Place 4 atoms per lattice
	// 0, 0, 0
	add_atom(x, y, z,
		 0.0, yspeed, 0.0,
		 2*xmin, 2*ymin, 2*zmin,
		 2*xmax, 2*ymax, 2*zmax);
	// 0, 1/2, 1/2
	add_atom(x, y + 1, z + 1,
		 0.0, yspeed, 0.0,
		 2*xmin, 2*ymin, 2*zmin,
		 2*xmax, 2*ymax, 2*zmax);
	// 1/2, 0, 1/2
	add_atom(x + 1, y, z + 1,
		 0.0, yspeed, 0.0,
		 2*xmin, 2*ymin, 2*zmin,
		 2*xmax, 2*ymax, 2*zmax);
	// 1/2, 1/2, 0 
	add_atom(x + 1, y + 1, z,
		 0.0, yspeed, 0.0,
		 2*xmin, 2*ymin, 2*zmin,
		 2*xmax, 2*ymax, 2*zmax);
      }
    }
  }
}

void generate_pointe(unsigned xmin, unsigned ymin, unsigned zmin, 
		     unsigned xmax, unsigned ymax, unsigned zmax,
		     float yspeed)
{
  for(int z = zmin*2; z <= zmax*2; z += 2) {
    unsigned skipped = xmax - xmin - 2;

    for(int y = ymin*2; y <= ymax*2; y += 2) {
      for(int x = xmin*2; x <= xmax*2; x += 2) {

	// Place 4 atoms per lattice
	// 0, 0, 0
	add_atom(x, y, z,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);

	// 1/2, 0, 1/2
	add_atom(x + 1, y, z + 1,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);

	// 0, 1/2, 1/2
	add_atom(x, y + 1, z + 1,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);

	// 1/2, 1/2, 0 
	add_atom(x + 1, y + 1, z,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);
      }
    if(skipped)
      skipped --;
    }
  }
}

void generate_pointe_inv(unsigned xmin, unsigned ymin, unsigned zmin, 
			 unsigned xmax, unsigned ymax, unsigned zmax,
			 float yspeed)
{
  for(int z = zmin*2; z <= zmax*2; z += 2) {
    unsigned skipped = xmax - xmin - 2;

    for(int y = ymax*2; y >= ymin*2; y -= 2) {
      for(int x = xmin*2; x <= xmax*2; x += 2) {

	// Place 4 atoms per lattice
	// 0, 0, 0
	add_atom(x, y, z,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);

	// 1/2, 0, 1/2
	add_atom(x + 1, y, z + 1,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);

	// 0, 1/2, 1/2
	add_atom(x, y + 1, z + 1,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);

	// 1/2, 1/2, 0 
	add_atom(x + 1, y + 1, z,
		 0.0, yspeed, 0.0,
		 2*xmin + skipped, 2*ymin, 2*zmin + skipped,
		 2*xmax - skipped, 2*ymax, 2*zmax - skipped);
      }
    if(skipped)
      skipped --;
    }
  }
}

int main(int argc, char **argv)
{
  fp = fopen(MD_FILE, "w");
  if(fp == NULL)
    error("Cannot open %s file\n", MD_FILE);

  fprintf(fp, "%09d\n", 0);

  for(int i = 0; i<3; i++)
    fprintf(fp, "%f %f\n", min_ext[i], max_ext[i]);

  // speed
  fprintf(fp, "%d\n", 1);

  //generate_bloc(3, 17, 3, 37, 18, 37, 0.0);
  //generate_bloc(18, 30, 18, 22, 36, 22, -0.03);

  // DIM = 80
  //generate_bloc(2, 39, 2, 78, 40, 78, 0.0);
  //generate_pointe(16, 60, 16, 24, 72, 24, -0.03);
  //generate_pointe_inv(56, 8, 56, 64, 20, 64, +0.03);

  // DIM = 160
  generate_bloc(4, 79, 4, 154, 80, 154, 0.0);
  generate_pointe(34, 100, 34, 46, 120, 46, -0.03);
  generate_pointe_inv(114, 39, 114, 126, 59, 126, +0.03);

  // Cube (DIM = 20)
  //generate_bloc(8, 8, 8, 10, 10, 10, -0.008);

  // sparse
  //for(unsigned z = 0; z<80; z++)
  //  generate_bloc(1, 1, z*2, 79, 79, z*2, 0.0);

  fseek(fp, 0, SEEK_SET);

  fprintf(fp, "%09d\n", nb_atoms); 

  fclose(fp);

  return 0;
}
