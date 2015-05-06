#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../jsmn.h"


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





int main(int argc, char** argv) {  /// arguments are in order: 1-raster_coord_geojson   2-cart map   3 - number of rows       4  - number of columns
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

FILE *cart_map









///////////









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

		int length1 = tokens[k+1].end - tokens[k+1].start; 
		char num1[length1];
		int length2 = tokens[k+2].end - tokens[k+2].start;
		char num2[length2];
		memcpy(num1, &geojson[tokens[k+1].start], length1);
		memcpy(num2, &geojson[tokens[k+2].start], length2);

		float coordx = atof(num1);
		float coordy = atof(num2);

		int x1 = (int) coordx;
		int x2 = x1 + 1;
		int y1 = (int) coordy;
		int y2 = y1 + 1;

		int newx = Bilerp(cart_map[(y1*columns + x1)*2], cart[(y1*columns+x2)*2], cart[(y2*columsn+x1)*2], cart[(y2*columns+x2)*2], x1, coordx, x2, y1, coordy, y2);
		int newy = Bilerp(cart_map[(y1*columns + x1)*2+1], cart[(y1*columns+x2)*2+1], cart[(y2*columsn+x1)*2+1], cart[(y2*columns+x2)*2+1], x1, coordx, x2, y1, coordy, y2);

		char newx_string[length1];
		char newy_string[length2];

		snprintf(&geojson[tokens[k+1].start], length1, "%f", newx);
		snprintf(&geojson[tokens[k+2].start], length2,"%f", newy);
	}

}


fclose(fp);
free(geojson);
free(cart_map);

}