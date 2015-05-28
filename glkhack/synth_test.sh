#!/usr/bin/env bash


NN=0
RK2=0
VERBOSE=1
XSZE=512
YSZE=512
NUM_REGIONS=5
TRLO=1

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
    -tr|--targetresolution)
    TRLO="$2"
    ;;
    -xsize|--xsize)
    XSZE="$2"
    ;;
    -ysize|--ysize)
    YSZE="$2"
    ;;
    -num|--numRectangles)
    NUM_REGIONS="$2"
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

Doo "python ../code/synth_data.py $NUM_REGIONS $XSZE $YSZE synth.json"
Doo "chmod 755 synth.json"

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
TE="$((-$XSZE/2 - 50)) $((-$YSZE/2 - 50)) $(($XSZE/2 + 50)) $(($YSZE/2 + 50))"

#swap the TE for tests
TE_S="$((-$YSZE/2-50)) $((-$XSZE/2-50)) $(($YSZE/2+50)) $(($XSZE/2+50))"


### reproject and rasterize json files
# if false: skip; if true: do it
#TRLO=8000
if true; then


  Doo "../code/CoordSwap synth.json synth_swap.json" > /dev/null

  Doo "gdal_rasterize -ts $XSZE $YSZE -te $TE -ot UInt16 -a STATE synth.json statelo.tiff" 
  Doo "gdal_rasterize -ts $XSZE $YSZE -te $TE -ot byte -a STATE synth.json synth1.tiff" 
  Doo "$T2N -i statelo.tiff -co false -o statelo.nhdr" > /dev/null

  Doo "gdal_rasterize -ts $XSZE $YSZE -te $TE_S -ot UInt16 -a STATE synth_swap.json statelo_swap.tiff" 
  Doo "gdal_rasterize -ts $XSZE $YSZE -te $TE_S -ot byte -a STATE synth_swap.json synth1_swap.tiff" 
  Doo "$T2N -i statelo_swap.tiff -co false -o statelo_swap.nhdr" > /dev/null
fi
  ### Just for high-res state and county maps: reproject and rasterize jsons
  # if false: skip; if true: do it

# to find number of unique IDs burned in tmp.png
#MIN=$(unu minmax tmp.png | grep min: | cut -d' ' -f 2)
#MAX=$(unu minmax tmp.png | grep max: | cut -d' ' -f 2)
#NUM=$(unu histo -i tmp.png -min $MIN -max $MAX -b $[$MAX-$MIN+1] | unu crop -min 1 -max M | unu 2op neq 0 - | unu project -a 0 -m sum | unu save -f text)

# lose the district of columbia in maryland

echo "11 24" | unu subst -i statelo.nhdr -s - -o statelo.nrrd
echo "11 24" | unu subst -i statelo_swap.nhdr -s - -o statelo_swap.nrrd

VGRIND="valgrind --leak-check=full --show-leak-kinds=all --dsymutil=yes"
OCART="../carteem/cart-1.2.2/ocart"
TCART="../carteem/cart-1.2.2/tcart" 

Doo "rm -f disp.nrrd"
Doo "rm -f disp_swap.nrrd"
Doo "$TCART -w wisdom.txt -pr m -i statelo.nrrd -s subst_synth.txt -v $VERBOSE -or rho.nrrd -te $TE -o disp.nrrd -nn $NN -rk2 $RK2"

Doo "$TCART -w wisdom.txt -pr m -i statelo_swap.nrrd -s subst_synth.txt -v $VERBOSE -or rho_swap.nrrd -te $TE -o disp_swap.nrrd -nn $NN -rk2 $RK2"

# this part at the end takes the cart output and makes a cartogram with it
Doo "../code/CoordShift synth.json disp.nrrd equal_area.json" > /dev/null
Doo "gdal_rasterize -ts $XSZE $YSZE -te $TE -ot byte -a STATE equal_area.json synth_cart.tiff" 
Doo "$T2N -i synth_cart.tiff -co false -o synth_cart.nhdr" > /dev/null

Doo "../code/CoordShift synth_swap.json disp_swap.nrrd equal_area_swap.json" > /dev/null
Doo "gdal_rasterize -ts $XSZE $YSZE -te $TE_S -ot byte -a STATE equal_area_swap.json synth_cart_swap.tiff" 
Doo "$T2N -i synth_cart_swap.tiff -co false -o synth_cart_swap.nhdr" > /dev/null