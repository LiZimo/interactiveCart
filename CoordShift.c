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
Bilerp(float a, float b, float c, float d, float umin, float uu, float umax, float lmin, float ll, float lmax) {
	
  float ret = 0;
  ret = Lerp(Lerp(a,b,umin, uu, umax), Lerp(c,d, umin, uu, umax), lmin, ll, lmax);	
  return ret;	
}





int main(int argc, char** argv) {

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
geojson = (char*) (malloc( 1, lSize+1 ));
if( !geojson ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

/* copy the file into the geojson */
if( 1!=fread( geojson , lSize, 1 , fp) )
  fclose(fp),free(geojson),fputs("entire read fails",stderr),exit(1);

/* do your work here, geojson is a string contains the whole text */
//////////////

int i;
int r;
int l;
jsmn_parser parser;
jsmntok_t * tokens;

jsmn_init(&p);
r = jsmn_parse(&parser, geojson, strlen(geojson), NULL, sizeof(t)/sizeof(t[0]));
if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}
tokens = (jsmntok_t *) malloc(sizeof(jsmntok_t) * r);
jsmn_parse(&parser, geojson, strlen(geojson), tokens, r);


char tmp[20];
for (int k = 0; k < r; k++) {
	jsmntok_t tok = tokens[k];
	if (tok.type !=JSMN_ARRAY || tok.size!=2) {
		continue;
	}

	if ((tokens[k+1].type == tokens[k+2].type) && (tokens[k+2].type == PRIMITIVE )) {
		printf("%s \n", tokens[k+1]->end - tokens[k+1]->start, geojson + tokens[k+1]->start);
		x = scanf("%s" &tmp);
	}

}


fclose(fp);
free(geojson);

}