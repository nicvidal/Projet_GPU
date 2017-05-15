#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define ALRND(a,n) (((size_t)(n)+(a)-1)&(~(size_t)((a)-1)))

#define ALIGN_MASK (~(15UL))

#if USE_DOUBLE == 0
#define coord_t float3
#define calc_t float
#else
#define coord_t double3 
#define calc_t double
#endif

static inline coord_t load3coord (__global calc_t * pos, int offset)
{
    coord_t f;

    f.x = *pos; pos += offset;
    f.y = *pos; pos += offset;
    f.z = *pos;

    return f;
}

static inline void store3coord (__global calc_t * pos, coord_t f, int offset)
{
    *pos = f.x; pos += offset;
    *pos = f.y; pos += offset;
    *pos = f.z;
}

static inline void inc3coord (__global calc_t * pos, coord_t f, int offset)
{
    *pos += f.x; pos += offset;
    *pos += f.y; pos += offset;
    *pos += f.z;
}

static inline calc_t squared_dist(coord_t a, coord_t b)
{
  coord_t f = a - b;
  return dot(f, f);
}

static inline calc_t lj_squared_v(calc_t r2)
{
    calc_t rr2 = 1.0 / r2;
    calc_t r6;

    r6 = LENNARD_SIGMA * LENNARD_SIGMA * rr2;
    r6 = r6 * r6 * r6;

    calc_t de = 24 * LENNARD_EPSILON * rr2 * (2.0f * r6 * r6 - r6);

    return de;
}

static inline void get_boxes (const coord_t coord, __constant calc_t *min_buffer,
			      int *box_x, int *box_y, int *box_z)
{
  *box_x = (int) ((coord.x - min_buffer[0]) * BOX_SIZE_INV);
  *box_y = (int) ((coord.y - min_buffer[1]) * BOX_SIZE_INV);
  *box_z = (int) ((coord.z - min_buffer[2]) * BOX_SIZE_INV);
}

static inline int get_num_box (const int box_x, const int box_y, const int box_z,
			       __constant int *domain_buff)
{
  return box_z * domain_buff[0] * domain_buff[1] +
    box_y * domain_buff[0] +
    box_x;
}

static inline bool get_num_box_ext (int *numbox,
			     const coord_t coord,
			    __constant calc_t *min_buffer,
			    __constant int *domain_buff)
{
  int box_x, box_y, box_z;
  bool border;

  get_boxes(coord, min_buffer, &box_x, &box_y, &box_z);

  *numbox = get_num_box(box_x, box_y, box_z, domain_buff);

  border  = (box_x == 0);
  border |= (box_y == 0);
  border |= (box_z == 0);
  border |= (box_x == domain_buff[0]-1);
  border |= (box_y == domain_buff[1]-1);
  border |= (box_z == domain_buff[2]-1);

  return border;
}

static inline int get_num_box_from_coord (const coord_t coord,
				   __constant calc_t *min_buffer,
				   __constant int *domain_buff)
{
    int box_x, box_y, box_z;

    get_boxes(coord, min_buffer, &box_x, &box_y, &box_z);

    return get_num_box(box_x, box_y, box_z, domain_buff);
}

__kernel
void update_position(__global calc_t *pos, __global calc_t *spd,
                     __constant calc_t *min, __constant calc_t *max,
                     unsigned offset)
{
    const unsigned gid = get_global_id(0);
    coord_t my_spd = load3coord(spd + gid, offset);

    inc3coord (pos + gid, my_spd, offset);
}

// This kernel is executed with offset x 3 threads
__kernel
void zero_speed(__global calc_t *speed)
{
  int index = get_global_id(0);

  speed[index] = 0.0;
}

// This kernel updates the Vertex Buffer Object according to positions of atoms
// It is executed by natoms * 3 threads
//
__kernel
void update_vertice(__global float* vbo, __global calc_t * pos, unsigned offset)
    // global = 3 * nb_vertices, local = 1
{
    unsigned index = get_global_id (0);
    unsigned axe = index % 3;
    unsigned no_atom = index / 3;

    vbo[index] = pos[no_atom + axe * offset];
}


// This kernel is executed with one thread per atom coordinate (+padding) = 3 * offset
__kernel
void border_collision (__global calc_t * pos, __global calc_t * speed,
		  __constant calc_t * min, __constant calc_t * max,
		  calc_t radius, unsigned natoms, unsigned offset)
{
  // TODO
}


static inline void freeze_atom (__global calc_t * speed, int offset)
{
    *speed = 0.0; speed += offset;
    *speed = 0.0; speed += offset;
    *speed = 0.0;
}

static void check_collision (__global calc_t * pos, __global calc_t * speed,
			     calc_t radius, unsigned index,
			     unsigned natoms, unsigned offset)
{
  // TODO
}

// This kernel is executed with one thread per atom
__kernel
void atom_collision (__global calc_t * pos, __global calc_t * speed,
		     calc_t radius, unsigned natoms, unsigned offset)
{
    unsigned index = get_global_id (0);

    if(index < natoms)
      check_collision (pos, speed, radius, index, natoms, offset);
}

// This kernel is executed with one thread per atom
__kernel
void gravity (__global calc_t * speed, calc_t gx, calc_t gy, calc_t gz,
	      unsigned natoms, unsigned offset)
{
  // TODO
}


__kernel
void lennard_jones (__global calc_t * pos,
		       __global calc_t * speed,
		       unsigned natoms, unsigned offset)
{
    unsigned index = get_global_id (0);
    unsigned local_id = get_local_id (0);

    coord_t mypos;
    mypos = load3coord (pos + index, offset);

    coord_t force = { 0.0f, 0.0f, 0.0f };

    __local coord_t tile[TILE_SIZE];
    unsigned nb_blocs = get_num_groups(0); // ROUND (natoms) / TILE_SIZE;
    unsigned i = !index;
    for (unsigned t = 0; t < nb_blocs && i < natoms; ++t)
    {
        tile[local_id] = load3coord (pos + (t * TILE_SIZE + local_id), offset);
        barrier (CLK_LOCAL_MEM_FENCE);

        unsigned start_i = i - i % TILE_SIZE;
        for (; i < start_i + TILE_SIZE && i < natoms;
                (i == index - 1 ? i += 2 : i++))
        {
            coord_t opos = tile[i % TILE_SIZE];
            calc_t dist = squared_dist (mypos, opos);
            if (dist < LENNARD_SQUARED_CUTOFF)
	      force += lj_squared_v(dist) * (mypos - opos);
        }

        barrier (CLK_LOCAL_MEM_FENCE);
    }

    // update speed
    if (index < natoms)
      inc3coord (speed + index, force, offset);
}

__kernel void null_kernel (void)
{}

// The following kernels only work under SPHERE_MODE

// This kernel is executed with 3 * vertices_per_atom threads
__kernel
void eating(__global float *model, float dy)
{
    int index = get_global_id(0);
    int N = get_global_size(0);

    // y coordinate?
    dy *= ((index % 3) == 1) ? 1.0f : 0.0f;
    // vertex belongs to upper part of sphere?
    dy *= (index >= N/2) ? 1.0f : -1.0f;

    model[index] += dy;
}

// This kernel is executed with 3 * vertices_per_atom threads
__kernel
void growing(__global float *model, float factor)
{
    unsigned index = get_global_id(0);
    unsigned N = get_global_size(0);

    // we only modify ghosts' shape, so we add N
    model[index + N] *= factor;
}

// This kernel updates vertices in whole vbo using vertices for 2 atom "models" : pacman & ghost
// It is executed by total number of vertices * 3
//
__kernel
void update_vertices(__global float *vbo, __global float *pos,
		     unsigned offset,
		     __global float *model, unsigned vertices_per_atom)
{
    unsigned index = get_global_id(0);
    unsigned tpa = 3 * vertices_per_atom;
    unsigned slice = (index % 3) * offset;
    __global float *p = pos + slice;
    unsigned idx = index / tpa;

    vbo[index] = model[index % (2 * tpa)] + p[idx];
}

__attribute__((vec_type_hint(int)))

__kernel
void reset_int_buffer(__global int *buffer, const unsigned begin,
                      const unsigned end)
{
    unsigned gid = get_global_id(0) + begin;

    if (gid >= end)
        return;
    buffer[gid] = 0;
}

__attribute__((vec_type_hint(int)))

__kernel
void copy_buffer (__global unsigned * buff_dst, __global unsigned* buff_src, unsigned nb)
{
  unsigned index = get_global_id (0);

  if (index < nb)
    buff_dst[index] = buff_src[index];
}

/**
 * This kernel counts the number of atoms per boxes (including ghosts and
 * leaving atoms). In single device, this is the default version. In multi
 * devices, this is currently used in order to count ghosts which are located
 * on min/max borders.
 */
__kernel
void box_count_all_atoms(__global calc_t *pos_buff, __global int *box_buff,
	                 __constant calc_t *min_buff, __constant int *domain_buff,
	                 unsigned offset, unsigned begin, unsigned end)
{
    unsigned gid = get_global_id(0) + begin;
    coord_t my_pos;
    int num_box;

    if (gid >= end)
        return;

    my_pos = load3coord(pos_buff + gid, offset);
    num_box = get_num_box_from_coord(my_pos, min_buff, domain_buff);
    atomic_inc(&box_buff[num_box]);
}

__attribute__((vec_type_hint(int)))


/**
 * This kernel sorts atoms (including ghosts and leaving atoms). In single
 * device, this is the default version. In multi devices, this is currently
 * used in order to sort ghosts which are located on min/max borders.
 */
__attribute__((vec_type_hint(calc_t)))
__kernel
void box_sort_all_atoms(__global calc_t *pos_buff, __global calc_t *alt_pos_buff,
		        __global calc_t *spd_buff, __global calc_t *alt_spd_buff,
		        __global int *calc_offset_buff, __constant calc_t *min_buff,
		        __constant int *domain_buff, unsigned offset,
                        unsigned begin, unsigned end)
{
    unsigned gid = get_global_id(0) + begin;
    coord_t my_pos, my_spd;
    int num_box;

    if (gid >= end)
        return;

    my_pos = load3coord(pos_buff + gid, offset);
    num_box = get_num_box_from_coord(my_pos, min_buff, domain_buff);

    int shift_atom = atomic_inc(calc_offset_buff + num_box);

    /* Sort atom position and speed. */
    my_spd = load3coord(spd_buff + gid, offset);
    store3coord(alt_pos_buff + shift_atom, my_pos, offset);
    store3coord(alt_spd_buff + shift_atom, my_spd, offset);
}

__attribute__((vec_type_hint(calc_t)))

__kernel
void box_force (__global calc_t *pos_buffer,
		__global calc_t *spd_buffer,
		__global int *box_buffer,
		__constant calc_t *min_buffer,
		__constant int *domain_buff,
		__global calc_t *alt_pos_buffer,
		__constant calc_t *min, __constant calc_t *max,
		unsigned offset, unsigned begin,
		unsigned end)
{
  const int shift_x = SUBCELL;            
  const int shift_y = domain_buff[0]; // shall be int to avoid promoting cy to unsigned...
  const int shift_z = domain_buff[0] * domain_buff[1];
  const unsigned num_groups = get_num_groups(0);
  const unsigned group_id = get_group_id(0);
  const unsigned gid = get_global_id(0) + (begin & (~(TILE_SIZE - 1)));
  const unsigned wid = get_local_id(0);
  coord_t force_total = { 0.0f, 0.0f, 0.0f };
  coord_t my_pos;
  int num_box;
  bool is_border;

  if (gid >= begin && gid < end) {
    /* Pre-compute the box ID of the current work-item. */
    my_pos = load3coord(pos_buffer + gid, offset);
    is_border = get_num_box_ext (&num_box, my_pos, min_buffer, domain_buff);
  }

  for (int cz = -SUBCELL * shift_z; cz <= SUBCELL * shift_z; cz += shift_z) {

    for (int cy = -SUBCELL * shift_y; cy <= SUBCELL * shift_y; cy += shift_y) {

      unsigned current_num_box;
      unsigned my_min_box, my_max_box;
      unsigned min_index, max_index, my_min_index, my_max_index;
      __local coord_t tile_pos[TILE_SIZE];
      __local unsigned local_min;
      __local unsigned local_max;

      if(gid >= begin && gid < end) {
	current_num_box = num_box + cz + cy;

        /* Trick for reducing divergence. */
        my_min_box = is_border * num_box + !is_border * (current_num_box - shift_x);
        my_max_box = is_border * num_box + !is_border * (current_num_box + shift_x);

	const unsigned last_thread = (num_groups - 1 == group_id ?
				      (end - 1) % TILE_SIZE :
				      TILE_SIZE - 1);
        const unsigned first_thread = (group_id == 0) ? begin % TILE_SIZE : 0;

        if (wid == first_thread)
          local_min = my_min_box;
        if (wid == last_thread)
          local_max = my_max_box;
      }

      barrier(CLK_LOCAL_MEM_FENCE);

      min_index = box_buffer[local_min] + begin;
      max_index = box_buffer[local_max + 1] + begin;

      barrier (CLK_LOCAL_MEM_FENCE);

      if (min_index == max_index) {
	continue;
      }

      if (gid >= begin && gid < end) {
	my_min_index = box_buffer[my_min_box] + begin;
	my_max_index = box_buffer[my_max_box + 1] + begin;
      }

      // For each neighbour tile
      //
      for (unsigned next_first_atom_to_load = min_index; // & ALIGN_MASK;
	   next_first_atom_to_load < max_index; 
	   next_first_atom_to_load += TILE_SIZE) {

	unsigned ind_pos = next_first_atom_to_load + wid;

	// Load tile
	//
	if (ind_pos < max_index) {
	  if (ind_pos == gid)
	    tile_pos[wid] = my_pos;
	  else
	    tile_pos[wid] = load3coord(pos_buffer + ind_pos, offset);
	}

	barrier (CLK_LOCAL_MEM_FENCE);

	if (gid >= begin && gid < end && !is_border) {

	  // Compute forces
	  //
	  for (unsigned j = 0; j < TILE_SIZE; j++) {

	    const unsigned c = (wid + j) % TILE_SIZE;
	    const unsigned atom_currently_checked = next_first_atom_to_load + c;

	    if (atom_currently_checked >= my_min_index &&
		atom_currently_checked < my_max_index &&
		atom_currently_checked != gid) {

	      // Lennard-jones
	      coord_t atom_checked_position = tile_pos[c];
	      calc_t dist2 = squared_dist(my_pos, atom_checked_position);

	      if(dist2 < LENNARD_SQUARED_CUTOFF) {
		force_total += lj_squared_v(dist2) * (my_pos - atom_checked_position);
	      }
	    }
	  }
	}

	barrier(CLK_LOCAL_MEM_FENCE);
      } // next_first_atom_to_load

    } // cy
  } // cz

  if (gid >= begin && gid < end)
  {
    force_total *= (calc_t)DELTA_T;
#ifndef FORCE_N_UPDATE
    inc3coord (spd_buffer + gid, force_total, offset);
#else
    coord_t speed = load3coord (spd_buffer + gid, offset);

      speed += force_total;
      store3coord (spd_buffer + gid, speed, offset);

    if(!is_border) {
      my_pos += speed * (calc_t)DELTA_T;
    }
    store3coord (alt_pos_buffer + gid, my_pos, offset);
#endif
  }
}
