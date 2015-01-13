#!/usr/bin/awk -f

BEGIN {
    printOut = 0
}

/\/\* BEGIN_LIBRARIES/ {
    printOut = 1
}

/END_LIBRARIES \*\// {
    printOut = 0
}

/^[A-Za-z0-9_]+$/ {
    if( printOut == 1 )
        print $0
}
