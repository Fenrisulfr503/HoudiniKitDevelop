    parm {
        name    "divs"      // Internal parameter name
        label   "Divisions" // Descriptive parameter name for user interface
        type    integer
        default { "5" }     // Default for this parameter on new nodes
        range   { 2! 50 }   // The value is prevented from going below 2 at all.
                            // The UI slider goes up to 50, but the value can go higher.
        export  all         // This makes the parameter show up in the toolbox
                            // above the viewport when it's in the node's state.
    }
    parm {
        name    "rad"
        label   "Radius"
        type    vector2
        size    2           // 2 components in a vector2
        default { "1" "0.3" } // Outside and inside radius defaults
    }
    parm {
        name    "nradius"
        label   "Allow Negative Radius"
        type    toggle
        default { "0" }
    }
    parm {
        name    "t"
        label   "Center"
        type    vector
        size    3           // 3 components in a vector
        default { "0" "0" "0" }
    }
    parm {
        name    "orient"
        label   "Orientation"
        type    ordinal
        default { "0" }     // Default to first entry in menu, "xy"
        menu    {
            "xy"    "XY Plane"
            "yz"    "YZ Plane"
            "zx"    "ZX Plane"
        }
    }