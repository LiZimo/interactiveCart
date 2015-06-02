#!/usr/bin/env bash
P=11
NN=0
RK2=0
NUM_REGIONS=5
TRLO=1

while [[ $# > 0 ]]
do
key="$1"

case $key in
    -p|--integer)
    P="$2"
    shift # past argument
    ;;
    -nn|--nearestneighbor)
    NN="$2"
    shift # past argument
    ;;
    -rk2|--rungkutta2)
    RK2="$2"
    shift # past argument
    ;;
    -tr|--targetresolution)
    TRLO="$2"
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

N=3
echo "P is $P"

while [ $N -le 11 ] 
do
  echo "$N"
  SIZE1=$((2**N))
  EXPONENT=$((2*P - N))

  SIZE2=$((2**$EXPONENT ))


  INPUTSIZE1=$(($SIZE1/8))
  INPUTSIZE2=$(($SIZE2/8))
  
  FILENAME="$N.$P.REG.txt"

  Doo "./synth_test.sh -xsize $INPUTSIZE1 -ysize $INPUTSIZE2 -nn $NN -rk2 $RK2 |tee outputs/$FILENAME stdout"

  N=$(($N + 1))
done
