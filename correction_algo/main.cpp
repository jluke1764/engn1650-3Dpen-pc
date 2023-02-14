#include <ctime>
#include <iostream>
#include <math.h>
/*
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define CHANNEL_NUM 3
#include "Image.cpp"
*/
#include <fstream>

using namespace std;

typedef struct Point3D {
    float x;
    float y;
    float z;
} Point3D;
static Point3D ihaf;

typedef struct rgba {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} RGBA;

int main()
{
   int width = 1024;
   int height = 768;
   int bpp;
   RGBA img[width][height];
   ifstream image;
   image.open("test.png",std::ios_base::binary);
   if (image.is_open()) cout<< image << endl;
   
   int BLOCK_SIZE = width * height;
   char buffer[BLOCK_SIZE];

   memset(buffer, 0, BLOCK_SIZE);
   image.read(buffer, BLOCK_SIZE);

   cout << " | buffer size: " << image.gcount() << std::endl;
   for(int i=0; i<BLOCK_SIZE; i++) cout << buffer[i];

   /*
   MImage img("test.png");
   img.write("output2.jpg");

   uint8_t* rgb_image;
   rgb_image = (uint8_t*)malloc(width*height*CHANNEL_NUM);
   stbi_load("test.png", &width, &height, &bpp, CHANNEL_NUM);
   stbi_write_png("output.png", width, height, CHANNEL_NUM, rgb_image,  width * CHANNEL_NUM);
   stbi_image_free(rgb_image);
   */

   return 0;
}

void getperpvec (Point3D n, Point3D a, Point3D b)
{
   float f;
   if (fabs(n.x) < fabs(n.y))
   {
      f = 1/sqrt(n.y*n.y + n.z*n.z);
      a.x = 0;
      a.y = n.z*f;
      a.z =-n.y*f;
   }
   else
   {
      f = 1/sqrt(n.x*n.x + n.z*n.z);
      a.x = -n.z*f;
      a.y =     0;
      a.z = n.x*f;
   }
   b.x = n.y*a.z - n.z*a.y;
   b.y = n.z*a.x - n.x*a.z;
   b.z = n.x*a.y - n.y*a.x;
   f = 1/sqrt(b.x*b.x + b.y*b.y + b.z*b.z);
   b.x *= f;
   b.y *= f;
   b.z *= f;
}

void ang2mat (float hang, float vang, float mat[9])
{
   mat[6] = cos(vang)*sin(hang); mat[0] = cos(hang);
   mat[7] = sin(vang);           mat[1] = 0;
   mat[8] = cos(vang)*cos(hang); mat[2] =-sin(hang);
   mat[3] = mat[7]*mat[2] - mat[8]*mat[1];
   mat[4] = mat[8]*mat[0] - mat[6]*mat[2];
   mat[5] = mat[6]*mat[1] - mat[7]*mat[0];
}
//-----------------------------------------------------------------------------
void sph2conic (float cx, float cy, float cz, float  &a, float &b, float &c, float &d, float &e,float &f)
{
   float cx2 = cx*cx; float cy2 = cy*cy; float cz2 = cz*cz;
   a = 1-cy2-cz2; e = cy*cz*2;
   c = 1-cx2-cz2; d = cx*cz*2;
   f = 1-cx2-cy2; b = cx*cy*2;
   
      //Xform conic from {-1..1} to screen coord: x = x*ihaf.z+ihaf.x; y = y*ihaf.z+ihaf.y;
   e = e*ihaf.z - c*ihaf.y*2 - b*ihaf.x;
   d = d*ihaf.z - a*ihaf.x*2 - b*ihaf.y;
   f = f*ihaf.z*ihaf.z - a*ihaf.x*ihaf.x - b*ihaf.x*ihaf.y - c*ihaf.y*ihaf.y - d*ihaf.x - e*ihaf.y;
}