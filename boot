#!/bin/bash
IMAGE="wd/bossix_micropolis_2011.dsk"

# parameters for xterm
XT_GEOMETRY="80x24+50+100"
XT_FONT="Monospace"
XT_FONTSIZE=12
XT_BACKGROUND=black
XT_FOREGROUND=white

TELNET="telnet"
PORT_BASE=4000
HOST=localhost

XT1="xterm -fa $XT_FONT -fs $XT_FONTSIZE -bg $XT_BACKGROUND -fg $XT_FOREGROUND -geometry $XT_GEOMETRY -T "
XT2=" -e "

GT1="gnome-terminal --geometry $XT_GEOMETRY -t "
GT2=" -- "

PU1="pterm -bg $XT_BACKGROUND -fg $XT_FOREGROUND -geometry $XT_GEOMETRY -title"
PU2="-e "

CR1="cool-retro-term -p 'Monochrome Green' -T"
CR2="-e "
CR3=" 2>/dev/null"

TERMCMD="$XT1"
TERMEXEC="$XT2"

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
    echo " -gt,--gnome     use gnome-terminal instead of xterm"
    echo " -pt,--putty     use putty instead of xterm"
    echo " -cr,--cool      use cool retro terminal instead of xterm"
    echo " -0              start first terminal -0 to -9 are supported"
    echo " -a, --all       start all terminals"
    echo " -g              go, start the emulation"
    echo "--commandline    show commandline but do not start"
}

TP=""
GO=""
ARGS="./eagleemu"
SHOWCOMMANDLINE="0"

terminalPort () {
    case $1 in
        0) TP="scc0"
        ;;
        1) TP="scc1"
        ;;
        2) TP="fourway_1A"
        ;;
        3) TP="fourway_1B"
        ;;
        4) TP="fourway_1C"
        ;;
        5) TP="fourway_1D"
        ;;
        6) TP="fourway_2A"
        ;;
        7) TP="fourway_2B"
        ;;
        8) TP="fourway_2C"
        ;;
        9) TP="fourway_2D"
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
        --commandline)
            SHOWCOMMANDLINE="1"
            shift
        ;;
        -0|-1|-2|-3|-4|-5|-6|-7|-8|-9)
            SOCKNUMS="$SOCKNUMS ${1:1}"
            shift
        ;;
        -g|--go)
            GO="g"
            shift
        ;;
        -gt|--gnome)
            TERMCMD="$GT1"
            TERMEXEC="$GT2"
            shift
        ;;
	-pt|--putty)
	    TERMCMD="$PU1"
	    TERMEXEC="$PU2"
	    shift
	;;
	-cr|--cool)
            TERMCMD="$CR1"
	    TERMEXEC="$CR2"
	    shift
	;;
        -t|--telnet)
            TELNET="$2"
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
        ;;
        -c|--cs)
            addarg "dev nv cs"
            shift
        ;;
        -f|--fd)
            addarg "dev nv fd"
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
    if [ $i = 0 ]; then addarg "dev scc socketio 0 1"
    elif [ $i = 1 ]; then addarg "dev scc socketio 1 1"
    else addarg "dev sock terminal $i 1"
    fi
    PORT=$((PORT_BASE + i))
    addarg "exec $TERMCMD $TP $TERMEXEC $TELNET $HOST $PORT$TERMEND"
done

if [ "$SHOWCOMMANDLINE" == "1" ]; then
  echo "$ARGS $POSITIONAL_ARGS $GO"
else
  eval $ARGS $POSITIONAL_ARGS $GO
fi



