#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
//#include "/home/zimo/include"
#include "teem/meet.h"
#include "../jsmn/jsmn.h"
/* compile with:
gcc -std=c99 -Wall -O2 -o CoordShift CoordShift.c -I $TEEM/include -L $TEEM/lib -L ../jsmn -lteem -lpng -lz -ljsmn -lm -lbz2
gcc -std=c99 -Wall -O2 -o CoordSwap CoordSwap.c -I $TEEM/include -L $TEEM/lib -L ../jsmn -lteem -lpng -lz -ljsmn 
*/

/* 3-vector U = 3x3 matrix M time 3-vector V */
#define MV3_MUL(U, M, V)                               \
  ((U)[0] = (M)[0]*(V)[0] + (M)[1]*(V)[1] + (M)[2]*(V)[2], \
   (U)[1] = (M)[3]*(V)[0] + (M)[4]*(V)[1] + (M)[5]*(V)[2], \
   (U)[2] = (M)[6]*(V)[0] + (M)[7]*(V)[1] + (M)[8]*(V)[2])

// sets I to upper inverse of 2x2 of 3x3  M
#define INVERSE_23M(I, M, T)   \
  ((T) = (M)[0]*(M)[4] - (M)[1]*(M)[3], \
   (I)[0] =  (M)[4]/T, (I)[2] = -(M)[3]/T, \
   (I)[1] = -(M)[1]/T, (I)[3] =  (M)[0]/T)

static float   
Lerp(float omin, float omax, float imin, float xx, float imax) {

    float ret=0;

 ret = ((xx - imin)/(imax - imin))*(omax-omin) + omin;
return ret;

}
   
static float
Bilerp(float tl, float tr, float bl, float br, float x1, float x, float x2, float y1, float y, float y2) {
   
  float ret = 0;
  ret = Lerp(Lerp(tl,tr,x1,x,x2), Lerp(bl, br, x1,x,x2), y1, y, y2);
  //lerp in x direction first
  return ret;
}

int main(int argc, char** argv) {  
/// arguments are in order: 1-world_space_geojson   2-cart map as a nrrd  3-outfile_name

if (argc!=4) {printf("Usage: coordshift areas.json cartmap.nrrd outfile \n");}

/////////////// reading in the geojson
FILE *fp;
long lSize;
char *geojson;

fp = fopen ( argv[1] , "rb" );
if( !fp ) {
    printf("can't open file");
    exit(1);
}
fseek( fp , 0L , SEEK_END);
lSize = ftell( fp );
rewind( fp );

/* allocate memory for entire content */
geojson = (char*) (malloc(lSize+1));
if( !geojson ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

/* copy the file into the geojson */
if( 1!=fread( geojson , lSize, 1 , fp) )
  fclose(fp),free(geojson),fputs("entire read fails",stderr),exit(1);
/////////////////////

//printf("geojson_part: %.*s \n", 600, &geojson[0]);

// read in the nrrd file, compute world-to-index transform from metadata, get numrows and numcolumns of raster image
Nrrd * cartmap;
cartmap = nrrdNew();
nrrdLoad(cartmap, argv[2], NULL);
double * diff_map = (double *) cartmap ->data;


double ItoW[9] = {cartmap->axis[1].spaceDirection[0], cartmap->axis[2].spaceDirection[0], cartmap->spaceOrigin[0], 
    cartmap->axis[1].spaceDirection[1], cartmap->axis[2].spaceDirection[1], cartmap->spaceOrigin[1], 
    0, 0, 1};

float T = 0;
float upper[4];
INVERSE_23M(upper, ItoW, T);
double WtoI[9] = {upper[0], upper[1], -1*(upper[0]*cartmap->spaceOrigin[0] + upper[1] * cartmap->spaceOrigin[1]), 
    upper[2], upper[3], -1*(upper[2]*cartmap->spaceOrigin[0] + upper[3] * cartmap->spaceOrigin[1]),
    0, 0 ,1};
int columns = cartmap->axis[1].size;

//printf("sizes: %d %d %d \n", cartmap->axis[0].size, cartmap->axis[1].size,cartmap->axis[2].size);
//printf("ItoW: \n %f, %f, %f \n %f, %f, %f \n %f, %f, %f \n", ItoW[0], ItoW[1],ItoW[2],ItoW[3],ItoW[4],ItoW[5],ItoW[6],ItoW[7],ItoW[8]);
//printf("WtoI: %f %f %f %f %f %f %f %f %f \n", WtoI[0], WtoI[1],WtoI[2],WtoI[3],WtoI[4],WtoI[5],WtoI[6],WtoI[7],WtoI[8]);
/* do your work here, geojson is a string contains the whole text */
//////////////

//printf("%s", geojson);

int r;
int l;
jsmn_parser parser;
jsmntok_t * tokens;

jsmn_init(&parser);
r = jsmn_parse(&parser, geojson, strlen(geojson), NULL, 100);
if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
    }



tokens = (jsmntok_t *) malloc(sizeof(jsmntok_t) * r);
jsmn_init(&parser);
l = jsmn_parse(&parser, geojson, strlen(geojson), tokens, r);


//char tmp[20];
for (int k = 0; k < r-2; k++) {
    //printf("r, end, start, k: %d %d %d %d\n", r, tokens[k].end, tokens[k].start, k);
    //scanf("%s", &tmp);
    //printf("%.*s \n", tokens[k].end - tokens[k].start, geojson + tokens[k].start);
    //scanf("%s", &tmp);
    jsmntok_t tok = tokens[k];
    if (tok.type !=JSMN_ARRAY || tok.size!=2) {
        continue;
    }
   
    if ((tokens[k+1].type == tokens[k+2].type) && (tokens[k+2].type == JSMN_PRIMITIVE )) {
        //printf("found a number! \n");
        //printf("worldx: %.*s ", tokens[k+1].end - tokens[k+1].start, geojson + tokens[k+1].start);
        //printf("worldy: %.*s \n", tokens[k+2].end - tokens[k+2].start, geojson + tokens[k+2].start);
        //scanf("%s", &tmp);

    	// First, read in the x and y coordinates into coordx and coord y
        int length1 = tokens[k+1].end - tokens[k+1].start;
        char num1[length1];
        int length2 = tokens[k+2].end - tokens[k+2].start;
        char num2[length2];

        //printf("length 1, length2: %d %d \n", length1, length2);
        memcpy(num1, &geojson[tokens[k+1].start], length1); // <---- actually the y value
        memcpy(num2, &geojson[tokens[k+2].start], length2); // <---- actually the x value

        float coordx = atof(num2);
        float coordy = atof(num1);
        float coords[3] = {coordx, coordy, 1};

        // we need to change these into index-space raster coordinates
        double raster[3];
        MV3_MUL(raster, WtoI, coords);
        float raster_x = raster[0];
        float raster_y = raster[1];

        //printf("raster_x, raster_y: %f %f\n", raster_x, raster_y);
        //scanf("%s", tmp);
        // now we bilerp using cart's diffusion map

        int x1 = (int) raster_x;
        int x2 = x1 + 1;
        int y1 = (int) raster_y;
        int y2 = y1 + 1;

        //printf("bounds x1 x2 y1 y2: %d %d %d %d \n", x1, x2, y1, y2);
        float newx;
        float newy;

        if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0) {newx = raster_x; newy = raster_y;}

        else {
            newx = Bilerp(diff_map[(y1*columns + x1)*2], diff_map[(y1*columns+x2)*2], diff_map[(y2*columns+x1)*2], diff_map[(y2*columns+x2)*2], x1, raster_x, x2, y1, raster_y, y2);
        newy = Bilerp(diff_map[(y1*columns + x1)*2+1], diff_map[(y1*columns+x2)*2+1], diff_map[(y2*columns+x1)*2+1], diff_map[(y2*columns+x2)*2+1], x1, raster_x, x2, y1, raster_y, y2);

        }

        float index_final[3] = {newx, newy, 1};
        float world_final[3];


        MV3_MUL(world_final, ItoW, index_final);
        if (isnan(world_final[0]) || isinf(world_final[0])) {world_final[0] = 0;}
        if (isnan(world_final[1]) || isinf(world_final[1]))  {world_final[1] = 0;}

        //printf("diffmap val: %f %f \n", diff_map[(y1*columns + x1)*2], diff_map[(y1*columns + x2)*2]);
        char newx_string[length1];
        char newy_string[length2];

        snprintf(newx_string, length1, "%f", world_final[0]);
        snprintf(newy_string, length2,"%f", world_final[1]);


        for (int e = 0; e < length1; e++) {
            if (e < strlen(newx_string))
                geojson[tokens[k+1].start + e] = newx_string[e]; 
                else{ geojson[tokens[k+1].start + e] = '0';}
            //if (geojson[tokens[k+1].start + e] == '\0') {geojson[tokens[k+1].start + e] = '0';}
        }
        for (int ll = 0; ll < length2; ll++) {
            if (ll <strlen(newy_string)) 
                geojson[tokens[k+2].start + ll] = newy_string[ll]; 
            else{ geojson[tokens[k+2].start + ll] = '0';}
        }


        //geojson[tokens[k+1].start + length1] = '0';
        //geojson[tokens[k+2].start + length2] = '0';
        //geojson[tokens[k+1].start + length1+1] = '0';
        //geojson[tokens[k+2].start + length2+1] = '0';
        //printf("newx, newy: %s %s \n", newx_string, newy_string);
        //printf("newx newy: %f %f \n", newx, newy);
        //printf("geojson_part: %.*s \n", 300, &geojson[tokens[k+1].start]);
        //scanf("%s", &tmp);

        //printf("tokens starts: %d %d \n",tokens[k+1].start, tokens[k+2].start);



       //snprintf(&geojson[tokens[k+1].start], length1, "%f", newx);
       //snprintf(&geojson[tokens[k+2].start], length2,"%f", newy);
       //geojson[tokens[k+1].start + length1-1] = ',';
       //geojson[tokens[k+2].start + length2-1] = ' ';
         //scanf("%s", tmp);



        //memcpy(&geojson[tokens[k+1].start], newx_string, length1);
        //memcpy(&geojson[tokens[k+2].start], newy_string, length2);

        //printf("%s", &geojson[tokens[k+2].start]);
    }

}

//printf("geojson_part: %.*s \n", 3000, &geojson[0]);

FILE *out = fopen(argv[3], "w+");
if (out == NULL) {printf("can't open outfilefile \n");}
fputs(geojson, out);
fclose(out);

fclose(fp);
free(geojson);
nrrdNuke(cartmap);

}
