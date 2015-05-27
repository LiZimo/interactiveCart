#!/usr/bin/env bash


NN=0
RK2=0
VERBOSE=1
USESWAP=0
TRLO=8000

while [[ $# > 0 ]]
do
key="$1"

case $key in
    -nn|--nearestneighbor)
    NN="$2"
    shift # past argument
    ;;
    -rk2|--rungkutta2)
    RK2="$2"
    shift # past argument
    ;;
    -v|--verbose)
    VERBOSE="$2"
    shift # past argument
    ;;
    -swap|--useswap)
    USESWAP="$2"
    ;;
    -tr|--targetresolution)
    TRLO="$2"
    ;;
    *)
            # unknown option
    ;;
esac
shift # past argument or value
done
if [[ -n $1 ]]; then
    echo "Last line of file specified as non-opt/last argument:"
    tail -1 $1
fi


set -o errexit
set -o nounset
JUNK=""
function junk {
  JUNK="$JUNK $@"
}
function cleanup {
  rm -rf $JUNK
}
trap cleanup err exit int term
function Doo {
  echo "  == $1"
  eval $1
  ret=$?
  if [ $ret != 0 ]; then
    Echoerr "==== ERROR (status $ret)"
    exit $ret
  fi
}


# http://eric.clst.org/wupl/Stuff/gz_2010_us_040_00_500k.json
# http://eric.clst.org/wupl/Stuff/gz_2010_us_050_00_500k.json

STATE_OUTL=gz_2010_us_040_00_500k.json
COUNT_OUTL=gz_2010_us_050_00_500k.json
T2N="../code/tiff2nhdr"

## TE= west south east north

# something like "-tr 8000 8000 -te -3000000 -3000000 3000000 3000000"
# shows a natural view of the projection output

# this is a tight bounds on the US, but which might bound everything
# after the diffusion cartograma displacement is applied
#TE="-2160000 -1580000 2470000 1270000"

# this gives more space to the north-east
TE="-2160000 -1700000 2700000 1500000"

#swap the TE for tests
if [ $USESWAP = "1" ] 
then
  TE="-1700000 -2160000 1500000 2700000" 
fi




### reproject and rasterize json files
# if false: skip; if true: do it
#TRLO=8000
if true; then
  PROJ="+proj=aea +lat_1=29.5 +lat_2=45.5 +lat_0=39.8 +lon_0=-98.6 +datum=NAD83 +units=m +no_defs"
  Doo "rm -f state.json; ogr2ogr -f geojson -t_srs \"$PROJ\" state.json $STATE_OUTL" > /dev/null
  if [ $USESWAP = "1" ] 
  then
    Doo "../code/CoordSwap state.json state1.json" > /dev/null
    Doo "rm -f state.json; mv state1.json state.json" > /dev/null
  fi
  Doo "gdal_rasterize -tr $TRLO $TRLO -te $TE -ot UInt16 -a STATE state.json statelo.tiff" > /dev/null
  Doo "$T2N -i statelo.tiff -co false -o statelo.nhdr" > /dev/null

  ### Just for high-res state and county maps: reproject and rasterize jsons
  # if false: skip; if true: do it
  if false; then
    TR=1900
    Doo "rm -f county.json; ogr2ogr -f geojson -t_srs \"$PROJ\" county.json $COUNT_OUTL" > /dev/null
    # Slowly lowering TR, and counting the number of unique counties rasterized,
    # the number seemed to max out at 3109 (TR=1950 acheives this)
    # This number was confirmed via looking through the json file for
    # things NOT in alaska, hawaii, and puerto rico
    Doo "gdal_rasterize -tr $TR $TR -te $TE -ot UInt16 -a STATE state.json state.tiff" > /dev/null
    Doo "gdal_rasterize -tr $TR $TR -te $TE -ot UInt16 -a COUNTY county.json county.tiff" > /dev/null
    Doo "$T2N -i state.tiff -co false -o state.nhdr" > /dev/null
    Doo "$T2N -i county.tiff -co false -o county.nhdr" > /dev/null
    Doo "unu 2op x 1000 state.nhdr | unu 2op + - county.nhdr -o state-county.png" > /dev/null
  fi
fi # end reproject and rasterize

# to find number of unique IDs burned in tmp.png
#MIN=$(unu minmax tmp.png | grep min: | cut -d' ' -f 2)
#MAX=$(unu minmax tmp.png | grep max: | cut -d' ' -f 2)
#NUM=$(unu histo -i tmp.png -min $MIN -max $MAX -b $[$MAX-$MIN+1] | unu crop -min 1 -max M | unu 2op neq 0 - | unu project -a 0 -m sum | unu save -f text)

# lose the district of columbia in maryland
echo "11 24" | unu subst -i statelo.nhdr -s - -o statelo.nrrd

MIN=$(unu minmax statelo.nrrd | grep min: | cut -d' ' -f 2)
MAX=$(unu minmax statelo.nrrd | grep max: | cut -d' ' -f 2)
unu histo -i statelo.nrrd -min $MIN -max $MAX -b $[$MAX-$MIN+1] -t double -o tmp.nrrd; junk tmp.nrrd
echo $MIN $MAX | unu reshape -s 2 |
unu resample -s $[$MAX-$MIN+1] -k tent -c node -t double |
unu join -i - tmp.nrrd -a 0 -incr |
unu save -f text |
grep -v " 0" > subst.txt
unu slice -i subst.txt -a 0 -p 1 |
unu crop -min 1 -max M -o aa.nrrd; junk aa.nrrd
unu project -i aa.nrrd -a 0 -m mean |
unu 2op / - aa.nrrd -w 1 |
unu axinsert -a 0 |
unu inset -i subst.txt -s - -min 1 1 -o subst.txt
echo "0 -1" | unu inset -i subst.txt -s - -min 0 0 -o subst.txt


VGRIND="valgrind --leak-check=full --show-leak-kinds=all --dsymutil=yes"
OCART="../carteem/cart-1.2.2/ocart"
TCART="../carteem/cart-1.2.2/tcart" 

Doo "$TCART -w wisdom.txt -pr m -i statelo.nrrd -s subst.txt -v $VERBOSE -or rho.nrrd -te $TE -o disp.nrrd -nn $NN -rk2 $RK2"

# this part at the end takes the cart output and makes a cartogram with it
Doo "../code/CoordShift state.json disp.nrrd equal_area.json" > /dev/null
Doo "gdal_rasterize -tr $TRLO $TRLO -te $TE -ot byte -a STATE equal_area.json statelo-cart-swap.tiff" > /dev/null
Doo "$T2N -i statelo-cart-swap.tiff -co false -o statelo-cart.nhdr" > /dev/null
