
#include <SDL.h>

#include "spiral.h"
#include "graphics.h"

static void spiral (int x, int y, int pas, int nbtours)
{
  int i = x, j = y, tour;
  
  for (tour = 1; tour <= nbtours; tour++) {
    for (; i < x + tour*pas;i++)
      img(i,j) = 0xffffffff;
    for (; j < y + tour*pas+1;j++)
      img(i,j) = 0xffffffff;
    for (; i > x - tour*pas-1 ;i--)
      img(i,j) = 0xffffffff;
    for (; j > y - tour*pas-1;j--)
      img(i,j) = 0xffffffff;
  }
}

void spiral_regular (int xdebut, int xfin, int ydebut, int yfin, int pas, int nbtours)
{
  int i,j;
  int taille = nbtours * pas + 2;
  
  for (i = xdebut + taille; i < xfin - taille; i += 2*taille)
    for (j = ydebut + taille; j < yfin - taille; j += 2*taille)
      spiral (i,j, pas, nbtours); 
}
