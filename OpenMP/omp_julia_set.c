# include <stdio.h>
# include <stdlib.h>
# include <omp.h>
# include <time.h>

int main ( );
unsigned char *julia_rgb ( int w, int h, float xl, float xr, float yb, float yt );
int julia_point ( int w, int h, float xl, float xr, float yb, float yt, int i, 
  int j );
void tga_write ( int w, int h, unsigned char rgb[], char *filename );
void timestamp ( );

/******************************************************************************/

int main ( ) 

/******************************************************************************/
/*
  Purpose:

    MAIN is the main program for the parallelized program OMP_JULIA_SET.

  Discussion:

    Consider points (X,Y) in a rectangular domain R = [XL,XR]x[YB,YT].

    Let Z be the complex number X+Yi, and let C be some complex constant.

    Let Z(0) = Z, Z(k+1) = Z(k)^2 + C

    The Julia set is the set of points Z in R with the property that
    the sequence of points Z(k) remain within R.

    To compute a picture of the Julia set, we choose a discrete array
    of WxH points in R.  We carry out up to 200 steps of the iteration for
    each point Z.  If 1000 < |Z| at any time, we assume Z is not in the
    Julia set.

  Licensing:

    This code is distributed under the GNU LGPL license. 

  Modified:

    6 March 2017

  Parameters:

    Local, int H, W, the height and width of the region in pixels.

    Local, float XL, XR, YB, YT, the left and right X limits, the
    bottom and top Y limits, of the region.

    Local, unsigned char *RGB, will hold W*H*3 values between 0 and 255,
    specifying the pixel color values.

OpenMP Modification:
30 June 2020 by Juan Arango, Universidad Industrial de Santander juan.arango2@correo.uis.edu.co                  
  This OpenMP Modification implements a parallelization of the original Code in its function 'julia_rgb()'.  
*/
{
  int h = 1000;
  unsigned char *rgb;
  int w = 1000;
  float xl = - 1.5;
  float xr = + 1.5;
  float yb = - 1.5;
  float yt = + 1.5;
  double wtime; // Se declara la varaible wtime para tomar el tiempo

  timestamp ( );
  printf ( "\n" );
  printf ( "OMP_JULIA_SET:\n" );
  printf ( "  C version.\n" );
  printf ( "  Plot a version of the Julia set using parallel programming for Z(k+1)=Z(k)^2-0.8+0.156i\n" );

/* 
Se hace uso de la función propia de omp omp_get_wtime() para establecer un inicio del momento antes de correr la función de mayor complejidad en el programa, para al final de su ejecución tomar de nuevo el tiempo y así averiguar el tiempo de ejecución que tomó esta función la cual está paralelizada.
*/

  wtime = omp_get_wtime ( ); 
  rgb = julia_rgb ( w, h, xl, xr, yb, yt );

/*
Se toma de nuevo el tiempo después de la ejecución de la función julia_rbg() y se calcula la duración total de la función, indicando la cantidad de hilos usados.
 */

  wtime = omp_get_wtime ( ) - wtime;  


  tga_write ( w, h, rgb, "julia_set.tga" );
/*
  Free memory.
*/
  free ( rgb );
/*
  Terminate.
*/
  printf ( "\n" );
  printf ( "OMP_JULIA_SET:\n" );
  printf ( "  Normal end of execution.\n" );
  printf ( "\n" );
/*
Finalmente la impresión de los resultados obtenidos con la ayuda de omp_get_max_threads() para obtener la cantidad de hilos empleados y el tiempo wtime calculado anteriormente.
*/

  printf ( "  The main parallelized function  was executed using %d threads, in the time of %g seconds \n", omp_get_max_threads ( ), wtime ); 

  timestamp ( );

  return 0;
}
/******************************************************************************/

unsigned char *julia_rgb ( int w, int h, float xl, float xr, float yb, float yt )

/******************************************************************************/
/*
  Purpose:

    JULIA_RGB applies JULIA to each point in the domain.

  Licensing:

    This code is distributed under the GNU LGPL license. 

  Modified:

    06 March 2017

  Parameters:

    Input, int W, H, the width and height of the region in pixels.

    Input, float XL, XR, YB, YT, the left, right, bottom and top limits.

    Output, unsigned char *JULIA_SET[W*H*3], the B, G, R values,
    between 0 and 255, of a plot of the Julia set.  We want
    [0,0,255] for points in the set (red), and [255,255,255]
    for points not in the set (white).

OpenMP Modification:
30 June 2020 by Juan Arango, Universidad Industrial de Santander juan.arango2@correo.uis.edu.co                  
  This OpenMP Modification makes a parallelization of the original Code in its function 'julia_rgb()'. 

The code below implements a use of parallelization specifically in its for loops due its high computational cost for being two followed loops and may be working on a two dimensional array O(n^2).
*/
{
  int i;
  int j;
  int juliaValue;
  int k;
  unsigned char *rgb;

  k = 0;
  rgb = ( unsigned char * ) malloc ( w * h * 3 * sizeof ( int ) );

/* 
pragma utilizado para llamar a la paralelizacion que realiza omp, en este caso se hace uso de ello en esta función ( julia_rgb() ) para trabajar paralelizadamente el empleo de dos bucles for anidados.

A continuacion su sintaxis para hacer referencia a omp incluyendo sus atributos privados y compartidos o parámetros recibidos.
*/

# pragma omp parallel \
  shared (w,h,xl,xr,yb,yt) \
  private (i,j,juliaValue,k) 
  {
# pragma omp for

	  for ( j = 0; j < h; j++ )
	  {
	    for ( i = 0; i < w; i++ )
	    {
	      juliaValue = julia_point ( w, h, xl, xr, yb, yt, i, j );
	/*
	  Demented format orders colors B/G/R!
	*/
		
/*
Propuesta del puntero k obtenido de una propuesta a la implementación en OMP en http://people.math.sc.edu/ pues el originalmente propuesto en la lógica era totalmente válida, sin embargo, al momento de ejecutar el for con 'omp for' arrojaba errores de violación de segmento.
*/	     
  	      k = 3 * ( j * w + i );	    
		
	      rgb[k]   = 255 * ( 1 - juliaValue );
	      rgb[k+1] = 255 * ( 1 - juliaValue );
	      rgb[k+2] = 255;
	      
	      
	    }
	  }
	}
  return rgb;
}
/******************************************************************************/

int julia_point ( int w, int h, float xl, float xr, float yb, float yt, int i, int j )

/******************************************************************************/
/*
  Purpose:

    JULIA_POINT returns 1 if a point is in the Julia set.

  Discussion:

    The iteration Z(k+1) = Z(k) + C is used, with C=-0.8+0.156i.

  Licensing:

    This code is distributed under the GNU LGPL license. 

  Modified:

    06 March 2017

  Parameters:

    Input, int W, H, the width and height of the region in pixels.

    Input, float XL, XR, YB, YT, the left, right, bottom and top limits.

    Input, int I, J, the indices of the point to be checked.

    Ouput, int JULIA, is 1 if the point is in the Julia set.
*/
{
  float ai;
  float ar;
  float ci = 0.156;
  float cr = -0.8;
  int k;
  float t;
  float x;
  float y;
/*
  Convert (I,J) indices to (X,Y) coordinates.
*/
  x = ( ( float ) ( w - i - 1 ) * xl
      + ( float ) (     i     ) * xr ) 
      / ( float ) ( w     - 1 );

  y = ( ( float ) ( h - j - 1 ) * yb
      + ( float ) (     j     ) * yt ) 
      / ( float ) ( h     - 1 );
/*
  Think of (X,Y) as real and imaginary components of
  a complex number A = x + y*i.
*/
  ar = x;
  ai = y;
/*
  A -> A * A + C
*/
  for ( k = 0; k < 200; k++ )
  {
    t  = ar * ar - ai * ai + cr;
    ai = ar * ai + ai * ar + ci;
    ar = t;
/*
  if 1000 < ||A||, reject the point.
*/
    if ( 1000 < ar * ar + ai * ai )
    {
      return 0;
    }
  }

  return 1;
}
/******************************************************************************/

void tga_write ( int w, int h, unsigned char rgb[], char *filename )

/******************************************************************************/
/*
  Purpose:

    TGA_WRITE writes a TGA or TARGA graphics file of the data.

  Licensing:

    This code is distributed under the GNU LGPL license.

  Modified:

    06 March 2017

  Parameters:

    Input, int W, H, the width and height of the image.

    Input, unsigned char RGB[W*H*3], the pixel data.

    Input, char *FILENAME, the name of the file to contain the screenshot.
*/
{
  FILE *file_unit;
  unsigned char header1[12] = { 0,0,2,0,0,0,0,0,0,0,0,0 };
  unsigned char header2[6] = { w%256, w/256, h%256, h/256, 24, 0 };
/* 
  Create the file.
*/
  file_unit = fopen ( filename, "wb" );
/*
  Write the headers.
*/
  fwrite ( header1, sizeof ( unsigned char ), 12, file_unit );
  fwrite ( header2, sizeof ( unsigned char ), 6, file_unit );
/*
  Write the image data.
*/
  fwrite ( rgb, sizeof ( unsigned char ), 3 * w * h, file_unit );
/*
  Close the file.
*/
  fclose ( file_unit );

  printf ( "\n" );
  printf ( "TGA_WRITE:\n" );
  printf ( "  Graphics data saved as '%s'\n", filename );

  return;
}
/******************************************************************************/

void timestamp ( )

/******************************************************************************/
/*
  Purpose:

    TIMESTAMP prints the current YMDHMS date as a time stamp.

  Example:

    17 June 2014 09:45:54 AM

  Licensing:

    This code is distributed under the GNU LGPL license. 

  Modified:

    17 June 2014

  Author:

    John Burkardt

  Parameters:

    None
*/
{
# define TIME_SIZE 40

  static char time_buffer[TIME_SIZE];
  const struct tm *tm;
  time_t now;

  now = time ( NULL );
  tm = localtime ( &now );

  strftime ( time_buffer, TIME_SIZE, "%d %B %Y %I:%M:%S %p", tm );

  printf ( "%s\n", time_buffer );

  return;
# undef TIME_SIZE
}

