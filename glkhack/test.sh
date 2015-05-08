#!/usr/bin/env bash
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

### reproject and rasterize json files
# if false: skip; if true: do it
TR=1900
TRLO=7000
if true; then
  PROJ="+proj=aea +lat_1=29.5 +lat_2=45.5 +lat_0=39.8 +lon_0=-98.6 +datum=NAD83 +units=m +no_defs"
  Doo "rm -f state.json; ogr2ogr -f geojson -t_srs \"$PROJ\" state.json $STATE_OUTL"
  Doo "gdal_rasterize -tr $TRLO $TRLO -te $TE -ot UInt16 -a STATE state.json statelo.tiff"
  Doo "$T2N -i statelo.tiff -co false -o statelo.nhdr"

  ### Just for high-res state and county maps: reproject and rasterize jsons
  # if false: skip; if true: do it
  if false; then
    Doo "rm -f county.json; ogr2ogr -f geojson -t_srs \"$PROJ\" county.json $COUNT_OUTL"
    # Slowly lowering TR, and counting the number of unique counties rasterized,
    # the number seemed to max out at 3109 (TR=1950 acheives this)
    # This number was confirmed via looking through the json file for
    # things NOT in alaska, hawaii, and puerto rico
    Doo "gdal_rasterize -tr $TR $TR -te $TE -ot UInt16 -a STATE state.json state.tiff"
    Doo "gdal_rasterize -tr $TR $TR -te $TE -ot UInt16 -a COUNTY county.json county.tiff"
    Doo "$T2N -i state.tiff -co false -o state.nhdr"
    Doo "$T2N -i county.tiff -co false -o county.nhdr"
    Doo "unu 2op x 1000 state.nhdr | unu 2op + - county.nhdr -o state-county.png"
  fi
fi # end reproject and rasterize

# to find number of unique IDs burned in tmp.png
#MIN=$(unu minmax tmp.png | grep min: | cut -d' ' -f 2)
#MAX=$(unu minmax tmp.png | grep max: | cut -d' ' -f 2)
#NUM=$(unu histo -i tmp.png -min $MIN -max $MAX -b $[$MAX-$MIN+1] | unu crop -min 1 -max M | unu 2op neq 0 - | unu project -a 0 -m sum | unu save -f text)
#echo "========== found $NUM counties with TR $TR"

# lose the district of columbia in maryland
echo "11 24" | unu subst -i statelo.nhdr -s - -o statelo.nrrd

MIN=$(unu minmax statelo.nrrd | grep min: | cut -d' ' -f 2)
MAX=$(unu minmax statelo.nrrd | grep max: | cut -d' ' -f 2)
unu histo -i statelo.nrrd -min $MIN -max $MAX -b $[$MAX-$MIN+1] -t ushort -o tmp.nrrd; junk tmp.nrrd
echo $MIN $MAX | unu reshape -s 2 |
unu resample -s $[$MAX-$MIN+1] -k tent -c node -t ushort |
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

# OLD: This all pre-dates tcart being able to do its own map processing
if false; then
  ### process areas.txt into lookup table
  ### and apply lookup table with "unu subst"
  unu slice -i areas.txt -a 0 -p 1 | unu 2op / 52830 - | unu splice -i areas.txt -a 0 -p 1 -s - | unu save -f text | sort -n > area-inverse.txt
  echo "0 nan" | unu join -i - area-inverse.txt -a 1 -o area-inverse.txt
  unu convert -t float -i statelo.nhdr | unu subst -s area-inverse.txt -o area.nrrd
  AVG=$(unu axmerge -i area.nrrd -a 0 | unu project -a 0 -m mean | unu save -f text)
  unu 2op exists area.nrrd $AVG -o cart-in0.nrrd

  SZA=($(unu head cart-in0.nrrd | grep sizes))
  SX=$[2*${SZA[1]}]
  SY=$[2*${SZA[2]}]
  PX=$[($SX - ${SZA[1]})/2]
  PY=$[($SY - ${SZA[2]})/2]

  unu pad -i cart-in0.nrrd -min -$PX -$PY -max m+$[$SX-1] m+$[$SY-1] | unu convert -t double -o cart-in.nrrd

  echo "======= SX=$SX  SY=$SY"
  Doo "unu save -i cart-in.nrrd -f nrrd -e ascii | unu data - > cart-in.txt"

  Doo "$OCART $SX $SY cart-in.txt ocart-out.nrrd"
  Doo "$TCART -i cart-in.nrrd  -o tcart-out.nrrd -r 1 -g refgrid.nrrd"
fi

# this is an example of new tcart invocation (as of Thu May 7)
Doo "$TCART -i statelo.nrrd -s subst.txt -or rho.nrrd -te $TE -o disp.nrrd"

# this part at the end takes the cart output and makes a cartogram with it
Doo "../code/CoordShift state.json disp.nrrd equal_area.json"
Doo "gdal_rasterize -tr $TRLO $TRLO -te $TE -ot Uint16 -a STATE equal_area.json cartogram.tiff"
Doo "$T2N -i cartogram.tiff -co false -o cartogram.nhdr"
