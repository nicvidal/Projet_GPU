
// NE PAS MODIFIER
static unsigned color_mean (unsigned c1, unsigned c2)
{
  uchar4 c;

  c.x = ((unsigned)(((uchar4 *) &c1)->x) + (unsigned)(((uchar4 *) &c2)->x)) / 2;
  c.y = ((unsigned)(((uchar4 *) &c1)->y) + (unsigned)(((uchar4 *) &c2)->y)) / 2;
  c.z = ((unsigned)(((uchar4 *) &c1)->z) + (unsigned)(((uchar4 *) &c2)->z)) / 2;
  c.w = ((unsigned)(((uchar4 *) &c1)->w) + (unsigned)(((uchar4 *) &c2)->w)) / 2;

  return (unsigned) c;
}

// NE PAS MODIFIER
static int4 color_to_int4 (unsigned c)
{
  uchar4 ci = *(uchar4 *) &c;
  return convert_int4 (ci);
}

// NE PAS MODIFIER
static unsigned int4_to_color (int4 i)
{
  return (unsigned) convert_uchar4 (i);
}


__kernel void scrollup (__global unsigned *in, __global unsigned *out)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  unsigned couleur;

  couleur = in [y * DIM + x];

  y = (y ? y - 1 : get_global_size (1) - 1);

  out [y * DIM + x] = couleur;
}

__kernel void scrolldown (__global unsigned *in, __global unsigned *out)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  unsigned couleur;

  couleur = in [y * DIM + x];

  y = (y == get_global_size (1) - 1)? 0 : y + 1;

  out [y * DIM + x] = couleur;
}


static unsigned saturate (unsigned input)
{
  for (int i = 0; i < 7; i++)
    input = (input << 1) | input;

  return input;
}

__kernel void divergence (__global unsigned *in, __global unsigned *out)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  int b = 3;

  if (((x >> b) & 1) == 0)
    out [y * DIM + x] = saturate (0x01000001); // will become red
  else
    out [y * DIM + x] = saturate (0x01010001); // will be yellow
}


__kernel void transpose_naif (__global unsigned *in, __global unsigned *out)
{
  int x = get_global_id (0);
  int y = get_global_id (1);

  out [x * DIM + y] = in [y * DIM + x];
}


__kernel void transpose (__global unsigned *in, __global unsigned *out)
{
  //done
  //d'après la correction : tampon[TILEY][TILEX]
  __local unsigned tampon[TILEY][TILEX];
  int x = get_global_id(0);
  int y = get_global_id(1);
  int xloc = get_local_id(0);
  int yloc = get_local_id(1);

  tampon[xloc][yloc] = in[y * DIM + x];
  barrier(CLK_LOCAL_MEM_FENCE);
  out[(x-xloc + yloc)*DIM + y - yloc + xloc] = tampon[yloc][xloc];
}


#define PIX_BLOC 4
__kernel void magic (__global unsigned *in, __global unsigned *out)
{
  __local unsigned tampon;
  if(get_local_id(0) == 0 && get_local_id(1) == 0){
    tampon = 0;
  }
  int x = get_global_id(0);
  int y = get_global_id(1);
  int xloc = get_local_id(0);
  int yloc = get_local_id(1);

  tampon += in[y * DIM + x];

  barrier(CLK_LOCAL_MEM_FENCE);

  out[y*DIM + x] = tampon/(TILEX*TILEY);
}
// ATTENTION: les tuiles doivent être multiples de PIX_BLOC x PIX_BLOC
__kernel void pixellize (__global unsigned *in, __global unsigned *out)
{
  __local unsigned tile[TILEY][TILEX];
  int x = get_global_id(0);
  int y = get_global_id(1);
  int xloc = get_local_id(0);
  int yloc = get_local_id(1);

  unsigned m=1;
  unsigned pm=0;

  tile[yloc][xloc] = in[y*DIM + x];

  for(unsigned d = 1; d < PIX_BLOC; d*=2){

    barrier(CLK_LOCAL_MEM_FENCE);
    if((xloc & m) == 0 /*&& (yloc & pm) == 0*/){

      tile[yloc][xloc] = color_mean(tile[yloc][xloc], tile[yloc][xloc+d]);
      barrier(CLK_LOCAL_MEM_FENCE);

      if((yloc & m) == 0){
        tile[yloc][xloc] = color_mean(tile[yloc][xloc], tile[yloc+d][xloc]);
      }
    }
    pm = m;
    m = (m << 1)+1;
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  out[y*DIM+x] = tile[yloc & (255-PIX_BLOC+1)][xloc & (255-PIX_BLOC+1)];

  /*tampon[yloc][xloc] = in[y * DIM + x];
  barrier(CLK_LOCAL_MEM_FENCE);

  if(xloc % 2 == 0){
    tampon[yloc][xloc] = color_mean(tampon[yloc][xloc], tampon[yloc][xloc+1]);
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  if(yloc%2 == 0){
    tampon[yloc][xloc] =  color_mean(tampon[yloc][xloc], tampon[yloc+1][xloc]);
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  out[y * DIM + x] = tampon[yloc & 0xFE][xloc & 0xFE];*/

}


// Flou utilisant la moyenne pondérée des 8 pixels environnants :
// celui du centre pèse 8, les autres 1. Les pixels se trouvant sur
// les bords sont également traités (mais ils ont moins de 8 voisins).
__kernel void blur (__global unsigned *in, __global unsigned *out)
{

    __local unsigned tile[TILEY+2][TILEX+2];
    int x = get_global_id(0);
    int y = get_global_id(1);
    int xloc = get_local_id(0);
    int yloc = get_local_id(1);

    tile[yloc + 1][xloc + 1] = in[y * DIM + x];

    if(x > 0 && x < DIM-1 && y > 0 && y < DIM-1){
        if (xloc == 0) {
          tile[yloc+1][0] = in[y * DIM + (x - 1)];
        }
        if (xloc == TILEX - 1) {
          tile[yloc+1][TILEX + 1] = in[y * DIM + (x + 1)];
        }

        if (yloc == 0) {
          tile[0][xloc + 1] = in[(y - 1) * DIM + x];
        }
        if (yloc == TILEY - 1) {
          tile[TILEY + 1][xloc + 1] = in[(y + 1) * DIM + x];
        }

        barrier(CLK_LOCAL_MEM_FENCE);
        int4 s = color_to_int4(tile[yloc][xloc + 1]) // [y - 1][x]
          + color_to_int4(tile[yloc + 1][xloc]) // [y][x - 1]
          + color_to_int4(tile[yloc + 1][xloc + 1]) // [y][x]
          + color_to_int4(tile[yloc + 1][xloc + 2]) // [y][x + 1]
          + color_to_int4(tile[yloc + 2][xloc + 1]); // [y + 1][x]

        unsigned color = int4_to_color((s/((int4)5)));

        out[y*DIM+x] = color;
    } else {
        out[y*DIM+x] = in[y * DIM + x];
    }
}


// NE PAS MODIFIER
static float4 color_scatter (unsigned c)
{
  uchar4 ci;

  ci.s0123 = (*((uchar4 *) &c)).s3210;
  return convert_float4 (ci) / (float4) 255;
}

// NE PAS MODIFIER: ce noyau est appelé lorsqu'une mise à jour de la
// texture de l'image affichée est requise
__kernel void update_texture (__global unsigned *cur, __write_only image2d_t tex)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  int2 pos = (int2)(x, y);
  unsigned c;

  c = cur [y * DIM + x];

  write_imagef (tex, pos, color_scatter (c));
}
