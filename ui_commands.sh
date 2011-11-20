#! /sbin/sh

ui_print() {
    echo "* print $*"
}

ui_print_n() {
    echo "* printn $*"
}

headers() {
    echo "* headers $#"
    while [ $# -gt 0 ]; do 
	echo "* header $1"
	shift
    done
}

items() {
    echo "* items $#"
    
    while [ $# -gt 0 ]; do
	echo "* item $1"
	shift
    done
}

show_menu() {
    echo "* show_menu"
}

ptotal() {
    echo "* ptotal $1"
}

pcur() {
    echo "* pcur $1"
}

check_items() {
    echo "* check_items $#"

    while [ $# -gt 0 ]; do
	echo "* check_item $1"
	shift
    done
}

show_check_menu() {
    echo "* show_check_menu"
}

ui_show_indeterminate_progress() {
    echo "* show_indeterminate_progress"
}

ui_reset_progress() {
    echo "* reset_progress"
}

menu() {
    local H=0
    local I=0
    OPTIND=1
    while getopts "h:i:v:" opt; do
	if [ "$opt" == "h" ]; then
	    let H=$H+1
	fi
	if [ "$opt" == "i" ]; then
	    let I=$I+1
	fi
    done
    echo "* headers $H"
    echo "* items $I"
    OPTIND=1
    while getopts "h:i:" opt; do
	if [ "$opt" == "h" ]; then
	    echo "* header $OPTARG"
	fi
	if [ "$opt" == "i" ]; then
	    echo "* item $OPTARG"
	fi
    done
    echo "* show_menu"
}

check_menu() {
    local H=0
    local I=0
    OPTIND=1
    while getopts "h:i:" opt; do
	if [ "$opt" == "h" ]; then
	    let H=$H+1
	fi
	if [ "$opt" == "i" ]; then
	    let I=$I+1
	fi
    done
    echo "* headers $H"
    echo "* check_items $I"
    OPTIND=1
    while getopts "h:i:" opt; do
	if [ "$opt" == "h" ]; then
	    echo "* header $OPTARG"
	fi
	if [ "$opt" == "i" ]; then
	    echo "* check_item $OPTARG"
	fi
    done
    echo "* show_check_menu"
}
