/**
 *  Copyright (c) 2011, Los Alamos National Security, LLC.
 *  All rights Reserved.
 *
 *  Copyright 2011. Los Alamos National Security, LLC. This software was produced 
 *  under U.S. Government contract DE-AC52-06NA25396 for Los Alamos National 
 *  Laboratory (LANL), which is operated by Los Alamos National Security, LLC 
 *  for the U.S. Department of Energy. The U.S. Government has rights to use, 
 *  reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR LOS 
 *  ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR 
 *  ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is modified
 *  to produce derivative works, such modified software should be clearly marked,
 *  so as not to confuse it with the version available from LANL.
 *
 *  Additionally, redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Los Alamos National Security, LLC, Los Alamos 
 *       National Laboratory, LANL, the U.S. Government, nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE LOS ALAMOS NATIONAL SECURITY, LLC AND 
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT 
 *  NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL
 *  SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *  
 *  CLAMR -- LA-CC-11-094
 *  This research code is being developed as part of the 
 *  2011 X Division Summer Workshop for the express purpose
 *  of a collaborative code for development of ideas in
 *  the implementation of AMR codes for Exascale platforms
 *  
 *  AMR implementation of the Wave code previously developed
 *  as a demonstration code for regular grids on Exascale platforms
 *  as part of the Supercomputing Challenge and Los Alamos 
 *  National Laboratory
 *  
 *  Authors: Bob Robey       XCP-2   brobey@lanl.gov
 *           Neal Davis              davis68@lanl.gov, davis68@illinois.edu
 *           David Nicholaeff        dnic@lanl.gov, mtrxknight@aol.com
 *           Dennis Trujillo         dptrujillo@lanl.gov, dptru10@gmail.com
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "display.h"                                                                                                   
#ifdef HAVE_OPENGL

#define ESCAPE 27
#define NCOLORS 1000
#define WINSIZE 800.0

int DrawString(float x, float y, float z, char* string);
void InitGL(int width, int height);
void DrawSquares(void);
void DrawBoxes(void);
void SelectionRec(void);
void mouseClick(int button, int state, int x, int y);
void mouseDrag(int x, int y);
void keyPressed(unsigned char key, int x, int y);
void Scale();

struct ColorTable {
   float Red;
   float Green;
   float Blue;
};

int autoscale = 0;

double display_circle_radius=-1.0;

struct ColorTable Rainbow[NCOLORS];

real xstart, ystart, xend, yend;
enum mode_choice {EYE, MOVE, DRAW};
int mode = MOVE;

int window;
real width;
real display_xmin=0.0, display_xmax=0.0, display_ymin=0.0, display_ymax=0.0;

#ifdef HAVE_OPENGL
GLfloat xrot = 0, yrot = 0, xloc = 0, zloc = 0;
#endif

int display_outline;
int display_view_mode = 0;
int display_mysize    = 0;
real *x=NULL, *y=NULL, *dx=NULL, *dy=NULL;
real *data=NULL;
int *display_proc=NULL;

int DrawString(float x, float y, float z, char* string) {
   char *c;
   glColor3f(0.0f, 0.0f, 0.0f);
   glRasterPos3f(x, y, z);
   for(c = string; *c != '\0'; c++) {
      glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
      //glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
   }
   return 1;
}
void InitGL(int width, int height) {
   glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
   glDepthFunc(GL_LESS);
   glShadeModel(GL_SMOOTH);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
}
void init_display(int *argc, char **argv, const char *title){
   Scale();
   width = (WINSIZE / (display_ymax - display_ymin)) * (display_xmax - display_xmin);
   glutInit(argc, argv);
   glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
   glutInitWindowSize(width, WINSIZE);
   glutInitWindowPosition(50, 50);
   window = glutCreateWindow(title);

   glutDisplayFunc(&DrawGLScene);
   glutKeyboardFunc(&keyPressed);
   glutMouseFunc(&mouseClick);
   glutMotionFunc(&mouseDrag);
   InitGL(width, WINSIZE);
}
void set_window(real display_xmin_in, real display_xmax_in, real display_ymin_in, real display_ymax_in){
   display_xmin = display_xmin_in;
   display_xmax = display_xmax_in;
   display_ymin = display_ymin_in;
   display_ymax = display_ymax_in;
}
void set_cell_data(real *data_in){
   data = data_in;
}
void set_cell_proc(int *display_proc_in){
   display_proc = display_proc_in;
}
void set_cell_coordinates(real *x_in, real *dx_in, real *y_in, real *dy_in){
   x = x_in;
   dx = dx_in;
   y = y_in;
   dy = dy_in;
}
void free_display(void){
   glutDestroyWindow(window);
}
void DisplayState(void) {
   double scaleMax = 25.0, scaleMin = 0.0;
   int i, color;
   //vector<real> &H = state->H;

   if (autoscale) {
      scaleMax=-1.0e30;
      scaleMin=1.0e30;
      for(i = 0; i<display_mysize; i++) {
         if (data[i] > scaleMax) scaleMax = data[i];
         if (data[i] < scaleMin) scaleMin = data[i];
      }
   }

   double step = NCOLORS/(scaleMax - scaleMin);
  
   //set up 2D viewing
   gluOrtho2D(display_xmin, display_xmax, display_ymin, display_ymax);
  
   for(i = 0; i < display_mysize; i++) {
      /*Draw filled cells with color set by state value*/
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_QUADS);

      color = (int)(data[i]-scaleMin)*step;
      if (color < 0) {
         color=0;
      }
      if (color >= NCOLORS) color = NCOLORS-1;
   
      glColor3f(Rainbow[color].Red, Rainbow[color].Green, Rainbow[color].Blue);
   
      glVertex2f(x[i],       y[i]);
      glVertex2f(x[i]+dx[i], y[i]);
      glVertex2f(x[i]+dx[i], y[i]+dy[i]);
      glVertex2f(x[i],       y[i]+dy[i]);
      glEnd();
   
      /*Draw cells again as outline*/
      if (display_outline) {
         glColor3f(0.0f, 0.0f, 0.0f);
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
         glBegin(GL_QUADS);
         glVertex2f(x[i],       y[i]);
         glVertex2f(x[i]+dx[i], y[i]);
         glVertex2f(x[i]+dx[i], y[i]+dy[i]);
         glVertex2f(x[i],       y[i]+dy[i]);
         glEnd();
      }
   }
  
   glColor3f(0.0f, 0.0f, 0.0f);

   /* Draw circle for initial conditions */
   if (display_circle_radius > 0.0) {
      double radians;
      glBegin(GL_LINE_LOOP);
      for (double angle = 0.0; angle <= 360.0; angle+=1.0) {
         radians=angle*3.14159/180.0;
         glVertex2f(sin(radians) * display_circle_radius,cos(radians) * display_circle_radius);
      }
      glEnd();
   }
  
}
void DrawSquares(void) {
   int i, color, step;
   if (display_proc == NULL) return;
   step = NCOLORS/(display_proc[display_mysize-1]+1);
   //step utilizes whole range of color, assumes last element of proc is last processor

   gluOrtho2D(display_xmin, display_xmax, display_ymin, display_ymax);
   //set up 2D viewing

   for(i = 0; i < display_mysize; i++) {
      /*Draw filled cells with color set by processor it is assigned to*/
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_QUADS);
         color = display_proc[i]*step;
         glColor3f(Rainbow[color].Red, Rainbow[color].Green, Rainbow[color].Blue);

         glVertex2f(x[i],       y[i]);
         glVertex2f(x[i]+dx[i], y[i]);
         glVertex2f(x[i]+dx[i], y[i]+dy[i]);
         glVertex2f(x[i],       y[i]+dy[i]);
      glEnd();

      /*Draw cells again as outline*/
      glColor3f(0.0f, 0.0f, 0.0f);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glBegin(GL_QUADS);
         glVertex2f(x[i],       y[i]);
         glVertex2f(x[i]+dx[i], y[i]);
         glVertex2f(x[i]+dx[i], y[i]+dy[i]);
         glVertex2f(x[i],       y[i]+dy[i]);
      glEnd();
   }
  
   /* Draw circle for initial conditions */
   if (display_circle_radius > 0.0) {
      double radians;
      glBegin(GL_LINE_LOOP);
      for (double angle = 0.0; angle <= 360.0; angle+=1.0) {
         radians=angle*3.14159/180.0;
         glVertex2f(sin(radians) * display_circle_radius,cos(radians) * display_circle_radius);
      }
      glEnd();
   }
  
   /*Trace order of cells with line going from center to center*/
   glBegin(GL_LINE_STRIP);
      for(i = 0; i < display_mysize; i++) {
         glVertex2f(x[i]+dx[i]/2, y[i]+dy[i]/2);
      }
   glEnd();
}
void DrawBoxes(void) {

   int i, color, step = NCOLORS/(display_proc[display_mysize-1]+1);
   //step utilizes whole range of color, assumes last element of proc is last processor

   glFrustum(display_xmin-1, display_xmax, display_ymin-1, display_ymax, 5.0, 10.0);
   glTranslatef(0.0f, 0.0f, -6.0f); //must move into screen to draw

   for(i = 0; i < display_mysize; i++) {
      /*Draw filled cells with color set by processor*/
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBegin(GL_QUADS);
         color = display_proc[i]*step;
         glColor3f(Rainbow[color].Red, Rainbow[color].Green, Rainbow[color].Blue);

         /*Front Face*/
         glVertex3f(x[i],       y[i],       0.0f);
         glVertex3f(x[i]+dx[i], y[i],       0.0f);
         glVertex3f(x[i]+dx[i], y[i]+dy[i], 0.0f);
         glVertex3f(x[i],       y[i]+dy[i], 0.0f);
         /*Right Face*/
         glVertex3f(x[i]+dx[i], y[i],       0.0f);
         glVertex3f(x[i]+dx[i], y[i]+dy[i], 0.0f);
         glVertex3f(x[i]+dx[i], y[i]+dy[i],-1.0f);
         glVertex3f(x[i]+dx[i], y[i],      -1.0f);
         /*Back Face*/
         glVertex3f(x[i]+dx[i], y[i],      -1.0f);
         glVertex3f(x[i]+dx[i], y[i]+dy[i],-1.0f);
         glVertex3f(x[i],       y[i]+dy[i],-1.0f);
         glVertex3f(x[i],       y[i],      -1.0f);
         /*Left Face*/
         glVertex3f(x[i],       y[i],       0.0f);
         glVertex3f(x[i],       y[i],      -1.0f);
         glVertex3f(x[i],       y[i]+dy[i],-1.0f);
         glVertex3f(x[i],       y[i]+dy[i], 0.0f);
         /*Bottom*/
         glVertex3f(x[i],       y[i],       0.0f);
         glVertex3f(x[i],       y[i],      -1.0f);
         glVertex3f(x[i]+dx[i], y[i],      -1.0f);
         glVertex3f(x[i]+dx[i], y[i],       0.0f);
         /*Top*/
         glVertex3f(x[i],       y[i]+dy[i], 0.0f);
         glVertex3f(x[i],       y[i]+dy[i],-1.0f);
         glVertex3f(x[i]+dx[i], y[i]+dy[i],-1.0f);
         glVertex3f(x[i]+dx[i], y[i]+dy[i], 0.0f);
      glEnd();
   }
}



void set_viewmode(int display_view_mode_in){
   display_view_mode = display_view_mode_in;
}
void set_mysize(int display_mysize_in){
   display_mysize = display_mysize_in;
}
void set_circle_radius(double display_circle_radius_in){
   display_circle_radius = display_circle_radius_in;
}
void set_outline(int display_outline_in){
   display_outline = display_outline_in;
}
void DrawGLScene(void) {

   char c[10];

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   if (display_view_mode == 0) {
      DrawSquares();
   } else {
      DisplayState();
   }

   /*for(int i = 0; i < display_mysize; i++) {
      sprintf(c, "%d", i);
      //DrawString(x[i]+dx[i]/2, y[i]+dy[i]/2, 0.0, c);
   }*/
   if(mode == DRAW) {
      SelectionRec();
   }

   glLoadIdentity();

   glutSwapBuffers();

   sleep(0);

}


void SelectionRec(void) {
   glColor3f(0.0f, 0.0f, 0.0f);
   glLineWidth(2.0f);
   glBegin(GL_QUADS);
      glVertex2f(xstart, ystart);
      glVertex2f(xstart, yend);
      glVertex2f(xend, yend);
      glVertex2f(xend, ystart);
   glEnd();
   glLineWidth(1.0f);
}

void mouseClick(int button, int state, int x, int y) {
   if(state == GLUT_DOWN) {
      mode = EYE;
      xstart = (display_xmax-display_ymin)*(x/width)+display_ymin;
      ystart = (display_ymax-display_ymin)*((float)(WINSIZE-y)/WINSIZE)+display_ymin;
   }
   if(state == GLUT_UP) {
      glutPostRedisplay();
      mode = DRAW;
      xend = (display_xmax-display_ymin)*(x/width)+display_ymin;
      yend = (display_xmax-display_ymin)*((float)(WINSIZE-y)/WINSIZE)+display_ymin;
   }
}
void mouseDrag(int x, int y) {
   glutPostRedisplay();
   mode = DRAW;
   xend = (display_xmax-display_ymin)*(x/width)+display_ymin;
   yend = (display_xmax-display_ymin)*((float)(WINSIZE-y)/WINSIZE)+display_ymin;
}

void keyPressed(unsigned char key, int x, int y) {

    usleep(100);

    if(key == ESCAPE) {
       glutDestroyWindow(window);
#ifdef HAVE_OPENCL
       ezcl_finish();
#endif
       exit(0);
    }
    if(key == 'm')   { mode = MOVE; }
    if(key == 'e')   { mode = EYE; }
    if(mode == EYE) {
      if(key == 'o') { xrot -= 5.0; }
      if(key == 'l') { xrot += 5.0; }
      if(key == 'k') { yrot -= 5.0; }
      if(key == ';') { yrot += 5.0; }
    }
    if(mode == MOVE) {
      if(key == 'o') { zloc += 1.0; }
      if(key == 'l') { zloc -= 1.0; }
      if(key == 'k') { xloc += 1.0; }
      if(key == ';') { xloc -= 1.0; }
    }
}
void Scale() {
   int i;
   double r;
   for (i=0, r=0.0; i<200; i++, r+=.005) {
         Rainbow[i].Red = 0.0;
         Rainbow[i].Green = r;
         Rainbow[i].Blue = 1.0;
   }
   for (i=0, r=1.0; i<200; i++, r-=.005) {
         Rainbow[200+i].Red = 0.0;
         Rainbow[200+i].Green = 1.0;
         Rainbow[200+i].Blue = r;
   }
   for (i=0, r=0.0; i<200; i++, r+=.005) {
         Rainbow[400+i].Red = r;
         Rainbow[400+i].Green = 1.0;
         Rainbow[400+i].Blue = 0.0;
   }
   for (i=0, r=1.0; i<200; i++, r-=.005) {
         Rainbow[600+i].Red = 1.0;
         Rainbow[600+i].Green = r;
         Rainbow[600+i].Blue = 0.0;
   }
   for (i=0, r=0.0; i<200; i++, r+=.005) {
         Rainbow[800+i].Red = 1.0;
         Rainbow[800+i].Green = 0.0;
         Rainbow[800+i].Blue = r;
   }
}


#endif
