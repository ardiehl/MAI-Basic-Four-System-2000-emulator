#!/bin/bash
IMAGE="wd/bossix_micropolis_2011.dsk"

# parameters for xterm
XT_GEOMETRY="80x24+50+100"
XT_FONT="Monospace"
XT_FONTSIZE=12
XT_BACKGROUND=black
XT_FOREGROUND=white

TELNET="-e telnet"
PORT_BASE=4000
HOST=localhost

XT1="xterm -fa $XT_FONT -fs $XT_FONTSIZE -bg $XT_BACKGROUND -fg $XT_FOREGROUND -geometry $XT_GEOMETRY"
# socket numbers to start
SOCKNUMS=""

# xterm -fa '$XT_FONT' -fs $XT_FONTSIZE -bg $XT_BACKGROUND -fg XT_FOREGROUND -geometry $XT_GEOMETRY -e $TELNET localhost 4001

usage () {
    echo "Usage: $0 [options]"
    echo " -i, --image     wd boot image or - to set no wd image"
    echo " -w  --wd        boot from wd0"
    echo " -c  --cs        boot from cs0"
    echo " -f  --fd        boot from fd0"
    echo " -t, --telnet    telnet program to use"
    echo " -0              start first terminal -0 to -9 are supported"
    echo " -a, --all       start all terminals"
    echo " -g              go, start the emulation"
}

TP=""
GO=""
ARGS="./eagleemu"

terminalPort () {
    case $1 in
        0) TP="scc0"
        ;;
        1) TP="scc1"
        ;;
        2) TP="fourway 1 port A"
        ;;
        3) TP="fourway 1 port B"
        ;;
        4) TP="fourway 1 port C"
        ;;
        5) TP="fourway 1 port D"
        ;;
        6) TP="fourway 2 port A"
        ;;
        7) TP="fourway 2 port B"
        ;;
        8) TP="fourway 2 port C"
        ;;
        9) TP="fourway 2 port D"
        ;;
    esac
}


addarg () {
    if [ -n "$1" ]; then
        ARGS="$ARGS \"$1\""
    fi
}


while [[ $# -gt 0 ]]; do
    case $1 in
        -0|-1|-2|-3|-4|-5|-6|-7|-8|-9)
            SOCKNUMS="$SOCKNUMS ${1:1}"
            shift
        ;;
        -g|--go)
            GO="g"
            shift
        ;;
        -t|--telnet)
            TELNET="$2"
            shift
            shift
        ;;
        -a|--all)
            SOCKNUMS="0 1 2 3 4 5 6 7 8 9"
            shift
        ;;
        -i|--image)
            IMAGE=$2
            [ $IMAGE = "-" ] && IMAGE=""
            shift
            shift
        ;;
        -w|--wd)
            addarg "dev nv wd"
            shift
            shift
        ;;
        -c|--cs)
            addarg "dev nv cs"
            shift
            shift
        ;;
        -f|--fd)
            addarg "dev nv fd"
            shift
            shift
        ;;
        -*|--*)
            echo "Unknown option $1"
            usage
            exit 1
        ;;
        *)
            POSITIONAL_ARGS="$POSITIONAL_ARGS \"$1\""
            shift
        ;;
    esac
done



if [ -n "$IMAGE" ]; then
    if [ ! -f $IMAGE ]; then
        echo "$0: unable to open $IMAGE"
        exit 1
    fi
    addarg "dev wd image $IMAGE"
fi

for i in $SOCKNUMS; do
    terminalPort $i
    [ $i = 0 ] && addarg "dev scc socketio 0 1"
    [ $i = 1 ] && addarg "dev scc socketio 1 1"
    PORT=$((PORT_BASE + i))
    addarg "exec $XT1 -T $TP $TELNET $HOST $PORT"
done

#echo "$ARGS $POSITIONAL_ARGS $GO"
eval $ARGS $POSITIONAL_ARGS $GO



