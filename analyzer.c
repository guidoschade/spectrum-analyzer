#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>          // file descriptor functions
#include <alsa/asoundlib.h> // sound library
#include <math.h>           // fabs function

#include <linux/fb.h>       // framebuffer
#include <sys/mman.h>       // memory map
#include <sys/ioctl.h>      // ioctl
#include <linux/kd.h>       // tty
#include <linux/input.h>    // input for touch screen

#include "font.h"           // font

// fourier transform headers
int cfast(float *, long *);    // FFT
 
typedef short pixel_t;      // Pixel specification 16bpp
typedef pixel_t* fbp_t;     // Framebuffer pointer
fbp_t fbp;

unsigned char *numbers_small[10]; // 10 numbers
unsigned int num_pixels = 0;

int nb = 10; // number of bars to display

// array with bar values - bar values, display bars, previous bars
unsigned int  bars[10] = { 0,0,0,0,0,0,0,0,0,0 }; // array which represents the nb bars with 0-100 level
unsigned int dbars[10] = { 10,10,10,10,10,10,10,10,10,10 }; // display bars
unsigned int pbars[10] = { 10,10,10,10,10,10,10,10,10,10 }; // previous bars (for deleting blocks)
unsigned long nums[10] = { 2,3,4,6,12,24,48,96,180,350 }; // centre positions for each bar
float   multiplier[10] = { 6,6,6,6,6,6,6,9,10,20 }; // multiplier to even out maximum for each bar

int mode = 0;
int doublebars = 1;
int efd; // event file descriptor

unsigned int col_dots, col_text, col_norm_bars, col_warn_bars, col_err_bars, col_norm_bars_bk, col_warn_bars_bk, col_err_bars_bk;

// opening touch screen event device
int open_touch_screen()
{
  char name[256] = "Unknown";
  if ((efd = open("/dev/input/event1", O_RDONLY)) < 0) { return 1; }

  // get flags and set to nonblocking
  int flags = fcntl(efd, F_GETFL, 0);
  fcntl(efd, F_SETFL, flags | O_NONBLOCK);

  // get device name string
  ioctl(efd, EVIOCGNAME(sizeof(name)), name);
  printf("Input device name: \"%s\"\n", name);

  return 0;
}

// get touch event - if any
int get_touch_event()
{
  int i;
  size_t rb;
  struct input_event ev[64];

  // printf("waiting for event ...\n");
  rb=read(efd,ev,sizeof(struct input_event)*64);
  if (rb != -1)
  {
    //printf("received event (rb = %d) ...\n", rb);
    for (i=0; i<(rb/sizeof(struct input_event)); i++) { if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1) { return 1; } }
    // printf("ev(%d) type = %d\n", i, ev[i].type); }
    return 1;
  }
  return 0;
}

// set pixel
void setpixel (int x, int y, int col) { fbp[y*480+x] = col; }

// draw number
void draw_num (int n, int x, int y, int col)
{
  int xc=x,yc=y,xi,yi;

  for (yi=0;yi<14;yi++) {
    for (xi=0;xi<10;xi++) {
      if (numbers_small[n][xi+10*yi]) { setpixel(xc,yc,col); }
      xc++;
    }
    yc++; xc=x;
  }
}

// draw k
void draw_k (int x, int y, int col)
{
  int xc=x,yc=y,xi,yi;

  for (yi=0;yi<14;yi++) {
    for (xi=0;xi<10;xi++) {
      if (K__10x14[xi+10*yi]) { setpixel(xc,yc,col); }
      xc++;
    }
    yc++; xc=x;
  }
}

// draw rectangle - or line with width 1
void rectangle (int x1, int y1, int x2, int y2, int col)
{
  int x, y, ys=y1*480;
  for (y=y1; y<=y2; y++) {
    for (x=x1; x<=x2; x++) { fbp[ys+x] = col; }
    ys+=480;
  }
}

// draw bars
void draw_bars(int gcol, int gdcol, int ycol, int ydcol, int rcol, int rdcol)
{
  int b, m, mc, bc, col;

  // nb frequencies, 480 pixels wide, 10 bars up
  for (m=0;m<nb;m++) {
    if (dbars[m] > pbars[m]) {
      mc = m*42;
      for (b=pbars[m];b<dbars[m];b++) {
        bc = b*20;
        col = gcol; if (b>6) { col = ycol; } ; if (b>7) { col = rcol; }
/*
        if (b == 0) { col = 9; }
        if (b == 1) { col = 11; }
        if (b == 2) { col = 13; }
        if (b == 3) { col = 15; }
        if (b == 4) { col = 17; }
        if (b == 5) { col = 19; }
        if (b == 6) { col = 21; }
        if (b == 7) { col = 24; }
        if (b == 8) { col = 26; }
        if (b == 9) { col = 29; }
        if (b == 10) { col = 31; }
*/
        if (doublebars) {
          rectangle (mc+40,230-bc,mc+52,240-bc,col); 
          rectangle (mc+58,230-bc,mc+70,240-bc,col); 
        } else { rectangle (mc+35,230-bc,mc+70,240-bc,col); };
      }
    }
  }

  // clear old bars not in use anymore - draw over darker
  for (m=0;m<nb;m++) {
    if (pbars[m] > dbars[m]) {
      mc = m*42;
      for (b=dbars[m];b<pbars[m];b++) {
        bc = b*20;
        col = gdcol; if (b>6) { col = ydcol; } ; if (b>7) { col = rdcol; }
        rectangle (mc+35,230-bc,mc+70,240-bc,col); 
      }
    }
    pbars[m] = dbars[m]; // set previous bar to be display bar after clearing
  }
}

// init screen
void init_screen()
{
  int i,m;

  printf("num_pixels:%d mode:%d\n", num_pixels, mode);

  if (mode == 2) // normal
  {
    col_dots = 65535; // white
    col_text = 65535; // white
    col_norm_bars = 2016; // green
    col_norm_bars_bk = 0; // 288; // dark green
    col_warn_bars = 63270; // yellow
    col_warn_bars_bk =  0; //16704; // dark yellow or brown
    col_err_bars = 63488; // red
    col_err_bars_bk =  0; //16384; // dark red
  }

  if (mode == 1) // blue
  {
    col_dots = 65535; // white
    col_text = 65535; // white
    col_norm_bars = 10719; // light blue
    col_norm_bars_bk = 11; // dark blue
    col_warn_bars = 10719; // light blue
    col_warn_bars_bk = 11; // dark blue
    col_err_bars = 10719; // light blue
    col_err_bars_bk = 11; // dark blue
  }

  if (mode == 3) // red
  {
    col_dots = 65535; // white
    col_text = 65535; // white
    col_norm_bars = 63488; // light red
    col_norm_bars_bk = 16384; // dark red
    col_warn_bars = 63488; // light red
    col_warn_bars_bk = 16384; // dark red
    col_err_bars = 63488; // light red
    col_err_bars_bk = 16384; // dark red
  }

  if (mode == 0) // white
  {
    col_dots = 65535; // white
    col_text = 65535; // white
    col_norm_bars = 65535; // white
    col_norm_bars_bk = 16904; // grey
    col_warn_bars = 65535; // white
    col_warn_bars_bk = 16904; // grey
    col_err_bars = 65535; // white
    col_err_bars_bk = 16904; // grey
  }

  // black background
  for (i=0; i<num_pixels; i++) { fbp[i] = 0; }

  // bottom row
  int mc = 0;
  for (m=0;m<nb;m++)
  {
    mc = m*42;
    if (doublebars)
    {
      rectangle (mc+40,250,mc+52,260,col_norm_bars);
      rectangle (mc+58,250,mc+70,260,col_norm_bars);
    }
    else { rectangle (m*42+35,250,m*42+70,260,col_norm_bars); }
  }

  // dots left and right hand side
  for (m=0;m<11;m++) { rectangle (10,m*20+52,12,m*20+54,col_text); rectangle (470,m*20+52,472,m*20+54,col_dots); }

  // 20 Hz, 90 Hz, 375 Hz, 1.5k, 6.0k
  draw_num(2,40,275,col_text); draw_num(0,53,275,col_text);
  draw_num(9,125,275,col_text); draw_num(0,138,275,col_text);
  draw_num(3,205,275,col_text); draw_num(7,217,275,col_text); draw_num(5,229,275,col_text);
  draw_num(1,292,275,col_text); draw_k(304,275,col_text); draw_num(5,316,275,col_text);
  draw_num(6,377,275,col_text); draw_k(390,275,col_text);

  // 45 Hz, 180 Hz, 750 Hz, 3k, 12k
  draw_num(4,83,275,col_text); draw_num(5,96,275,col_text);
  draw_num(1,163,275,col_text); draw_num(8,171,275,col_text); draw_num(0,183,275,col_text);
  draw_num(7,249,275,col_text); draw_num(5,261,275,col_text); draw_num(0,273,275,col_text);
  draw_num(3,340,275,col_text); draw_k(353,275,col_text);
  draw_num(1,410,275,col_text); draw_num(2,423,275,col_text); draw_k(436,275,col_text);

  // full throttle - all bars to 10 and display initially
  for (i=0; i<nb; i++) { dbars[i] = 10; pbars[i] = 0; }
}

// #############################################
// ##              MAIN                       ##
// #############################################
int main (int argc, char *argv[])
{
  int err = 0;

  // number bitmaps
  numbers_small[0] = AR0__10x14; numbers_small[1] = AR1__10x14; numbers_small[2] = AR2__10x14; numbers_small[3] = AR3__10x14;
  numbers_small[4] = AR4__10x14; numbers_small[5] = AR5__10x14; numbers_small[6] = AR6__10x14; numbers_small[7] = AR7__10x14;
  numbers_small[8] = AR8__10x14; numbers_small[9] = AR9__10x14;

  long points = 1024; // samples to use
  float vec[points];
  int i;

  int peakval = 150000; // maximum absolute value

  int buffer_frames = (int) points;
  unsigned int rate = 44100; // sample rate
  unsigned long int screensize; // screen size in bytes = x * y * bpp / 8

  signed char *buffer = NULL; // buffer for audio samples

  snd_pcm_t *capture_handle = NULL;
  snd_pcm_hw_params_t *hw_params = NULL;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

  int fbfd; // framebuffer file descriptor
  int ttyfd; // tty file descriptor - if we can't open the tty because it is not writeable then we'll just leave it in text mode

  // structures for important fb information
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;

  // opening touch screen event file descriptor
  open_touch_screen();

  // open framebuffer
  fbfd = open("/dev/fb1", O_RDWR);
  if(fbfd == -1) { printf("Error: cannot open framebuffer device"); exit(1); }

  // set the tty to graphics mode - Get fixed screen information
  if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) { printf("Error reading fixed information"); exit(2); }

  // get variable screen information
  if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) { printf("Error reading variable information"); exit(3); }

  printf("framebuffer: %s %dx%d, %dbpp\n", finfo.id, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
  
  screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8 ;
  printf("%lu bytes\n", screensize);
  
  // memory mapping frame buffer
  fbp = (fbp_t)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
  if ((int)fbp == -1) { printf("Error: failed to map framebuffer device to memory\n"); exit(4); }

  // attempt to open the tty and set it to graphics mode
  ttyfd = open("/dev/tty1", O_RDWR);
  if (ttyfd == -1) { printf("Error: could not open the tty\n"); }
  else{ ioctl(ttyfd, KDSETMODE, KD_GRAPHICS); }
  
  num_pixels = screensize / sizeof ( pixel_t );
  printf("number of pixels: %d\n", num_pixels);

  // clear screen, draw scale etc.
  init_screen();

  // setting bars and previous bars again
  for (i=0; i<nb; i++) { dbars[i] = 10; pbars[i] = 0; }

  // SOUND: opening sound handle
  if ((err = snd_pcm_open (&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) { fprintf (stderr, "cannot open audio device %s (%s)\n", argv[1], snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) { fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) { fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) { fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) { fprintf (stderr, "cannot set sample format (%s)\n",  snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) { fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) { fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err)); exit (1); }
  if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) { fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err)); exit (1); }
  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (capture_handle)) < 0) { fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err)); exit (1); }

  // allocate memory for buffer
  buffer = malloc(buffer_frames * snd_pcm_format_width(format) / 8 * 2);
  if (!buffer) { fprintf(stderr, "buffer allocation failed\n"); exit (1); }

  float multi = 0.7; // multplier for surrounding spectrum frequency slots
  int idx = 0;

  unsigned long count = 0;
  int prev, next, z, a;

  // main sampling loop
  while (1) 
  {
    count++;
    prev = 0; next = 0;

    // read audio
    if ((err = snd_pcm_readi (capture_handle, buffer, buffer_frames)) != buffer_frames)
      { fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err)); exit (1); }

    // go through all frames received - only use most significant byte in new spectrum buffer
    for (z=0; z<(buffer_frames-1); z++) { vec[z] = (float) buffer[z*2+1]; }

    // spectrum analysis of buffer, storing result in vec
    cfast((float *)&vec, (long *)&points);

    // calculate bar levels based on spectrum
    for (i=0; i<nb; i++) {
      idx = nums[i];
      bars[i] = (int) fabs(((float)(vec[idx] * multiplier[i] / (float) peakval) * 100));

      // adding surrounding spectrum slots (up to x percent) at a lower percentage - next
      for (a=1;a<350;a++) { 
        next = idx+a;
        if (next >= (int) ((float) idx * 1.3)) { break; }
        bars[i] += (int) fabs(((float)(vec[next] * multiplier[i] * multi / (float) peakval) * 100));
      }

      // adding surrounding spectrum slots (up to x percent) at a lower percentage - previous
      for (a=1;a<350;a++) { 
        prev = idx-a;
        if (prev < 0) { continue; }
        if (prev <= (int) ((float) idx * 0.7)) { break; }
        bars[i] += (int) fabs(((float)(vec[prev] * multiplier[i] * multi / (float) peakval) * 100));
      }
    }

    // copy current bar count to bar display array (non linear)
    for (i=0; i<nb; i++)
    {
      if      (bars[i] >= 40) { if (dbars[i]<10) { dbars[i] = 10; } }
      else if (bars[i] >= 25) { if (dbars[i]<9)  { dbars[i] =  9; } }
      else if (bars[i] >= 16) { if (dbars[i]<8)  { dbars[i] =  8; } }
      else if (bars[i] >= 12) { if (dbars[i]<7)  { dbars[i] =  7; } }
      else if (bars[i] >=  8) { if (dbars[i]<6)  { dbars[i] =  6; } }
      else if (bars[i] >=  6) { if (dbars[i]<5)  { dbars[i] =  5; } }
      else if (bars[i] >=  4) { if (dbars[i]<4)  { dbars[i] =  4; } }
      else if (bars[i] >=  3) { if (dbars[i]<3)  { dbars[i] =  3; } }
      else if (bars[i] >=  2) { if (dbars[i]<2)  { dbars[i] =  2; } }
      else if (bars[i] >=  1) { if (dbars[i]<1)  { dbars[i] =  1; } }
    }

    // actually draw bars
    draw_bars(col_norm_bars,col_norm_bars_bk,col_warn_bars,col_warn_bars_bk,col_err_bars,col_err_bars_bk);

    // drop bar level down after some cycles (decay / fall time)
    if (!(count % 5)) { for (int z=0; z<nb; z++) { if (dbars[z]) { dbars[z]--; } } }

    if (count > 25)
    {
      count = 0;
      // check if screen was touched to update mode
      if (get_touch_event() == 1)
      {
        if (++mode > 3) { mode = 0; doublebars++; if (doublebars > 1) { doublebars=0; } }
        init_screen();
      }
    }
  }

  // Cleanup SOUND
  free(buffer);
  snd_pcm_close (capture_handle);

  // Cleanup DISPLAY - unmap the memory and release all the files
  munmap(fbp, screensize);
  if (ttyfd != -1) { ioctl(ttyfd, KDSETMODE, KD_TEXT); close(ttyfd); }
  close(fbfd); // frame buffer
  close(efd); // event
  exit (0);
}
