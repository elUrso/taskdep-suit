/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify                      */
/*  it under the terms of the GNU General Public License as published by                      */
/*  the Free Software Foundation; either version 2 of the License, or                         */
/*  (at your option) any later version.                                                       */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful,                           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             */
/*  GNU General Public License for more details.                                              */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License                         */
/*  along with this program; if not, write to the Free Software                               */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            */
/**********************************************************************************************/

/* Original code from the Application Kernel Matrix by Cray */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <omp.h>
#include "../../common/BOTSCommonUtils.h"

#define ROWS 64
#define COLS 64
#define DMAX 64
#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

int solution = -1, solution_seq = -1;

int cutoff_value = 5;

typedef int  coor[2];
typedef char ibrd[ROWS][COLS];
typedef char (*pibrd)[COLS];

int manual_cutoff, if_cutoff, final_cutoff;

FILE * inputFile;

struct cell {
  int   n;
  coor *alt;
  int   top;
  int   bot;
  int   lhs;
  int   rhs;
  int   left;
  int   above;
  int   next;
};

struct cell * gcells;

int  MIN_AREA;
ibrd BEST_BOARD;
coor MIN_FOOTPRINT;

int N;

/* compute all possible locations for nw corner for cell */
static int starts(int id, int shape, coor *NWS, struct cell *cells) {
  int i, n, top, bot, lhs, rhs;
  int rows, cols, left, above;

/* size of cell */
  rows  = cells[id].alt[shape][0];
  cols  = cells[id].alt[shape][1];

/* the cells to the left and above */
  left  = cells[id].left;
  above = cells[id].above;

/* if there is a vertical and horizontal dependence */
  if ((left >= 0) && (above >= 0)) {

     top = cells[above].bot + 1;
     lhs = cells[left].rhs + 1;
     bot = top + rows;
     rhs = lhs + cols;

/* if footprint of cell touches the cells to the left and above */
     if ((top <= cells[left].bot) && (bot >= cells[left].top) &&
         (lhs <= cells[above].rhs) && (rhs >= cells[above].lhs))
          { n = 1; NWS[0][0] = top; NWS[0][1] = lhs;  }
     else { n = 0; }

/* if there is only a horizontal dependence */
   } else if (left >= 0) {

/* highest initial row is top of cell to the left - rows */
     top = max(cells[left].top - rows + 1, 0);
/* lowest initial row is bottom of cell to the left */
     bot = min(cells[left].bot, ROWS);
     n   = bot - top + 1;

     for (i = 0; i < n; i++) {
         NWS[i][0] = i + top;
         NWS[i][1] = cells[left].rhs + 1;
     }

  } else {

/* leftmost initial col is lhs of cell above - cols */
     lhs = max(cells[above].lhs - cols + 1, 0);
/* rightmost initial col is rhs of cell above */
     rhs = min(cells[above].rhs, COLS);
     n   = rhs - lhs + 1;

     for (i = 0; i < n; i++) {
         NWS[i][0] = cells[above].bot + 1;
         NWS[i][1] = i + lhs;
  }  }

  return (n);
}



/* lay the cell down on the board in the rectangular space defined
   by the cells top, bottom, left, and right edges. If the cell can
   not be layed down, return 0; else 1.
*/
static int lay_down(int id, ibrd board, struct cell *cells) {
  int  i, j, top, bot, lhs, rhs;

  top = cells[id].top;
  bot = cells[id].bot;
  lhs = cells[id].lhs;
  rhs = cells[id].rhs;

  for (i = top; i <= bot; i++) {
  for (j = lhs; j <= rhs; j++) {
      if (board[i][j] == 0) board[i][j] = (char)id;
      else                  return(0);
  } }

  return (1);
}


#define read_integer(file,var) \
  if ( fscanf(file, "%d", &var) == EOF ) {\
	fprintf(stdout," Bogus input file\n");\
	exit(-1);\
  }

static void read_inputs() {
  int i, j, n;

  read_integer(inputFile,n);
  N = n;

  gcells = (struct cell *) malloc((n + 1) * sizeof(struct cell));

  gcells[0].n     =  0;
  gcells[0].alt   =  0;
  gcells[0].top   =  0;
  gcells[0].bot   =  0;
  gcells[0].lhs   = -1;
  gcells[0].rhs   = -1;
  gcells[0].left  =  0;
  gcells[0].above =  0;
  gcells[0].next  =  0;

  for (i = 1; i < n + 1; i++) {

      read_integer(inputFile, gcells[i].n);
      gcells[i].alt = (coor *) malloc(gcells[i].n * sizeof(coor));

      for (j = 0; j < gcells[i].n; j++) {
          read_integer(inputFile, gcells[i].alt[j][0]);
          read_integer(inputFile, gcells[i].alt[j][1]);
      }

      read_integer(inputFile, gcells[i].left);
      read_integer(inputFile, gcells[i].above);
      read_integer(inputFile, gcells[i].next);
      }

  if (!feof(inputFile)) {
      read_integer(inputFile, solution_seq);
  }
}


static void write_outputs() {
  int i, j;

    fprintf(stdout,"Minimum area = %d\n\n", MIN_AREA);

    for (i = 0; i < MIN_FOOTPRINT[0]; i++) {
      for (j = 0; j < MIN_FOOTPRINT[1]; j++) {
          if (BEST_BOARD[i][j] == 0) {fprintf(stdout," ");}
          else                       fprintf(stdout,"%c", 'A' + BEST_BOARD[i][j] - 1);
      }
      fprintf(stdout,"\n");
    }
}
//manual
static int add_cell_ser (int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS) {
  int  i, j, nn, nn2, area;

  ibrd board;
  coor footprint, NWS[DMAX];

  nn2 = 0;

/* for each possible shape */
  for (i = 0; i < CELLS[id].n; i++) {
/* compute all possible locations for nw corner */
      nn = starts(id, i, NWS, CELLS);
      nn2 += nn;
/* for all possible locations */
      for (j = 0; j < nn; j++) {
	  struct cell *cells = CELLS;
/* extent of shape */
          cells[id].top = NWS[j][0];
          cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
          cells[id].lhs = NWS[j][1];
          cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

          memcpy(board, BOARD, sizeof(ibrd));

/* if the cell cannot be layed down, prune search */
          if (! lay_down(id, board, cells)) {
             //fprintf(stdout,"Chip %d, shape %d does not fit\n", id, i);
             goto _end;
          }

/* calculate new footprint of board and area of footprint */
          footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
          footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
          area         = footprint[0] * footprint[1];

/* if last cell */
          if (cells[id].next == 0) {

/* if area is minimum, update global values */
		  if (area < MIN_AREA) {
#pragma omp critical
			  if (area < MIN_AREA) {
				  MIN_AREA         = area;
				  MIN_FOOTPRINT[0] = footprint[0];
				  MIN_FOOTPRINT[1] = footprint[1];
				  memcpy(BEST_BOARD, board, sizeof(ibrd));
				  //fprintf(stdout,"N  %d\n", MIN_AREA);
			  }
		  }

/* if area is less than best area */
          } else if (area < MIN_AREA) {
            #pragma omp atomic
             nn2 += add_cell_ser(cells[id].next, footprint, board,cells);

/* if area is greater than or equal to best area, prune search */
          } else {

             //fprintf(stdout,"T  %d, %d\n", area, MIN_AREA);

	  }
_end:;
}
}
  return nn2;
  }


static int add_cell_if(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS,int level) {
  int  i, j, nn, area, nnc, nnl;

  ibrd board;
  coor footprint, NWS[DMAX];

  nnc = nnl = 0;

/* for each possible shape */
  for (i = 0; i < CELLS[id].n; i++) {
/* compute all possible locations for nw corner */
      nn = starts(id, i, NWS, CELLS);
      nnl += nn;
/* for all possible locations */
      for (j = 0; j < nn; j++) {
#pragma omp task untied private(board, footprint,area) \
firstprivate(NWS,i,j,id,nn,level) \
shared(FOOTPRINT,BOARD,CELLS,MIN_AREA,MIN_FOOTPRINT,N,BEST_BOARD,nnc) \
if(level<cutoff_value)
{
	  struct cell cells[N+1];
	  memcpy(cells,CELLS,sizeof(struct cell)*(N+1));
/* extent of shape */
          cells[id].top = NWS[j][0];
          cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
          cells[id].lhs = NWS[j][1];
          cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

          memcpy(board, BOARD, sizeof(ibrd));

/* if the cell cannot be layed down, prune search */
          if (! lay_down(id, board, cells)) {
             //fprintf(stdout,"Chip %d, shape %d does not fit\n", id, i);
             goto _end;
          }

/* calculate new footprint of board and area of footprint */
          footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
          footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
          area         = footprint[0] * footprint[1];

/* if last cell */
          if (cells[id].next == 0) {

/* if area is minimum, update global values */
		  if (area < MIN_AREA) {
#pragma omp critical
			  if (area < MIN_AREA) {
				  MIN_AREA         = area;
				  MIN_FOOTPRINT[0] = footprint[0];
				  MIN_FOOTPRINT[1] = footprint[1];
				  memcpy(BEST_BOARD, board, sizeof(ibrd));
				  //fprintf(stdout,"N  %d\n", MIN_AREA);
			  }
		  }

/* if area is less than best area */
          } else if (area < MIN_AREA) {
 	    #pragma omp atomic
                nnc += add_cell_if(cells[id].next, footprint, board,cells,level+1);
/* if area is greater than or equal to best area, prune search */
          } else {

             //fprintf(stdout,"T  %d, %d\n", area, MIN_AREA);

	  }
_end:;
}
      }
}
#pragma omp taskwait
return nnc+nnl;
}

static int add_cell_final(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS,int level) {
  int  i, j, nn, area, nnc, nnl;

  coor footprint, NWS[DMAX];

  nnc = nnl = 0;

/* for each possible shape */
  for (i = 0; i < CELLS[id].n; i++) {
/* compute all possible locations for nw corner */
      nn = starts(id, i, NWS, CELLS);
      nnl += nn;
/* for all possible locations */
      for (j = 0; j < nn; j++) {
#pragma omp task untied private(footprint,area) \
firstprivate(NWS,i,j,id,nn,level,cutoff_value) \
shared(FOOTPRINT,BOARD,CELLS,MIN_AREA,MIN_FOOTPRINT,N,BEST_BOARD,nnc) \
final(level >= cutoff_value) mergeable
{
          ibrd board;
          struct cell *cells;

          if ( omp_in_final() && level > cutoff_value ) {
            cells = CELLS;
          } else {
            cells = alloca(sizeof(struct cell)*(N+1));
	    memcpy(cells,CELLS,sizeof(struct cell)*(N+1));
          }

/* extent of shape */
          cells[id].top = NWS[j][0];
          cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
          cells[id].lhs = NWS[j][1];
          cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

          memcpy(board, BOARD, sizeof(ibrd));

/* if the cell cannot be layed down, prune search */
          if (! lay_down(id, board, cells)) {
             //fprintf(stdout,"Chip %d, shape %d does not fit\n", id, i);
             goto _end;
          }

/* calculate new footprint of board and area of footprint */
          footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
          footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
          area         = footprint[0] * footprint[1];

/* if last cell */
          if (cells[id].next == 0) {

/* if area is minimum, update global values */
		  if (area < MIN_AREA) {
#pragma omp critical
			  if (area < MIN_AREA) {
				  MIN_AREA         = area;
				  MIN_FOOTPRINT[0] = footprint[0];
				  MIN_FOOTPRINT[1] = footprint[1];
				  memcpy(BEST_BOARD, board, sizeof(ibrd));
				  //fprintf(stdout,"N  %d\n", MIN_AREA);
			  }
		  }

/* if area is less than best area */
          } else if (area < MIN_AREA) {
 	    #pragma omp atomic
                nnc += add_cell_final(cells[id].next, footprint, board,cells,level+1);
/* if area is greater than or equal to best area, prune search */
          } else {

            // fprintf(stdout,"T  %d, %d\n", area, MIN_AREA);

	  }
_end:;
}
      }
}
#pragma omp taskwait
return nnc+nnl;
}

static int add_cell_manual(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS,int level) {
  int  i, j, nn, area, nnc, nnl;

  ibrd board;
  coor footprint, NWS[DMAX];

  nnc = nnl = 0;

/* for each possible shape */
  for (i = 0; i < CELLS[id].n; i++) {
/* compute all possible locations for nw corner */
      nn = starts(id, i, NWS, CELLS);
      nnl += nn;
/* for all possible locations */
      for (j = 0; j < nn; j++) {
#pragma omp task untied private(board, footprint,area) \
firstprivate(NWS,i,j,id,nn,level,cutoff_value) shared(nnc) \
shared(FOOTPRINT,BOARD,CELLS,MIN_AREA,MIN_FOOTPRINT,N,BEST_BOARD)
{
	  struct cell *cells;

          cells = alloca(sizeof(struct cell)*(N+1));
	  memcpy(cells,CELLS,sizeof(struct cell)*(N+1));

/* extent of shape */
          cells[id].top = NWS[j][0];
          cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
          cells[id].lhs = NWS[j][1];
          cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

          memcpy(board, BOARD, sizeof(ibrd));

/* if the cell cannot be layed down, prune search */
          if (! lay_down(id, board, cells)) {
             //fprintf(stdout,"Chip %d, shape %d does not fit\n", id, i);
             goto _end;
          }

/* calculate new footprint of board and area of footprint */
          footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
          footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
          area         = footprint[0] * footprint[1];

/* if last cell */
          if (cells[id].next == 0) {

/* if area is minimum, update global values */
		  if (area < MIN_AREA) {
#pragma omp critical
			  if (area < MIN_AREA) {
				  MIN_AREA         = area;
				  MIN_FOOTPRINT[0] = footprint[0];
				  MIN_FOOTPRINT[1] = footprint[1];
				  memcpy(BEST_BOARD, board, sizeof(ibrd));
				  //fprintf(stdout,"N  %d\n", MIN_AREA);
			  }
		  }

/* if area is less than best area */
          } else if (area < MIN_AREA) {
	     if(level+1 < cutoff_value ) {
 	       #pragma omp atomic
                nnc += add_cell_manual(cells[id].next, footprint, board,cells,level+1);
	     } else {
 	       #pragma omp atomic
		nnc += add_cell_ser(cells[id].next, footprint, board,cells);
	     }
/* if area is greater than or equal to best area, prune search */
          } else {
             //fprintf(stdout,"T  %d, %d\n", area, MIN_AREA);
	  }
_end:;
}
      }
}
#pragma omp taskwait

return nnc+nnl;
}

static int add_cell(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS) {
  int  i, j, nn, area, nnc,nnl;

  ibrd board;
  coor footprint, NWS[DMAX];

  nnc = nnl = 0;

/* for each possible shape */
  for (i = 0; i < CELLS[id].n; i++) {
/* compute all possible locations for nw corner */
      nn = starts(id, i, NWS, CELLS);
      nnl += nn;
/* for all possible locations */
      for (j = 0; j < nn; j++) {
#pragma omp task untied private(board, footprint,area) \
firstprivate(NWS,i,j,id,nn) \
shared(FOOTPRINT,BOARD,CELLS,MIN_AREA,MIN_FOOTPRINT,N,BEST_BOARD,nnc)
{
	  struct cell cells[N+1];
	  memcpy(cells,CELLS,sizeof(struct cell)*(N+1));
/* extent of shape */
          cells[id].top = NWS[j][0];
          cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
          cells[id].lhs = NWS[j][1];
          cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

          memcpy(board, BOARD, sizeof(ibrd));

/* if the cell cannot be layed down, prune search */
          if (! lay_down(id, board, cells)) {
             //fprintf(stderr,"Chip %d, shape %d does not fit\n", id, i);
             goto _end;
          }

/* calculate new footprint of board and area of footprint */
          footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
          footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
          area         = footprint[0] * footprint[1];

/* if last cell */
          if (cells[id].next == 0) {

/* if area is minimum, update global values */
		  if (area < MIN_AREA) {
#pragma omp critical
			  if (area < MIN_AREA) {
				  MIN_AREA         = area;
				  MIN_FOOTPRINT[0] = footprint[0];
				  MIN_FOOTPRINT[1] = footprint[1];
				  memcpy(BEST_BOARD, board, sizeof(ibrd));
				  //fprintf(stderr,"N  %d\n", MIN_AREA);
			  }
		  }

/* if area is less than best area */
          } else if (area < MIN_AREA) {
 	    #pragma omp atomic
 	      nnc += add_cell(cells[id].next, footprint, board,cells);
/* if area is greater than or equal to best area, prune search */
          } else {

             //fprintf(stderr,"T  %d, %d\n", area, MIN_AREA);

	  }
_end:;
}
      }
}
#pragma omp taskwait
return nnc+nnl;
}

static int add_cell_seq(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS) {
  int  i, j, nn, area, nnc,nnl;

  ibrd board;
  coor footprint, NWS[DMAX];

  nnc = nnl = 0;

/* for each possible shape */
  for (i = 0; i < CELLS[id].n; i++) {
/* compute all possible locations for nw corner */
      nn = starts(id, i, NWS, CELLS);
      nnl += nn;
/* for all possible locations */
      for (j = 0; j < nn; j++) {
          struct cell cells[N+1];
          memcpy(cells,CELLS,sizeof(struct cell)*(N+1));
/* extent of shape */
          cells[id].top = NWS[j][0];
          cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
          cells[id].lhs = NWS[j][1];
          cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

          memcpy(board, BOARD, sizeof(ibrd));

/* if the cell cannot be layed down, prune search */
          if (! lay_down(id, board, cells)) {
             //fprintf(stderr,"Chip %d, shape %d does not fit\n", id, i);
             goto _end;
          }

/* calculate new footprint of board and area of footprint */
          footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
          footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
          area         = footprint[0] * footprint[1];

/* if last cell */
          if (cells[id].next == 0) {

/* if area is minimum, update global values */
      if (area < MIN_AREA) {
        if (area < MIN_AREA) {
          MIN_AREA         = area;
          MIN_FOOTPRINT[0] = footprint[0];
          MIN_FOOTPRINT[1] = footprint[1];
          memcpy(BEST_BOARD, board, sizeof(ibrd));
          //fprintf(stderr,"N  %d\n", MIN_AREA);
        }
      }

/* if area is less than best area */
          } else if (area < MIN_AREA) {
        nnc += add_cell_seq(cells[id].next, footprint, board,cells);
/* if area is greater than or equal to best area, prune search */
          } else {

             //fprintf(stderr,"T  %d, %d\n", area, MIN_AREA);

    }
_end:;
      }
}
return nnc+nnl;
}

ibrd board;

void floorplan_init (char *filename)
{
    int i,j;

    inputFile = fopen(filename, "r");

    if(NULL == inputFile) {
        fprintf(stdout,"Couldn't open %s file for reading\n", filename);
        exit(1);
    }

    /* read input file and initialize global minimum area */
    read_inputs();
    MIN_AREA = ROWS * COLS;

    /* initialize board is empty */
    for (i = 0; i < ROWS; i++)
    for (j = 0; j < COLS; j++) board[i][j] = 0;

}

void compute_floorplan_seq (void)
{
    coor footprint;
    /* footprint of initial board is zero */
    footprint[0] = 0;
    footprint[1] = 0;
    fprintf(stdout,"Computing floorplan ");
    add_cell_seq(1, footprint, board, gcells);
    fprintf(stdout," completed!\n");

}

void compute_floorplan (void)
{
    coor footprint;
    /* footprint of initial board is zero */
    footprint[0] = 0;
    footprint[1] = 0;
    fprintf(stdout,"Computing floorplan ");

    if (manual_cutoff) {
      #pragma omp parallel
      #pragma omp single
      add_cell_manual(1, footprint, board, gcells,0);
    }
    else if (if_cutoff) {
      #pragma omp parallel
      #pragma omp single
      add_cell_if(1, footprint, board, gcells,0);
    }
        else if (final_cutoff) {
      #pragma omp parallel
      #pragma omp single
      add_cell_final(1, footprint, board, gcells,0);
    }
    else {
      #pragma omp parallel
      #pragma omp single
      add_cell(1, footprint, board, gcells);
    }
    fprintf(stdout," completed!\n");

}

void floorplan_end (void)
{
    /* write results */
    write_outputs();
}

int floorplan_verify (void)
{
    if (solution != -1 && solution_seq != -1)
      if (solution == solution_seq && MIN_AREA == solution) {
        return 1;
      }
      else {
        return 0;
      }
    else
      return -1;
}

void print_usage() {

   fprintf(stderr, "\n");
   fprintf(stderr, "Usage: %s -[options]\n", "Floorplan");
   fprintf(stderr, "\n");
   fprintf(stderr, "Where options are:\n");
   fprintf(stderr, "  -f <file>  : Cell description file (mandatory)\n");
   fprintf(stderr, "  -a <flag> : Set if-cutoff on\n");
   fprintf(stderr, "  -b <flag> : Set manual-cutoff on\n");
   fprintf(stderr, "  -c <flag> : Set final-cutoff on (choose one or none)\n");
   fprintf(stderr, "  -s         : Runs sequential program\n");
   fprintf(stderr, "  -h         : Print program's usage (this help).\n");
   fprintf(stderr, "\n");

}

int main(int argc, char* argv[]) {

  int i;
  int parallel = 1;
  char filename[100];
  for (i=1; i<argc; i++) {
        if (argv[i][0] == '-') {
          switch (argv[i][1]) {
              case 'f': /* read argument size 0 */
                     argv[i][1] = '*';
                     i++;
                     if (argc == i) { "Erro\n"; exit(100); }
                     strcpy(filename, argv[i]);
                     break;
              case 'a': /* read argument size 0 */
                     argv[i][1] = '*';
                     //i++;
                     //if (argc == i) { "Erro\n"; exit(100); }
                     if_cutoff = 1;
                     manual_cutoff = 0;
                     final_cutoff = 0;
                     break;
              case 'b': /* read argument size 0 */
                     argv[i][1] = '*';
                     //i++;
                     //if (argc == i) { "Erro\n"; exit(100); }
                     manual_cutoff = 1;
                     if_cutoff = 0;
                     final_cutoff = 0;
                     break;
              case 'c': /* read argument size 0 */
                     argv[i][1] = '*';
                     //i++;
                     //if (argc == i) { "Erro\n"; exit(100); }
                     final_cutoff = 1;
                     if_cutoff = 0;
                     manual_cutoff = 0;
                     break;
              case 's': /* print usage */
                      argv[i][1] = '*';
                       parallel = 0;
                      break;
               case 'h': /* print usage */
                     argv[i][1] = '*';
                     print_usage();
                     exit (100);
                     break;
             }
        }
  }

  floorplan_init(filename);

  double t_start, t_end;
  if (parallel) {
    t_start = rtclock();
    compute_floorplan();
    t_end = rtclock();
    floorplan_end();
    fprintf(stdout, "Parallel Runtime: %0.6lfs\n", t_end - t_start);
  } else {
  //floorplan_init(filename);
    t_start = rtclock();
    compute_floorplan_seq();
    t_end = rtclock();
    floorplan_end();
    fprintf(stdout, "Sequential Runtime: %0.6lfs\n", t_end - t_start);
  }



  /*if (floorplan_verify() == 1) {
      fprintf(stdout, "Result: Successful\n");
  } else {
      fprintf(stdout, "Result: Unsuccessful\n");
  }*/


}