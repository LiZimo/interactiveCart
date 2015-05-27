#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
//#include "/home/zimo/include"
#include "teem/meet.h"
#include "../jsmn/jsmn.h"
/* compile with:
gcc -std=c99 -Wall -O2 -o CoordSwap CoordSwap.c -I $TEEM/include -L $TEEM/lib -L ../jsmn -lteem -lpng -lz -ljsmn
*/

/* 3-vector U = 3x3 matrix M time 3-vector V */


int main(int argc, char** argv) {  
/// arguments are in order: 1-world_space_geojson   2-cart map as a nrrd  3-outfile_name

if (argc!=3) {printf("Usage: coordshift areas.json outfile.json \n");}

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
        int length2 = tokens[k+2].end - tokens[k+2].start;
        char num1[length1];
        char num2[length2];

        //printf("length 1, length2: %d %d \n", length1, length2);
        memcpy(num2, &geojson[tokens[k+1].start], length1);
        memcpy(num1, &geojson[tokens[k+2].start], length2);

        for (int e = 0; e < length1; e++) {
            if (e < strlen(num1))
                geojson[tokens[k+1].start + e] = num1[e]; 
            else{ geojson[tokens[k+1].start + e] = '0';}
            //if (geojson[tokens[k+1].start + e] == '\0') {geojson[tokens[k+1].start + e] = '0';}
        }
        for (int ll = 0; ll < length2; ll++) {
            if (ll <strlen(num2)) 
                geojson[tokens[k+2].start + ll] = num2[ll]; 
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

FILE *out = fopen(argv[2], "w+");
if (out == NULL) {printf("can't open outfile \n");}
fputs(geojson, out);
fclose(out);

fclose(fp);
free(geojson);

}
