#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "/home/zimo/include"
#include <teem.h>
#include "jsmn/jsmn.h"


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


}





int main(int argc, char** argv) {  
/// arguments are in order: 1-world_space_geojson   2-cart map   3 - number of rows  
	//4-number of columns 5-world x min 6-world x max 7 - world y min    8- world y max
	// 9 - outfile name




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


////// reading in the cart_map
int rows = argv[3];
int columns = argv[4];

//
Nrrd * cartmap;
cartmap = nrrdNew();
nrrdLoad(cartmap, argv[2], NULL);
float * diff_map = (float *) cartmap ->data;



/* do your work here, geojson is a string contains the whole text */
//////////////

//printf("%s", geojson);

int i;
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


char tmp[20];
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
        //printf("%.*s \n", tokens[k+1].end - tokens[k+1].start, geojson + tokens[k+1].start);
        //printf("%.*s \n", tokens[k+2].end - tokens[k+2].start, geojson + tokens[k+2].start);
        //scanf("%s", &tmp);










    	// First, read in the x and y coordinates into coordx and coord y

        int length1 = tokens[k+1].end - tokens[k+1].start;
        char num1[length1];
        int length2 = tokens[k+2].end - tokens[k+2].start;
        char num2[length2];
        memcpy(num1, &geojson[tokens[k+1].start], length1);
        memcpy(num2, &geojson[tokens[k+2].start], length2);

        float coordx = atof(num1);
        float coordy = atof(num2);

        // we need to change these into index-space raster coordinates
        float raster_x = Lerp(0, columns -1, xmin, coordx, xmax);
        float raster_y = Lerp(0, rows -1, ymax, coordy, ymin);


        // now we bilerp using cart's diffusion map

        int x1 = (int) raster_x;
        int x2 = x1 + 1;
        int y1 = (int) raster_y;
        int y2 = y1 + 1;

        int newx = Bilerp(diff_map[(y1*columns + x1)*2], diff_map[(y1*columns+x2)*2], diff_map[(y2*columsn+x1)*2], diff_map[(y2*columns+x2)*2], x1, raster_x, x2, y1, raster_y, y2);
        int newy = Bilerp(diff_map[(y1*columns + x1)*2+1], diff_map[(y1*columns+x2)*2+1], diff_map[(y2*columsn+x1)*2+1], diff_map[(y2*columns+x2)*2+1], x1, raster_x, x2, y1, raster_y, y2);

        //char newx_string[length1];
        //char newy_string[length2];

        snprintf(&geojson[tokens[k+1].start], length1, "%f", newx);
        snprintf(&geojson[tokens[k+2].start], length2,"%f", newy);
    }

}

FILE *out = fopen(outfile, "ab");
if (out == NULL) {printf("can't open outfilefile \n");}
fputs(geojson, out);
flcose(out);

fclose(fp);
free(geojson);
free(cart_map);

}